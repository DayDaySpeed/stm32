#ifndef DRIVERS_USART1_H
#define DRIVERS_USART1_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * USART1 串口驱动（默认 PA9/PA10）。
 *
 * 约定：
 *   *_write_*_blocking — 忙等 TXE 后发送
 *   *_read_*_try       — 非阻塞；无数据返回 STM_ERR_BUSY
 * 前置：bsp_board_init() 已开 GPIOA/USART1 时钟。
 */

typedef enum {
  USART_OVERSAMPLING_16 = 0,
  USART_OVERSAMPLING_8 = 1
} usart_oversampling_t;

typedef enum {
  USART1_LINE_CR_OR_LF = 0, /* \r 或 \n 均视为行结束 */
  USART1_LINE_CR_ONLY = 1,
  USART1_LINE_LF_ONLY = 2,
  USART1_LINE_CRLF = 3      /* \r 后可选吞掉 \n */
} usart1_line_policy_t;

typedef struct {
  uint32_t baudrate;
  usart_oversampling_t oversampling;
  usart1_line_policy_t line_policy;
  uint8_t enable_rx_interrupt; /* 非 0 则在 init 末尾自动开 RX 中断 */
} usart1_config_t;

stm_status_t usart1_init_with_config(const usart1_config_t *config);
stm_status_t usart1_enable_rx_interrupt(void);

void usart1_irq_handler(void);

stm_status_t usart1_write_byte_blocking(uint8_t data);
stm_status_t usart1_write_string_blocking(const char *str);
stm_status_t usart1_read_byte_try(uint8_t *out);
stm_status_t usart1_read_line_try(char *out, uint16_t out_size);

#endif
