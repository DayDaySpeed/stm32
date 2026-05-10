#ifndef DRIVERS_SYSTICK_H
#define DRIVERS_SYSTICK_H

#include <stdint.h>

void systick_init_1ms(void);
void systick_delay_ms(uint32_t ms);
void systick_on_interrupt(void);
/* 自启动以来经过的毫秒数（在 SysTick 1ms 中断里递增）。 */
uint32_t systick_get_ms(void);

#endif
