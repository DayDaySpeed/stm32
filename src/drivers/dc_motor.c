/*
 * TB6612 + 有刷直流电机（TIM4_CH1/PB6 PWM，PB7=AIN1，AIN2 接 GND）
 */

#include "drivers/dc_motor.h"

#include "bsp/board_pins.h"
#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"

#include <stddef.h>
#include <stdint.h>

#define DC_MOTOR_PWM_HZ_DEFAULT     (10000UL)
#define DC_MOTOR_DUTY_MAX           (1000U)

static uint32_t g_ticks_per_period;
static uint16_t g_duty_permille;

static stm_status_t dc_motor_validate_duty(uint16_t duty_permille) {
  if (duty_permille > DC_MOTOR_DUTY_MAX) {
    return STM_ERR_INVALID_ARG;
  }
  return STM_OK;
}

static uint32_t dc_motor_duty_to_ccr1(uint16_t duty_permille,
                                      uint32_t ticks_per_period) {
  return ((uint32_t)duty_permille * ticks_per_period + 500U) / DC_MOTOR_DUTY_MAX;
}

static uint32_t dc_motor_tim4_input_clock_hz(void) {
  uint32_t pclk1_hz = bsp_clock_get_pclk1_hz();
  uint32_t tim_clk_hz = pclk1_hz;

  if (pclk1_hz != bsp_clock_get_hclk_hz()) {
    tim_clk_hz = pclk1_hz * 2UL;
  }
  return tim_clk_hz;
}

static stm_status_t dc_motor_resolve_timebase(uint32_t tim_clk_hz, uint32_t pwm_hz,
                                              uint16_t *psc_out, uint16_t *arr_out) {
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

void dc_motor_gpio_safe_early(void) {
  RCC_APB2ENR |= RCC_IOPBEN_BIT;
  GPIOB_CRL = (GPIOB_CRL & ~(BOARD_GPIO_PB6_CRL_MASK | BOARD_GPIO_PB7_CRL_MASK)) |
              BOARD_GPIO_PB6_OUT_PP_50MHZ | BOARD_GPIO_PB7_OUT_PP_50MHZ;
  GPIOB_BSRR = (1U << (6U + 16U)) | (1U << (BOARD_TB6612_AIN1_PIN + 16U));
}

static void dc_motor_gpio_init(void) {
  GPIOB_CRL = (GPIOB_CRL & ~(BOARD_GPIO_PB6_CRL_MASK | BOARD_GPIO_PB7_CRL_MASK)) |
              BOARD_GPIO_PB6_AF_PP_50MHZ | BOARD_GPIO_PB7_OUT_PP_50MHZ;
  GPIOB_BSRR = (1U << (BOARD_TB6612_AIN1_PIN + 16U));
}

static void dc_motor_apply_tb6612_outputs(uint16_t duty_permille) {
  if (duty_permille == 0U) {
    GPIOB_BSRR = (1U << (BOARD_TB6612_AIN1_PIN + 16U));
    if (g_ticks_per_period != 0U) {
      TIM4_CCR1 = 0U;
    }
    return;
  }

  GPIOB_BSRR = (1U << BOARD_TB6612_AIN1_PIN);
  if (g_ticks_per_period != 0U) {
    TIM4_CCR1 = dc_motor_duty_to_ccr1(duty_permille, g_ticks_per_period);
  }
}

static void dc_motor_tim4_apply_hw(uint16_t psc, uint16_t arr, uint16_t duty_permille) {
  uint32_t ticks = (uint32_t)arr + 1U;

  TIM4_CR1 &= ~TIM_CR1_CEN_BIT;
  TIM4_DIER &= ~TIM_DIER_UIE_BIT;

  TIM4_CCMR1 &= ~(TIM_CCMR1_CC1S_MASK | TIM_CCMR1_OC1M_MASK);
  TIM4_CCMR1 |= TIM_CCMR1_CC1S_OUT | TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE_BIT;

  TIM4_CCER &= ~TIM_CCER_CC1P_BIT;
  TIM4_CCER |= TIM_CCER_CC1E_BIT;

  TIM4_PSC = (uint32_t)psc;
  TIM4_ARR = (uint32_t)arr;
  TIM4_CR1 |= TIM_CR1_ARPE_BIT;

  g_ticks_per_period = ticks;
  dc_motor_apply_tb6612_outputs(duty_permille);

  TIM4_EGR = TIM_EGR_UG_BIT;
  TIM4_SR = ~TIM_SR_UIF_BIT;
  TIM4_CR1 |= TIM_CR1_CEN_BIT;
}

stm_status_t dc_motor_init_with_config(const dc_motor_config_t *config) {
  uint32_t tim_clk = 0U;
  uint16_t psc = 0U;
  uint16_t arr = 0U;
  stm_status_t st = STM_OK;

  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (config->pwm_hz == 0U) {
    return STM_ERR_INVALID_ARG;
  }

  st = dc_motor_validate_duty(config->duty_permille);
  if (st != STM_OK) {
    return st;
  }

  tim_clk = dc_motor_tim4_input_clock_hz();
  st = dc_motor_resolve_timebase(tim_clk, config->pwm_hz, &psc, &arr);
  if (st != STM_OK) {
    return st;
  }

  dc_motor_gpio_init();
  dc_motor_tim4_apply_hw(psc, arr, config->duty_permille);
  g_duty_permille = config->duty_permille;
  return STM_OK;
}

stm_status_t dc_motor_init(void) {
  const dc_motor_config_t config = {
      .pwm_hz = DC_MOTOR_PWM_HZ_DEFAULT,
      .duty_permille = 0U,
  };
  return dc_motor_init_with_config(&config);
}

stm_status_t dc_motor_set_duty_permille(uint16_t duty_permille) {
  stm_status_t st = dc_motor_validate_duty(duty_permille);
  if (st != STM_OK) {
    return st;
  }
  if (g_ticks_per_period == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  g_duty_permille = duty_permille;
  dc_motor_apply_tb6612_outputs(duty_permille);
  return STM_OK;
}

stm_status_t dc_motor_get_duty_permille(uint16_t *out_duty_permille) {
  if (out_duty_permille == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (g_ticks_per_period == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  *out_duty_permille = g_duty_permille;
  return STM_OK;
}

stm_status_t dc_motor_stop(void) {
  return dc_motor_set_duty_permille(0U);
}
