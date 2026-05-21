#ifndef BSP_BOARD_DEVICES_H
#define BSP_BOARD_DEVICES_H

#include <stdint.h>

#include "common/stm_status.h"
#include "drivers/adc1_dual_scan_dma.h"

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
/* I2C 总线恢复 + SSD1306 重新初始化（OLED 卡死时由 app 调用）。 */
stm_status_t bsp_display_recover(void);
stm_status_t bsp_display_clear(void);
stm_status_t bsp_display_write_text_atf(uint16_t page, uint16_t col_px,
                                        const char *fmt, ...);

stm_status_t bsp_status_led_init(void);
stm_status_t bsp_status_led_set_duty_permille(uint16_t duty_permille);

stm_status_t bsp_wheel_encoder_init(void);
stm_status_t bsp_wheel_encoder_read_count(int16_t *out_count);
stm_status_t bsp_wheel_encoder_read_direction(uint8_t *out_direction);

void bsp_dc_motor_gpio_safe_early(void);
stm_status_t bsp_dc_motor_init(void);
stm_status_t bsp_dc_motor_set_speed_signed(int16_t speed_permille);
stm_status_t bsp_dc_motor_get_speed_signed(int16_t *out_speed_permille);
stm_status_t bsp_dc_motor_set_speed_permille(uint16_t duty_permille);
stm_status_t bsp_dc_motor_get_speed_permille(uint16_t *out_duty_permille);
stm_status_t bsp_dc_motor_stop(void);

/* 一次性初始化 ADC1 双通道 SCAN+DMA 及光敏/热敏逻辑驱动。 */
stm_status_t bsp_analog_sensors_init(void);

stm_status_t bsp_ambient_light_init(void);
stm_status_t bsp_ambient_light_read_raw_average(uint16_t *out_raw12,
                                                uint8_t sample_count);

/*
 * 一次 ADC SCAN+DMA 同时得到光敏/热敏平均原始值（0~4095）。
 * out_photo_raw12、out_therm_raw12 均不可为 NULL；scan_count==0 非法。
 */
stm_status_t bsp_analog_sensors_read_pair_average(uint16_t *out_photo_raw12,
                                                  uint16_t *out_therm_raw12,
                                                  uint8_t scan_count);

/*
 * 一次 ADC SCAN+DMA 同时得到光敏/热敏/红外平均原始值（0~4095）。
 * out_samples 至少 3 个元素，对应 ADC1_DUAL_SLOT_*。
 */
stm_status_t bsp_analog_sensors_read_all_average(
    uint16_t out_samples[ADC1_DUAL_SLOT_COUNT], uint8_t scan_count);

/* 反射红外（PA3 / ADC1_IN3）平均原始值 0~4095。 */
stm_status_t bsp_ir_reflect_read_raw_average(uint16_t *out_raw12,
                                             uint8_t sample_count);

stm_status_t bsp_buzzer_init(void);
stm_status_t bsp_buzzer_beep_blocking(uint32_t duration_ms);

stm_status_t bsp_sensor_led_init(void);
stm_status_t bsp_sensor_led_update_from_sensors(void);

/* 由热敏 ADC 原始值换算温度，单位 0.1 摄氏度（例如 253 = 25.3°C）。 */
stm_status_t bsp_temperature_read_celsius_x10_from_raw(uint16_t therm_raw12,
                                                       int16_t *out_celsius_x10);

/* 一次性初始化本板默认设备。 */
stm_status_t bsp_default_devices_init(void);

#endif
