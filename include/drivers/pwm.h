#ifndef DRIVERS_PWM_H
#define DRIVERS_PWM_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * TIM2 通道 1 PWM（引脚 PA0，复用推挽输出，默认映射）
 *
 * 约定：
 *   - `*_init[_with_config]` / `*_set_*` 统一返回 `stm_status_t`
 *   - 本驱动不启用 TIM2 更新中断，独占 TIM2_CH1 做 PWM 输出
 *
 * 须先 `bsp_clock_apply_profile()` 再 `bsp_board_init()`（打开 GPIOA / TIM2 时钟）。
 */

typedef struct {
  uint32_t pwm_hz;
  uint16_t duty_permille; /* 0 = 全低，1000 = 100% */
} tim2_ch1_pwm_config_t;

stm_status_t tim2_ch1_pwm_init_with_config(const tim2_ch1_pwm_config_t *config);
stm_status_t tim2_ch1_pwm_init_hz(uint32_t pwm_frequency_hz, uint16_t duty_permille);
stm_status_t tim2_ch1_pwm_set_duty_permille(uint16_t duty_permille);
stm_status_t tim2_ch1_pwm_set_config(const tim2_ch1_pwm_config_t *config);
stm_status_t tim2_ch1_pwm_set_hz(uint32_t pwm_frequency_hz, uint16_t duty_permille);
stm_status_t tim2_ch1_pwm_stop(void);

#endif
