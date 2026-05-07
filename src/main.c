#include "app/app.h"
#include "bsp/board_init.h"
#include "drivers/systick.h"
#include "drivers/usart1.h"

void SysTick_Handler(void) { systick_on_interrupt(); }

void USART1_IRQHandler(void) { usart1_irq_handler(); }

int main(void) {
  bsp_board_init();
  app_init();
  app_run_forever();
}
