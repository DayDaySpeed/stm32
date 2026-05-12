#include "app/app.h"
#include "bsp/board_init.h"
#include "bsp/board_devices.h"
#include "bsp/clock.h"
#include "common/stm_fault.h"
#include "drivers/systick.h"

void SysTick_Handler(void) { systick_on_interrupt(); }

void USART1_IRQHandler(void) { bsp_console_irq_handler(); }

int main(void) {
  stm_status_t st = STM_OK;

  /* 可切换时钟方案：HSI 8MHz / HSE+PLL 72MHz */
  st = bsp_clock_apply_profile(BSP_CLOCK_PROFILE_HSE_PLL_72MHZ);
  if (st != STM_OK) {
    stm_fault_halt("clock", st);
  }
  bsp_board_init();
  st = app_init();
  if (st != STM_OK) {
    stm_fault_halt("app_init", st);
  }
  app_run_forever();
}
