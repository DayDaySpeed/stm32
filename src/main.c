#include "app/app.h"
#include "bsp/board_init.h"
#include "bsp/clock.h"
#include "drivers/systick.h"
#include "drivers/usart1.h"

void SysTick_Handler(void) { systick_on_interrupt(); }

void USART1_IRQHandler(void) { usart1_irq_handler(); }

int main(void) {
  /* 可切换时钟方案：HSI 8MHz / HSE+PLL 72MHz */
  if (bsp_clock_apply_profile(BSP_CLOCK_PROFILE_HSE_PLL_72MHZ) != STM_OK) {
    while (1) {
    }
  }
  bsp_board_init();
  app_init();
  app_run_forever();
}
