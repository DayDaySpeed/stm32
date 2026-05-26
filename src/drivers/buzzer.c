/*
 * 有源蜂鸣器 —— GPIO 电平驱动（默认 PA4）
 */

#include "drivers/buzzer.h"

#include "bsp/board_pins.h"
#include "bsp/board_gpio.h"
#include "bsp/stm32f103_regs.h"
#include "drivers/systick.h"

#include <stddef.h>

static uint8_t s_initialized;
static uint8_t s_active_high = 1U;

static void buzzer_apply_level(uint8_t on) {
  uint8_t level = on ? 1U : 0U;

  if (s_active_high == 0U) {
    level = (uint8_t)(1U - level);
  }

  if (level != 0U) {
    board_gpio_write(BOARD_GPIO_BUZZER_BSRR_REG, BOARD_GPIO_BUZZER_PIN, 1U);
  } else {
    board_gpio_write(BOARD_GPIO_BUZZER_BSRR_REG, BOARD_GPIO_BUZZER_PIN, 0U);
  }
}

stm_status_t buzzer_init_with_config(const buzzer_config_t *config) {
  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  s_active_high = (config->active_high != 0U) ? 1U : 0U;

  board_gpio_apply_crl(BOARD_GPIO_BUZZER_CR_REG, BOARD_GPIO_BUZZER_CR_MASK,
                       BOARD_GPIO_BUZZER_MODE_OUT);
  buzzer_apply_level(0U);
  s_initialized = 1U;
  return STM_OK;
}

stm_status_t buzzer_on(void) {
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  buzzer_apply_level(1U);
  return STM_OK;
}

stm_status_t buzzer_off(void) {
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  buzzer_apply_level(0U);
  return STM_OK;
}

stm_status_t buzzer_beep_blocking(uint32_t duration_ms) {
  stm_status_t st = buzzer_on();
  if (st != STM_OK) {
    return st;
  }
  if (duration_ms > 0U) {
    systick_delay_ms(duration_ms);
  }
  return buzzer_off();
}
