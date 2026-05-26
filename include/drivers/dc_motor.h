#ifndef DRIVERS_DC_MOTOR_H
#define DRIVERS_DC_MOTOR_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * TB6612 有刷直流电机 —— TIM4 CH1 PWM + AIN1/AIN2 方向。
 *
 * dc_motor_gpio_safe_early() 须在 main 最早阶段调用，防止上电误转。
 * speed_permille：-1000..1000，符号表示正反转，绝对值表示 PWM 占空。
 */

typedef struct {
  uint32_t pwm_hz;
  int16_t speed_permille;
} dc_motor_config_t;

void dc_motor_gpio_safe_early(void);

stm_status_t dc_motor_init_with_config(const dc_motor_config_t *config);
stm_status_t dc_motor_set_speed_signed(int16_t speed_permille);
stm_status_t dc_motor_get_speed_signed(int16_t *out_speed_permille);
stm_status_t dc_motor_stop(void);

#endif
