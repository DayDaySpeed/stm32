/*
 * 光敏电阻模块 —— ADC1 规则组单次转换（默认 PA1 = ADC1_IN1）
 *
 * 电路常识：
 *   模块上的光敏电阻与固定电阻组成分压器，AO 输出 0~VDDA 之间的模拟电压。
 *   亮度变化 → 分压比变化 → AO 电压变化 → ADC 数字化为 0~4095（12 位）。
 *
 * STM32F103 要点：
 *   - ADC 时钟源固定来自 PCLK2，再经 RCC_CFGR.ADCPRE 做 /2 /4 /6 /8 分频。
 *     注意：F103 没有 ADC /1，所以即使系统是 8MHz，ADC 也只能最快跑到 4MHz。
 *   - ADC 时钟不得超过 14MHz；例如 PCLK2=72MHz 时至少要 /6，得到 12MHz。
 *   - PA1 须配成「模拟输入」（MODE=00, CNF=00），关闭数字缓冲，减小漏电与噪声。
 *   - 上电后须执行复位校准 + 启动校准（RSTCAL/CAL），否则读数可能漂。
 *   - 单次转换：写 CR2.SWSTART → 等 SR.EOC → 读 DR（读 DR 会清 EOC）。
 *   - 采样时间 SMP 越长，对高阻源（分压节点）越稳，转换总时间变长；光敏应用取最长档即可。
 */

#include "drivers/photoresistor.h"

#include "bsp/board_pins.h"
#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"

#include <stddef.h>
#include <stdint.h>

/* 与 board_pins 一致：PA1 = ADC1 通道 1（IN1） */
#define PHOTO_ADC_CHANNEL           (1U)
/*
 * ADC1_SMPR2 中：
 *   SMP0 占 [2:0]
 *   SMP1 占 [5:3]
 * 这里我们配置的是通道 1（PA1 = ADC1_IN1），所以要改的是 SMP1 字段，而不是最低 3 位。
 */
#define PHOTO_SMPR2_CH1_SHIFT       (3U)
#define PHOTO_SMPR2_CH1_MASK        (7U << PHOTO_SMPR2_CH1_SHIFT)
/* SMP1 = 111 -> 239.5 ADC 周期；高阻分压节点需要较长采样时间让采样电容充满。 */
#define PHOTO_SMPR2_CH1_VALUE_MAX   (7U << PHOTO_SMPR2_CH1_SHIFT)

#define PHOTO_VDDA_MV_DEFAULT       (3300U)
#define PHOTO_ADC_MAX               (4095U)

#define PHOTO_CALIB_DELAY_LOOPS     (2000U)
#define PHOTO_FLAG_TIMEOUT_LOOPS    (200000U)
#define PHOTO_EOC_TIMEOUT_LOOPS     (200000U)
#define PHOTO_ADCCLK_MAX_HZ         (14000000UL)

static uint8_t s_initialized;

/*
 * 函数名：photoresistor_adc_stabilize_delay
 * 参数：无
 * 作用：
 *   ADC 上电（ADON=1）后，给内部模拟电路一个很短的稳定时间，再继续做校准。
 *   这是最简单的裸机延时做法，不依赖 SysTick，也不依赖中断。
 * 返回值：无
 */
static void photoresistor_adc_stabilize_delay(void) {
  for (volatile uint32_t n = 0U; n < PHOTO_CALIB_DELAY_LOOPS; n++) {
  }
}

/*
 * 函数名：photoresistor_wait_cr2_clear
 * 参数：
 *   - mask：要等待清零的 ADC1_CR2 位掩码，例如 RSTCAL / CAL
 * 作用：
 *   轮询 ADC1_CR2，直到指定标志位被硬件清零。
 *   用于等待“复位校准完成”或“校准完成”。
 * 返回值：
 *   - STM_OK：等待成功
 *   - STM_ERR_TIMEOUT：超时，说明硬件状态迟迟没变化
 */
static stm_status_t photoresistor_wait_cr2_clear(uint32_t mask) {
  uint32_t t = PHOTO_FLAG_TIMEOUT_LOOPS;
  while ((ADC1_CR2 & mask) != 0U) {
    if (t == 0U) {
      return STM_ERR_TIMEOUT;
    }
    t--;
  }
  return STM_OK;
}

/*
 * 函数名：photoresistor_wait_eoc
 * 参数：无
 * 作用：
 *   轮询 ADC1_SR.EOC（End Of Conversion，转换结束）。
 *   规则组单次转换启动后，必须等 EOC=1 才能安全读取 ADC1_DR。
 * 返回值：
 *   - STM_OK：转换完成
 *   - STM_ERR_TIMEOUT：等待超时
 */
static stm_status_t photoresistor_wait_eoc(void) {
  uint32_t t = PHOTO_EOC_TIMEOUT_LOOPS;
  while ((ADC1_SR & ADC_SR_EOC_BIT) == 0U) {
    if (t == 0U) {
      return STM_ERR_TIMEOUT;
    }
    t--;
  }
  return STM_OK;
}

/*
 * 函数名：photoresistor_validate_config
 * 参数：
 *   - config：光敏驱动初始化配置
 * 作用：
 *   检查配置结构体是否合法。
 *   当前 STM32F103 只支持 ADC 时钟源来自 PCLK2，分频只能是 AUTO /2 /4 /6 /8。
 * 返回值：
 *   - STM_OK：配置合法
 *   - STM_ERR_INVALID_ARG：配置为空或字段非法
 */
static stm_status_t photoresistor_validate_config(const photoresistor_config_t *config) {
  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (config->clock_source != PHOTO_ADC_CLOCK_SOURCE_PCLK2) {
    return STM_ERR_INVALID_ARG;
  }
  switch (config->adc_prescaler) {
  case PHOTO_ADC_PRESCALER_AUTO:
  case PHOTO_ADC_PRESCALER_DIV2:
  case PHOTO_ADC_PRESCALER_DIV4:
  case PHOTO_ADC_PRESCALER_DIV6:
  case PHOTO_ADC_PRESCALER_DIV8:
    return STM_OK;
  default:
    return STM_ERR_INVALID_ARG;
  }
}

/*
 * 按当前 PCLK2 自动选择 ADC 预分频：
 *   ADCCLK = PCLK2 / ADCPRE，且必须 <= 14MHz。
 *
 * 策略：从 /2、/4、/6、/8 里选“满足限制的最小分频”，
 * 这样 ADC 尽可能快，同时不越过 F103 的 14MHz 上限。
 *
 * 典型结果：
 *   - PCLK2 = 8MHz  -> /2  -> 4MHz
 *   - PCLK2 = 72MHz -> /6  -> 12MHz
 */
static stm_status_t photoresistor_configure_adc_clock_auto(void) {
  uint32_t pclk2_hz = bsp_clock_get_pclk2_hz();
  uint32_t adcpre_bits = RCC_CFGR_ADCPRE_DIV2;

  if (pclk2_hz == 0U) {
    return STM_ERR_INVALID_ARG;
  }

  if ((pclk2_hz / 2U) <= PHOTO_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV2;
  } else if ((pclk2_hz / 4U) <= PHOTO_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV4;
  } else if ((pclk2_hz / 6U) <= PHOTO_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV6;
  } else if ((pclk2_hz / 8U) <= PHOTO_ADCCLK_MAX_HZ) {
    adcpre_bits = RCC_CFGR_ADCPRE_DIV8;
  } else {
    return STM_ERR_INVALID_ARG;
  }

  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_MASK) | adcpre_bits;
  return STM_OK;
}

/*
 * 函数名：photoresistor_configure_adc_clock_manual
 * 参数：
 *   - prescaler：用户指定的 ADC 分频（/2 /4 /6 /8）
 * 作用：
 *   按用户给定的分频写入 RCC_CFGR.ADCPRE。
 *   写之前先校验：PCLK2 / 分频后不能超过 F103 的 14MHz 限制。
 * 返回值：
 *   - STM_OK：配置成功
 *   - STM_ERR_INVALID_ARG：分频值非法，或算出的 ADCCLK 超规格
 */
static stm_status_t photoresistor_configure_adc_clock_manual(
    photoresistor_adc_prescaler_t prescaler) {
  uint32_t pclk2_hz = bsp_clock_get_pclk2_hz();
  uint32_t adcpre_bits = RCC_CFGR_ADCPRE_DIV2;
  uint32_t adc_div = 2U;

  if (pclk2_hz == 0U) {
    return STM_ERR_INVALID_ARG;
  }

  switch (prescaler) {
  case PHOTO_ADC_PRESCALER_DIV2:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV2;
    adc_div = 2U;
    break;
  case PHOTO_ADC_PRESCALER_DIV4:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV4;
    adc_div = 4U;
    break;
  case PHOTO_ADC_PRESCALER_DIV6:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV6;
    adc_div = 6U;
    break;
  case PHOTO_ADC_PRESCALER_DIV8:
    adcpre_bits = RCC_CFGR_ADCPRE_DIV8;
    adc_div = 8U;
    break;
  default:
    return STM_ERR_INVALID_ARG;
  }

  if ((pclk2_hz / adc_div) > PHOTO_ADCCLK_MAX_HZ) {
    return STM_ERR_INVALID_ARG;
  }

  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_MASK) | adcpre_bits;
  return STM_OK;
}

/*
 * 函数名：photoresistor_configure_adc_clock
 * 参数：
 *   - config：初始化配置
 * 作用：
 *   统一分发 ADC 时钟配置逻辑：
 *   - 若配置为 AUTO，就根据当前 PCLK2 自动选择最小合法分频；
 *   - 若配置为手动，就使用用户指定的分频。
 * 返回值：
 *   - STM_OK：配置成功
 *   - 其他：自动/手动路径中的错误码
 */
static stm_status_t photoresistor_configure_adc_clock(
    const photoresistor_config_t *config) {
  if (config->adc_prescaler == PHOTO_ADC_PRESCALER_AUTO) {
    return photoresistor_configure_adc_clock_auto();
  }
  return photoresistor_configure_adc_clock_manual(config->adc_prescaler);
}

/*
 * 函数名：photoresistor_adc_calibrate
 * 参数：无
 * 作用：
 *   完成 STM32F103 ADC 的标准校准流程：
 *   1) ADON=1，上电 ADC 模拟部分
 *   2) 等待内部电路稳定
 *   3) 置位 RSTCAL，复位校准寄存器，并等待硬件清零
 *   4) 置位 CAL，开始正式校准，并等待硬件清零
 * 返回值：
 *   - STM_OK：校准成功
 *   - STM_ERR_TIMEOUT：等待 RSTCAL/CAL 清零超时
 */
static stm_status_t photoresistor_adc_calibrate(void) {
  /* 上电后先等内部模拟电路稳定，再复位校准寄存器 */
  ADC1_CR2 |= ADC_CR2_ADON_BIT;
  photoresistor_adc_stabilize_delay();
  /* 初始化校准寄存器：硬件完成后会自动清零该位 */
  ADC1_CR2 |= ADC_CR2_RSTCAL_BIT;
  {
    stm_status_t w = photoresistor_wait_cr2_clear(ADC_CR2_RSTCAL_BIT);
    if (w != STM_OK) {
      return w;
    }
  }
  /* 开始 A/D 校准：硬件完成后会自动清零该位 */
  ADC1_CR2 |= ADC_CR2_CAL_BIT;
  return photoresistor_wait_cr2_clear(ADC_CR2_CAL_BIT);
}

/*
 * 每次转换前都把规则通道和采样时间切回光敏通道，
 * 这样即使别的 ADC1 单次采样驱动临时改过 SQR/SMPR，本驱动也能读回正确引脚。
 */
static void photoresistor_prepare_conversion_channel(void) {
  ADC1_SMPR2 = (ADC1_SMPR2 & ~PHOTO_SMPR2_CH1_MASK) | PHOTO_SMPR2_CH1_VALUE_MAX;
  ADC1_SQR1 = 0U;
  ADC1_SQR3 = (uint32_t)(PHOTO_ADC_CHANNEL & 0x1FU);
}

/*
 * 函数名：photoresistor_start_once_and_read
 * 参数：
 *   - out：输出参数，返回本次 12 位 ADC 原始值
 * 作用：
 *   触发一次规则组单次转换，并在转换完成后读取 ADC1_DR。
 *   这是本驱动最底层的“单次采样原语”，上层平均采样也是重复调用它。
 * 返回值：
 *   - STM_OK：采样成功
 *   - STM_ERR_TIMEOUT：等待 EOC 超时
 */
static stm_status_t photoresistor_start_once_and_read(uint16_t *out) {
  photoresistor_prepare_conversion_channel();

  /* 清掉上一次遗留的 EOC，确保下面等到的是“这一次转换完成”。 */
  ADC1_SR &= ~ADC_SR_EOC_BIT;

  /* 软件触发规则组单次转换 */
  ADC1_CR2 |= ADC_CR2_SWSTART_BIT;
  {
    stm_status_t w = photoresistor_wait_eoc();
    if (w != STM_OK) {
      return w;
    }
  }
  /* ADC1_DR 低 12 位就是转换结果；读 DR 的同时会清 EOC */
  *out = (uint16_t)(ADC1_DR & 0xFFFU);
  return STM_OK;
}

/*
 * 函数名：photoresistor_init_with_config
 * 参数：
 *   - config：初始化配置（ADC 时钟源/分频策略）
 * 作用：
 *   完成光敏驱动初始化：
 *   1) 校验配置
 *   2) 配 ADC 时钟分频
 *   3) 把 PA1 配成模拟输入
 *   4) 配规则组软件触发链路（EXTSEL=SWSTART, EXTTRIG=1）
 *   5) 设通道 1 采样时间为最长档
 *   6) 设规则组长度为 1、规则通道为 IN1
 *   7) 执行 ADC 校准
 * 返回值：
 *   - STM_OK：初始化成功
 *   - 其他：配置错误、时钟配置失败、校准超时等
 */
stm_status_t photoresistor_init_with_config(const photoresistor_config_t *config) {
  stm_status_t st = photoresistor_validate_config(config);
  if (st != STM_OK) {
    return st;
  }

  /* 若已经初始化过，允许按新参数重新配置，因此此处不直接返回。 */
  s_initialized = 0U;

  /* 1) ADC 时钟 ≤14MHz：按参数决定自动选还是手动指定 */
  {
    stm_status_t clk = photoresistor_configure_adc_clock(config);
    if (clk != STM_OK) {
      return clk;
    }
  }

  /* 2) PA1 → 模拟输入（不影响 CRL 上 PA0/PA6/PA7 等其它半字节） */
  GPIOA_CRL = (GPIOA_CRL & ~BOARD_GPIO_PA1_CRL_MASK) | BOARD_GPIO_PA1_ANALOG;

  /* 3) 默认单次、非扫描；先清控制寄存器。 */
  ADC1_CR1 = 0U;
  ADC1_CR2 = 0U;

  /*
   * 4) 规则组软件触发链路：
   *    - EXTSEL=111：规则组触发源选择为 SWSTART
   *    - EXTTRIG=1：允许规则组触发
   *
   * 若这里只写 SWSTART、却没有先配置 EXTSEL/EXTTRIG，
   * 在 STM32F103 上规则组转换可能根本不会真正开始，
   * 随后就会一直等不到 EOC，最终表现为 TIMEOUT。
   */
  ADC1_CR2 = (ADC1_CR2 & ~ADC_CR2_EXTSEL_MASK) |
             ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG_BIT;

  /*
   * 5) 采样时间：把通道 1 的 SMP1[5:3] 设为 111（239.5 cycles）
   *
   * 为什么要“先清字段，再写字段”：
   *   - 这里只想改 SMP1，不想误伤 SMPR2 里其它通道的采样时间配置；
   *   - 这是一种通用字段写法，后面如果不是写 111，而是写别的值，也同样成立；
   *   - 比“直接 OR 一个值进去”更稳，也更符合工程化代码风格。
   */
  photoresistor_prepare_conversion_channel();

  {
    /* 7) 最后执行 ADC 校准。 */
    stm_status_t cal = photoresistor_adc_calibrate();
    if (cal != STM_OK) {
      return cal;
    }
  }

  s_initialized = 1U;
  return STM_OK;
}

/*
 * 函数名：photoresistor_init
 * 参数：无
 * 作用：
 *   使用默认配置初始化光敏驱动。
 *   当前默认策略是：
 *   - ADC 时钟源 = PCLK2
 *   - ADC 分频 = AUTO（根据当前 PCLK2 自动选 /2 /4 /6 /8）
 * 返回值：
 *   - STM_OK：初始化成功
 *   - 其他：见 photoresistor_init_with_config
 */
stm_status_t photoresistor_init(void) {
  const photoresistor_config_t config = {
      .clock_source = PHOTO_ADC_CLOCK_SOURCE_PCLK2,
      .adc_prescaler = PHOTO_ADC_PRESCALER_AUTO,
  };
  return photoresistor_init_with_config(&config);
}

/*
 * 函数名：photoresistor_read_raw_blocking
 * 参数：
 *   - out_raw12：输出参数，返回 0~4095 的 12 位原始 ADC 值
 * 作用：
 *   执行一次阻塞式单次采样。
 * 返回值：
 *   - STM_OK：采样成功
 *   - STM_ERR_INVALID_ARG：输出指针为空
 *   - STM_ERR_NOT_INITIALIZED：驱动尚未初始化
 *   - STM_ERR_TIMEOUT：转换超时
 */
stm_status_t photoresistor_read_raw_blocking(uint16_t *out_raw12) {
  if (out_raw12 == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  return photoresistor_start_once_and_read(out_raw12);
}

/*
 * 函数名：photoresistor_read_raw_average_blocking
 * 参数：
 *   - out_raw12：输出参数，返回平均后的 12 位原始值
 *   - sample_count：采样次数，越大越稳，但耗时越长
 * 作用：
 *   连续做多次阻塞采样，然后取算术平均，降低噪声抖动。
 * 返回值：
 *   - STM_OK：采样成功
 *   - STM_ERR_INVALID_ARG：参数非法
 *   - STM_ERR_NOT_INITIALIZED：驱动尚未初始化
 *   - STM_ERR_TIMEOUT：某次采样超时
 */
stm_status_t photoresistor_read_raw_average_blocking(uint16_t *out_raw12,
                                                     uint8_t sample_count) {
  if ((out_raw12 == NULL) || (sample_count == 0U)) {
    return STM_ERR_INVALID_ARG;
  }
  if (s_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  uint32_t sum = 0U;
  for (uint8_t i = 0U; i < sample_count; i++) {
    uint16_t one = 0U;
    stm_status_t st = photoresistor_start_once_and_read(&one);
    if (st != STM_OK) {
      return st;
    }
    sum += (uint32_t)one;
  }
  *out_raw12 = (uint16_t)(sum / (uint32_t)sample_count);
  return STM_OK;
}

/*
 * 函数名：photoresistor_read_millivolts_blocking
 * 参数：
 *   - out_mv：输出参数，返回估算得到的毫伏值
 * 作用：
 *   先读取一次原始 ADC 值，再按 VDDA≈3.3V 折算成毫伏。
 *   这个值更方便 OLED/串口直接显示，但它是估算值，不是精密标定值。
 * 返回值：
 *   - STM_OK：转换成功
 *   - 其他：来自底层单次采样的错误
 */
stm_status_t photoresistor_read_millivolts_blocking(uint32_t *out_mv) {
  if (out_mv == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  uint16_t raw = 0U;
  stm_status_t st = photoresistor_read_raw_blocking(&raw);
  if (st != STM_OK) {
    return st;
  }

  *out_mv = ((uint32_t)raw * (uint32_t)PHOTO_VDDA_MV_DEFAULT + (PHOTO_ADC_MAX / 2U)) /
            (uint32_t)PHOTO_ADC_MAX;
  return STM_OK;
}

/* 兼容旧接口：内部直接转调新的 `_blocking` 版本。 */
stm_status_t photoresistor_read_raw(uint16_t *out_raw12) {
  return photoresistor_read_raw_blocking(out_raw12);
}

stm_status_t photoresistor_read_raw_average(uint16_t *out_raw12,
                                            uint8_t sample_count) {
  return photoresistor_read_raw_average_blocking(out_raw12, sample_count);
}

stm_status_t photoresistor_read_millivolts(uint32_t *out_mv) {
  return photoresistor_read_millivolts_blocking(out_mv);
}
