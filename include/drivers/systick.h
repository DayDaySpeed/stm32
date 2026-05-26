#ifndef DRIVERS_SYSTICK_H
#define DRIVERS_SYSTICK_H

#include <stdint.h>

/*
 * Cortex-M3 SysTick 1ms 节拍。
 * systick_on_interrupt() 由 SysTick_Handler 调用；应用层用 get_ms 做周期任务。
 */

void systick_init_1ms(void);
void systick_delay_ms(uint32_t ms);
void systick_on_interrupt(void);
uint32_t systick_get_ms(void);

#endif
