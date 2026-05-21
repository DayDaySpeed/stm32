#ifndef DRIVERS_SENSOR_LED_H
#define DRIVERS_SENSOR_LED_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 光敏/热敏 指示 LED（TIM1 PWM）
 *   PA8  = CH1，LDR 越亮灯越亮
 *   PA11 = CH4，NTC 温度越高灯越亮
 *
 * 接线：引脚 → 限流电阻(330Ω~1kΩ) → LED+ → LED- → GND
 */

typedef struct {
  uint32_t pwm_hz;
} sensor_led_config_t;

stm_status_t sensor_led_init_with_config(const sensor_led_config_t *config);
stm_status_t sensor_led_init(void);

stm_status_t sensor_led_set_ldr_permille(uint16_t duty_permille);
stm_status_t sensor_led_set_ntc_permille(uint16_t duty_permille);

#endif
