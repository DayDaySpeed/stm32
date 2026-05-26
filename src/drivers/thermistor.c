/*
 * 热敏电阻模块 —— 温度换算（ADC 采样由 adc1_dual_scan_dma 负责）
 */

#include "drivers/thermistor.h"

#include <stddef.h>
#include <stdint.h>

#define THERMISTOR_ADC_MAX (4095U)

typedef struct {
  int16_t temperature_celsius_x10;
  uint32_t resistance_ohms;
} thermistor_lut_entry_t;

static const thermistor_lut_entry_t g_ntc_10k_b3950_table[] = {
    {-400, 401860U}, {-350, 281577U}, {-300, 200204U}, {-250, 144317U},
    {-200, 105385U}, {-150, 77898U},  {-100, 58246U},  {-50, 44026U},
    {0, 33621U},     {50, 25925U},    {100, 20175U},   {150, 15837U},
    {200, 12535U},   {250, 10000U},   {300, 8037U},    {350, 6506U},
    {400, 5301U},    {450, 4348U},    {500, 3588U},    {550, 2978U},
    {600, 2486U},    {650, 2086U},    {700, 1760U},    {750, 1492U},
    {800, 1270U},    {850, 1087U},    {900, 934U},     {950, 805U},
    {1000, 698U},    {1050, 606U},    {1100, 529U},    {1150, 463U},
    {1200, 407U},    {1250, 359U},
};

static uint8_t s_initialized;
static thermistor_config_t s_config;

static stm_status_t thermistor_validate_config(const thermistor_config_t *config) {
  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (config->clock_source != THERMISTOR_ADC_CLOCK_SOURCE_PCLK2) {
    return STM_ERR_INVALID_ARG;
  }
  switch (config->adc_prescaler) {
  case THERMISTOR_ADC_PRESCALER_AUTO:
  case THERMISTOR_ADC_PRESCALER_DIV2:
  case THERMISTOR_ADC_PRESCALER_DIV4:
  case THERMISTOR_ADC_PRESCALER_DIV6:
  case THERMISTOR_ADC_PRESCALER_DIV8:
    break;
  default:
    return STM_ERR_INVALID_ARG;
  }
  switch (config->divider_topology) {
  case THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN:
  case THERMISTOR_DIVIDER_NTC_UP_FIXED_DOWN:
    break;
  default:
    return STM_ERR_INVALID_ARG;
  }
  if ((config->fixed_resistor_ohms == 0U) || (config->vdda_mv == 0U)) {
    return STM_ERR_INVALID_ARG;
  }
  return STM_OK;
}

static stm_status_t thermistor_raw_to_resistance_ohms(uint16_t raw12,
                                                      uint32_t *out_ohms) {
  uint32_t denominator = 0U;
  uint64_t numerator = 0U;

  if (out_ohms == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  if (s_config.divider_topology == THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN) {
    denominator = THERMISTOR_ADC_MAX - (uint32_t)raw12;
    if (denominator == 0U) {
      return STM_ERR_INVALID_ARG;
    }
    numerator = (uint64_t)s_config.fixed_resistor_ohms * (uint32_t)raw12;
  } else {
    denominator = (uint32_t)raw12;
    if (denominator == 0U) {
      return STM_ERR_INVALID_ARG;
    }
    numerator = (uint64_t)s_config.fixed_resistor_ohms *
                (THERMISTOR_ADC_MAX - (uint32_t)raw12);
  }

  *out_ohms = (uint32_t)((numerator + (denominator / 2U)) / denominator);
  return STM_OK;
}

static stm_status_t thermistor_lookup_temperature_celsius_x10(
    uint32_t resistance_ohms, int16_t *out_celsius_x10) {
  uint32_t table_size = (uint32_t)(sizeof(g_ntc_10k_b3950_table) /
                                   sizeof(g_ntc_10k_b3950_table[0]));

  if (out_celsius_x10 == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (table_size < 2U) {
    return STM_ERR_INVALID_ARG;
  }

  if (resistance_ohms >= g_ntc_10k_b3950_table[0].resistance_ohms) {
    *out_celsius_x10 = g_ntc_10k_b3950_table[0].temperature_celsius_x10;
    return STM_OK;
  }
  if (resistance_ohms <= g_ntc_10k_b3950_table[table_size - 1U].resistance_ohms) {
    *out_celsius_x10 =
        g_ntc_10k_b3950_table[table_size - 1U].temperature_celsius_x10;
    return STM_OK;
  }

  for (uint32_t i = 0U; i + 1U < table_size; i++) {
    uint32_t r0 = g_ntc_10k_b3950_table[i].resistance_ohms;
    uint32_t r1 = g_ntc_10k_b3950_table[i + 1U].resistance_ohms;
    int16_t t0 = g_ntc_10k_b3950_table[i].temperature_celsius_x10;
    int16_t t1 = g_ntc_10k_b3950_table[i + 1U].temperature_celsius_x10;

    if ((resistance_ohms <= r0) && (resistance_ohms >= r1)) {
      uint32_t dr = r0 - r1;
      uint32_t offset_r = r0 - resistance_ohms;
      int32_t dt = (int32_t)t1 - (int32_t)t0;
      int32_t interp = (int32_t)(((int64_t)offset_r * dt + (dr / 2U)) / dr);
      *out_celsius_x10 = (int16_t)((int32_t)t0 + interp);
      return STM_OK;
    }
  }

  return STM_ERR_INVALID_ARG;
}

stm_status_t thermistor_init_with_config(const thermistor_config_t *config) {
  stm_status_t st = thermistor_validate_config(config);
  if (st != STM_OK) {
    return st;
  }

  s_config = *config;
  s_initialized = 1U;
  return STM_OK;
}

stm_status_t thermistor_read_temperature_from_raw_blocking(
    uint16_t raw12, int16_t *out_celsius_x10) {
  uint32_t resistance_ohms = 0U;
  stm_status_t st = STM_OK;

  if (out_celsius_x10 == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  st = thermistor_raw_to_resistance_ohms(raw12, &resistance_ohms);
  if (st != STM_OK) {
    return st;
  }

  return thermistor_lookup_temperature_celsius_x10(resistance_ohms,
                                                   out_celsius_x10);
}
