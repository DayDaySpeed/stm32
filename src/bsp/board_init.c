#include "bsp/board_init.h"

#include "bsp/rcc_board.h"
#include "bsp/stm32f103_regs.h"

void bsp_board_init(void) {
  /* AHB：DMA1（ADC1 双通道 SCAN 搬运）。 */
  RCC_AHBENR |= RCC_BOARD_AHB_ENABLE_MASK;
  /* APB2：高速外设总线（最大 72MHz），挂 GPIOA/GPIOB/USART1/AFIO 等。 */
  RCC_APB2ENR |= RCC_BOARD_APB2_ENABLE_MASK;
  /* APB1：低速外设总线（最大 36MHz），挂 TIM2/TIM3/I2C1/USART2 等。 */
  RCC_APB1ENR |= RCC_BOARD_APB1_ENABLE_MASK;
}
