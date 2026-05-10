#include "app/app.h"

#include <stdint.h>

#include "drivers/pwm.h"
#include "drivers/ssd1306_oled.h"
#include "drivers/systick.h"
#include "drivers/usart1.h"

/* PA0：TIM2_CH1 PWM，PWM 频率（载波）；占空比在循环里按“呼吸”曲线改。 */
#define APP_BREATH_PWM_HZ       (1000U)
/* 每多少毫秒改一次占空比；越小变化越快。 */
#define APP_BREATH_STEP_MS      (12U)
/* 相位 0..PHASE_MAX-1 走一个“先亮后暗”的三角周期；约 (STEP_MS * PHASE_MAX) ms 一整次呼吸。 */
#define APP_BREATH_PHASE_MAX    (400U)

/*
 * 呼吸灯轮询函数（非阻塞）
 *
 * 设计思路：
 *   - 由 app_run_forever() 在 while(1) 里反复调用，可能每秒被调几万次。
 *   - 但真正“干活”（改 PWM 占空比）只发生在每 STEP_MS=12ms 一次。
 *   - 其他时间立即 return，让 CPU 去跑别的任务（如 UART 接收）——
 *     这就是嵌入式里典型的“合作式多任务 / cooperative multitasking”。
 *
 * 占空比变化曲线（三角波）：
 *   phase 从 0 计到 PHASE_MAX-1 = 399，每 12ms +1：
 *     phase ∈ [0, 200]   → 占空比 0 → 1000（亮起）
 *     phase ∈ [201, 399] → 占空比 995 → 5（变暗）
 *   一整圈 400 × 12ms ≈ 4.8 秒。
 */
static void app_breath_led_poll(void) {
  /* static 局部变量：函数返回后值不丢，下次调用接着用。
   * 这是 C 里实现“函数自己的私有状态”的标准做法，比全局变量更内聚。 */
  static uint32_t s_next_ms;     /* 上次推进 phase 的时间戳（ms） */
  static uint8_t  s_time_inited; /* 0 = 还没首次校准 s_next_ms */
  static uint16_t s_phase;       /* 当前相位 0..399，驱动三角波 */

  uint32_t now = systick_get_ms();  /* 获取系统启动以来的毫秒数 */

  /* ---------- 首次进入：把 s_next_ms 校准到当前时间 ----------
   * 否则 s_next_ms 默认为 0，第一次跑 (now - 0) 远大于 12ms，
   * 会立刻连续推进很多个 phase，亮度跳变。 */
  if (s_time_inited == 0U) {
    s_time_inited = 1U;
    s_next_ms = now;
  }

  /* ---------- 节流：12ms 还没到就立刻返回 ----------
   * 重点是 (uint32_t) 强转：让减法在无符号下进行，
   * 即便 systick 计数将来溢出回零（49.7 天后），
   * (now - s_next_ms) 仍然能给出正确的“经过的毫秒数”。
   *
   * 例：now=10, s_next_ms=4_294_967_290（接近 uint32 最大）
   *     now - s_next_ms = 16（自动回绕，正确）。 */
  if ((uint32_t)(now - s_next_ms) < APP_BREATH_STEP_MS) {
    return;
  }
  s_next_ms = now;  /* 时间点已到，记下本次时间，等下一个 12ms */

  /* ---------- 推进相位 phase = (phase + 1) % 400 ---------- */
  s_phase = (uint16_t)(s_phase + 1U);
  if (s_phase >= APP_BREATH_PHASE_MAX) {
    s_phase = 0U;
  }

  /* ---------- 由 phase 算三角波 tri ∈ [0, 200] ----------*/
  uint16_t tri = (s_phase <= 200U) ? s_phase
                                   : (uint16_t)(APP_BREATH_PHASE_MAX - s_phase);

  /* ---------- 把 tri ∈ [0, 200] 线性映射到 duty ∈ [0, 1000] ----------
    亮度

    200        /\         400
              /  \
            /    \
    0 ______/      \______*/
  uint16_t duty = (uint16_t)(((uint32_t)tri * 1000U) / 200U);


  (void)tim2_ch1_pwm_set_duty_permille(duty);
}

void app_init(void) {
  systick_init_1ms();
  if (usart1_init(115200UL, USART_OVERSAMPLING_16) == 0U) {
    while (1) {
    }
  }
  usart1_set_line_policy(USART1_LINE_CR_OR_LF);
  usart1_enable_rx_interrupt();

  if (ssd1306_init(ssd1306_default()) != STM_OK) {
    while (1) {
    }
  }

  /* 呼吸灯：固定 PWM 频率，仅改 CCR1 占空比（见 app_breath_led_poll）。 */
  if (tim2_ch1_pwm_init_hz(APP_BREATH_PWM_HZ, 0U) != STM_OK) {
    while (1) {
    }
  }

  usart1_send_string("\r\nUSART1 ready (PA9/PA10,115200 8N1)\r\n");
  usart1_send_string("OLED ready (I2C1 remap PB8/PB9).\r\n");
  usart1_send_string("Breath LED: TIM2_CH1 PWM on PA0.\r\n");
  usart1_send_string("Type a line, press Enter to flush to OLED.\r\n");
}

static uint8_t page_count = 0U;

void app_run_forever(void) {
  char line[64];

  while (1) {
    app_breath_led_poll();
    ssd1306_oled_write_text_atf(page_count, 0U, "line=%s --- page=%u", "hello", page_count);
    if (usart1_try_read_string(line, (uint16_t)sizeof(line)) != 0U) {
      usart1_send_string("recv: ");
      usart1_send_string(line);
      usart1_send_string("\r\n");

      ssd1306_oled_write_text_atf(page_count, 0U, "line=%s --- page=%u", line, page_count);
      ++page_count;
    }
  }
}
