#ifndef BSP_BOARD_DEVICES_H
#define BSP_BOARD_DEVICES_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 板级默认逻辑设备入口
 *
 * 目标：把 app 层看到的资源名从具体外设/引脚收敛成逻辑角色。
 * app 只关心“控制台 / 显示 / 状态灯 / 编码器 / 环境光”，
 * 不直接关心 USART1 / TIM2_CH1 / ADC1_IN1 这些底层绑定。
 */

stm_status_t bsp_console_init(void);
stm_status_t bsp_console_enable_rx_interrupt(void);
stm_status_t bsp_console_write_string_blocking(const char *text);
stm_status_t bsp_console_read_line_try(char *out, uint16_t out_size);
void bsp_console_irq_handler(void);

stm_status_t bsp_display_init(void);
stm_status_t bsp_display_clear(void);
stm_status_t bsp_display_write_text_atf(uint16_t page, uint16_t col_px,
                                        const char *fmt, ...);

stm_status_t bsp_status_led_init(void);
stm_status_t bsp_status_led_set_duty_permille(uint16_t duty_permille);

stm_status_t bsp_wheel_encoder_init(void);
stm_status_t bsp_wheel_encoder_read_count(int16_t *out_count);
stm_status_t bsp_wheel_encoder_read_direction(uint8_t *out_direction);

stm_status_t bsp_ambient_light_init(void);
stm_status_t bsp_ambient_light_read_raw_average(uint16_t *out_raw12,
                                                uint8_t sample_count);

/* 一次性初始化本板默认设备。 */
stm_status_t bsp_default_devices_init(void);

#endif
