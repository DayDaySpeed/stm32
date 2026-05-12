/*
 * TIM2 通道 1 边沿对齐 PWM（STM32F103，PA0 复用推挽输出）
 *
 * PWM 模式 1：CNT < CCR1 输出高，否则低。
 * 周期 = (PSC+1)(ARR+1) / TIM_CLK；占空比由 CCR1 决定。
 * 不启用 TIM2 更新中断。
 */

#include "drivers/pwm.h"

#include "bsp/board_pins.h"
#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"

#include <stddef.h>
#include <stdint.h>

/* 一周期 tick 数 = ARR+1；为 0 表示未 init / 已 stop。 */
static uint32_t g_ticks_per_period;

static stm_status_t tim2_ch1_pwm_validate_duty(uint16_t duty_permille);

static stm_status_t tim2_ch1_pwm_validate_config(
    const tim2_ch1_pwm_config_t *config) {
  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (config->pwm_hz == 0U) {
    return STM_ERR_INVALID_ARG;
  }
  return tim2_ch1_pwm_validate_duty(config->duty_permille);
}

static stm_status_t tim2_ch1_pwm_validate_duty(uint16_t duty_permille) {
  if (duty_permille > 1000U) {
    return STM_ERR_INVALID_ARG;
  }
  return STM_OK;
}

/* 千分比 → CCR1。+500/1000 做四舍五入；
 * duty=1000 时 CCR1 = ticks（> ARR），保证真正 100% 高电平。 */
static uint32_t duty_permille_to_ccr1(uint16_t duty_permille,
                                      uint32_t ticks_per_period) {
  return ((uint32_t)duty_permille * ticks_per_period + 500U) / 1000U;
}

/* TIM2 实际输入时钟。
 * STM32F1 规则：APB1 预分频 ≠ 1 时，APB1 上的定时器时钟自动 ×2。 */
static uint32_t tim2_input_clock_hz(void) {
  uint32_t pclk1_hz = bsp_clock_get_pclk1_hz();
  uint32_t tim_clk_hz = pclk1_hz;

  if (pclk1_hz != bsp_clock_get_hclk_hz()) {
    tim_clk_hz = pclk1_hz * 2UL;  /* 例：HCLK=72, PCLK1=36 → TIM=72 */
  }
  return tim_clk_hz;
}

/*
 * 由目标 PWM 频率反推 (PSC, ARR)。
 *
 * 关系式：total = TIM_CLK / pwm_hz = (PSC+1)(ARR+1)。
 *
 * 策略：
 *   1) 精确解：枚举 (PSC+1)，找能整除 total 的因子；首次迭代即覆盖
 *      total ≤ 65536 的所有情形。
 *   2) 近似解：PSC+1 = ⌈total/65536⌉ 让 ARR 不溢出，丢一点点频率精度。
 *
 * uint32_t 入参保证 total ≤ 2^32-1，全程 32 位运算够用。
 */
static stm_status_t tim2_ch1_pwm_resolve_timebase(uint32_t tim_clk_hz,
                                                  uint32_t pwm_hz,
                                                  uint16_t *psc_out,
                                                  uint16_t *arr_out) {
  if ((pwm_hz == 0U) || (tim_clk_hz < pwm_hz)) {
    return STM_ERR_INVALID_ARG;
  }

  uint32_t total = tim_clk_hz / pwm_hz;

  /* 策略 1：PSC 越小 → ARR 越大 → 占空比分辨率越高，故从小往大扫。 */
  for (uint32_t psc = 1U; psc <= 0x10000U; psc++) {
    if ((total % psc) != 0U) {
      continue;
    }
    uint32_t arr = total / psc;
    if ((arr >= 1U) && (arr <= 0x10000U)) {
      *psc_out = (uint16_t)(psc - 1U);
      *arr_out = (uint16_t)(arr - 1U);
      return STM_OK;
    }
  }

  /* 策略 2：(total + 0xFFFF)/0x10000 是无浮点的向上取整写法。
   * 构造保证 psc ≤ 65536, arr ∈ [1, 65536]，无需运行时检查。 */
  uint32_t psc = (total + 0xFFFFU) / 0x10000U;
  uint32_t arr = total / psc;
  *psc_out = (uint16_t)(psc - 1U);
  *arr_out = (uint16_t)(arr - 1U);
  return STM_OK;
}

/* PA0：[1101] CNF=复用推挽，MODE=50MHz。 */
static void tim2_ch1_pwm_gpio_pa0_init(void) {
  GPIOA_CRL = (GPIOA_CRL & ~BOARD_GPIO_PA0_CRL_MASK) | BOARD_GPIO_PA0_AF_PP_50MHZ;
}

/*
 * 写 PWM 寄存器并启动。
 *
 * 顺序：停CNT → 配PWM模式 → 配输出 → 写PSC/ARR/CCR → 开影子 → UG同步 → 清UIF → 启CNT
 *
 * 关键点：
 *   - OC1PE / ARPE：CCR1/ARR 走影子寄存器，跨周期改值不会有毛刺。
 *   - EGR.UG：强制刷新影子寄存器，否则首个周期会用旧值。
 *   - CC1E：输出引脚总开关，忘了开就只是比较器空转、PA0 不动。
 */
static void tim2_ch1_pwm_apply_hw(uint16_t psc, uint16_t arr,
                                  uint16_t duty_permille) {
  uint32_t ticks = (uint32_t)arr + 1U;

  /* 1. 停 CNT、屏蔽更新中断。 */
  TIM2_CR1 &= ~TIM_CR1_CEN_BIT;
  TIM2_DIER &= ~TIM_DIER_UIE_BIT;

  /* 2. 通道 1 功能：CC1S=00（输出）、OC1M=110（PWM1）、OC1PE=1（CCR1 影子）。 */
  TIM2_CCMR1 &= ~(TIM_CCMR1_CC1S_MASK | TIM_CCMR1_OC1M_MASK);
  TIM2_CCMR1 |= TIM_CCMR1_CC1S_OUT | TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE_BIT;

  /* 3. 输出极性 CC1P=0（有效=高）、输出使能 CC1E=1。 */
  TIM2_CCER &= ~TIM_CCER_CC1P_BIT;
  TIM2_CCER |= TIM_CCER_CC1E_BIT;

  /* 4. 写数值寄存器并打开 ARR 影子。 */
  TIM2_PSC = (uint32_t)psc;
  TIM2_ARR = (uint32_t)arr;
  TIM2_CR1 |= TIM_CR1_ARPE_BIT;
  TIM2_CCR1 = duty_permille_to_ccr1(duty_permille, ticks);

  /* 5. UG 立刻刷影子；清 UG 顺手置位的 UIF；启动 CNT。
   * SR 是 rc_w0：写 0 清除，写 1 无效。所以 ~UIF 表示「只清 UIF，其它 bit 不动」。 */
  TIM2_EGR = TIM_EGR_UG_BIT;
  TIM2_SR = ~TIM_SR_UIF_BIT;
  TIM2_CR1 |= TIM_CR1_CEN_BIT;

  g_ticks_per_period = ticks;
}

stm_status_t tim2_ch1_pwm_init_hz(uint32_t pwm_frequency_hz,
                                  uint16_t duty_permille) {
  const tim2_ch1_pwm_config_t config = {
      .pwm_hz = pwm_frequency_hz,
      .duty_permille = duty_permille,
  };
  return tim2_ch1_pwm_init_with_config(&config);
}

stm_status_t tim2_ch1_pwm_init_with_config(const tim2_ch1_pwm_config_t *config) {
  stm_status_t st = tim2_ch1_pwm_validate_config(config);
  if (st != STM_OK) {
    return st;
  }

  uint32_t tim_clk = tim2_input_clock_hz();
  uint16_t psc = 0U;
  uint16_t arr = 0U;

  st = tim2_ch1_pwm_resolve_timebase(tim_clk, config->pwm_hz, &psc, &arr);
  if (st != STM_OK) {
    return st;
  }

  tim2_ch1_pwm_gpio_pa0_init();
  tim2_ch1_pwm_apply_hw(psc, arr, config->duty_permille);
  return STM_OK;
}

stm_status_t tim2_ch1_pwm_set_config(const tim2_ch1_pwm_config_t *config) {
  stm_status_t st = tim2_ch1_pwm_validate_config(config);
  if (st != STM_OK) {
    return st;
  }
  if (g_ticks_per_period == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  uint32_t tim_clk = tim2_input_clock_hz();
  uint16_t psc = 0U;
  uint16_t arr = 0U;

  st = tim2_ch1_pwm_resolve_timebase(tim_clk, config->pwm_hz, &psc, &arr);
  if (st != STM_OK) {
    return st;
  }

  tim2_ch1_pwm_apply_hw(psc, arr, config->duty_permille);
  return STM_OK;
}

stm_status_t tim2_ch1_pwm_set_duty_permille(uint16_t duty_permille) {
  stm_status_t st = tim2_ch1_pwm_validate_duty(duty_permille);
  if (st != STM_OK) {
    return st;
  }
  if (g_ticks_per_period == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  /* 仅改 CCR1：PSC/ARR 不变，PWM 频率不变，适合呼吸灯。 */
  TIM2_CCR1 = duty_permille_to_ccr1(duty_permille, g_ticks_per_period);
  return STM_OK;
}

stm_status_t tim2_ch1_pwm_set_hz(uint32_t pwm_frequency_hz,
                                 uint16_t duty_permille) {
  const tim2_ch1_pwm_config_t config = {
      .pwm_hz = pwm_frequency_hz,
      .duty_permille = duty_permille,
  };
  return tim2_ch1_pwm_set_config(&config);
}

stm_status_t tim2_ch1_pwm_stop(void) {
  if (g_ticks_per_period == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  TIM2_CR1 &= ~TIM_CR1_CEN_BIT;
  TIM2_CCER &= ~TIM_CCER_CC1E_BIT;
  g_ticks_per_period = 0U;
  return STM_OK;
}
