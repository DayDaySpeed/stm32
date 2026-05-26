/*
 * 状态呼吸灯 —— TIM2 CH1 边沿对齐 PWM。
 * 引脚由 board_pin_mux.h / board_pins.h 解析；占空比运行时改 CCR1 即可。
 */

#include "drivers/breathing_led.h"

#include "bsp/board_pins.h"
#include "bsp/board_gpio.h"
#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"
#include "common/tim_timebase.h"

#include <stddef.h>
#include <stdint.h>

static uint32_t g_ticks_per_period;

static stm_status_t breathing_led_validate_duty(uint16_t duty_permille);

static stm_status_t breathing_led_validate_config(
    const breathing_led_config_t *config) {
  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (config->carrier_hz == 0U) {
    return STM_ERR_INVALID_ARG;
  }
  return breathing_led_validate_duty(config->duty_permille);
}

static stm_status_t breathing_led_validate_duty(uint16_t duty_permille) {
  if (duty_permille > 1000U) {
    return STM_ERR_INVALID_ARG;
  }
  return STM_OK;
}

static void breathing_led_gpio_init(void) {
  if (BOARD_AFIO_STATUS_LED_REMAP != 0U) {
    board_gpio_afio_apply(AFIO_MAPR_TIM2_REMAP_MASK, BOARD_AFIO_STATUS_LED_REMAP);
  }
  if (BOARD_STATUS_LED_PIN_MUX == BOARD_STATUS_LED_MUX_PA0_TIM2_CH1) {
    board_gpio_apply_crl(GPIOA_CRL, BOARD_GPIO_STATUS_LED_CR_MASK,
                         BOARD_GPIO_STATUS_LED_MODE_AF);
  } else {
    board_gpio_apply_crh(GPIOA_CRH, BOARD_GPIO_STATUS_LED_CR_MASK,
                         BOARD_GPIO_STATUS_LED_MODE_AF);
  }
}

static void breathing_led_apply_hw(uint16_t psc, uint16_t arr,
                                   uint16_t duty_permille) {
  uint32_t ticks = (uint32_t)arr + 1U;

  TIM2_CR1 &= ~TIM_CR1_CEN_BIT;
  TIM2_DIER &= ~TIM_DIER_UIE_BIT;

  TIM2_CCMR1 &= ~(TIM_CCMR1_CC1S_MASK | TIM_CCMR1_OC1M_MASK);
  TIM2_CCMR1 |= TIM_CCMR1_CC1S_OUT | TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE_BIT;

  TIM2_CCER &= ~TIM_CCER_CC1P_BIT;
  TIM2_CCER |= TIM_CCER_CC1E_BIT;

  TIM2_PSC = (uint32_t)psc;
  TIM2_ARR = (uint32_t)arr;
  TIM2_CR1 |= TIM_CR1_ARPE_BIT;
  TIM2_CCR1 = stm_tim_duty_permille_to_ccr(duty_permille, ticks);

  TIM2_EGR = TIM_EGR_UG_BIT;
  TIM2_SR = ~TIM_SR_UIF_BIT;
  TIM2_CR1 |= TIM_CR1_CEN_BIT;

  g_ticks_per_period = ticks;
}

/* 见 breathing_led_init_with_config 头文件说明。 */
stm_status_t breathing_led_init_with_config(const breathing_led_config_t *config) {
  stm_status_t st = breathing_led_validate_config(config);
  if (st != STM_OK) {
    return st;
  }

  uint32_t tim_clk = bsp_clock_get_apb1_timer_hz();
  uint16_t psc = 0U;
  uint16_t arr = 0U;

  st = stm_tim_resolve_timebase(tim_clk, config->carrier_hz, &psc, &arr);
  if (st != STM_OK) {
    return st;
  }

  breathing_led_gpio_init();
  breathing_led_apply_hw(psc, arr, config->duty_permille);
  return STM_OK;
}

/* duty_permille：0..1000，仅更新 TIM2_CCR1。 */
stm_status_t breathing_led_set_duty_permille(uint16_t duty_permille) {
  stm_status_t st = breathing_led_validate_duty(duty_permille);
  if (st != STM_OK) {
    return st;
  }
  if (g_ticks_per_period == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  TIM2_CCR1 = stm_tim_duty_permille_to_ccr(duty_permille, g_ticks_per_period);
  return STM_OK;
}
