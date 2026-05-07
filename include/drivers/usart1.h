#ifndef DRIVERS_USART1_H
#define DRIVERS_USART1_H

#include <stdint.h>

/* 须先调用 bsp_board_init()，否则 GPIOA/USART1 时钟未开。 */

typedef enum {
  USART_OVERSAMPLING_16 = 0,
  USART_OVERSAMPLING_8 = 1
} usart_oversampling_t;

typedef enum {
  USART1_LINE_CR_OR_LF = 0,
  USART1_LINE_CR_ONLY = 1,
  USART1_LINE_LF_ONLY = 2,
  USART1_LINE_CRLF = 3
} usart1_line_policy_t;

uint8_t usart1_init(uint32_t baudrate, usart_oversampling_t oversampling);
void usart1_enable_rx_interrupt(void);
void usart1_irq_handler(void);
void usart1_send_byte(uint8_t data);
void usart1_send_string(const char *str);
uint8_t usart1_try_read_byte(uint8_t *out);
void usart1_set_line_policy(usart1_line_policy_t policy);
/* 非阻塞读取一行：满足策略的行结束符后返回 1，并写入以 '\0' 结尾字符串。 */
uint8_t usart1_try_read_string(char *out, uint16_t out_size);

#endif
