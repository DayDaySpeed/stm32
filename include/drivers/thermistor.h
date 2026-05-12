#ifndef DRIVERS_THERMISTOR_H
#define DRIVERS_THERMISTOR_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * STM32F103 的 ADC 时钟源固定来自 PCLK2，只能再经 ADCPRE 做 /2 /4 /6 /8。
 * 因此热敏驱动的“时钟配置”本质上是选择 ADC 预分频策略。
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

/*
 * 分压拓扑：
 *   - FIXED_UP_NTC_DOWN：上拉固定电阻、下拉 NTC，温度升高时 AO 一般下降
 *   - NTC_UP_FIXED_DOWN：上拉 NTC、下拉固定电阻，温度升高时 AO 一般上升
 */
typedef enum {
  THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN = 0,
  THERMISTOR_DIVIDER_NTC_UP_FIXED_DOWN = 1
} thermistor_divider_topology_t;

typedef struct {
  thermistor_adc_clock_source_t clock_source;
  thermistor_adc_prescaler_t adc_prescaler;
  thermistor_divider_topology_t divider_topology;
  uint32_t fixed_resistor_ohms;
  uint32_t vdda_mv;
} thermistor_config_t;

/*
 * 热敏电阻模块驱动（默认 PA2 = ADC1_IN2）
 *
 * 约定硬件：
 *   - 常见 10k NTC（B=3950）模拟模块
 *   - AO 接到 PA2，电源为 3.3V
 *
 * 说明：
 *   - 原始值/电压/电阻读取只依赖配置里的分压参数
 *   - 温度换算使用内置的 10k NTC B3950 查表插值，结果为近似值
 */

stm_status_t thermistor_init_with_config(const thermistor_config_t *config);
stm_status_t thermistor_init(void);

stm_status_t thermistor_read_raw_blocking(uint16_t *out_raw12);
stm_status_t thermistor_read_raw_average_blocking(uint16_t *out_raw12,
                                                  uint8_t sample_count);
stm_status_t thermistor_read_millivolts_blocking(uint32_t *out_mv);
stm_status_t thermistor_read_resistance_ohms_blocking(uint32_t *out_ohms);
stm_status_t thermistor_read_temperature_celsius_x10_blocking(
    int16_t *out_celsius_x10);

/* 兼容包装：新代码优先使用 `_blocking` 命名。 */
stm_status_t thermistor_read_raw(uint16_t *out_raw12);
stm_status_t thermistor_read_raw_average(uint16_t *out_raw12,
                                         uint8_t sample_count);
stm_status_t thermistor_read_millivolts(uint32_t *out_mv);
stm_status_t thermistor_read_resistance_ohms(uint32_t *out_ohms);
stm_status_t thermistor_read_temperature_celsius_x10(int16_t *out_celsius_x10);

#endif
