#ifndef BSP_STM32F103_REGS_H
#define BSP_STM32F103_REGS_H

#include <stdint.h>

/* RCC */
#define RCC_BASE            (0x40021000UL)
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x18UL))

#define RCC_AFIOEN_BIT      (1U << 0)
#define RCC_IOPAEN_BIT      (1U << 2)
#define RCC_IOPCEN_BIT      (1U << 4)
#define RCC_USART1EN_BIT    (1U << 14)

/* GPIOA */
#define GPIOA_BASE          (0x40010800UL)
#define GPIOA_CRH           (*(volatile uint32_t *)(GPIOA_BASE + 0x04UL))

/* GPIOC */
#define GPIOC_BASE          (0x40011000UL)
#define GPIOC_CRH           (*(volatile uint32_t *)(GPIOC_BASE + 0x04UL))
#define GPIOC_ODR           (*(volatile uint32_t *)(GPIOC_BASE + 0x0CUL))

/* USART1 */
#define USART1_BASE         (0x40013800UL)
#define USART1_SR           (*(volatile uint32_t *)(USART1_BASE + 0x00UL))
#define USART1_DR           (*(volatile uint32_t *)(USART1_BASE + 0x04UL))
#define USART1_BRR          (*(volatile uint32_t *)(USART1_BASE + 0x08UL))
#define USART1_CR1          (*(volatile uint32_t *)(USART1_BASE + 0x0CUL))

#define USART_SR_RXNE_BIT   (1U << 5)   /* 接收数据寄存器非空（有新数据可读） */
#define USART_SR_TXE_BIT    (1U << 7)   /* 发送数据寄存器空（可写入下一个字节） */
#define USART_CR1_RE_BIT    (1U << 2)   /* 接收使能 */
#define USART_CR1_TE_BIT    (1U << 3)   /* 发送使能 */
#define USART_CR1_UE_BIT    (1U << 13)  /* USART 总使能 */

/* SysTick */
#define SYSTICK_BASE        (0xE000E010UL)
#define SYST_CSR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x00UL))
#define SYST_RVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x04UL))
#define SYST_CVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x08UL))

#define SYSTICK_ENABLE_BIT  (1U << 0)
#define SYSTICK_TICKINT_BIT (1U << 1)
#define SYSTICK_CLKSRC_BIT  (1U << 2)

#endif
