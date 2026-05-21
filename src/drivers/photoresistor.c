/*
 * 光敏电阻模块 —— 经 ADC1 双通道 SCAN+DMA 读取 IN1（PA1）
 *
 * 硬件与采样细节见 drivers/adc1_dual_scan_dma.c。
 * 本文件只负责光敏侧的初始化包装与原始值/电压换算。
 */

#include "drivers/photoresistor.h"

#include "drivers/adc1_dual_scan_dma.h"

#include <stddef.h>
#include <stdint.h>

#define PHOTO_VDDA_MV_DEFAULT (3300U)
#define PHOTO_ADC_MAX         (4095U)

static uint8_t s_initialized;

static adc1_dual_config_t photoresistor_to_adc_config(
    const photoresistor_config_t *config) {
  adc1_dual_config_t out = {
      .clock_source = ADC1_DUAL_CLOCK_SOURCE_PCLK2,
      .adc_prescaler = ADC1_DUAL_PRESCALER_AUTO,
  };

  if (config != NULL) {
    out.adc_prescaler = (adc1_dual_prescaler_t)config->adc_prescaler;
  }
  return out;
}

/*
 * 函数名：photoresistor_init_with_config
 * 参数：
 *   - config：光敏驱动配置（ADC 分频策略会传给共享 ADC 模块）
 * 作用：
 *   初始化共享 ADC1 双通道 SCAN+DMA，并标记光敏驱动可用。
 * 返回值：
 *   - STM_OK：成功
 *   - 其他：共享 ADC 初始化错误
 */
stm_status_t photoresistor_init_with_config(const photoresistor_config_t *config) {
  adc1_dual_config_t adc_cfg = photoresistor_to_adc_config(config);
  stm_status_t st = adc1_dual_init_with_config(&adc_cfg);
  if (st != STM_OK) {
    return st;
  }
  s_initialized = 1U;
  return STM_OK;
}

stm_status_t photoresistor_init(void) {
  const photoresistor_config_t config = {
      .clock_source = PHOTO_ADC_CLOCK_SOURCE_PCLK2,
      .adc_prescaler = PHOTO_ADC_PRESCALER_AUTO,
  };
  return photoresistor_init_with_config(&config);
}

stm_status_t photoresistor_read_raw_blocking(uint16_t *out_raw12) {
  uint16_t pair[ADC1_DUAL_SLOT_COUNT] = {0U, 0U, 0U};
  stm_status_t st = STM_OK;

  if (out_raw12 == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  st = adc1_dual_read_pair_blocking(pair);
  if (st != STM_OK) {
    return st;
  }

  *out_raw12 = pair[ADC1_DUAL_SLOT_PHOTO];
  return STM_OK;
}

stm_status_t photoresistor_read_raw_average_blocking(uint16_t *out_raw12,
                                                     uint8_t sample_count) {
  uint16_t pair[ADC1_DUAL_SLOT_COUNT] = {0U, 0U, 0U};
  stm_status_t st = STM_OK;

  if ((out_raw12 == NULL) || (sample_count == 0U)) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  st = adc1_dual_read_pair_average_blocking(pair, sample_count);
  if (st != STM_OK) {
    return st;
  }

  *out_raw12 = pair[ADC1_DUAL_SLOT_PHOTO];
  return STM_OK;
}

stm_status_t photoresistor_read_millivolts_blocking(uint32_t *out_mv) {
  uint16_t raw = 0U;
  stm_status_t st = STM_OK;

  if (out_mv == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  st = photoresistor_read_raw_blocking(&raw);
  if (st != STM_OK) {
    return st;
  }

  *out_mv = ((uint32_t)raw * PHOTO_VDDA_MV_DEFAULT + (PHOTO_ADC_MAX / 2U)) /
            PHOTO_ADC_MAX;
  return STM_OK;
}

stm_status_t photoresistor_read_raw(uint16_t *out_raw12) {
  return photoresistor_read_raw_blocking(out_raw12);
}

stm_status_t photoresistor_read_raw_average(uint16_t *out_raw12,
                                            uint8_t sample_count) {
  return photoresistor_read_raw_average_blocking(out_raw12, sample_count);
}

stm_status_t photoresistor_read_millivolts(uint32_t *out_mv) {
  return photoresistor_read_millivolts_blocking(out_mv);
}
