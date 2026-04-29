#ifndef __GPIO_H
#define __GPIO_H

#include <stdint.h>

#define GPIOA_BASE      (0x40010800UL)
#define GPIOB_BASE      (0x40010C00UL)
#define GPIOC_BASE      (0x40011000UL)


#define GPIOB_CRL       (*(volatile uint32_t *)(GPIOB_BASE + 0x00UL))
#define GPIOB_CRH       (*(volatile uint32_t *)(GPIOB_BASE + 0x04UL))
#define GPIOB_IDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x08UL))
#define GPIOB_ODR       (*(volatile uint32_t *)(GPIOB_BASE + 0x0CUL))

#define GPIOA_CRL       (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x0CUL))

#define GPIOC_CRH       (*(volatile uint32_t *)(GPIOC_BASE + 0x04UL))    // GPIOC 端口配置寄存器高 8 位（管脚 8~15）
#define GPIOC_ODR       (*(volatile uint32_t *)(GPIOC_BASE + 0x0CUL))    // GPIOC 输出数据寄存器

#define GPIO_SET_1       (0xFU)

#define PC13_MODE_POS       (20U)                         // CRH 中 PC13 对应字段起始位
#define PC13_MODE_MSK       (GPIO_SET_1 << PC13_MODE_POS)       // 清掉 PC13 的那 4 位配置
#define PC13_OUT_2M_PP      (0x2U << PC13_MODE_POS)       // PC13 输出推挽 2MHz CNF=00 MODE=10
#define PC13_ODR_BIT        (1U << 13)

#define PA0_MODE_POS        (0U)
#define PA0_MODE_MSK        (GPIO_SET_1 << PA0_MODE_POS)
#define PA0_OUT_2M_PP       (0x2U << PA0_MODE_POS)         // PA0 推挽输出 2MHz CNF=00 MODE=10
#define PA0_ODR_BIT         (1U << 0)

#define PA1_MODE_POS        (4U)
#define PA1_MODE_MSK        (GPIO_SET_1 << PA1_MODE_POS)
#define PA1_OUT_2M_PP       (0x2U << PA1_MODE_POS)         // PA1 推挽输出 2MHz CNF=00 MODE=10
#define PA1_ODR_BIT         (1U << 1)

 

#define PB0_MODE_POS        (0U)
#define PB0_MODE_MSK        (GPIO_SET_1 << PB0_MODE_POS)
#define PB0_IN_PUPD         (0x8U << PB0_MODE_POS)         //CNF=10 MODE=00 上拉输入
#define PB0_ODR_BIT         (1U << 0)

#define PB1_MODE_POS        (4U)
#define PB1_MODE_MSK        (GPIO_SET_1 << PB1_MODE_POS)
#define PB1_IN_PUPD         (0x8U << PB1_MODE_POS)         //CNF=10 MODE=00 上拉输入
#define PB1_ODR_BIT         (1U << 1)

#define PB10_CRH_POS         (8U)
#define PB10_CRH_MSK         (GPIO_SET_1 << PB10_CRH_POS)
#define PB10_IN_PUPD         (0x8U << PB10_CRH_POS)         //CNF=10 MODE=00 上拉输入
#define PB10_ODR_BIT         (1U << 10)






#endif