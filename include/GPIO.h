#ifndef __GPIO_H
#define __GPIO_H

#include <stdint.h>

#define GPIOA_BASE      (0x40010800UL)
#define GPIOB_BASE      (0x40010C00UL)
#define GPIOC_BASE      (0x40011000UL)
#define GPIOD_BASE      (0x40011400UL)
#define GPIOE_BASE      (0x40011800UL)
#define GPIOD_
#define GPIOC_CRH       (*(volatile uint32_t *)(GPIOC_BASE + 0x04UL))    // GPIOC 端口配置寄存器高 8 位（管脚 8~15）
#define GPIOC_ODR       (*(volatile uint32_t *)(GPIOC_BASE + 0x0CUL))    // GPIOC 输出数据寄存器


#define GPIO13_MODE_POS (20U)                        // CRH 中 PC13 对应字段起始位
#define GPIO13_MODE_MSK (0xFU << GPIO13_MODE_POS)   //清掉 PC13 的那 4 位配置
#define GPIO13_OUT_2M_PP (0x2U << GPIO13_MODE_POS)  //把 PC13 配成目标模式（这里是输出推挽 2MHz）
#define GPIO13_ODR_BIT  (1U << 13)                 // GPIO 端口输出数据寄存器（Output Data Register）










#endif