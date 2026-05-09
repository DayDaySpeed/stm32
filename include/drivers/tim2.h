#ifndef DRIVERS_TIM2_H
#define DRIVERS_TIM2_H

#include <stdint.h>

/* 初始化 TIM2 为按秒周期触发更新中断。period_seconds=1 表示每秒一次。 */
uint8_t tim2_init_periodic_interrupt_seconds(uint32_t period_seconds);

/* 在 TIM2_IRQHandler 中调用，负责清中断标志并回调用户钩子。 */
void tim2_irq_handler(void);

/* 用户可在其他 .c 文件中实现同名强符号，编写自己的定时中断业务逻辑。 */
void tim2_on_second_interrupt(void);

#endif
