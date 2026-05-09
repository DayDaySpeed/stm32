#include "drivers/tim2.h"

#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"
#include <stdint.h>

/* 默认空实现：用户可在应用层覆盖此弱符号。 */
__attribute__((weak)) 
void tim2_on_second_interrupt(void) {
}

/*
 * 初始化 TIM2 为按秒周期触发更新中断。
 *
 * 配置目标：
 * - 先把 TIM2 计数时钟整形成 10kHz；
 * - ARR 按 period_seconds 计算，使更新周期 = period_seconds 秒；
 * - 开启更新中断（UIE）并在 NVIC 放行 TIM2 IRQ；
 * - 最后启动计数器。
 *
 * 返回值：
 * - 1: 初始化成功
 * - 0: 参数非法或无法在当前位宽下生成目标周期
 */
uint8_t tim2_init_periodic_interrupt_seconds(uint32_t period_seconds) {
  uint32_t pclk1_hz = BSP_PCLK1_HZ;
  uint32_t tim_clk_hz = pclk1_hz;
  uint32_t psc = 0U;
  uint32_t arr = 0U;
  uint32_t target_ticks = 0U;

  /* 周期参数必须大于 0 秒。 */
  if (period_seconds == 0U) {
    return 0U;
  }

  /* APB1 分频不为 1 时，通用定时器时钟 = 2 * PCLK1。 */
  if (pclk1_hz != bsp_clock_get_hclk_hz()) {
    tim_clk_hz = pclk1_hz * 2UL;
  }
  /* 10kHz 分频基准要求定时器输入时钟不低于 10kHz。 */
  if (tim_clk_hz < 10000UL) {
    return 0U;
  }

  /* 先关计数器，避免配置过程中异常更新事件。 */
  TIM2_CR1 &= ~TIM_CR1_CEN_BIT;

  /* TIMCLK -> 10kHz */
  psc = (tim_clk_hz / 10000UL) - 1UL;
  /* 10kHz 计数下，每秒 10000 tick。 */
  if (period_seconds > (0x10000UL / 10000UL)) {
    return 0U;
  }
  target_ticks = period_seconds * 10000UL;
  if ((target_ticks == 0U) || (target_ticks > 0x10000UL)) {
    /* TIM2 ARR 在当前工程配置中按 16 位使用，最大 65536 tick。 */
    return 0U;
  }
  arr = target_ticks - 1UL;

  TIM2_PSC = psc;            /* 预分频器：把 TIMCLK 降到 10kHz */
  TIM2_ARR = arr;            /* 自动重装值：每 target_ticks 个计数产生更新事件 */
  TIM2_EGR = TIM_EGR_UG_BIT; /* 产生更新事件，立即装载 PSC/ARR */
  TIM2_SR = 0U;              /* 清除历史状态位，避免脏标志触发假中断 */
  TIM2_DIER |= TIM_DIER_UIE_BIT; /* 允许更新中断请求（外设侧） */

  NVIC_ISER0 |= NVIC_TIM2_IRQ_BIT; /* NVIC 放行 TIM2 IRQ（IRQn=28） */
  TIM2_CR1 |= TIM_CR1_CEN_BIT;     /* 启动计数器，开始按设定秒周期产生更新 */
  return 1U;
}

void tim2_irq_handler(void) {
  if ((TIM2_SR & TIM_SR_UIF_BIT) == 0U) {
    return;
  }
  TIM2_SR &= ~TIM_SR_UIF_BIT;
  tim2_on_second_interrupt();
}
