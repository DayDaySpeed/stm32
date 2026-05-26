#ifndef BSP_BOARD_DEVICES_H
#define BSP_BOARD_DEVICES_H

#include <stdint.h>

#include "common/stm_status.h"
#include "drivers/adc1_dual_scan_dma.h"

/*
 * 板级逻辑设备 API —— app 只依赖本头文件，不直接调用 drivers/hal。
 */

/* --- 串口控制台（USART1） --- */

/* 按板级默认 115200 初始化 USART1，并注册 stm_log 输出到串口。 */
stm_status_t bsp_console_init(void);
/* 开启 RX 中断与环形缓冲（init 后单独调用以便先完成基础配置）。 */
stm_status_t bsp_console_enable_rx_interrupt(void);
/* text：以阻塞方式发送以 '\\0' 结尾的字符串。 */
stm_status_t bsp_console_write_string_blocking(const char *text);
/* 非阻塞读一行；out/out_size 为输出缓冲；无完整行时返回 STM_ERR_BUSY。 */
stm_status_t bsp_console_read_line_try(char *out, uint16_t out_size);
/* USART1 中断服务程序入口，由 main 中 USART1_IRQHandler 转发。 */
void bsp_console_irq_handler(void);

/* --- OLED 显示（SSD1306 + I2C1） --- */

/* 初始化 I2C1 + SSD1306 默认实例并清屏。 */
stm_status_t bsp_display_init(void);
/* I2C 总线恢复并重新初始化 OLED（卡死时由 app 调用）。 */
stm_status_t bsp_display_recover(void);
/* 清帧缓冲并整屏刷新。 */
stm_status_t bsp_display_clear(void);
/* page：0..7 行（每行 8px）；col_px：列像素；fmt：printf 风格格式串。 */
stm_status_t bsp_display_write_text_atf(uint16_t page, uint16_t col_px,
                                        const char *fmt, ...);

/* --- 状态呼吸灯（TIM2 PWM） --- */

/* 按板级 TIM2 PWM 参数初始化状态呼吸灯。 */
stm_status_t bsp_status_led_init(void);
/* duty_permille：0=灭，1000=最亮。 */
stm_status_t bsp_status_led_set_duty_permille(uint16_t duty_permille);

/* --- 旋钮编码器（TIM3） --- */

/* 按 board_config 方向初始化 TIM3 编码器。 */
stm_status_t bsp_wheel_encoder_init(void);
/* out_count：当前 16 位有符号累计计数值。 */
stm_status_t bsp_wheel_encoder_read_count(int16_t *out_count);

/* --- 直流电机（TB6612 + TIM4） --- */

/* 上电最早阶段调用：PWM/方向脚拉低，防误转。须在 bsp_board_init 之后。 */
void bsp_dc_motor_gpio_safe_early(void);
/* 初始化 TIM4 PWM 与 TB6612 方向脚。 */
stm_status_t bsp_dc_motor_init(void);
/* speed_permille：-1000..1000，符号表方向；经 board_config 可选取反。 */
stm_status_t bsp_dc_motor_set_speed_signed(int16_t speed_permille);
/* out_speed_permille：当前速度，经 board_config 取反还原用户视角。 */
stm_status_t bsp_dc_motor_get_speed_signed(int16_t *out_speed_permille);
/* 等价于 set_speed_signed(0)。 */
stm_status_t bsp_dc_motor_stop(void);

/* --- 模拟传感器（ADC1 三路 SCAN+DMA） --- */

/* 初始化 ADC + 热敏分压参数 + 红外通道标记。 */
stm_status_t bsp_analog_sensors_init(void);
/* 一次 SCAN 取光敏/热敏平均 raw（0..4095）；scan_count：平均次数，须 >0。 */
stm_status_t bsp_analog_sensors_read_pair_average(uint16_t *out_photo_raw12,
                                                  uint16_t *out_therm_raw12,
                                                  uint8_t scan_count);
/* 一次 SCAN 取三路平均；out_samples 至少 ADC1_DUAL_SLOT_COUNT 元素。 */
stm_status_t bsp_analog_sensors_read_all_average(
    uint16_t out_samples[ADC1_DUAL_SLOT_COUNT], uint8_t scan_count);
/* 红外反射 raw 平均；sample_count：扫描平均次数。 */
stm_status_t bsp_ir_reflect_read_raw_average(uint16_t *out_raw12,
                                             uint8_t sample_count);

/* --- 蜂鸣器 / 传感器指示 LED --- */

/* 配置蜂鸣器 GPIO 极性并默认关闭。 */
stm_status_t bsp_buzzer_init(void);
/* duration_ms：鸣叫时长（阻塞 delay）。 */
stm_status_t bsp_buzzer_beep_blocking(uint32_t duration_ms);
/* 初始化 TIM1 双路传感器指示 LED PWM。 */
stm_status_t bsp_sensor_led_init(void);
/* 读 LDR/NTC raw 并更新 TIM1 双 LED PWM 占空比。 */
stm_status_t bsp_sensor_led_update_from_sensors(void);

/* therm_raw12：热敏 ADC 原始值；out_celsius_x10：输出 0.1°C（253=25.3°C）。 */
stm_status_t bsp_temperature_read_celsius_x10_from_raw(uint16_t therm_raw12,
                                                       int16_t *out_celsius_x10);

/* 按本板顺序初始化电机/串口/OLED/灯/编码器/传感器/蜂鸣器等。 */
stm_status_t bsp_default_devices_init(void);

#endif
