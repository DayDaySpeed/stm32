#ifndef COMMON_TIM_TIMEBASE_H
#define COMMON_TIM_TIMEBASE_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 通用定时器 PWM 时基求解与占空比换算。
 *
 * 关系式：total_ticks = tim_clk_hz / pwm_hz = (PSC+1)(ARR+1)
 * 占空比千分比 → CCR：duty_permille / 1000 * ticks_per_period（四舍五入）
 */

stm_status_t stm_tim_resolve_timebase(uint32_t tim_clk_hz, uint32_t pwm_hz,
                                      uint16_t *psc_out, uint16_t *arr_out);

uint32_t stm_tim_duty_permille_to_ccr(uint16_t duty_permille,
                                      uint32_t ticks_per_period);

#endif
