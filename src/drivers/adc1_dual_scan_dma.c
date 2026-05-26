/*
 * ADC1 双通道 SCAN + DMA1 Channel1
 *
 * 电路/数据流：
 *   PA1(IN1 光敏) ─┐
 *                  ├─ ADC1 规则组 SCAN：SQ1→SQ2，一次 SWSTART 连续采 2 路
 *   PA2(IN2 热敏) ─┤
 *   PA3(IN3 反射红外)┘
 *                  └─ 每完成一路转换，硬件把 ADC1_DR 经 DMA 写入 RAM 缓冲区
 *
 * STM32F103 要点：
 *   - CR1.SCAN=1：按 SQR 序列扫描
 *   - SQR1.L=2：序列长度 3（L 表示 N-1）
 *   - CR2.DMA=1：规则组结果自动触发 DMA
 *   - DMA1 通道 1 专用于 ADC1；轮询 TCIF1 完成，本工程暂不用 DMA 中断
 */

#include "drivers/adc1_dual_scan_dma.h"

#include "bsp/board_pins.h"
#include "bsp/board_gpio.h"
#include "bsp/clock.h"
#include "bsp/rcc_board.h"
#include "bsp/stm32f103_regs.h"

#include <stddef.h>
#include <stdint.h>

#define ADC1_DUAL_CH_PHOTO          BOARD_ADC_CH_PHOTO
#define ADC1_DUAL_CH_THERM          BOARD_ADC_CH_THERM
#define ADC1_DUAL_CH_IR_REFLECT     BOARD_ADC_CH_IR

#define ADC1_DUAL_CALIB_DELAY_LOOPS (2000U)
#define ADC1_DUAL_FLAG_TIMEOUT_LOOPS (200000U)
#define ADC1_DUAL_DMA_TIMEOUT_LOOPS  (200000U)
#define ADC1_DUAL_ADCCLK_MAX_HZ      (14000000UL)

static uint8_t s_initialized;
static adc1_dual_config_t s_config = {
    .clock_source = ADC1_DUAL_CLOCK_SOURCE_PCLK2,
    .adc_prescaler = ADC1_DUAL_PRESCALER_AUTO,
};

static void adc1_dual_adc_stabilize_delay(void) {
  for (volatile uint32_t n = 0U; n < ADC1_DUAL_CALIB_DELAY_LOOPS; n++) {
  }
}

static stm_status_t adc1_dual_wait_cr2_clear(uint32_t mask) {
  uint32_t t = ADC1_DUAL_FLAG_TIMEOUT_LOOPS;
  while ((ADC1_CR2 & mask) != 0U) {
    if (t == 0U) {
      return STM_ERR_TIMEOUT;
    }
    t--;
  }
  return STM_OK;
}

static stm_status_t adc1_dual_validate_config(const adc1_dual_config_t *config) {
  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (config->clock_source != ADC1_DUAL_CLOCK_SOURCE_PCLK2) {
    return STM_ERR_INVALID_ARG;
  }
  switch (config->adc_prescaler) {
  case ADC1_DUAL_PRESCALER_AUTO:
  case ADC1_DUAL_PRESCALER_DIV2:
  case ADC1_DUAL_PRESCALER_DIV4:
  case ADC1_DUAL_PRESCALER_DIV6:
  case ADC1_DUAL_PRESCALER_DIV8:
    return STM_OK;
  default:
    return STM_ERR_INVALID_ARG;
  }
}

static stm_status_t adc1_dual_configure_adc_clock_auto(void) {
  uint32_t pclk2_hz = bsp_clock_get_pclk2_hz();
  uint32_t adcpre_bits = RCC_CFGR_ADCPRE_DIV2;

  if (pclk2_hz == 0U) {
    return STM_ERR_INVALID_ARG;
  }

  if ((pclk2_hz / 2U) <= ADC1_DUAL_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV2;
  } else if ((pclk2_hz / 4U) <= ADC1_DUAL_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV4;
  } else if ((pclk2_hz / 6U) <= ADC1_DUAL_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV6;
  } else if ((pclk2_hz / 8U) <= ADC1_DUAL_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV8;
  } else {
    return STM_ERR_INVALID_ARG;
  }

  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_MASK) | adcpre_bits;
  return STM_OK;
}

static stm_status_t adc1_dual_configure_adc_clock_manual(
    adc1_dual_prescaler_t prescaler) {
  uint32_t pclk2_hz = bsp_clock_get_pclk2_hz();
  uint32_t adcpre_bits = RCC_CFGR_ADCPRE_DIV2;
  uint32_t adc_div = 2U;

  if (pclk2_hz == 0U) {
    return STM_ERR_INVALID_ARG;
  }

  switch (prescaler) {
  case ADC1_DUAL_PRESCALER_DIV2:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV2;
    adc_div = 2U;
    break;
  case ADC1_DUAL_PRESCALER_DIV4:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV4;
    adc_div = 4U;
    break;
  case ADC1_DUAL_PRESCALER_DIV6:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV6;
    adc_div = 6U;
    break;
  case ADC1_DUAL_PRESCALER_DIV8:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV8;
    adc_div = 8U;
    break;
  default:
    return STM_ERR_INVALID_ARG;
  }

  if ((pclk2_hz / adc_div) > ADC1_DUAL_ADCCLK_MAX_HZ) {
    return STM_ERR_INVALID_ARG;
  }

  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_MASK) | adcpre_bits;
  return STM_OK;
}

static stm_status_t adc1_dual_configure_adc_clock(
    const adc1_dual_config_t *config) {
  if (config->adc_prescaler == ADC1_DUAL_PRESCALER_AUTO) {
    return adc1_dual_configure_adc_clock_auto();
  }
  return adc1_dual_configure_adc_clock_manual(config->adc_prescaler);
}

//校准
static stm_status_t adc1_dual_adc_calibrate(void) {
  ADC1_CR2 |= ADC_CR2_ADON_BIT;
  adc1_dual_adc_stabilize_delay();

  ADC1_CR2 |= ADC_CR2_RSTCAL_BIT;
  {
    stm_status_t st = adc1_dual_wait_cr2_clear(ADC_CR2_RSTCAL_BIT);
    if (st != STM_OK) {
      return st;
    }
  }

  ADC1_CR2 |= ADC_CR2_CAL_BIT;
  return adc1_dual_wait_cr2_clear(ADC_CR2_CAL_BIT);
}

static void adc1_dual_configure_gpio_analog(void) {
  board_gpio_apply_crl(BOARD_GPIO_ADC_CR_REG, BOARD_GPIO_ADC_CR_MASK,
                       BOARD_GPIO_ADC_ANALOG_MODE);
}

static void adc1_dual_configure_scan_sequence(void) {
  const uint32_t ch_photo = ADC1_DUAL_CH_PHOTO;
  const uint32_t ch_therm = ADC1_DUAL_CH_THERM;
  const uint32_t ch_ir = ADC1_DUAL_CH_IR_REFLECT;
  uint32_t smpr_mask = (ADC_SMPR2_SMP_MASK << (ch_photo * 3U)) |
                       (ADC_SMPR2_SMP_MASK << (ch_therm * 3U)) |
                       (ADC_SMPR2_SMP_MASK << (ch_ir * 3U));

  ADC1_SMPR2 = (ADC1_SMPR2 & ~smpr_mask) |
               (7U << (ch_photo * 3U)) | (7U << (ch_therm * 3U)) |
               (7U << (ch_ir * 3U));
  ADC1_SQR1 = (ADC1_SQR1 & ~ADC_CR1_L_MASK) | ADC_CR1_L_3_CONV;
  ADC1_SQR3 = ADC_SQR3_SQ1(ADC1_DUAL_CH_PHOTO) |
              ADC_SQR3_SQ2(ADC1_DUAL_CH_THERM) |
              ADC_SQR3_SQ3(ADC1_DUAL_CH_IR_REFLECT);
}

static void adc1_dual_dma_disable_channel(void) {
  //开启通道
  DMA1_CCR1 &= ~DMA_CCR_EN_BIT;
  DMA1_IFCR = DMA_IFCR_CTCIF1_BIT;
}

static void adc1_dual_dma_setup_transfer(uint16_t *buffer, uint16_t halfword_count) {
  adc1_dual_dma_disable_channel();

  DMA1_CPAR1 = (uint32_t)(uintptr_t)&ADC1_DR;
  DMA1_CMAR1 = (uint32_t)(uintptr_t)buffer;
  DMA1_CNDTR1 = (uint32_t)halfword_count;
  DMA1_CCR1 = DMA_CCR_MINC_BIT | DMA_CCR_PSIZE_16_BIT | DMA_CCR_MSIZE_16_BIT;
}

static stm_status_t adc1_dual_dma_wait_tc(void) {
  uint32_t t = ADC1_DUAL_DMA_TIMEOUT_LOOPS;
  while ((DMA1_ISR & DMA_ISR_TCIF1_BIT) == 0U) {
    if (t == 0U) {
      return STM_ERR_TIMEOUT;
    }
    t--;
  }
  DMA1_IFCR = DMA_IFCR_CTCIF1_BIT;
  return STM_OK;
}

static stm_status_t adc1_dual_start_scan_dma_blocking(uint16_t *buffer,
                                                      uint16_t halfword_count) {
  stm_status_t st = STM_OK;

  if ((buffer == NULL) || (halfword_count == 0U)) {
    return STM_ERR_INVALID_ARG;
  }

  adc1_dual_dma_setup_transfer(buffer, halfword_count);

  ADC1_SR &= ~ADC_SR_EOC_BIT;
  DMA1_CCR1 |= DMA_CCR_EN_BIT;
  ADC1_CR2 |= ADC_CR2_SWSTART_BIT;

  st = adc1_dual_dma_wait_tc();
  adc1_dual_dma_disable_channel();
  return st;
}

stm_status_t adc1_dual_init_with_config(const adc1_dual_config_t *config) {
  stm_status_t st = adc1_dual_validate_config(config);
  if (st != STM_OK) {
    return st;
  }

  s_config = *config;

  if (s_initialized != 0U) {
    st = adc1_dual_configure_adc_clock(config);
    if (st != STM_OK) {
      return st;
    }
    adc1_dual_configure_gpio_analog();
    adc1_dual_configure_scan_sequence();
    return STM_OK;
  }

  RCC_AHBENR |= RCC_BOARD_AHB_ENABLE_MASK;

  st = adc1_dual_configure_adc_clock(config);
  if (st != STM_OK) {
    return st;
  }

  adc1_dual_configure_gpio_analog();

  ADC1_CR1 = ADC_CR1_SCAN_BIT;
  ADC1_CR2 = 0U;
  ADC1_CR2 = (ADC1_CR2 & ~ADC_CR2_EXTSEL_MASK) | ADC_CR2_EXTSEL_SWSTART |
             ADC_CR2_EXTTRIG_BIT | ADC_CR2_DMA_BIT;

  adc1_dual_configure_scan_sequence();

  st = adc1_dual_adc_calibrate();
  if (st != STM_OK) {
    return st;
  }

  s_initialized = 1U;
  return STM_OK;
}

stm_status_t adc1_dual_read_all_average_blocking(
    uint16_t out_samples[ADC1_DUAL_SLOT_COUNT], uint8_t scan_count) {
  uint32_t sum_photo = 0U;
  uint32_t sum_therm = 0U;
  uint32_t sum_ir = 0U;

  if ((out_samples == NULL) || (scan_count == 0U)) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  for (uint8_t i = 0U; i < scan_count; i++) {
    uint16_t sample[ADC1_DUAL_SLOT_COUNT] = {0U, 0U, 0U};
    stm_status_t st = adc1_dual_start_scan_dma_blocking(sample, ADC1_DUAL_SLOT_COUNT);
    if (st != STM_OK) {
      return st;
    }
    sum_photo += sample[ADC1_DUAL_SLOT_PHOTO];
    sum_therm += sample[ADC1_DUAL_SLOT_THERM];
    sum_ir += sample[ADC1_DUAL_SLOT_IR_REFLECT];
  }

  out_samples[ADC1_DUAL_SLOT_PHOTO] =
      (uint16_t)(sum_photo / (uint32_t)scan_count);
  out_samples[ADC1_DUAL_SLOT_THERM] =
      (uint16_t)(sum_therm / (uint32_t)scan_count);
  out_samples[ADC1_DUAL_SLOT_IR_REFLECT] =
      (uint16_t)(sum_ir / (uint32_t)scan_count);
  return STM_OK;
}

stm_status_t adc1_dual_read_pair_average_blocking(uint16_t out_pair[2],
                                                  uint8_t scan_count) {
  uint16_t sample[ADC1_DUAL_SLOT_COUNT] = {0U, 0U, 0U};
  stm_status_t st = STM_OK;

  if (out_pair == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  st = adc1_dual_read_all_average_blocking(sample, scan_count);
  if (st != STM_OK) {
    return st;
  }

  out_pair[ADC1_DUAL_SLOT_PHOTO] = sample[ADC1_DUAL_SLOT_PHOTO];
  out_pair[ADC1_DUAL_SLOT_THERM] = sample[ADC1_DUAL_SLOT_THERM];
  return STM_OK;
}
