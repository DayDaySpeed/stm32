#ifndef DRIVERS_USART1_H
#define DRIVERS_USART1_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * USART1 默认控制台驱动（PA9/PA10）
 *
 * 约定：
 *   - `*_init[_with_config]` / `*_write_*` / `*_read_*` 统一返回 `stm_status_t`
 *   - `*_read_*_try` 为非阻塞：无数据时返回 `STM_ERR_BUSY`
 *   - `*_write_*_blocking` 为忙等阻塞：直到 TXE 可写再返回
 *
 * 前置条件：
 *   - 须先调用 `bsp_board_init()`，否则 GPIOA/USART1 时钟未开。
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
  uint8_t enable_rx_interrupt;
} usart1_config_t;

stm_status_t usart1_init_with_config(const usart1_config_t *config);
stm_status_t usart1_init(uint32_t baudrate, usart_oversampling_t oversampling);
stm_status_t usart1_set_line_policy(usart1_line_policy_t policy);
stm_status_t usart1_enable_rx_interrupt(void);

void usart1_irq_handler(void);

stm_status_t usart1_write_byte_blocking(uint8_t data);
stm_status_t usart1_write_string_blocking(const char *str);
stm_status_t usart1_read_byte_try(uint8_t *out);
stm_status_t usart1_read_line_try(char *out, uint16_t out_size);

/* 兼容旧接口：新代码优先使用上面的 status 风格 API。 */
void usart1_send_byte(uint8_t data);
void usart1_send_string(const char *str);
uint8_t usart1_try_read_byte(uint8_t *out);
uint8_t usart1_try_read_string(char *out, uint16_t out_size);

#endif
