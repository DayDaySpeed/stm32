#ifndef __SYS_H
#define __SYS_H

#include <stdint.h>


#define SYSTICK_BASE    (0xE000E010UL)
#define SYST_CSR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00UL))  // SysTick 控制与状态寄存器
#define SYST_RVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04UL))  // SysTick 重装载值寄存器
#define SYST_CVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x08UL))  // SysTick 当前计数值寄存器

#define SYSTICK_ENABLE_BIT    (1U << 0)             // 使能 SysTick
#define SYSTICK_TICKINT_BIT   (1U << 1)             // 计数到 0 触发中断
#define SYSTICK_CLKSRC_BIT    (1U << 2)             // 时钟源选择：处理器时钟


#define SYSCLK_HZ       (8000000UL)                 // 系统时钟 8MHz

extern volatile uint32_t g_ms_ticks;
void systick_init(void);
void delay_ms(uint32_t ms);

#endif