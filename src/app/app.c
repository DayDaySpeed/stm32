#include "app/app.h"

#include <stdint.h>

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

/* 这些静态状态由三个任务函数共享，但不暴露到文件外。 */
static uint32_t s_last_step_ms;
static uint8_t s_time_inited;
static uint16_t s_phase;
static uint16_t s_duty_permille;
static uint32_t s_last_oled_ms;
static int16_t s_enc_prev;

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

/*
 * 编码器调试显示任务：
 * - 读取当前累计计数
 * - 与上次值做差得到近似速度增量
 * - 刷新 OLED 第 0/1/2/3/5 行
 */
static void app_encoder_oled_task(void) {
  int16_t enc_now = 0;
  int16_t enc_delta = (int16_t)(enc_now - s_enc_prev);
  char dir_sym = '0';

  if (bsp_wheel_encoder_read_count(&enc_now) == STM_OK) {
    enc_delta = (int16_t)(enc_now - s_enc_prev);
    s_enc_prev = enc_now;
  } else {
    enc_now = 0;
    enc_delta = 0;
  }

  dir_sym = (enc_delta > 0) ? '+' : (enc_delta < 0) ? '-' : '0';

  (void)bsp_display_write_text_atf(0U, 0U, "wheel encoder      ");
  (void)bsp_display_write_text_atf(1U, 0U, "CNT = %d        ", enc_now);
  (void)bsp_display_write_text_atf(2U, 0U, "dlt = %d        ", enc_delta);
  (void)bsp_display_write_text_atf(3U, 0U, "dir = %c          ", dir_sym);
  (void)bsp_display_write_text_atf(5U, 0U, "phase=%u duty=%u  ",
                                   s_phase, s_duty_permille);
}

/*
 * 光敏调试显示任务：
 * - 读取 4 次平均 ADC 原始值
 * - 换算为 x.xxxV 的定点显示
 * - 刷新 OLED 第 4 行；失败时显示错误码
 */
static void app_ambient_light_oled_task(void) {
  uint16_t ldr_raw = 0U;
  stm_status_t ldr_st = bsp_ambient_light_read_raw_average(&ldr_raw, 4U);

  if (ldr_st == STM_OK) {
    uint32_t ldr_mv = ((uint32_t)ldr_raw * 3300U + 2047U) / 4095U;
    uint32_t ldr_v_int = ldr_mv / 1000U;
    uint32_t ldr_v_frac = ldr_mv % 1000U;
    uint32_t frac_hundreds = ldr_v_frac / 100U;
    uint32_t frac_tens = (ldr_v_frac / 10U) % 10U;
    uint32_t frac_ones = ldr_v_frac % 10U;

    (void)bsp_display_write_text_atf(4U, 0U,
                                     "LDR=%u %u.%u%u%uV ",
                                     ldr_raw,
                                     ldr_v_int,
                                     frac_hundreds,
                                     frac_tens,
                                     frac_ones);
    return;
  }

  (void)bsp_display_write_text_atf(4U, 0U, "LDR err=%d      ", (int32_t)ldr_st);
}

/* 主循环里的合作式任务调度器：先跑呼吸灯，再按 500ms 节流刷新 OLED 调试页。 */
static void tasks(void) {
  uint32_t now = systick_get_ms();

  app_breath_led_task(now);

  if ((uint32_t)(now - s_last_oled_ms) < APP_DEBUG_OLED_PERIOD_MS) {
    return;
  }
  s_last_oled_ms = now;

  app_encoder_oled_task();
  app_ambient_light_oled_task();
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
  st = bsp_console_write_string_blocking("display/status-led/encoder/light ready\r\n");
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
