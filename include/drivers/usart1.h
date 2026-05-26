#ifndef DRIVERS_USART1_H
#define DRIVERS_USART1_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * USART1 串口：阻塞发送 + 中断接收环形缓冲 + 行解析。
 * 前置：bsp_board_init() 已开 GPIOA/USART1 时钟。
 */

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

typedef struct {
  uint32_t baudrate;
  usart_oversampling_t oversampling;
  usart1_line_policy_t line_policy;
  uint8_t enable_rx_interrupt; /* 非 0 则 init 末尾自动 enable_rx_interrupt */
} usart1_config_t;

/* config：波特率、过采样、行结束策略；可选 init 末尾开 RX 中断。 */
stm_status_t usart1_init_with_config(const usart1_config_t *config);
/* 开 RXNE 中断与 NVIC；须先 init。 */
stm_status_t usart1_enable_rx_interrupt(void);
/* 在 USART1_IRQHandler 中调用，将字节推入 RX 环形缓冲。 */
void usart1_irq_handler(void);
/* data/str：待发送字节或以 '\\0' 结尾的字符串；阻塞至 TXE。 */
stm_status_t usart1_write_byte_blocking(uint8_t data);
stm_status_t usart1_write_string_blocking(const char *str);
/* 非阻塞取 1 字节；空则 STM_ERR_BUSY。 */
stm_status_t usart1_read_byte_try(uint8_t *out);
/* 非阻塞取一行；out_size 含 '\\0'；policy 见 config.line_policy。 */
stm_status_t usart1_read_line_try(char *out, uint16_t out_size);

#endif
