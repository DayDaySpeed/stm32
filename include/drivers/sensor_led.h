#ifndef DRIVERS_SENSOR_LED_H
#define DRIVERS_SENSOR_LED_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * TIM1 双路 PWM：CH1=光敏 LED，CH4=热敏 LED。
 */

typedef struct {
  uint32_t pwm_hz; /* 两路共用载波频率 */
} sensor_led_config_t;

/* config->pwm_hz：两路 LED 共用载波；初始占空比 0。 */
stm_status_t sensor_led_init_with_config(const sensor_led_config_t *config);
/* duty_permille：LDR 灯亮度 0..1000。 */
stm_status_t sensor_led_set_ldr_permille(uint16_t duty_permille);
/* duty_permille：NTC 灯亮度 0..1000。 */
stm_status_t sensor_led_set_ntc_permille(uint16_t duty_permille);

#endif
