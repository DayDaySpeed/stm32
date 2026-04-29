#include "SYS.h"

volatile uint32_t g_ms_ticks = 0U;

void SysTick_Handler(void)
{
  g_ms_ticks++;
}

void systick_init(void)
{
  SYST_RVR = (SYSCLK_HZ / 1000UL) - 1UL;  //每 8000 个时钟计一次 1ms
  SYST_CVR = 0UL;  //清当前计数器，确保从头开始计数，避免上电残值影响
  /**
  *一次性打开 3 个功能：
  * CLKSRC：用内核时钟作为 SysTick 时钟源
  * TICKINT：计到 0 产生中断
  * ENABLE：启动 SysTick 计数
  */
  SYST_CSR = SYSTICK_CLKSRC_BIT | SYSTICK_TICKINT_BIT | SYSTICK_ENABLE_BIT;
}

void delay_ms(uint32_t ms)
{
  uint32_t start = g_ms_ticks;
  while ((g_ms_ticks - start) < ms) {
  }
}
