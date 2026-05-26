#ifndef DRIVERS_SENSOR_LED_H
#define DRIVERS_SENSOR_LED_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 传感器指示 LED —— TIM1 双通道 PWM。
 * CH1：光敏 LDR 灯；CH4：热敏 NTC 灯。占空比 0~1000 permille。
 */

typedef struct {
  uint32_t pwm_hz;
} sensor_led_config_t;

stm_status_t sensor_led_init_with_config(const sensor_led_config_t *config);
stm_status_t sensor_led_set_ldr_permille(uint16_t duty_permille);
stm_status_t sensor_led_set_ntc_permille(uint16_t duty_permille);

#endif
