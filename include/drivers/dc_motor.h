#ifndef DRIVERS_DC_MOTOR_H
#define DRIVERS_DC_MOTOR_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * TB6612 有刷直流电机 —— TIM4 CH1 PWM + AIN1/AIN2 方向。
 */

typedef struct {
  uint32_t pwm_hz;          /* PWM 频率，如 10000 */
  int16_t speed_permille;   /* 初始速度 -1000..1000 */
} dc_motor_config_t;

/* 最早安全态：方向脚与 PWM 拉低；main 在 app_init 前调用。 */
void dc_motor_gpio_safe_early(void);
/* 配置 GPIO、TIM4 PWM 并应用初始速度。 */
stm_status_t dc_motor_init_with_config(const dc_motor_config_t *config);
/* speed_permille：-1000..1000，符号决定正反转，绝对值决定 PWM。 */
stm_status_t dc_motor_set_speed_signed(int16_t speed_permille);
/* out_speed_permille：输出当前 -1000..1000 速度。 */
stm_status_t dc_motor_get_speed_signed(int16_t *out_speed_permille);
/* 等价于 set_speed_signed(0)。 */
stm_status_t dc_motor_stop(void);

#endif
