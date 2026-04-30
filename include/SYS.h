#ifndef STM32_INCLUDE_SYS_H
#define STM32_INCLUDE_SYS_H

#include "bsp/clock.h"

/* 1ms SysTick 重装载值（基于系统主频） */
#define SYS_SYSTICK_RELOAD_1MS         ((SYSCLK_HZ / 1000UL) - 1UL)

#endif /* STM32_INCLUDE_SYS_H */