#ifndef DRIVERS_THERMISTOR_H
#define DRIVERS_THERMISTOR_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * NTC 热敏电阻温度换算 —— 不负责 ADC 采样（由 adc1_dual 完成）。
 *
 * 默认 10k NTC B3950 查表插值；分压拓扑与固定电阻在 config 中指定。
 * 配合 bsp_analog_sensors_read_pair_average 一次 SCAN 读两路时使用 from_raw。
 */

typedef enum {
  THERMISTOR_ADC_CLOCK_SOURCE_PCLK2 = 0
} thermistor_adc_clock_source_t;

typedef enum {
  THERMISTOR_ADC_PRESCALER_AUTO = 0,
  THERMISTOR_ADC_PRESCALER_DIV2 = 2,
  THERMISTOR_ADC_PRESCALER_DIV4 = 4,
  THERMISTOR_ADC_PRESCALER_DIV6 = 6,
  THERMISTOR_ADC_PRESCALER_DIV8 = 8
} thermistor_adc_prescaler_t;

typedef enum {
  THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN = 0, /* 上拉固定电阻、下拉 NTC */
  THERMISTOR_DIVIDER_NTC_UP_FIXED_DOWN = 1
} thermistor_divider_topology_t;

typedef struct {
  thermistor_adc_clock_source_t clock_source;
  thermistor_adc_prescaler_t adc_prescaler;
  thermistor_divider_topology_t divider_topology;
  uint32_t fixed_resistor_ohms;
  uint32_t vdda_mv;
} thermistor_config_t;

stm_status_t thermistor_init_with_config(const thermistor_config_t *config);
stm_status_t thermistor_read_temperature_from_raw_blocking(
    uint16_t raw12, int16_t *out_celsius_x10);

#endif
