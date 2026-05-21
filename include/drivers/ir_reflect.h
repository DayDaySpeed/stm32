#ifndef DRIVERS_IR_REFLECT_H
#define DRIVERS_IR_REFLECT_H

#include <stdint.h>

#include "common/stm_status.h"

typedef enum {
  IR_REFLECT_ADC_CLOCK_SOURCE_PCLK2 = 0
} ir_reflect_adc_clock_source_t;

typedef enum {
  IR_REFLECT_ADC_PRESCALER_AUTO = 0,
  IR_REFLECT_ADC_PRESCALER_DIV2 = 2,
  IR_REFLECT_ADC_PRESCALER_DIV4 = 4,
  IR_REFLECT_ADC_PRESCALER_DIV6 = 6,
  IR_REFLECT_ADC_PRESCALER_DIV8 = 8
} ir_reflect_adc_prescaler_t;

typedef struct {
  ir_reflect_adc_clock_source_t clock_source;
  ir_reflect_adc_prescaler_t adc_prescaler;
} ir_reflect_config_t;

/*
 * 反射红外传感器模块（TCRT5000 等，模拟 AO 输出）
 *
 * 硬件：模块 AO → PA3（ADC1_IN3），与光敏/热敏同一次 ADC SCAN+DMA 采样。
 * 反射越强（白底/近距）通常 AO 越高；黑底/远距越低——以实测为准。
 *
 * 前置：bsp_clock_apply_profile()、bsp_board_init()。
 */

stm_status_t ir_reflect_init_with_config(const ir_reflect_config_t *config);
stm_status_t ir_reflect_init(void);

stm_status_t ir_reflect_read_raw_blocking(uint16_t *out_raw12);
stm_status_t ir_reflect_read_raw_average_blocking(uint16_t *out_raw12,
                                                  uint8_t sample_count);
stm_status_t ir_reflect_read_millivolts_blocking(uint32_t *out_mv);

#endif
