#ifndef DRIVERS_BREATHING_LED_H
#define DRIVERS_BREATHING_LED_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 状态呼吸灯 —— TIM2 CH1 边沿对齐 PWM。
 *
 * 前置：bsp_clock_apply_profile()、bsp_board_init()（GPIOA/TIM2 时钟）。
 * 运行时改占空比即可平滑调亮度，无需重启定时器。
 */

typedef struct {
  uint32_t carrier_hz;    /* PWM 载波频率，如 1000 Hz */
  uint16_t duty_permille; /* 0=全灭，1000=最亮 */
} breathing_led_config_t;

stm_status_t breathing_led_init_with_config(const breathing_led_config_t *config);
stm_status_t breathing_led_set_duty_permille(uint16_t duty_permille);

#endif
