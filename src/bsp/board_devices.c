#include "bsp/board_devices.h"

#include <stdarg.h>

#include "common/stm_log.h"
#include "drivers/encoder.h"
#include "drivers/photoresistor.h"
#include "drivers/pwm.h"
#include "drivers/ssd1306_oled.h"
#include "drivers/usart1.h"

#define BOARD_CONSOLE_BAUDRATE 115200UL
#define BOARD_STATUS_LED_PWM_HZ 1000UL

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
    .direction = TIM3_ENCODER_DIR_NORMAL,
};

static const photoresistor_config_t g_board_ambient_light_config = {
    .clock_source = PHOTO_ADC_CLOCK_SOURCE_PCLK2,
    .adc_prescaler = PHOTO_ADC_PRESCALER_AUTO,
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

stm_status_t bsp_ambient_light_init(void) {
  return photoresistor_init_with_config(&g_board_ambient_light_config);
}

stm_status_t bsp_ambient_light_read_raw_average(uint16_t *out_raw12,
                                                uint8_t sample_count) {
  return photoresistor_read_raw_average_blocking(out_raw12, sample_count);
}

stm_status_t bsp_default_devices_init(void) {
  stm_status_t st = bsp_console_init();
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
  return bsp_ambient_light_init();
}
