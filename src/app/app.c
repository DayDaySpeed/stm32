#include "app/app.h"

#include <stdint.h>

#include "drivers/systick.h"
#include "drivers/ssd1306_oled.h"
#include "drivers/usart1.h"

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
  usart1_send_string("SSD1306 OLED PB8=SCL PB9=SDA, 128x64.\r\n");
  usart1_send_string("Type text; shown on OLED + echoed here.\r\n");
}

void app_run_forever(void)
{
  while (1) {
    uint8_t ch = 0U;
    
    if (usart1_try_read_byte(&ch) != 0U) {
      ssd1306_oled_putc(ch);
      usart1_send_string("recv: ");
      usart1_send_byte(ch);
      usart1_send_string("\r\n");
    }
  }
}
