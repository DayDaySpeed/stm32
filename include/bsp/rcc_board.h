#ifndef BSP_RCC_BOARD_H
#define BSP_RCC_BOARD_H

#include <stdint.h>

#include "bsp/stm32f103_regs.h"

/*
 * 本板外设时钟使能掩码组合 —— 由 bsp_board_init() 一次性写入 RCC_*ENR。
 * 换板增删外设时只改此处与 board_pin_mux.h。
 */

/* APB2：AFIO、GPIOA/B、USART1、ADC1、TIM1 */
#define RCC_BOARD_APB2_ENABLE_MASK                                               \
  ((uint32_t)(RCC_AFIOEN_BIT | RCC_IOPAEN_BIT | RCC_IOPBEN_BIT |                 \
              RCC_USART1EN_BIT | RCC_ADC1EN_BIT | RCC_TIM1EN_BIT))

/* APB1：I2C1、TIM2/3/4（呼吸灯、编码器、电机 PWM） */
#define RCC_BOARD_APB1_ENABLE_MASK                                               \
  ((uint32_t)(RCC_I2C1EN_BIT | RCC_TIM2EN_BIT | RCC_TIM3EN_BIT | RCC_TIM4EN_BIT))

/* AHB：DMA1（ADC SCAN 搬运） */
#define RCC_BOARD_AHB_ENABLE_MASK ((uint32_t)(RCC_DMA1EN_BIT))

#endif
