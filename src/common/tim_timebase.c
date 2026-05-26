/*
 * 通用定时器 PWM 时基求解：由 tim_clk 与目标 pwm_hz 反推 PSC/ARR。
 */
#include "common/tim_timebase.h"

#include <stddef.h>
#include <stdint.h>

stm_status_t stm_tim_resolve_timebase(uint32_t tim_clk_hz, uint32_t pwm_hz,
                                      uint16_t *psc_out, uint16_t *arr_out) {
  if ((psc_out == NULL) || (arr_out == NULL)) {
    return STM_ERR_INVALID_ARG;
  }
  if ((pwm_hz == 0U) || (tim_clk_hz < pwm_hz)) {
    return STM_ERR_INVALID_ARG;
  }

  uint32_t total = tim_clk_hz / pwm_hz;

  for (uint32_t psc = 1U; psc <= 0x10000U; psc++) {
    if ((total % psc) != 0U) {
      continue;
    }
    uint32_t arr = total / psc;
    if ((arr >= 1U) && (arr <= 0x10000U)) {
      *psc_out = (uint16_t)(psc - 1U);
      *arr_out = (uint16_t)(arr - 1U);
      return STM_OK;
    }
  }

  {
    uint32_t psc = (total + 0xFFFFU) / 0x10000U;
    uint32_t arr = total / psc;
    *psc_out = (uint16_t)(psc - 1U);
    *arr_out = (uint16_t)(arr - 1U);
  }
  return STM_OK;
}

uint32_t stm_tim_duty_permille_to_ccr(uint16_t duty_permille,
                                      uint32_t ticks_per_period) {
  return ((uint32_t)duty_permille * ticks_per_period + 500U) / 1000U;
}
