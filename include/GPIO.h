#ifndef __GPIO_H
#define __GPIO_H

#include <stdint.h>

#define GPIO_BASE       (0x40010800UL)
#define GPIOA_BASE      (0x40010800UL)
#define GPIOB_BASE      (0x40010C00UL)
#define GPIOC_BASE      (0x40011000UL)

#define GPIO_SET_1      (0xFFFFFFFFUL)
#define GPIO_RESET      (0x44444444UL)
#define GPIO_ODR_OFFSET       (0x0CUL)   // GPIO 输出数据寄存器


#define GPIO_OUT_2M_PP   (0x2U)


#endif