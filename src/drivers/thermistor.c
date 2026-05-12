/*
 * 热敏电阻模块 —— ADC1 规则组单次转换（默认 PA2 = ADC1_IN2）
 *
 * 模块约定：
 *   - 常见 10k NTC（B3950）模拟输出模块
 *   - 通过 AO 输出一个分压点电压，ADC 读到后先还原 NTC 电阻，再查表估算温度
 *
 * 设计取舍：
 *   - 不依赖 libm，避免在裸机工程里额外引入 `log()` 链接负担
 *   - 温度换算采用 10k NTC B3950 电阻表 + 线性插值
 */

#include "drivers/thermistor.h"

#include "bsp/board_pins.h"
#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"

#include <stddef.h>
#include <stdint.h>

#define THERMISTOR_ADC_CHANNEL          (2U)
#define THERMISTOR_SMPR2_CH2_SHIFT      (6U)
#define THERMISTOR_SMPR2_CH2_MASK       (7U << THERMISTOR_SMPR2_CH2_SHIFT)
#define THERMISTOR_SMPR2_CH2_VALUE_MAX  (7U << THERMISTOR_SMPR2_CH2_SHIFT)

#define THERMISTOR_VDDA_MV_DEFAULT      (3300U)
#define THERMISTOR_FIXED_R_DEFAULT      (10000U)
#define THERMISTOR_ADC_MAX              (4095U)

#define THERMISTOR_CALIB_DELAY_LOOPS    (2000U)
#define THERMISTOR_FLAG_TIMEOUT_LOOPS   (200000U)
#define THERMISTOR_EOC_TIMEOUT_LOOPS    (200000U)
#define THERMISTOR_ADCCLK_MAX_HZ        (14000000UL)

typedef struct {
  int16_t temperature_celsius_x10;
  uint32_t resistance_ohms;
} thermistor_lut_entry_t;

static const thermistor_lut_entry_t g_ntc_10k_b3950_table[] = {
    {-400, 401860U}, {-350, 281577U}, {-300, 200204U}, {-250, 144317U},
    {-200, 105385U}, {-150, 77898U},  {-100, 58246U},  {-50, 44026U},
    {0, 33621U},     {50, 25925U},    {100, 20175U},   {150, 15837U},
    {200, 12535U},   {250, 10000U},   {300, 8037U},    {350, 6506U},
    {400, 5301U},    {450, 4348U},    {500, 3588U},    {550, 2978U},
    {600, 2486U},    {650, 2086U},    {700, 1760U},    {750, 1492U},
    {800, 1270U},    {850, 1087U},    {900, 934U},     {950, 805U},
    {1000, 698U},    {1050, 606U},    {1100, 529U},    {1150, 463U},
    {1200, 407U},    {1250, 359U},
};

static uint8_t s_initialized;
static thermistor_config_t s_config = {
    .clock_source = THERMISTOR_ADC_CLOCK_SOURCE_PCLK2,
    .adc_prescaler = THERMISTOR_ADC_PRESCALER_AUTO,
    .divider_topology = THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN,
    .fixed_resistor_ohms = THERMISTOR_FIXED_R_DEFAULT,
    .vdda_mv = THERMISTOR_VDDA_MV_DEFAULT,
};

static void thermistor_adc_stabilize_delay(void) {
  for (volatile uint32_t n = 0U; n < THERMISTOR_CALIB_DELAY_LOOPS; n++) {
  }
}

static stm_status_t thermistor_wait_cr2_clear(uint32_t mask) {
  uint32_t t = THERMISTOR_FLAG_TIMEOUT_LOOPS;
  while ((ADC1_CR2 & mask) != 0U) {
    if (t == 0U) {
      return STM_ERR_TIMEOUT;
    }
    t--;
  }
  return STM_OK;
}

static stm_status_t thermistor_wait_eoc(void) {
  uint32_t t = THERMISTOR_EOC_TIMEOUT_LOOPS;
  while ((ADC1_SR & ADC_SR_EOC_BIT) == 0U) {
    if (t == 0U) {
      return STM_ERR_TIMEOUT;
    }
    t--;
  }
  return STM_OK;
}

static stm_status_t thermistor_validate_config(const thermistor_config_t *config) {
  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (config->clock_source != THERMISTOR_ADC_CLOCK_SOURCE_PCLK2) {
    return STM_ERR_INVALID_ARG;
  }
  switch (config->adc_prescaler) {
  case THERMISTOR_ADC_PRESCALER_AUTO:
  case THERMISTOR_ADC_PRESCALER_DIV2:
  case THERMISTOR_ADC_PRESCALER_DIV4:
  case THERMISTOR_ADC_PRESCALER_DIV6:
  case THERMISTOR_ADC_PRESCALER_DIV8:
    break;
  default:
    return STM_ERR_INVALID_ARG;
  }
  switch (config->divider_topology) {
  case THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN:
  case THERMISTOR_DIVIDER_NTC_UP_FIXED_DOWN:
    break;
  default:
    return STM_ERR_INVALID_ARG;
  }
  if ((config->fixed_resistor_ohms == 0U) || (config->vdda_mv == 0U)) {
    return STM_ERR_INVALID_ARG;
  }
  return STM_OK;
}

static stm_status_t thermistor_configure_adc_clock_auto(void) {
  uint32_t pclk2_hz = bsp_clock_get_pclk2_hz();
  uint32_t adcpre_bits = RCC_CFGR_ADCPRE_DIV2;

  if (pclk2_hz == 0U) {
    return STM_ERR_INVALID_ARG;
  }

  if ((pclk2_hz / 2U) <= THERMISTOR_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV2;
  } else if ((pclk2_hz / 4U) <= THERMISTOR_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV4;
  } else if ((pclk2_hz / 6U) <= THERMISTOR_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV6;
  } else if ((pclk2_hz / 8U) <= THERMISTOR_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV8;
  } else {
    return STM_ERR_INVALID_ARG;
  }

  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_MASK) | adcpre_bits;
  return STM_OK;
}

static stm_status_t thermistor_configure_adc_clock_manual(
    thermistor_adc_prescaler_t prescaler) {
  uint32_t pclk2_hz = bsp_clock_get_pclk2_hz();
  uint32_t adcpre_bits = RCC_CFGR_ADCPRE_DIV2;
  uint32_t adc_div = 2U;

  if (pclk2_hz == 0U) {
    return STM_ERR_INVALID_ARG;
  }

  switch (prescaler) {
  case THERMISTOR_ADC_PRESCALER_DIV2:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV2;
    adc_div = 2U;
    break;
  case THERMISTOR_ADC_PRESCALER_DIV4:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV4;
    adc_div = 4U;
    break;
  case THERMISTOR_ADC_PRESCALER_DIV6:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV6;
    adc_div = 6U;
    break;
  case THERMISTOR_ADC_PRESCALER_DIV8:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV8;
    adc_div = 8U;
    break;
  default:
    return STM_ERR_INVALID_ARG;
  }

  if ((pclk2_hz / adc_div) > THERMISTOR_ADCCLK_MAX_HZ) {
    return STM_ERR_INVALID_ARG;
  }

  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_MASK) | adcpre_bits;
  return STM_OK;
}

static stm_status_t thermistor_configure_adc_clock(
    const thermistor_config_t *config) {
  if (config->adc_prescaler == THERMISTOR_ADC_PRESCALER_AUTO) {
    return thermistor_configure_adc_clock_auto();
  }
  return thermistor_configure_adc_clock_manual(config->adc_prescaler);
}

static stm_status_t thermistor_adc_calibrate(void) {
  ADC1_CR2 |= ADC_CR2_ADON_BIT;
  thermistor_adc_stabilize_delay();

  ADC1_CR2 |= ADC_CR2_RSTCAL_BIT;
  {
    stm_status_t st = thermistor_wait_cr2_clear(ADC_CR2_RSTCAL_BIT);
    if (st != STM_OK) {
      return st;
    }
  }

  ADC1_CR2 |= ADC_CR2_CAL_BIT;
  return thermistor_wait_cr2_clear(ADC_CR2_CAL_BIT);
}

static void thermistor_prepare_conversion_channel(void) {
  ADC1_SMPR2 = (ADC1_SMPR2 & ~THERMISTOR_SMPR2_CH2_MASK) |
               THERMISTOR_SMPR2_CH2_VALUE_MAX;
  ADC1_SQR1 = 0U;
  ADC1_SQR3 = (uint32_t)(THERMISTOR_ADC_CHANNEL & 0x1FU);
}

static stm_status_t thermistor_start_once_and_read(uint16_t *out) {
  thermistor_prepare_conversion_channel();

  ADC1_SR &= ~ADC_SR_EOC_BIT;
  ADC1_CR2 |= ADC_CR2_SWSTART_BIT;
  {
    stm_status_t st = thermistor_wait_eoc();
    if (st != STM_OK) {
      return st;
    }
  }

  *out = (uint16_t)(ADC1_DR & 0x0FFFU);
  return STM_OK;
}

static uint32_t thermistor_raw_to_millivolts(uint16_t raw12) {
  return ((uint32_t)raw12 * s_config.vdda_mv + (THERMISTOR_ADC_MAX / 2U)) /
         THERMISTOR_ADC_MAX;
}

static stm_status_t thermistor_raw_to_resistance_ohms(uint16_t raw12,
                                                      uint32_t *out_ohms) {
  uint32_t denominator = 0U;
  uint64_t numerator = 0U;

  if (out_ohms == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  if (s_config.divider_topology == THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN) {
    denominator = THERMISTOR_ADC_MAX - (uint32_t)raw12;
    if (denominator == 0U) {
      return STM_ERR_INVALID_ARG;
    }
    numerator = (uint64_t)s_config.fixed_resistor_ohms * (uint32_t)raw12;
  } else {
    denominator = (uint32_t)raw12;
    if (denominator == 0U) {
      return STM_ERR_INVALID_ARG;
    }
    numerator = (uint64_t)s_config.fixed_resistor_ohms *
                (THERMISTOR_ADC_MAX - (uint32_t)raw12);
  }

  *out_ohms = (uint32_t)((numerator + (denominator / 2U)) / denominator);
  return STM_OK;
}

static stm_status_t thermistor_lookup_temperature_celsius_x10(
    uint32_t resistance_ohms, int16_t *out_celsius_x10) {
  uint32_t table_size = (uint32_t)(sizeof(g_ntc_10k_b3950_table) /
                                   sizeof(g_ntc_10k_b3950_table[0]));

  if (out_celsius_x10 == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (table_size < 2U) {
    return STM_ERR_INVALID_ARG;
  }

  if (resistance_ohms >= g_ntc_10k_b3950_table[0].resistance_ohms) {
    *out_celsius_x10 = g_ntc_10k_b3950_table[0].temperature_celsius_x10;
    return STM_OK;
  }
  if (resistance_ohms <= g_ntc_10k_b3950_table[table_size - 1U].resistance_ohms) {
    *out_celsius_x10 =
        g_ntc_10k_b3950_table[table_size - 1U].temperature_celsius_x10;
    return STM_OK;
  }

  for (uint32_t i = 0U; i + 1U < table_size; i++) {
    uint32_t r0 = g_ntc_10k_b3950_table[i].resistance_ohms;
    uint32_t r1 = g_ntc_10k_b3950_table[i + 1U].resistance_ohms;
    int16_t t0 = g_ntc_10k_b3950_table[i].temperature_celsius_x10;
    int16_t t1 = g_ntc_10k_b3950_table[i + 1U].temperature_celsius_x10;

    if ((resistance_ohms <= r0) && (resistance_ohms >= r1)) {
      uint32_t dr = r0 - r1;
      uint32_t offset_r = r0 - resistance_ohms;
      int32_t dt = (int32_t)t1 - (int32_t)t0;
      int32_t interp = (int32_t)(((int64_t)offset_r * dt + (dr / 2U)) / dr);
      *out_celsius_x10 = (int16_t)((int32_t)t0 + interp);
      return STM_OK;
    }
  }

  return STM_ERR_INVALID_ARG;
}

stm_status_t thermistor_init_with_config(const thermistor_config_t *config) {
  stm_status_t st = thermistor_validate_config(config);
  if (st != STM_OK) {
    return st;
  }

  s_initialized = 0U;
  s_config = *config;

  st = thermistor_configure_adc_clock(config);
  if (st != STM_OK) {
    return st;
  }

  GPIOA_CRL = (GPIOA_CRL & ~BOARD_GPIO_PA2_CRL_MASK) | BOARD_GPIO_PA2_ANALOG;

  ADC1_CR1 = 0U;
  ADC1_CR2 = 0U;
  ADC1_CR2 = (ADC1_CR2 & ~ADC_CR2_EXTSEL_MASK) |
             ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG_BIT;

  thermistor_prepare_conversion_channel();

  st = thermistor_adc_calibrate();
  if (st != STM_OK) {
    return st;
  }

  s_initialized = 1U;
  return STM_OK;
}

stm_status_t thermistor_init(void) {
  const thermistor_config_t config = {
      .clock_source = THERMISTOR_ADC_CLOCK_SOURCE_PCLK2,
      .adc_prescaler = THERMISTOR_ADC_PRESCALER_AUTO,
      .divider_topology = THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN,
      .fixed_resistor_ohms = THERMISTOR_FIXED_R_DEFAULT,
      .vdda_mv = THERMISTOR_VDDA_MV_DEFAULT,
  };
  return thermistor_init_with_config(&config);
}

stm_status_t thermistor_read_raw_blocking(uint16_t *out_raw12) {
  if (out_raw12 == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  return thermistor_start_once_and_read(out_raw12);
}

stm_status_t thermistor_read_raw_average_blocking(uint16_t *out_raw12,
                                                  uint8_t sample_count) {
  if ((out_raw12 == NULL) || (sample_count == 0U)) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  uint32_t sum = 0U;
  for (uint8_t i = 0U; i < sample_count; i++) {
    uint16_t raw = 0U;
    stm_status_t st = thermistor_start_once_and_read(&raw);
    if (st != STM_OK) {
      return st;
    }
    sum += raw;
  }

  *out_raw12 = (uint16_t)(sum / (uint32_t)sample_count);
  return STM_OK;
}

stm_status_t thermistor_read_millivolts_blocking(uint32_t *out_mv) {
  uint16_t raw = 0U;
  stm_status_t st = STM_OK;

  if (out_mv == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  st = thermistor_read_raw_blocking(&raw);
  if (st != STM_OK) {
    return st;
  }

  *out_mv = thermistor_raw_to_millivolts(raw);
  return STM_OK;
}

stm_status_t thermistor_read_resistance_ohms_blocking(uint32_t *out_ohms) {
  uint16_t raw = 0U;
  stm_status_t st = STM_OK;

  if (out_ohms == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  st = thermistor_read_raw_blocking(&raw);
  if (st != STM_OK) {
    return st;
  }

  return thermistor_raw_to_resistance_ohms(raw, out_ohms);
}

stm_status_t thermistor_read_temperature_celsius_x10_blocking(
    int16_t *out_celsius_x10) {
  uint32_t resistance_ohms = 0U;
  stm_status_t st = STM_OK;

  if (out_celsius_x10 == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  st = thermistor_read_resistance_ohms_blocking(&resistance_ohms);
  if (st != STM_OK) {
    return st;
  }

  return thermistor_lookup_temperature_celsius_x10(resistance_ohms,
                                                   out_celsius_x10);
}

stm_status_t thermistor_read_raw(uint16_t *out_raw12) {
  return thermistor_read_raw_blocking(out_raw12);
}

stm_status_t thermistor_read_raw_average(uint16_t *out_raw12,
                                         uint8_t sample_count) {
  return thermistor_read_raw_average_blocking(out_raw12, sample_count);
}

stm_status_t thermistor_read_millivolts(uint32_t *out_mv) {
  return thermistor_read_millivolts_blocking(out_mv);
}

stm_status_t thermistor_read_resistance_ohms(uint32_t *out_ohms) {
  return thermistor_read_resistance_ohms_blocking(out_ohms);
}

stm_status_t thermistor_read_temperature_celsius_x10(int16_t *out_celsius_x10) {
  return thermistor_read_temperature_celsius_x10_blocking(out_celsius_x10);
}
