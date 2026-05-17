#ifndef DRIVERS_ADC1_DUAL_SCAN_DMA_H
#define DRIVERS_ADC1_DUAL_SCAN_DMA_H

#include <stdint.h>

#include "common/stm_status.h"

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

/*
 * ADC1 双通道 SCAN + DMA1 Channel1
 *
 * 硬件绑定（本板默认）：
 *   - SQ1 = ADC1_IN1（PA1，光敏）
 *   - SQ2 = ADC1_IN2（PA2，热敏）
 *
 * 一次 SWSTART 后硬件按序列扫 2 路，DMA 把 DR 依次写入缓冲区：
 *   buffer[ADC1_DUAL_SLOT_PHOTO]、buffer[ADC1_DUAL_SLOT_THERM]
 *
 * 前置：bsp_clock_apply_profile()、bsp_board_init()（含 GPIOA/ADC1/DMA1 时钟）。
 */
typedef enum {
  ADC1_DUAL_SLOT_PHOTO = 0,
  ADC1_DUAL_SLOT_THERM = 1,
  ADC1_DUAL_SLOT_COUNT = 2
} adc1_dual_slot_t;

stm_status_t adc1_dual_init_with_config(const adc1_dual_config_t *config);
stm_status_t adc1_dual_init(void);
uint8_t adc1_dual_is_initialized(void);

/*
 * 触发一次双通道扫描，阻塞等待 DMA 传完 2 个半字。
 * out_pair 至少 2 个元素；可只关心其中一路，另一路会被顺带更新。
 */
stm_status_t adc1_dual_read_pair_blocking(uint16_t out_pair[2]);

/*
 * 连续扫 scan_count 次，对光敏/热敏各自做算术平均。
 * scan_count==0 非法。
 */
stm_status_t adc1_dual_read_pair_average_blocking(uint16_t out_pair[2],
                                                 uint8_t scan_count);

#endif
