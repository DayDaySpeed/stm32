/*
 * 反射红外模块 —— ADC1 SCAN+DMA 的 IN3（PA3）
 */

#include "drivers/ir_reflect.h"

#include "drivers/adc1_dual_scan_dma.h"

#include <stddef.h>
#include <stdint.h>

#define IR_REFLECT_VDDA_MV_DEFAULT (3300U)
#define IR_REFLECT_ADC_MAX         (4095U)

static uint8_t s_initialized;

static adc1_dual_config_t ir_reflect_to_adc_config(const ir_reflect_config_t *config) {
  adc1_dual_config_t out = {
      .clock_source = ADC1_DUAL_CLOCK_SOURCE_PCLK2,
      .adc_prescaler = ADC1_DUAL_PRESCALER_AUTO,
  };

  if (config != NULL) {
    out.adc_prescaler = (adc1_dual_prescaler_t)config->adc_prescaler;
  }
  return out;
}

stm_status_t ir_reflect_init_with_config(const ir_reflect_config_t *config) {
  adc1_dual_config_t adc_cfg = ir_reflect_to_adc_config(config);
  stm_status_t st = adc1_dual_init_with_config(&adc_cfg);
  if (st != STM_OK) {
    return st;
  }
  s_initialized = 1U;
  return STM_OK;
}

stm_status_t ir_reflect_init(void) {
  const ir_reflect_config_t config = {
      .clock_source = IR_REFLECT_ADC_CLOCK_SOURCE_PCLK2,
      .adc_prescaler = IR_REFLECT_ADC_PRESCALER_AUTO,
  };
  return ir_reflect_init_with_config(&config);
}

stm_status_t ir_reflect_read_raw_blocking(uint16_t *out_raw12) {
  uint16_t sample[ADC1_DUAL_SLOT_COUNT] = {0U, 0U, 0U};
  stm_status_t st = STM_OK;

  if (out_raw12 == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  st = adc1_dual_read_all_blocking(sample);
  if (st != STM_OK) {
    return st;
  }

  *out_raw12 = sample[ADC1_DUAL_SLOT_IR_REFLECT];
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

stm_status_t ir_reflect_read_millivolts_blocking(uint32_t *out_mv) {
  uint16_t raw = 0U;
  stm_status_t st = STM_OK;

  if (out_mv == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  st = ir_reflect_read_raw_blocking(&raw);
  if (st != STM_OK) {
    return st;
  }

  *out_mv = ((uint32_t)raw * IR_REFLECT_VDDA_MV_DEFAULT +
             (IR_REFLECT_ADC_MAX / 2U)) /
            IR_REFLECT_ADC_MAX;
  return STM_OK;
}
