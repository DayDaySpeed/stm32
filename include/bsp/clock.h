#ifndef BSP_CLOCK_H
#define BSP_CLOCK_H

/* 当前工程未做 PLL 升频，复位后 SYSCLK 为 HSI 8MHz */
#define SYSCLK_HZ (8000000UL)

/* 默认 CFGR 下 APB1 未分频时与 HCLK 相同；I2C CR2 FREQ / CCR 按此计算 */
#define BSP_PCLK1_HZ SYSCLK_HZ

/* SysTick 1ms 重装载值（向下计数，LOAD = 周期 tick 数 − 1） */
#define BSP_SYSTICK_RELOAD_1MS ((SYSCLK_HZ / 1000UL) - 1UL)

#endif
