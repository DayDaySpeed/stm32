#ifndef COMMON_TIM_TIMEBASE_H
#define COMMON_TIM_TIMEBASE_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 定时器 PWM 时基：由 tim_clk 与 pwm_hz 反推 PSC/ARR。
 */

/* tim_clk_hz：定时器输入时钟；pwm_hz：目标 PWM 频率；psc_out/arr_out：输出分频。 */
stm_status_t stm_tim_resolve_timebase(uint32_t tim_clk_hz, uint32_t pwm_hz,
                                      uint16_t *psc_out, uint16_t *arr_out);
/* duty_permille：0..1000；ticks_per_period：ARR+1；返回 CCR 值。 */
uint32_t stm_tim_duty_permille_to_ccr(uint16_t duty_permille,
                                      uint32_t ticks_per_period);

#endif
