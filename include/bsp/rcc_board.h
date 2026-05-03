#ifndef BSP_RCC_BOARD_H
#define BSP_RCC_BOARD_H

#include <stdint.h>

#include "bsp/stm32f103_regs.h"

/* 与本板外设一致：AFIO、GPIOA（USART1）、GPIOB（OLED 位带 I2C）、USART1 */
#define RCC_BOARD_APB2_ENABLE_MASK                                               \
  ((uint32_t)(RCC_AFIOEN_BIT | RCC_IOPAEN_BIT | RCC_IOPBEN_BIT | RCC_USART1EN_BIT))

#endif
