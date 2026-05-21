/*
 * TIM1：CH1/PA8（LDR 灯）、CH4/PA11（NTC 灯）
 */

#include "drivers/sensor_led.h"

#include "bsp/board_pins.h"
#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"
#include "common/tim_timebase.h"

#include <stddef.h>

#define SENSOR_LED_PWM_HZ_DEFAULT (1000UL)

static uint32_t g_ticks_per_period;


static void sensor_led_gpio_init(void) {
  GPIOA_CRH = (GPIOA_CRH & ~(BOARD_GPIO_PA8_CRH_MASK | BOARD_GPIO_PA11_CRH_MASK)) |
              BOARD_GPIO_PA8_AF_PP_50MHZ | BOARD_GPIO_PA11_AF_PP_50MHZ;
}

static void sensor_led_apply_hw(uint16_t psc, uint16_t arr, uint16_t ldr_permille,
                                uint16_t ntc_permille) {
  uint32_t ticks = (uint32_t)arr + 1U;

  TIM1_CR1 &= ~TIM_CR1_CEN_BIT;
  TIM1_DIER &= ~TIM_DIER_UIE_BIT;

  TIM1_CCMR1 &= ~(TIM_CCMR1_CC1S_MASK | TIM_CCMR1_OC1M_MASK);
  TIM1_CCMR1 |= TIM_CCMR1_CC1S_OUT | TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE_BIT;

  TIM1_CCMR2 &= ~(TIM_CCMR2_OC4M_MASK);
  TIM1_CCMR2 |= TIM_CCMR2_OC4M_PWM1 | TIM_CCMR2_OC4PE_BIT;

  TIM1_CCER &= ~(TIM_CCER_CC1P_BIT | TIM_CCER_CC4P_BIT);
  TIM1_CCER |= TIM_CCER_CC1E_BIT | TIM_CCER_CC4E_BIT;

  TIM1_PSC = (uint32_t)psc;
  TIM1_ARR = (uint32_t)arr;
  TIM1_CR1 |= TIM_CR1_ARPE_BIT;
  TIM1_CCR1 = stm_tim_duty_permille_to_ccr(ldr_permille, ticks);
  TIM1_CCR4 = stm_tim_duty_permille_to_ccr(ntc_permille, ticks);

  TIM1_BDTR = TIM_BDTR_MOE_BIT;
  TIM1_EGR = TIM_EGR_UG_BIT;
  TIM1_SR = ~TIM_SR_UIF_BIT;
  TIM1_CR1 |= TIM_CR1_CEN_BIT;

  g_ticks_per_period = ticks;
}

stm_status_t sensor_led_init_with_config(const sensor_led_config_t *config) {
  uint32_t tim_clk;
  uint16_t psc = 0U;
  uint16_t arr = 0U;
  stm_status_t st;

  if ((config == NULL) || (config->pwm_hz == 0U)) {
    return STM_ERR_INVALID_ARG;
  }

  tim_clk = bsp_clock_get_apb2_timer_hz();
  st = stm_tim_resolve_timebase(tim_clk, config->pwm_hz, &psc, &arr);
  if (st != STM_OK) {
    return st;
  }

  sensor_led_gpio_init();
  sensor_led_apply_hw(psc, arr, 0U, 0U);
  return STM_OK;
}

stm_status_t sensor_led_init(void) {
  const sensor_led_config_t config = {.pwm_hz = SENSOR_LED_PWM_HZ_DEFAULT};
  return sensor_led_init_with_config(&config);
}

static stm_status_t sensor_led_validate_duty(uint16_t duty_permille) {
  if (duty_permille > 1000U) {
    return STM_ERR_INVALID_ARG;
  }
  if (g_ticks_per_period == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  return STM_OK;
}

stm_status_t sensor_led_set_ldr_permille(uint16_t duty_permille) {
  stm_status_t st = sensor_led_validate_duty(duty_permille);
  if (st != STM_OK) {
    return st;
  }
  TIM1_CCR1 = stm_tim_duty_permille_to_ccr(duty_permille, g_ticks_per_period);
  return STM_OK;
}

stm_status_t sensor_led_set_ntc_permille(uint16_t duty_permille) {
  stm_status_t st = sensor_led_validate_duty(duty_permille);
  if (st != STM_OK) {
    return st;
  }
  TIM1_CCR4 = stm_tim_duty_permille_to_ccr(duty_permille, g_ticks_per_period);
  return STM_OK;
}
