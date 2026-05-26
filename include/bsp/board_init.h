#ifndef BSP_BOARD_INIT_H
#define BSP_BOARD_INIT_H

/*
 * 板级最小硬件准备：使能本板 AHB/APB1/APB2 外设时钟。
 * 须在 USART/ADC/TIM/I2C 等驱动 init 之前调用一次。
 */
void bsp_board_init(void);

#endif
