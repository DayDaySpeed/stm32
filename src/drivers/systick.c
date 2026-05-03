#include "drivers/systick.h"

#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"

static volatile uint32_t g_ms_ticks = 0;

void systick_on_interrupt(void)
{
  g_ms_ticks++;
}

void systick_init_1ms(void)
{
  SYST_RVR = BSP_SYSTICK_RELOAD_1MS;
  SYST_CVR = 0UL;
  SYST_CSR = SYSTICK_CLKSRC_BIT | SYSTICK_TICKINT_BIT | SYSTICK_ENABLE_BIT;
}

void systick_delay_ms(uint32_t ms)
{
  uint32_t start = g_ms_ticks;
  while ((g_ms_ticks - start) < ms) {
  }
}
