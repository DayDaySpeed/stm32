#ifndef DRIVERS_SYSTICK_H
#define DRIVERS_SYSTICK_H

#include <stdint.h>

/*
 * SysTick 1ms 系统节拍。
 */

/* 配置 RVR 为 1ms 并重载；须先 bsp_clock_apply_profile()。 */
void systick_init_1ms(void);
/* ms：阻塞延时毫秒数。 */
void systick_delay_ms(uint32_t ms);
/* SysTick_Handler 内调用，递增毫秒计数。 */
void systick_on_interrupt(void);
/* 返回自 init 以来经过的毫秒数（32 位回绕）。 */
uint32_t systick_get_ms(void);

#endif
