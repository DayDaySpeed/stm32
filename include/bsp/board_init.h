#ifndef BSP_BOARD_INIT_H
#define BSP_BOARD_INIT_H

/*
 * 板级最小硬件准备：使能本板用到的 APB2 外设时钟等。
 * 须在 USART1 / SSD1306 等驱动初始化之前调用一次（通常由 main 最先调用）。
 */
void bsp_board_init(void);

#endif
