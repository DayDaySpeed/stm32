#ifndef DRIVERS_USART1_H
#define DRIVERS_USART1_H

#include <stdint.h>

typedef enum {
  USART_OVERSAMPLING_16 = 0,
  USART_OVERSAMPLING_8 = 1
} usart_oversampling_t;

uint8_t usart1_init(uint32_t baudrate, usart_oversampling_t oversampling);
void usart1_enable_rx_interrupt(void);
void usart1_irq_handler(void);
void usart1_send_byte(uint8_t data);
void usart1_send_string(const char *str);
uint8_t usart1_try_read_byte(uint8_t *out);

#endif
