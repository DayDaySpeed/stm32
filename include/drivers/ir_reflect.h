#ifndef DRIVERS_IR_REFLECT_H
#define DRIVERS_IR_REFLECT_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 反射红外传感器（TCRT5000 类）—— ADC1 IN3 槽位。
 *
 * 本板实测：远离 raw~4000，靠近 raw~100（越低越近）。
 * init 仅做参数校验与标记；实际采样走 adc1_dual_read_all_average_blocking。
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

stm_status_t ir_reflect_init_with_config(const ir_reflect_config_t *config);
stm_status_t ir_reflect_read_raw_average_blocking(uint16_t *out_raw12,
                                                  uint8_t sample_count);

#endif
