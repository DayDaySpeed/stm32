#include "bsp/board_devices.h"

#include <stdarg.h>

#include "common/stm_log.h"
#include "drivers/adc1_dual_scan_dma.h"
#include "drivers/dc_motor.h"
#include "drivers/encoder.h"
#include "drivers/photoresistor.h"
#include "bsp/board_config.h"
#include "drivers/buzzer.h"
#include "drivers/ir_reflect.h"
#include "drivers/thermistor.h"
#include "drivers/pwm.h"
#include "drivers/ssd1306_oled.h"
#include "hal/i2c1_master.h"
#include "drivers/usart1.h"

#define BOARD_CONSOLE_BAUDRATE 115200UL
#define BOARD_STATUS_LED_PWM_HZ 1000UL
#define BOARD_DC_MOTOR_PWM_HZ   10000UL

static const usart1_config_t g_board_console_config = {
    .baudrate = BOARD_CONSOLE_BAUDRATE,
    .oversampling = USART_OVERSAMPLING_16,
    .line_policy = USART1_LINE_CR_OR_LF,
    .enable_rx_interrupt = 0U,
};

static const tim2_ch1_pwm_config_t g_board_status_led_config = {
    .pwm_hz = BOARD_STATUS_LED_PWM_HZ,
    .duty_permille = 0U,
};

static const tim3_encoder_config_t g_board_wheel_encoder_config = {
    .direction = BOARD_WHEEL_ENCODER_DIRECTION,
};

static const dc_motor_config_t g_board_dc_motor_config = {
    .pwm_hz = BOARD_DC_MOTOR_PWM_HZ,
    .duty_permille = 0U,
};

static const adc1_dual_config_t g_board_adc_dual_config = {
    .clock_source = ADC1_DUAL_CLOCK_SOURCE_PCLK2,
    .adc_prescaler = ADC1_DUAL_PRESCALER_AUTO,
};

static const photoresistor_config_t g_board_ambient_light_config = {
    .clock_source = PHOTO_ADC_CLOCK_SOURCE_PCLK2,
    .adc_prescaler = PHOTO_ADC_PRESCALER_AUTO,
};

static const thermistor_config_t g_board_temperature_config = {
    .clock_source = THERMISTOR_ADC_CLOCK_SOURCE_PCLK2,
    .adc_prescaler = THERMISTOR_ADC_PRESCALER_AUTO,
    .divider_topology = THERMISTOR_DIVIDER_FIXED_UP_NTC_DOWN,
    .fixed_resistor_ohms = 10000U,
    .vdda_mv = 3300U,
};

static const ir_reflect_config_t g_board_ir_reflect_config = {
    .clock_source = IR_REFLECT_ADC_CLOCK_SOURCE_PCLK2,
    .adc_prescaler = IR_REFLECT_ADC_PRESCALER_AUTO,
};

static const buzzer_config_t g_board_buzzer_config = {
    .active_high = BOARD_BUZZER_ACTIVE_HIGH,
};

stm_status_t bsp_console_init(void) {
  stm_status_t st = usart1_init_with_config(&g_board_console_config);
  if (st != STM_OK) {
    return st;
  }
  stm_log_set_writer(usart1_write_string_blocking);
  return STM_OK;
}

stm_status_t bsp_console_enable_rx_interrupt(void) {
  return usart1_enable_rx_interrupt();
}

stm_status_t bsp_console_write_string_blocking(const char *text) {
  return usart1_write_string_blocking(text);
}

stm_status_t bsp_console_read_line_try(char *out, uint16_t out_size) {
  return usart1_read_line_try(out, out_size);
}

void bsp_console_irq_handler(void) { usart1_irq_handler(); }

stm_status_t bsp_display_init(void) { return ssd1306_default_init(); }

stm_status_t bsp_display_recover(void) {
  i2c1_master_bus_recover();
  return ssd1306_default_init();
}

stm_status_t bsp_display_clear(void) { return ssd1306_default_clear(); }

stm_status_t bsp_display_write_text_atf(uint16_t page, uint16_t col_px,
                                        const char *fmt, ...) {
  va_list ap;
  stm_status_t st = STM_OK;

  va_start(ap, fmt);
  st = ssd1306_default_vwrite_text_atf(page, col_px, fmt, ap);
  va_end(ap);
  return st;
}

stm_status_t bsp_status_led_init(void) {
  return tim2_ch1_pwm_init_with_config(&g_board_status_led_config);
}

stm_status_t bsp_status_led_set_duty_permille(uint16_t duty_permille) {
  return tim2_ch1_pwm_set_duty_permille(duty_permille);
}

stm_status_t bsp_wheel_encoder_init(void) {
  return tim3_encoder_init_with_config(&g_board_wheel_encoder_config);
}

stm_status_t bsp_wheel_encoder_read_count(int16_t *out_count) {
  return tim3_encoder_read_count(out_count);
}

stm_status_t bsp_wheel_encoder_read_direction(uint8_t *out_direction) {
  return tim3_encoder_read_direction(out_direction);
}

void bsp_dc_motor_gpio_safe_early(void) { dc_motor_gpio_safe_early(); }

stm_status_t bsp_dc_motor_init(void) {
  return dc_motor_init_with_config(&g_board_dc_motor_config);
}

stm_status_t bsp_dc_motor_set_speed_permille(uint16_t duty_permille) {
  return dc_motor_set_duty_permille(duty_permille);
}

stm_status_t bsp_dc_motor_get_speed_permille(uint16_t *out_duty_permille) {
  return dc_motor_get_duty_permille(out_duty_permille);
}

stm_status_t bsp_dc_motor_stop(void) { return dc_motor_stop(); }

stm_status_t bsp_analog_sensors_init(void) {
  stm_status_t st = adc1_dual_init_with_config(&g_board_adc_dual_config);
  if (st != STM_OK) {
    return st;
  }
  st = photoresistor_init_with_config(&g_board_ambient_light_config);
  if (st != STM_OK) {
    return st;
  }
  st = thermistor_init_with_config(&g_board_temperature_config);
  if (st != STM_OK) {
    return st;
  }
  return ir_reflect_init_with_config(&g_board_ir_reflect_config);
}

stm_status_t bsp_ir_reflect_read_raw_average(uint16_t *out_raw12,
                                             uint8_t sample_count) {
  return ir_reflect_read_raw_average_blocking(out_raw12, sample_count);
}

stm_status_t bsp_buzzer_init(void) {
  return buzzer_init_with_config(&g_board_buzzer_config);
}

stm_status_t bsp_buzzer_beep_blocking(uint32_t duration_ms) {
  return buzzer_beep_blocking(duration_ms);
}

stm_status_t bsp_ambient_light_init(void) {
  return photoresistor_init_with_config(&g_board_ambient_light_config);
}

stm_status_t bsp_ambient_light_read_raw_average(uint16_t *out_raw12,
                                                uint8_t sample_count) {
  return photoresistor_read_raw_average_blocking(out_raw12, sample_count);
}

stm_status_t bsp_analog_sensors_read_pair_average(uint16_t *out_photo_raw12,
                                                  uint16_t *out_therm_raw12,
                                                  uint8_t scan_count) {
  uint16_t pair[ADC1_DUAL_SLOT_COUNT] = {0U, 0U, 0U};
  stm_status_t st = STM_OK;

  if ((out_photo_raw12 == NULL) || (out_therm_raw12 == NULL)) {
    return STM_ERR_INVALID_ARG;
  }

  st = adc1_dual_read_pair_average_blocking(pair, scan_count);
  if (st != STM_OK) {
    return st;
  }

  *out_photo_raw12 = pair[ADC1_DUAL_SLOT_PHOTO];
  *out_therm_raw12 = pair[ADC1_DUAL_SLOT_THERM];
  return STM_OK;
}

stm_status_t bsp_temperature_read_celsius_x10_from_raw(uint16_t therm_raw12,
                                                       int16_t *out_celsius_x10) {
  return thermistor_read_temperature_from_raw_blocking(therm_raw12,
                                                       out_celsius_x10);
}

stm_status_t bsp_default_devices_init(void) {
  stm_status_t st = bsp_dc_motor_init();
  if (st != STM_OK) {
    return st;
  }
  st = bsp_console_init();
  if (st != STM_OK) {
    return st;
  }
  st = bsp_console_enable_rx_interrupt();
  if (st != STM_OK) {
    return st;
  }
  st = bsp_display_init();
  if (st != STM_OK) {
    return st;
  }
  st = bsp_status_led_init();
  if (st != STM_OK) {
    return st;
  }
  st = bsp_wheel_encoder_init();
  if (st != STM_OK) {
    return st;
  }
  st = bsp_analog_sensors_init();
  if (st != STM_OK) {
    return st;
  }
  return bsp_buzzer_init();
}
