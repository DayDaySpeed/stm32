/*
 * 系统时钟树配置：HSI 8MHz 或 HSE+PLL 72MHz，并缓存 HCLK/PCLK 供驱动查询。
 */
#include "bsp/clock.h"

#include "bsp/stm32f103_regs.h"

/*
 * 时钟切换轮询上限：超过则视为硬件未就绪，返回 STM_ERR_TIMEOUT。
 * 经验值：HSE/HSI/PLL 在正常硬件下数百次循环内必稳定，留 2_000_000 是保守冗余。
 */
#define BSP_CLOCK_WAIT_MAX_ITER (2000000UL)

/*
 * 运行时频率缓存：
 * - 由 apply_* 在切换成功后更新；
 * - 所有驱动统一通过 getter 获取，避免硬编码。
 */
static uint32_t g_hclk_hz = 8000000UL;
static uint32_t g_pclk1_hz = 8000000UL;
static uint32_t g_pclk2_hz = 8000000UL;

/*
 * 轮询寄存器某一位/某字段达到目标值。
 *   reg    : 目标寄存器指针
 *   mask   : 关注的位字段
 *   expect : 期望的字段值（&mask 之后）
 * 返回 1 表示在超时窗口内达到期望，0 表示超时。
 */
static uint8_t wait_reg_bits(volatile uint32_t *reg, uint32_t mask,
                             uint32_t expect) {
  for (volatile uint32_t i = 0U; i < BSP_CLOCK_WAIT_MAX_ITER; ++i) {
    if ((*reg & mask) == expect) {
      return 1U;
    }
  }
  return 0U;
}

static stm_status_t apply_hsi_8mhz(void) {
  /* 1) 打开 HSI 并等待就绪。 */
  RCC_CR |= RCC_CR_HSION_BIT;
  if (wait_reg_bits(&RCC_CR, RCC_CR_HSIRDY_BIT, RCC_CR_HSIRDY_BIT) == 0U) {
    return STM_ERR_TIMEOUT;
  }

  /* 2) 切系统时钟到 HSI，并确认 SWS 已反映为 HSI（SWS=00）。 */
  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_HSI;
  if (wait_reg_bits(&RCC_CFGR, RCC_CFGR_SWS_MASK, RCC_CFGR_SWS_HSI) == 0U) {
    return STM_ERR_TIMEOUT;
  }

  /* 3) 关闭 PLL（8MHz 档不需要），尽量等待 PLLRDY 清零（不强制超时报错）。 */
  RCC_CR &= ~RCC_CR_PLLON_BIT;
  (void)wait_reg_bits(&RCC_CR, RCC_CR_PLLRDY_BIT, 0U);

  /* 4) 总线分频回归默认：AHB=/1，APB1=/1，APB2=/1。 */
  RCC_CFGR &= ~(RCC_CFGR_HPRE_MASK | RCC_CFGR_PPRE1_MASK | RCC_CFGR_PPRE2_MASK);
  RCC_CFGR |= (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);

  /* 5) 更新软件频率缓存。 */
  g_hclk_hz = 8000000UL;
  g_pclk1_hz = 8000000UL;
  g_pclk2_hz = 8000000UL;
  return STM_OK;
}

/*
 * HSE 8MHz -> PLL ×9 -> SYSCLK 72MHz 时钟树：
 *
 *      HSE 8MHz
 *           │
 *           ▼
 *       PLL ×9
 *           │
 *           ▼
 *    SYSCLK 72MHz
 *           │
 *      AHB /1
 *           │
 *      HCLK 72MHz
 *      /        \
 * APB1 /2     APB2 /1
 *    │            │
 * PCLK1 36   PCLK2 72
 *
 * 切换步骤：开 HSE -> 配 Flash 等待周期 -> 配总线分频 -> 配 PLL -> 开 PLL -> SYSCLK 切 PLL。
 */
static stm_status_t apply_hse_pll_72mhz(void) {
  /* 1) 打开外部高速时钟 HSE，并等待稳定。 */
  RCC_CR |= RCC_CR_HSEON_BIT;
  if (wait_reg_bits(&RCC_CR, RCC_CR_HSERDY_BIT, RCC_CR_HSERDY_BIT) == 0U) {
    return STM_ERR_TIMEOUT;
  }

  /* 2) 提高 Flash 访问能力：开预取，等待周期设为 2（72MHz 常用）。 */
  FLASH_ACR |= FLASH_ACR_PRFTBE_BIT;
  FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) | FLASH_ACR_LATENCY_2;

  /* 3) 配置时钟树：AHB=/1、APB1=/2（F1 上 APB1 ≤ 36MHz）、APB2=/1、PLL=HSE×9。 */
  RCC_CFGR &= ~(RCC_CFGR_SW_MASK | RCC_CFGR_PLLSRC_MASK |
                RCC_CFGR_PLLXTPRE_MASK | RCC_CFGR_PLLMUL_MASK |
                RCC_CFGR_PPRE1_MASK | RCC_CFGR_PPRE2_MASK | RCC_CFGR_HPRE_MASK);
  RCC_CFGR |= (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1 |
               RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLXTPRE_HSE_DIV1 |
               RCC_CFGR_PLLMUL9);

  /* 4) 打开 PLL 并等待锁定。 */
  RCC_CR |= RCC_CR_PLLON_BIT;
  if (wait_reg_bits(&RCC_CR, RCC_CR_PLLRDY_BIT, RCC_CR_PLLRDY_BIT) == 0U) {
    return STM_ERR_TIMEOUT;
  }

  /* 5) 切系统时钟到 PLL，并等待 SWS=PLL。 */
  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
  if (wait_reg_bits(&RCC_CFGR, RCC_CFGR_SWS_MASK, RCC_CFGR_SWS_PLL) == 0U) {
    return STM_ERR_TIMEOUT;
  }

  /* 6) 更新软件频率缓存。 */
  g_hclk_hz = 72000000UL;
  g_pclk1_hz = 36000000UL;
  g_pclk2_hz = 72000000UL;
  return STM_OK;
}

/* profile：HSI 8MHz 或 HSE+PLL 72MHz；更新频率缓存。 */
stm_status_t bsp_clock_apply_profile(bsp_clock_profile_t profile) {
  if (profile == BSP_CLOCK_PROFILE_HSI_8MHZ) {
    return apply_hsi_8mhz();
  }
  if (profile == BSP_CLOCK_PROFILE_HSE_PLL_72MHZ) {
    return apply_hse_pll_72mhz();
  }
  return STM_ERR_INVALID_ARG;
}

uint32_t bsp_clock_get_hclk_hz(void) { return g_hclk_hz; }
uint32_t bsp_clock_get_pclk1_hz(void) { return g_pclk1_hz; }
uint32_t bsp_clock_get_pclk2_hz(void) { return g_pclk2_hz; }

static uint32_t bsp_clock_timer_hz_from_apb(uint32_t pclk_hz) {
  if (pclk_hz != g_hclk_hz) {
    return pclk_hz * 2UL;
  }
  return pclk_hz;
}

uint32_t bsp_clock_get_apb1_timer_hz(void) {
  return bsp_clock_timer_hz_from_apb(g_pclk1_hz);
}

uint32_t bsp_clock_get_apb2_timer_hz(void) {
  return bsp_clock_timer_hz_from_apb(g_pclk2_hz);
}
