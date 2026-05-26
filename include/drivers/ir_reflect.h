#ifndef DRIVERS_IR_REFLECT_H
#define DRIVERS_IR_REFLECT_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 反射红外 ADC 通道包装（槽位 ADC1_DUAL_SLOT_IR_REFLECT）。
 */

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

/* 校验 config 并标记模块已就绪；须在 adc1_dual_init 之后。 */
stm_status_t ir_reflect_init_with_config(const ir_reflect_config_t *config);
/* out_raw12：平均 raw 0..4095；sample_count：扫描平均次数，须 >0。 */
stm_status_t ir_reflect_read_raw_average_blocking(uint16_t *out_raw12,
                                                  uint8_t sample_count);

#endif
