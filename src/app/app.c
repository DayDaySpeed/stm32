#include "app/app.h"

#include <stdint.h>

#include "drivers/systick.h"
#include "drivers/ssd1306_oled.h"
#include "drivers/usart1.h"

static char hex_digit(uint8_t v)
{
  v &= 0x0FU;
  return (v < 10U) ? (char)('0' + v) : (char)('A' + (v - 10U));
}

void app_init(void)
{
  systick_init_1ms();
  if (usart1_init(115200UL, USART_OVERSAMPLING_16) == 0U) {
    while (1) {
    }
  }
  usart1_enable_rx_interrupt();
  ssd1306_oled_init();
  usart1_send_string("\r\nUSART1 ready (PA9/PA10,115200 8N1)\r\n");
  usart1_send_string("Debug: print each RX byte in hex.\r\n");
  usart1_send_string("Press Enter to see 0D / 0A behavior.\r\n");
}

void app_run_forever(void)
{
  while (1) {
    uint8_t ch = 0U;
    if (usart1_try_read_byte(&ch) != 0U) {
      char b0 = hex_digit((uint8_t)(ch >> 4U));
      char b1 = hex_digit(ch);
      usart1_send_string("RX=0x");
      usart1_send_byte((uint8_t)b0);
      usart1_send_byte((uint8_t)b1);
      usart1_send_string("\r\n");

      ssd1306_oled_putc((uint8_t)b0);
      ssd1306_oled_putc((uint8_t)b1);
      ssd1306_oled_putc((uint8_t)' ');
    }
  }
}
