#ifndef DRIVERS_ADC1_DUAL_SCAN_DMA_H
#define DRIVERS_ADC1_DUAL_SCAN_DMA_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * ADC1 三路 SCAN + DMA1 通道 1（文件名保留 dual 历史）。
 *
 * 一次 SWSTART 依次采：光敏 IN1、热敏 IN2、红外 IN3，结果写入 RAM。
 * 前置：bsp_clock_apply_profile()、bsp_board_init()。
 */

typedef enum {
  ADC1_DUAL_CLOCK_SOURCE_PCLK2 = 0
} adc1_dual_clock_source_t;

typedef enum {
  ADC1_DUAL_PRESCALER_AUTO = 0,
  ADC1_DUAL_PRESCALER_DIV2 = 2,
  ADC1_DUAL_PRESCALER_DIV4 = 4,
  ADC1_DUAL_PRESCALER_DIV6 = 6,
  ADC1_DUAL_PRESCALER_DIV8 = 8
} adc1_dual_prescaler_t;

typedef struct {
  adc1_dual_clock_source_t clock_source;
  adc1_dual_prescaler_t adc_prescaler;
} adc1_dual_config_t;

typedef enum {
  ADC1_DUAL_SLOT_PHOTO = 0,       /* 光敏 */
  ADC1_DUAL_SLOT_THERM = 1,       /* 热敏 */
  ADC1_DUAL_SLOT_IR_REFLECT = 2, /* 反射红外 */
  ADC1_DUAL_SLOT_COUNT = 3
} adc1_dual_slot_t;

stm_status_t adc1_dual_init_with_config(const adc1_dual_config_t *config);

/* 连续 scan_count 次扫描，对三路各自算术平均。 */
stm_status_t adc1_dual_read_all_average_blocking(uint16_t out_samples[ADC1_DUAL_SLOT_COUNT],
                                               uint8_t scan_count);

/* 只取光敏/热敏两路平均（仍一次扫完三路，丢弃红外槽）。out_pair[0]=photo, [1]=therm。 */
stm_status_t adc1_dual_read_pair_average_blocking(uint16_t out_pair[2],
                                                 uint8_t scan_count);

#endif
