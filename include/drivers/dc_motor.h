#ifndef DRIVERS_DC_MOTOR_H
#define DRIVERS_DC_MOTOR_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * TB6612 单路直流电机驱动（本板接线）
 *
 *   PWMA  <- PB6 (TIM4_CH1 PWM)
 *   AIN1  <- PB7 (GPIO)
 *   AIN2  <- GND（固定）
 *   STBY  <- 3.3V（模块侧常使能）
 *
 * AIN2=0 时方向真值表：
 *   AIN1=0 → 停；AIN1=1 + PWMA>0 → 单方向转
 *
 * 前置：bsp_clock_apply_profile()、bsp_board_init()
 */

typedef struct {
  uint32_t pwm_hz;
  uint16_t duty_permille;
} dc_motor_config_t;

stm_status_t dc_motor_init_with_config(const dc_motor_config_t *config);
stm_status_t dc_motor_init(void);

stm_status_t dc_motor_set_duty_permille(uint16_t duty_permille);
stm_status_t dc_motor_get_duty_permille(uint16_t *out_duty_permille);
stm_status_t dc_motor_stop(void);

#endif
