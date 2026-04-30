#ifndef DRIVERS_SYSTICK_H
#define DRIVERS_SYSTICK_H

#include <stdint.h>

void systick_init_1ms(void);
void systick_delay_ms(uint32_t ms);
void systick_on_interrupt(void);

#endif
