#ifndef STM32_INCLUDE_GPIO_H
#define STM32_INCLUDE_GPIO_H

#include "bsp/stm32f103_regs.h"

/* GPIOA_CRH 里每个引脚占 4bit：CNF[1:0] + MODE[1:0] */
#define GPIO_CRL_CRH_PIN_FIELD_MASK   (0xFU)

/* PA9 = USART1_TX */
#define GPIO_PA9_CRH_POS              (4U)
#define GPIO_PA9_CRH_MASK             (GPIO_CRL_CRH_PIN_FIELD_MASK << GPIO_PA9_CRH_POS)
#define GPIO_PA9_AF_PP_50M            (0xBU << GPIO_PA9_CRH_POS)  /* 复用推挽输出 50MHz */

/* PA10 = USART1_RX */
#define GPIO_PA10_CRH_POS             (8U)
#define GPIO_PA10_CRH_MASK            (GPIO_CRL_CRH_PIN_FIELD_MASK << GPIO_PA10_CRH_POS)
#define GPIO_PA10_INPUT_FLOATING      (0x4U << GPIO_PA10_CRH_POS) /* 浮空输入 */

/* USART1 默认引脚所在寄存器（便于驱动层统一引用） */
#define GPIO_USART1_PINS_CRH_REG      GPIOA_CRH

#endif /* STM32_INCLUDE_GPIO_H */
