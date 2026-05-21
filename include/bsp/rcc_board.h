#ifndef BSP_RCC_BOARD_H
#define BSP_RCC_BOARD_H

#include <stdint.h>

#include "bsp/stm32f103_regs.h"

/* 与本板外设一致：AFIO、GPIOA、GPIOB、USART1、ADC1（光敏电阻等模拟量） */
#define RCC_BOARD_APB2_ENABLE_MASK                                               \
  ((uint32_t)(RCC_AFIOEN_BIT | RCC_IOPAEN_BIT | RCC_IOPBEN_BIT |                 \
              RCC_USART1EN_BIT | RCC_ADC1EN_BIT | RCC_TIM1EN_BIT))

#define RCC_BOARD_APB1_ENABLE_MASK                                               \
  ((uint32_t)(RCC_I2C1EN_BIT | RCC_TIM2EN_BIT | RCC_TIM3EN_BIT | RCC_TIM4EN_BIT))

/* DMA1：ADC1 双通道 SCAN 搬运 */
#define RCC_BOARD_AHB_ENABLE_MASK ((uint32_t)(RCC_DMA1EN_BIT))

#endif
