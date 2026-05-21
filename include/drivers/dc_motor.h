#ifndef DRIVERS_DC_MOTOR_H
#define DRIVERS_DC_MOTOR_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * TB6612 单路直流电机（双方向）
 *
 *   PWMA  <- PB6 (TIM4_CH1 PWM)
 *   AIN1  <- PB7 (GPIO)  ─┐ 真值表控制 AO1/AO2 电流方向
 *   AIN2  <- PB5 (GPIO)  ─┘（勿再接 GND）
 *   STBY  <- 3.3V
 *
 *   正转：AIN1=1, AIN2=0, PWMA=PWM
 *   反转：AIN1=0, AIN2=1, PWMA=PWM
 *   停止：AIN1=0, AIN2=0, PWMA=0
 *
 * speed_permille：-1000..+1000，符号=方向，绝对值=占空比。
 *
 * dc_motor_gpio_safe_early() 须在其它外设 init 前调用（见 main.c）。
 */

void dc_motor_gpio_safe_early(void);

typedef struct {
  uint32_t pwm_hz;
  int16_t speed_permille;
} dc_motor_config_t;

stm_status_t dc_motor_init_with_config(const dc_motor_config_t *config);
stm_status_t dc_motor_init(void);

stm_status_t dc_motor_set_speed_signed(int16_t speed_permille);
stm_status_t dc_motor_get_speed_signed(int16_t *out_speed_permille);
stm_status_t dc_motor_stop(void);

/* 仅正转、0~1000；等价于 set_speed_signed(+duty)。 */
stm_status_t dc_motor_set_duty_permille(uint16_t duty_permille);

#endif
