#ifndef DRIVERS_PHOTORESISTOR_H
#define DRIVERS_PHOTORESISTOR_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * STM32F103 的 ADC 时钟源是固定的：ADC 时钟 = PCLK2 / ADCPRE。
 * 也就是说，这里不能像 USART 那样在 HSI/HSE/PLL 之间直接切源；
 * 真正可配的是：
 *   1) 系统/总线时钟先怎么配（由 bsp_clock_apply_profile 决定）
 *   2) ADC 再对 PCLK2 做 /2 /4 /6 /8 分频
 *
 * 注意：F103 没有 ADC /1，最小就是 /2。
 */
typedef enum {
  PHOTO_ADC_CLOCK_SOURCE_PCLK2 = 0
} photoresistor_adc_clock_source_t;

typedef enum {
  PHOTO_ADC_PRESCALER_AUTO = 0,
  PHOTO_ADC_PRESCALER_DIV2 = 2,
  PHOTO_ADC_PRESCALER_DIV4 = 4,
  PHOTO_ADC_PRESCALER_DIV6 = 6,
  PHOTO_ADC_PRESCALER_DIV8 = 8
} photoresistor_adc_prescaler_t;

typedef struct {
  /* F103 上当前固定只能填 PHOTO_ADC_CLOCK_SOURCE_PCLK2。 */
  photoresistor_adc_clock_source_t clock_source;
  /* AUTO = 运行时按当前 PCLK2 自动选；其余为手动指定。 */
  photoresistor_adc_prescaler_t adc_prescaler;
} photoresistor_config_t;

/*
 * 光敏电阻模块（分压模拟输出）驱动 —— ADC1 单次采样
 *
 * 默认硬件：模块 AO → PA1（ADC1_IN1），VCC/GND 按模块要求接 3.3V/地。
 * 常见模块：光越强 AO 电压越高或越低取决于分压接法；本驱动只读 0~4095 原始值，
 *          由你在应用层做「亮/暗」映射或标定。
 *
 * 前置：bsp_clock_apply_profile()、bsp_board_init()（已含 ADC1、GPIOA 时钟）。
 *
 * 约定：
 *   - 采样接口均为阻塞轮询型，会等待 EOC 结束
 *   - 初始化/配置/采样统一返回 `stm_status_t`
 */

/* 单次规则转换，得到 12 位原始值（约 0~4095）。 */
stm_status_t photoresistor_read_raw_blocking(uint16_t *out_raw12);

/*
 * 连续采 n 次取算术平均，抑制噪声；n 越大越稳、越慢。
 * n==0 视为非法。
 */
stm_status_t photoresistor_read_raw_average_blocking(uint16_t *out_raw12,
                                                     uint8_t sample_count);

/*
 * 按 VDDA≈3.3V 估算输入电压（mV），便于串口/OLED 显示。
 * 实际 VDDA 随电源略有偏差，精确测量需外部基准或校准。
 */
stm_status_t photoresistor_read_millivolts_blocking(uint32_t *out_mv);

/*
 * 按给定配置初始化/重配 ADC1 光敏驱动。
 * 允许重复调用；若传入不同分频配置，会按新参数重新配置 ADC。
 */
stm_status_t photoresistor_init_with_config(const photoresistor_config_t *config);

/* 默认初始化：clock_source=PCLK2, adc_prescaler=AUTO。 */
stm_status_t photoresistor_init(void);

/* 兼容旧接口：新代码优先使用上面的 `_blocking` 命名。 */
stm_status_t photoresistor_read_raw(uint16_t *out_raw12);
stm_status_t photoresistor_read_raw_average(uint16_t *out_raw12,
                                            uint8_t sample_count);
stm_status_t photoresistor_read_millivolts(uint32_t *out_mv);

#endif
