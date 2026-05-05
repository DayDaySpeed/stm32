#ifndef BSP_STM32F103_REGS_H
#define BSP_STM32F103_REGS_H

#include <stdint.h>

/* RCC */
#define RCC_BASE            (0x40021000UL)
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x1CUL))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x18UL))

#define RCC_I2C1EN_BIT      (1U << 21)

#define RCC_AFIOEN_BIT      (1U << 0)
#define RCC_IOPAEN_BIT      (1U << 2)
#define RCC_IOPBEN_BIT      (1U << 3)
#define RCC_IOPCEN_BIT      (1U << 4)
#define RCC_USART1EN_BIT    (1U << 14)

/* AFIO（重映射等） */
#define AFIO_BASE           (0x40010000UL)
#define AFIO_MAPR           (*(volatile uint32_t *)(AFIO_BASE + 0x04UL))
#define AFIO_MAPR_I2C1_REMAP_BIT (1U << 1) /* 1: I2C1_SCL=PB8, I2C1_SDA=PB9 */

/* GPIOA */
#define GPIOA_BASE          (0x40010800UL)
#define GPIOA_CRH           (*(volatile uint32_t *)(GPIOA_BASE + 0x04UL))

/* GPIOB */
#define GPIOB_BASE          (0x40010C00UL)
#define GPIOB_CRH           (*(volatile uint32_t *)(GPIOB_BASE + 0x04UL))
#define GPIOB_IDR           (*(volatile uint32_t *)(GPIOB_BASE + 0x08UL))
#define GPIOB_ODR           (*(volatile uint32_t *)(GPIOB_BASE + 0x0CUL))
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x10UL))

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
#define USART_CR1_RXNEIE_BIT (1U << 5)  /* RXNE 中断使能 */
#define USART_CR1_UE_BIT    (1U << 13)  /* USART 总使能 */
#define USART_CR1_OVER8_BIT (1U << 15)  /* 1=8倍过采样，0=16倍过采样 */

/* I2C1（APB1） */
#define I2C1_BASE           (0x40005400UL)
#define I2C1_CR1            (*(volatile uint32_t *)(I2C1_BASE + 0x00UL))
#define I2C1_CR2            (*(volatile uint32_t *)(I2C1_BASE + 0x04UL))
#define I2C1_DR             (*(volatile uint32_t *)(I2C1_BASE + 0x10UL))
#define I2C1_SR1            (*(volatile uint32_t *)(I2C1_BASE + 0x14UL))
#define I2C1_SR2            (*(volatile uint32_t *)(I2C1_BASE + 0x18UL))
#define I2C1_CCR            (*(volatile uint32_t *)(I2C1_BASE + 0x1CUL))
#define I2C1_TRISE          (*(volatile uint32_t *)(I2C1_BASE + 0x20UL))

#define I2C_CR1_PE_BIT      (1U << 0)   /* 外设使能（Peripheral Enable） */
#define I2C_CR1_START_BIT   (1U << 8)   /* 主机发 START 条件 */
#define I2C_CR1_STOP_BIT    (1U << 9)   /* 主机发 STOP 条件，结束当前传输 */

#define I2C_SR1_SB_BIT      (1U << 0)   /* START 已发送（EV5） */
#define I2C_SR1_ADDR_BIT    (1U << 1)   /* 地址阶段完成并收到应答（EV6） */
#define I2C_SR1_BTF_BIT     (1U << 2)   /* 字节传输完成（Data register + shift register 都空） */
#define I2C_SR1_TXE_BIT     (1U << 7)   /* DR 发送寄存器空，可写下一个字节（EV8） */
#define I2C_SR1_AF_BIT      (1U << 10)  /* 应答失败（Acknowledge Failure，常见于收到 NACK） */

#define I2C_SR2_BUSY_BIT    (1U << 1)   /* 总线忙：检测 START 到 STOP 之间的总线占用状态 */

/* NVIC */
#define NVIC_BASE           (0xE000E100UL)
#define NVIC_ISER0          (*(volatile uint32_t *)(NVIC_BASE + 0x00UL))
#define NVIC_ISER1          (*(volatile uint32_t *)(NVIC_BASE + 0x04UL))
#define NVIC_USART1_IRQ_BIT (1U << 5)   /* USART1 IRQn=37 -> ISER1 bit5 */


/* SysTick */
#define SYSTICK_BASE        (0xE000E010UL)
#define SYST_CSR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x00UL))
#define SYST_RVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x04UL))
#define SYST_CVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x08UL))

#define SYSTICK_ENABLE_BIT  (1U << 0)
#define SYSTICK_TICKINT_BIT (1U << 1)
#define SYSTICK_CLKSRC_BIT  (1U << 2)

#endif
