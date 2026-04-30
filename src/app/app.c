#include "app/app.h"

#include <stdint.h>

#include "drivers/systick.h"
#include "drivers/usart1.h"

void app_init(void)
{
  systick_init_1ms();
  if (usart1_init(115200UL, USART_OVERSAMPLING_16) == 0U) {
    while (1) {
    }
  }
  usart1_send_string("\r\nUSART1 ready (PA9/PA10,115200 8N1)\r\n");
  usart1_send_string("Type any key, STM32 will echo it.\r\n");
}

void app_run_forever(void)
{
  while (1) {
    uint8_t ch = 0U;

    if (usart1_try_read_byte(&ch) != 0U) {
      usart1_send_string("recv: ");
      usart1_send_byte(ch);
      usart1_send_string("\r\n");
    }
  }
}
