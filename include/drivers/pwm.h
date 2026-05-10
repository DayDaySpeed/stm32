#ifndef DRIVERS_PWM_H
#define DRIVERS_PWM_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * TIM2 通道 1 PWM（引脚 PA0，复用推挽输出，默认映射）。
 *
 * 独占 TIM2：本模块把 TIM2 配置为 PWM 模式，不使用 TIM2 更新中断。
 *
 * 须先 `bsp_clock_apply_profile()` 再 `bsp_board_init()`（打开 GPIOA / TIM2 时钟）。
 */

/* duty_permille：占空比千分比，0 = 全低，1000 = 100%。 */
stm_status_t tim2_ch1_pwm_init_hz(uint32_t pwm_frequency_hz, uint16_t duty_permille);

stm_status_t tim2_ch1_pwm_set_duty_permille(uint16_t duty_permille);

/* 改频率并可选同时更新占空比；duty_permille 范围同上。 */
stm_status_t tim2_ch1_pwm_set_hz(uint32_t pwm_frequency_hz, uint16_t duty_permille);

/* 停止计数并关闭 CH1 引脚输出（CC1E=0）。 */
void tim2_ch1_pwm_stop(void);

#endif
