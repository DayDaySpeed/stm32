/*
 * TB6612 + 有刷直流电机（PB6=PWM，PB7=AIN1，PB5=AIN2 → AO1/AO2 可正反转）
 */

#include "drivers/dc_motor.h"

#include "bsp/board_pins.h"
#include "bsp/board_gpio.h"
#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"
#include "common/tim_timebase.h"

#include <stddef.h>
#include <stdint.h>

#define DC_MOTOR_PWM_HZ_DEFAULT     (10000UL)
#define DC_MOTOR_SPEED_MAX          (1000)

static uint32_t g_ticks_per_period;
static int16_t g_speed_permille;

static stm_status_t dc_motor_validate_speed(int16_t speed_permille) {
  if ((speed_permille < -DC_MOTOR_SPEED_MAX) || (speed_permille > DC_MOTOR_SPEED_MAX)) {
    return STM_ERR_INVALID_ARG;
  }
  return STM_OK;
}


static void dc_motor_set_ain1(uint8_t level) {
  board_gpio_write(BOARD_GPIO_MOTOR_BSRR_REG, BOARD_GPIO_MOTOR_AIN1_PIN, level);
}

static void dc_motor_set_ain2(uint8_t level) {
  board_gpio_write(BOARD_GPIO_MOTOR_BSRR_REG, BOARD_GPIO_MOTOR_AIN2_PIN, level);
}

void dc_motor_gpio_safe_early(void) {
  board_gpio_enable_port_b_clock();
  board_gpio_apply_crl(GPIOB_CRL, BOARD_GPIO_MOTOR_GPIO_MASK,
                       BOARD_GPIO_MOTOR_MODE_SAFE);
  board_gpio_write(BOARD_GPIO_MOTOR_BSRR_REG, BOARD_GPIO_MOTOR_PWM_PIN, 0U);
  board_gpio_write(BOARD_GPIO_MOTOR_BSRR_REG, BOARD_GPIO_MOTOR_AIN1_PIN, 0U);
  board_gpio_write(BOARD_GPIO_MOTOR_BSRR_REG, BOARD_GPIO_MOTOR_AIN2_PIN, 0U);
}

static void dc_motor_gpio_init(void) {
  board_gpio_apply_crl(GPIOB_CRL, BOARD_GPIO_MOTOR_GPIO_MASK,
                       BOARD_GPIO_MOTOR_MODE_INIT);
  dc_motor_set_ain1(0U);
  dc_motor_set_ain2(0U);
}

static void dc_motor_apply_tb6612_outputs(int16_t speed_permille) {
  if (speed_permille == 0) {
    dc_motor_set_ain1(0U);
    dc_motor_set_ain2(0U);
    if (g_ticks_per_period != 0U) {
      TIM4_CCR1 = 0U;
    }
    return;
  }

  {
    uint16_t duty = (uint16_t)(speed_permille > 0 ? speed_permille : -speed_permille);

    if (speed_permille > 0) {
      dc_motor_set_ain1(1U);
      dc_motor_set_ain2(0U);
    } else {
      dc_motor_set_ain1(0U);
      dc_motor_set_ain2(1U);
    }

    if (g_ticks_per_period != 0U) {
      TIM4_CCR1 = stm_tim_duty_permille_to_ccr(duty, g_ticks_per_period);
    }
  }
}

static void dc_motor_tim4_apply_hw(uint16_t psc, uint16_t arr, int16_t speed_permille) {
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
  dc_motor_apply_tb6612_outputs(speed_permille);

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

  st = dc_motor_validate_speed(config->speed_permille);
  if (st != STM_OK) {
    return st;
  }

  tim_clk = bsp_clock_get_apb1_timer_hz();
  st = stm_tim_resolve_timebase(tim_clk, config->pwm_hz, &psc, &arr);
  if (st != STM_OK) {
    return st;
  }

  dc_motor_gpio_init();
  dc_motor_tim4_apply_hw(psc, arr, config->speed_permille);
  g_speed_permille = config->speed_permille;
  return STM_OK;
}

stm_status_t dc_motor_init(void) {
  const dc_motor_config_t config = {
      .pwm_hz = DC_MOTOR_PWM_HZ_DEFAULT,
      .speed_permille = 0,
  };
  return dc_motor_init_with_config(&config);
}

stm_status_t dc_motor_set_speed_signed(int16_t speed_permille) {
  stm_status_t st = dc_motor_validate_speed(speed_permille);
  if (st != STM_OK) {
    return st;
  }
  if (g_ticks_per_period == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  g_speed_permille = speed_permille;
  dc_motor_apply_tb6612_outputs(speed_permille);
  return STM_OK;
}

stm_status_t dc_motor_get_speed_signed(int16_t *out_speed_permille) {
  if (out_speed_permille == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (g_ticks_per_period == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  *out_speed_permille = g_speed_permille;
  return STM_OK;
}

stm_status_t dc_motor_set_duty_permille(uint16_t duty_permille) {
  if (duty_permille > DC_MOTOR_SPEED_MAX) {
    return STM_ERR_INVALID_ARG;
  }
  return dc_motor_set_speed_signed((int16_t)duty_permille);
}

stm_status_t dc_motor_stop(void) { return dc_motor_set_speed_signed(0); }
