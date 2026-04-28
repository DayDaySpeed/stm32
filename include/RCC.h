#ifndef __RCC_H
#define __RCC_H

#include <stdint.h>

#define RCC_BASE        (0x40021000UL)
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18UL))      // APB2 外设时钟使能寄存器

#define RCC_IOPCEN_BIT  (1U << 4)                    // IOPC 时钟使能位



#endif