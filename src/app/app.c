#include "app/app.h"

#include <stdint.h>

#include "bsp/board_config.h"
#include "bsp/board_devices.h"
#include "common/stm_assert.h"
#include "common/stm_log.h"
#include "drivers/systick.h"

/* 每多少毫秒改一次占空比；越小变化越快。 */
#define APP_BREATH_STEP_MS      (12U)
/* 相位 0..PHASE_MAX-1 走一个“先亮后暗”的三角周期；约 (STEP_MS * PHASE_MAX) ms 一整次呼吸。 */
#define APP_BREATH_PHASE_MAX    (400U)
/* OLED 调试信息刷新节流：太快会阻塞 CPU 拖慢呼吸；500ms 人眼也跟得上。 */
#define APP_DEBUG_OLED_PERIOD_MS (500U)
/* 连续失败达到此次数后触发 I2C+SSD1306 重新初始化（至少间隔 RECOVER_MIN_MS）。 */
#define APP_OLED_RECOVER_FAIL_STREAK (2U)
#define APP_OLED_RECOVER_MIN_MS      (1000U)
/* 电机占空比高于此值时跳过 ADC+热敏行（减轻满速时主循环阻塞）。 */
#define APP_MOTOR_OLED_SKIP_DUTY    (600U)
/* 编码器调速：每 20ms 读一次增量并更新电机占空比。 */
#define APP_MOTOR_PERIOD_MS     (20U)
/* 旋钮每 1 个计数改变多少千分比占空比；越大越灵敏。 */
#define APP_MOTOR_ENC_STEP      (8U)
/* 红外靠近检测周期（毫秒）。 */
#define APP_IR_CHECK_PERIOD_MS  (80U)

/* 这些静态状态由任务函数共享，但不暴露到文件外。 */
static uint32_t s_last_step_ms;
static uint8_t s_time_inited;
static uint16_t s_phase;
static uint16_t s_duty_permille;
static uint32_t s_last_oled_ms;
static uint8_t s_oled_fail_streak;
static uint32_t s_oled_last_recover_ms;
static uint32_t s_last_motor_ms;
static int16_t s_motor_enc_prev;
static uint16_t s_motor_duty_permille;
static uint8_t s_hand_near;
static uint32_t s_last_ir_check_ms;
static uint32_t s_ir_beep_cooldown_until_ms;

/*
 * 手靠近反射红外（raw 超过 BOARD_IR_NEAR_RAW_HIGH）时鸣叫一次；
 * 手离开（raw 低于 BOARD_IR_LEAVE_RAW_LOW）后才允许再次触发。
 */
static void app_ir_proximity_buzzer_task(uint32_t now_ms) {
  uint16_t ir_raw = 0U;

  if ((uint32_t)(now_ms - s_last_ir_check_ms) < APP_IR_CHECK_PERIOD_MS) {
    return;
  }
  s_last_ir_check_ms = now_ms;

  if (bsp_ir_reflect_read_raw_average(&ir_raw, 2U) != STM_OK) {
    return;
  }

  if (ir_raw >= BOARD_IR_NEAR_RAW_HIGH) {
    if ((s_hand_near == 0U) && (now_ms >= s_ir_beep_cooldown_until_ms)) {
      (void)bsp_buzzer_beep_blocking(BOARD_BUZZER_BEEP_MS);
      s_ir_beep_cooldown_until_ms = now_ms + BOARD_IR_BEEP_COOLDOWN_MS;
    }
    s_hand_near = 1U;
    return;
  }

  if (ir_raw < BOARD_IR_LEAVE_RAW_LOW) {
    s_hand_near = 0U;
  }
}

/*
 * 呼吸灯步进任务：
 * - 每 APP_BREATH_STEP_MS 推进一步相位
 * - 把三角波相位映射成占空比
 * - 更新板级状态灯 PWM
 */
static void app_breath_led_task(uint32_t now_ms) {
  if (s_time_inited == 0U) {
    s_time_inited = 1U;
    s_last_step_ms = now_ms;
  }

  if ((uint32_t)(now_ms - s_last_step_ms) < APP_BREATH_STEP_MS) {
    return;
  }
  s_last_step_ms = now_ms;

  s_phase = (uint16_t)(s_phase + 1U);
  if (s_phase >= APP_BREATH_PHASE_MAX) {
    s_phase = 0U;
  }

  {
    uint16_t tri = (s_phase <= 200U) ? s_phase
                                     : (uint16_t)(APP_BREATH_PHASE_MAX - s_phase);
    s_duty_permille = (uint16_t)(((uint32_t)tri * 1000U) / 200U);
  }

  (void)bsp_status_led_set_duty_permille(s_duty_permille);
}

static void app_oled_note_success(void) { s_oled_fail_streak = 0U; }

static void app_oled_note_failure(void) {
  if (s_oled_fail_streak < 255U) {
    s_oled_fail_streak++;
  }
}

static stm_status_t app_oled_try_recover(uint32_t now_ms) {
  if (s_oled_fail_streak < APP_OLED_RECOVER_FAIL_STREAK) {
    return STM_ERR_IO;
  }
  if ((now_ms - s_oled_last_recover_ms) < APP_OLED_RECOVER_MIN_MS) {
    return STM_ERR_BUSY;
  }
  s_oled_last_recover_ms = now_ms;
  return bsp_display_recover();
}

/*
 * 编码器 / 电机 / 呼吸灯 OLED（page 0..2）：
 *   0 ENC   1 MOT   2 LED(phase/duty)
 */
static stm_status_t app_encoder_oled_task(void) {
  int16_t enc_now = 0;
  stm_status_t st = STM_OK;

  if (bsp_wheel_encoder_read_count(&enc_now) != STM_OK) {
    enc_now = 0;
  }

  st = bsp_display_write_text_atf(0U, 0U, "ENC=%d            ", enc_now);
  if (st != STM_OK) {
    return st;
  }
  st = bsp_display_write_text_atf(1U, 0U, "MOT=%u/1000       ",
                                  s_motor_duty_permille);
  if (st != STM_OK) {
    return st;
  }
  return bsp_display_write_text_atf(2U, 0U, "LED %u/%u         ",
                                    s_phase, s_duty_permille);
}

/*
 * 旋钮调速任务（AIN2 接 GND：仅停/单方向转）：
 * - 顺时针增量 -> 提高电机占空比
 * - 逆时针增量 -> 降低占空比（到 0 停）
 */
static void app_motor_encoder_task(uint32_t now_ms) {
  int16_t enc_now = 0;
  int16_t enc_delta = 0;
  int32_t next_duty = 0;

  if (s_last_motor_ms == 0U) {
    s_last_motor_ms = now_ms;
    s_motor_duty_permille = 0U;
    if (bsp_wheel_encoder_read_count(&enc_now) == STM_OK) {
      s_motor_enc_prev = enc_now;
    }
    return;
  }

  if ((uint32_t)(now_ms - s_last_motor_ms) < APP_MOTOR_PERIOD_MS) {
    return;
  }
  s_last_motor_ms = now_ms;

  if (bsp_wheel_encoder_read_count(&enc_now) != STM_OK) {
    return;
  }

  enc_delta = (int16_t)(enc_now - s_motor_enc_prev);
  s_motor_enc_prev = enc_now;
  if (enc_delta == 0) {
    return;
  }

  next_duty = (int32_t)s_motor_duty_permille + (int32_t)enc_delta * APP_MOTOR_ENC_STEP;
  if (next_duty < 0) {
    next_duty = 0;
  } else if (next_duty > 1000) {
    next_duty = 1000;
  }

  s_motor_duty_permille = (uint16_t)next_duty;
  (void)bsp_dc_motor_set_speed_permille(s_motor_duty_permille);
}

/*
 * 光敏 + 热敏 + 反射红外（page 3..5，一次 SCAN+DMA 三路平均）：
 *   3 LDR   4 NTC   5 IR
 */
static stm_status_t app_analog_sensors_oled_task(void) {
  uint16_t ldr_raw = 0U;
  uint16_t ntc_raw = 0U;
  uint16_t ir_raw = 0U;
  stm_status_t st =
      bsp_analog_sensors_read_pair_average(&ldr_raw, &ntc_raw, 4U);

  if (st != STM_OK) {
    stm_status_t w = bsp_display_write_text_atf(3U, 0U, "LDR err=%d        ",
                                                (int32_t)st);
    if (w != STM_OK) {
      return w;
    }
    return bsp_display_write_text_atf(4U, 0U, "NTC err=%d        ", (int32_t)st);
  }

  {
    uint32_t ldr_mv = ((uint32_t)ldr_raw * 3300U + 2047U) / 4095U;
    uint32_t ldr_v_int = ldr_mv / 1000U;
    uint32_t ldr_v_frac = ldr_mv % 1000U;

    st = bsp_display_write_text_atf(3U, 0U,
                                    "LDR=%u %u.%u%u%uV ",
                                    ldr_raw,
                                    ldr_v_int,
                                    ldr_v_frac / 100U,
                                    (ldr_v_frac / 10U) % 10U,
                                    ldr_v_frac % 10U);
    if (st != STM_OK) {
      return st;
    }
  }

  {
    int16_t temp_x10 = 0;
    stm_status_t temp_st =
        bsp_temperature_read_celsius_x10_from_raw(ntc_raw, &temp_x10);

    if (temp_st != STM_OK) {
      return bsp_display_write_text_atf(4U, 0U, "NTC err=%d raw=%u ",
                                        (int32_t)temp_st, ntc_raw);
    }

    {
      char sign = (temp_x10 < 0) ? '-' : ' ';
      int32_t t_abs = (temp_x10 < 0) ? -(int32_t)temp_x10 : (int32_t)temp_x10;
      int32_t t_int = t_abs / 10;
      int32_t t_frac = t_abs % 10;

      st = bsp_display_write_text_atf(4U, 0U,
                                        "NTC=%c%d.%dC raw=%u  ",
                                        sign, t_int, t_frac, ntc_raw);
      if (st != STM_OK) {
        return st;
      }
    }
  }

  st = bsp_ir_reflect_read_raw_average(&ir_raw, 4U);
  if (st != STM_OK) {
    return bsp_display_write_text_atf(5U, 0U, "IR err=%d        ", (int32_t)st);
  }

  {
    uint32_t ir_mv = ((uint32_t)ir_raw * 3300U + 2047U) / 4095U;
    return bsp_display_write_text_atf(5U, 0U, "IR=%u %umV       ", ir_raw, ir_mv);
  }
}

/* 主循环里的合作式任务调度器：先跑呼吸灯，再按 500ms 节流刷新 OLED 调试页。 */
static void tasks(void) {
  uint32_t now = systick_get_ms();

  app_breath_led_task(now);
  app_motor_encoder_task(now);
  app_ir_proximity_buzzer_task(now);

  if ((uint32_t)(now - s_last_oled_ms) < APP_DEBUG_OLED_PERIOD_MS) {
    return;
  }
  s_last_oled_ms = now;

  {
    stm_status_t st = app_encoder_oled_task();

    if (st != STM_OK) {
      app_oled_note_failure();
      if (app_oled_try_recover(now) == STM_OK) {
        st = app_encoder_oled_task();
      }
    }
    if (st != STM_OK) {
      return;
    }
    app_oled_note_success();
  }

  if (s_motor_duty_permille > APP_MOTOR_OLED_SKIP_DUTY) {
    return;
  }

  {
    stm_status_t st = app_analog_sensors_oled_task();

    if (st != STM_OK) {
      app_oled_note_failure();
      if (app_oled_try_recover(now) == STM_OK) {
        (void)app_encoder_oled_task();
        st = app_analog_sensors_oled_task();
      }
      if (st != STM_OK) {
        return;
      }
    }
    app_oled_note_success();
  }
}

stm_status_t app_init(void) {
  stm_status_t st = STM_OK;

  STM_ASSERT(APP_BREATH_STEP_MS > 0U, "app_cfg");
  STM_ASSERT(APP_BREATH_PHASE_MAX > 200U, "app_cfg");

  systick_init_1ms();
  st = bsp_default_devices_init();
  if (st != STM_OK) {
    return st;
  }

  st = bsp_console_write_string_blocking("\r\nboard console ready\r\n");
  if (st != STM_OK) {
    return st;
  }
  st = bsp_console_write_string_blocking(
      "display/led/encoder/motor/light/temp/ir/buzzer ready\r\n");
  if (st != STM_OK) {
    return st;
  }
  st = bsp_console_write_string_blocking("Type a line, press Enter to flush to OLED.\r\n");
  if (st != STM_OK) {
    return st;
  }

  STM_LOG_TEXT("app init ok\r\n");
  return STM_OK;
}

/* SSD1306 屏幕共 8 个 page（0..7），每行 8 像素高。 */
#define APP_OLED_PAGE_COUNT     (8U)

void app_run_forever(void) {
  char line[64];
  /* 当前要写入的 page 行；每收到一行串口输入就写一行并 ++。
   * static 局部：作用域限定在本函数内，比文件级全局变量更内聚。 */
  static uint8_t s_page_count = 0U;

  while (1) {
    tasks();

    stm_status_t line_st = bsp_console_read_line_try(line, (uint16_t)sizeof(line));
    if (line_st == STM_ERR_OVERFLOW) {
      (void)bsp_console_write_string_blocking("line too long\r\n");
      continue;
    }

    if (line_st == STM_OK) {
      (void)bsp_console_write_string_blocking("recv: ");
      (void)bsp_console_write_string_blocking(line);
      (void)bsp_console_write_string_blocking("\r\n");

      /* 8 行写满后，清屏再从第 0 行开始；
       * 否则 page_count 越界，ssd1306_write_text_at 会静默失败。 */
      if (s_page_count >= APP_OLED_PAGE_COUNT) {
        (void)bsp_display_clear();
        s_page_count = 0U;
      }

      (void)bsp_display_write_text_atf(s_page_count, 0U,
                                       "line=%s --- page=%u", line, s_page_count);
      ++s_page_count;
    }
  }
}
