#include "bsp/clock.h"

#include "bsp/stm32f103_regs.h"

/* 运行时频率缓存：
 * - 由时钟配置函数在切换成功后更新
 * - 所有驱动统一通过 getter 获取，避免各处硬编码 */
static uint32_t g_sysclk_hz = 8000000UL;
static uint32_t g_hclk_hz = 8000000UL;
static uint32_t g_pclk1_hz = 8000000UL;
static uint32_t g_pclk2_hz = 8000000UL;

/* 轮询寄存器某一位达到目标状态（置位/清零）。
 * set=1: 等待 bit=1；set=0: 等待 bit=0
 * 返回 1 表示成功，0 表示超时。 */
static uint8_t wait_reg_bit(volatile uint32_t *reg, uint32_t bit, uint32_t set)
{
  for (volatile uint32_t i = 0U; i < 2000000UL; ++i) {
    uint32_t v = *reg;
    if (set != 0U) {
      if ((v & bit) != 0U) {
        return 1U;
      }
    } else {
      if ((v & bit) == 0U) {
        return 1U;
      }
    }
  }
  return 0U;
}

static stm_status_t apply_hsi_8mhz(void)
{
  /* 1) 打开 HSI 并等待就绪。 */
  RCC_CR |= RCC_CR_HSION_BIT;
  if (wait_reg_bit(&RCC_CR, RCC_CR_HSIRDY_BIT, 1U) == 0U) {
    return STM_ERR_TIMEOUT;
  }

  /* 2) 切系统时钟到 HSI，并确认 SWS 已反映为 HSI。 */
  RCC_CFGR &= ~RCC_CFGR_SW_MASK;
  RCC_CFGR |= RCC_CFGR_SW_HSI;
  /* SWS 字段的“HSI”状态是 00，因此等待相应位清零。 */
  if (wait_reg_bit(&RCC_CFGR, RCC_CFGR_SWS_MASK, 0U) == 0U) {
    return STM_ERR_TIMEOUT;
  }

  /* 3) 关闭 PLL（8MHz 档不需要），并尽量等待 PLLRDY 清零。 */
  RCC_CR &= ~RCC_CR_PLLON_BIT;
  (void)wait_reg_bit(&RCC_CR, RCC_CR_PLLRDY_BIT, 0U);

  /* 4) 总线分频回归默认：AHB=/1，APB1=/1，APB2=/1。 */
  RCC_CFGR &= ~(RCC_CFGR_HPRE_MASK | RCC_CFGR_PPRE1_MASK | RCC_CFGR_PPRE2_MASK);
  RCC_CFGR |= (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);

  /* 5) 更新软件频率缓存。 */
  g_sysclk_hz = 8000000UL;
  g_hclk_hz = 8000000UL;
  g_pclk1_hz = 8000000UL;
  g_pclk2_hz = 8000000UL;
  return STM_OK;
}

/*
1. 开 HSE
2. 等 HSE 稳定
3. 配 Flash 等待周期
4. 配总线分频
5. 配 PLL
6. 开 PLL
7. 等 PLL 锁定
8. SYSCLK 切到 PLL
9. 更新软件变量
*/
/*
         HSE 8MHz
             │
             ▼
         PLL ×9
             │
             ▼
      SYSCLK 72MHz
             │
        AHB /1
             │
        HCLK 72MHz
         /        \
        /          \
   APB1 /2       APB2 /1
      │              │
  PCLK1 36MHz    PCLK2 72MHz
*/
static stm_status_t apply_hse_pll_72mhz(void)
{
  /* 1) 打开外部高速时钟 HSE，并等待稳定。 */
  RCC_CR |= RCC_CR_HSEON_BIT;
  if (wait_reg_bit(&RCC_CR, RCC_CR_HSERDY_BIT, 1U) == 0U) {
    return STM_ERR_TIMEOUT;
  }

  /* 2) 提高 Flash 访问能力：开预取，等待周期设为 2（72MHz 常用）。 */
  FLASH_ACR |= FLASH_ACR_PRFTBE_BIT;
  FLASH_ACR = (FLASH_ACR & ~0x7U) | FLASH_ACR_LATENCY_2;

  /* 3) 配置时钟树：
   * - AHB=/1
   * - APB1=/2（F1 上 APB1 <= 36MHz）
   * - APB2=/1
   * - PLL 源 = HSE，倍频 x9 -> 72MHz */
  RCC_CFGR &= ~(RCC_CFGR_SW_MASK | RCC_CFGR_PLLSRC_MASK | RCC_CFGR_PLLXTPRE_MASK |
                RCC_CFGR_PLLMUL_MASK | RCC_CFGR_PPRE1_MASK | RCC_CFGR_PPRE2_MASK |
                RCC_CFGR_HPRE_MASK);
  RCC_CFGR |= (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1 |
               RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLXTPRE_HSE_DIV1 | RCC_CFGR_PLLMUL9);

  /* 4) 打开 PLL 并等待锁定。 */
  RCC_CR |= RCC_CR_PLLON_BIT;
  if (wait_reg_bit(&RCC_CR, RCC_CR_PLLRDY_BIT, 1U) == 0U) {
    return STM_ERR_TIMEOUT;
  }

  /* 5) 切系统时钟到 PLL，并等待 SWS=PLL。 */
  RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
  if ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
    for (volatile uint32_t i = 0U; i < 2000000UL; ++i) {
      if ((RCC_CFGR & RCC_CFGR_SWS_MASK) == RCC_CFGR_SWS_PLL) {
        break;
      }
      if (i == 1999999UL) {
        return STM_ERR_TIMEOUT;
      }
    }
  }

  /* 6) 更新软件频率缓存。 */
  g_sysclk_hz = 72000000UL;
  g_hclk_hz = 72000000UL;
  g_pclk1_hz = 36000000UL;
  g_pclk2_hz = 72000000UL;
  return STM_OK;
}

stm_status_t bsp_clock_apply_profile(bsp_clock_profile_t profile)
{
  /* 对外统一入口：调用者只关心 profile，不关心 RCC 细节。 */
  if (profile == BSP_CLOCK_PROFILE_HSI_8MHZ) {
    return apply_hsi_8mhz();
  }
  if (profile == BSP_CLOCK_PROFILE_HSE_PLL_72MHZ) {
    return apply_hse_pll_72mhz();
  }
  return STM_ERR_INVALID_ARG;
}

/* 下面这些 getter 是驱动层唯一可信时钟来源。 */
uint32_t bsp_clock_get_sysclk_hz(void) { return g_sysclk_hz; }
uint32_t bsp_clock_get_hclk_hz(void) { return g_hclk_hz; }
uint32_t bsp_clock_get_pclk1_hz(void) { return g_pclk1_hz; }
uint32_t bsp_clock_get_pclk2_hz(void) { return g_pclk2_hz; }
