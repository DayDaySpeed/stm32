#include "bsp/board_init.h"

#include "bsp/rcc_board.h"
#include "bsp/stm32f103_regs.h"

void bsp_board_init(void)
{
  RCC_APB2ENR |= RCC_BOARD_APB2_ENABLE_MASK;
}
