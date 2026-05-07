#include "app/app.h"

#include <stdint.h>

#include "drivers/ssd1306_oled.h"
#include "drivers/systick.h"
#include "drivers/usart1.h"

void app_init(void) {
  systick_init_1ms();
  if (usart1_init(115200UL, USART_OVERSAMPLING_16) == 0U) {
    while (1) {
    }
  }
  usart1_set_line_policy(USART1_LINE_CR_OR_LF);
  usart1_enable_rx_interrupt();

  if (ssd1306_init(ssd1306_default()) != STM_OK) {
    while (1) {
    }
  }

  usart1_send_string("\r\nUSART1 ready (PA9/PA10,115200 8N1)\r\n");
  usart1_send_string("OLED ready (I2C1 remap PB8/PB9).\r\n");
  usart1_send_string("Type a line, press Enter to flush to OLED.\r\n");
}

void app_run_forever(void) {
  char line[64];
  static uint8_t page_count = 0U;
  /* 测试滚动 */
  while(page_count < 200){
    ssd1306_oled_write_text_atf(page_count, 80U, "page=%u", page_count);
    ++page_count;
  }
  while (1) {
    if (usart1_try_read_string(line, (uint16_t)sizeof(line)) != 0U) {
      usart1_send_string("recv: ");
      usart1_send_string(line);
      usart1_send_string("\r\n");

      ssd1306_oled_write_text_atf(page_count, 0U, "line=%s --- page=%u", line, page_count);
      ++page_count;
    }
  }
}
