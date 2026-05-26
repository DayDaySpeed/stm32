/*
 * SysTick 1ms 节拍：供 delay、任务周期调度、超时判断使用。
 * 须在 bsp_clock_apply_profile() 之后调用 systick_init_1ms()。
 */
#include "drivers/systick.h"

#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"

static volatile uint32_t g_ms_ticks = 0U;

/* SysTick 中断内调用，毫秒计数 +1。 */
void systick_on_interrupt(void) { g_ms_ticks++; }

/* 须先 bsp_clock_apply_profile()；配置 1ms 重载并启动。 */
void systick_init_1ms(void) {
  SYST_RVR = BSP_SYSTICK_RELOAD_1MS;
  SYST_CVR = 0UL;
  SYST_CSR = SYSTICK_CLKSRC_BIT | SYSTICK_TICKINT_BIT | SYSTICK_ENABLE_BIT;
}

/* ms：阻塞等待毫秒数（依赖 systick 中断）。 */
void systick_delay_ms(uint32_t ms) {
  uint32_t start = g_ms_ticks;
  while ((g_ms_ticks - start) < ms) {
  }
}

/* 返回自 init 以来的毫秒 tick（32 位自然回绕）。 */
uint32_t systick_get_ms(void) { return g_ms_ticks; }
