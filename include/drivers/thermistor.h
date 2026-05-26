#ifndef DRIVERS_THERMISTOR_H
#define DRIVERS_THERMISTOR_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * NTC 温度换算（不负责 ADC 采样，采样由 adc1_dual 完成）。
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
  THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN = 0,
  THERMISTOR_DIVIDER_NTC_UP_FIXED_DOWN = 1
} thermistor_divider_topology_t;

typedef struct {
  thermistor_adc_clock_source_t clock_source;
  thermistor_adc_prescaler_t adc_prescaler;
  thermistor_divider_topology_t divider_topology;
  uint32_t fixed_resistor_ohms; /* 分压固定电阻，如 10000 */
  uint32_t vdda_mv;             /* 参考电压 mV，如 3300 */
} thermistor_config_t;

/* 保存分压参数；须在 adc1_dual_init_with_config() 之后调用。 */
stm_status_t thermistor_init_with_config(const thermistor_config_t *config);
/* raw12：12 位 ADC 值；out_celsius_x10：输出 0.1°C 单位。不再触发采样。 */
stm_status_t thermistor_read_temperature_from_raw_blocking(
    uint16_t raw12, int16_t *out_celsius_x10);

#endif
