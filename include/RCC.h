#ifndef __RCC_H
#define __RCC_H
/**
 * @brief RCC 寄存器
 * @note RCC复位和时钟控制寄存器
 * @note 先开启时钟，才能使用外设
 * @note RCC_BASE 是 RCC 寄存器基地址
 * @note RCC_APB2ENR 是 APB2 外设时钟使能寄存器
 * @note RCC_IOPAEN_BIT 是 IOPA 时钟使能位
 * @note RCC_IOPCEN_BIT 是 IOPC 时钟使能位
 */
#include <stdint.h>

#define RCC_BASE        (0x40021000UL)
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18UL))      // APB2 外设时钟使能寄存器

#define RCC_IOPAEN_BIT  (1U << 2)                    // IOPA 时钟使能位
#define RCC_IOPBEN_BIT  (1U << 3)                    // IOPB 时钟使能位
#define RCC_IOPCEN_BIT  (1U << 4)                    // IOPC 时钟使能位



#endif