#ifndef DRIVERS_BREATHING_LED_H
#define DRIVERS_BREATHING_LED_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 状态灯呼吸效果驱动（TIM2_CH1 / PA0，边沿对齐 PWM）
 *
 * 本板 TIM2 专用于状态 LED 亮度调节；应用层周期改占空比即可实现呼吸灯。
 * 不启用 TIM2 更新中断。
 *
 * 须先 `bsp_clock_apply_profile()` 再 `bsp_board_init()`（GPIOA / TIM2 时钟）。
 */

typedef struct {
  uint32_t carrier_hz;       /* PWM 载波频率，如 1000 */
  uint16_t duty_permille;    /* 0 = 全低，1000 = 100% */
} breathing_led_config_t;

stm_status_t breathing_led_init_with_config(const breathing_led_config_t *config);
stm_status_t breathing_led_init_hz(uint32_t carrier_hz, uint16_t duty_permille);
stm_status_t breathing_led_set_duty_permille(uint16_t duty_permille);
stm_status_t breathing_led_set_config(const breathing_led_config_t *config);
stm_status_t breathing_led_set_hz(uint32_t carrier_hz, uint16_t duty_permille);
stm_status_t breathing_led_stop(void);

#endif
