/*
 * 反射红外模块 —— ADC1 SCAN+DMA 的 IN3
 */

#include "drivers/ir_reflect.h"

#include "drivers/adc1_dual_scan_dma.h"

#include <stddef.h>
#include <stdint.h>

static uint8_t s_initialized;

stm_status_t ir_reflect_init_with_config(const ir_reflect_config_t *config) {
  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (config->clock_source != IR_REFLECT_ADC_CLOCK_SOURCE_PCLK2) {
    return STM_ERR_INVALID_ARG;
  }
  switch (config->adc_prescaler) {
  case IR_REFLECT_ADC_PRESCALER_AUTO:
  case IR_REFLECT_ADC_PRESCALER_DIV2:
  case IR_REFLECT_ADC_PRESCALER_DIV4:
  case IR_REFLECT_ADC_PRESCALER_DIV6:
  case IR_REFLECT_ADC_PRESCALER_DIV8:
    break;
  default:
    return STM_ERR_INVALID_ARG;
  }

  (void)config;
  s_initialized = 1U;
  return STM_OK;
}

stm_status_t ir_reflect_read_raw_average_blocking(uint16_t *out_raw12,
                                                  uint8_t sample_count) {
  uint16_t sample[ADC1_DUAL_SLOT_COUNT] = {0U, 0U, 0U};
  stm_status_t st = STM_OK;

  if ((out_raw12 == NULL) || (sample_count == 0U)) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  st = adc1_dual_read_all_average_blocking(sample, sample_count);
  if (st != STM_OK) {
    return st;
  }

  *out_raw12 = sample[ADC1_DUAL_SLOT_IR_REFLECT];
  return STM_OK;
}
