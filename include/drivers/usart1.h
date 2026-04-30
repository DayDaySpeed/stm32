#ifndef DRIVERS_USART1_H
#define DRIVERS_USART1_H

#include <stdint.h>

void usart1_init_115200_8n1(void);
void usart1_send_byte(uint8_t data);
void usart1_send_string(const char *str);
uint8_t usart1_try_read_byte(uint8_t *out);

#endif
