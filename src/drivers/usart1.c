#include "drivers/usart1.h"

#include <stddef.h>

#include "bsp/board_pins.h"
#include "bsp/clock.h"
#include "common/ring_buffer.h"

#define USART1_RX_BUF_SIZE 64U

static uint8_t g_usart1_rx_storage[USART1_RX_BUF_SIZE];
static ring_buffer_t g_usart1_rx_rb;

static usart1_line_policy_t g_line_policy = USART1_LINE_CR_OR_LF;
static uint8_t g_usart1_initialized;
static uint8_t g_usart1_rx_overflow;

/*
 * 由 PCLK 和波特率反推 BRR。
 * USART1 挂在 APB2，输入时钟 = PCLK2。
 *
 * 16 倍过采样：BRR = round(PCLK / baud)
 *  8 倍过采样：BRR = mantissa(div/8) << 4 | fraction(div%8)（注意 BRR[3] 始终为 0）
 *
 * 失败返回非 STM_OK，调用方据此中止初始化。
 */
static stm_status_t usart1_compute_brr(uint32_t pclk_hz, uint32_t baudrate,
                                       usart_oversampling_t oversampling,
                                       uint16_t *out_brr) {
  uint32_t brr = 0U;
  uint32_t div = 0U;
  uint32_t mantissa = 0U;
  uint32_t fraction = 0U;

  if ((out_brr == NULL) || (baudrate == 0U) ||
      ((oversampling != USART_OVERSAMPLING_16) &&
       (oversampling != USART_OVERSAMPLING_8))) {
    return STM_ERR_INVALID_ARG;
  }

  if (oversampling == USART_OVERSAMPLING_16) {
    brr = (pclk_hz + (baudrate / 2U)) / baudrate;
    if ((brr < 16U) || (brr > 0xFFFFU)) {
      return STM_ERR_INVALID_ARG;
    }
  } else {
    div = (pclk_hz + (baudrate / 2U)) / baudrate;
    if (div < 8U) {
      return STM_ERR_INVALID_ARG;
    }
    mantissa = div / 8U;
    fraction = div % 8U;
    if (mantissa > 0x0FFFU) {
      return STM_ERR_INVALID_ARG;
    }
    brr = (mantissa << 4U) | fraction;
  }

  *out_brr = (uint16_t)brr;
  return STM_OK;
}

static uint8_t usart1_is_line_end(uint8_t ch, uint8_t *consume_next_lf) {
  if (consume_next_lf != NULL) {
    *consume_next_lf = 0U;
  }

  switch (g_line_policy) {
  case USART1_LINE_CR_ONLY:
    return (ch == '\r') ? 1U : 0U;

  case USART1_LINE_LF_ONLY:
    return (ch == '\n') ? 1U : 0U;

  case USART1_LINE_CRLF:
    if (ch == '\r') {
      if (consume_next_lf != NULL) {
        *consume_next_lf = 1U;
      }
      return 1U;
    }
    return 0U;

  case USART1_LINE_CR_OR_LF:
  default:
    return ((ch == '\r') || (ch == '\n')) ? 1U : 0U;
  }
}

static stm_status_t usart1_validate_config(const usart1_config_t *config) {
  if ((config == NULL) || (config->baudrate == 0U)) {
    return STM_ERR_INVALID_ARG;
  }
  if ((config->oversampling != USART_OVERSAMPLING_16) &&
      (config->oversampling != USART_OVERSAMPLING_8)) {
    return STM_ERR_INVALID_ARG;
  }
  switch (config->line_policy) {
  case USART1_LINE_CR_OR_LF:
  case USART1_LINE_CR_ONLY:
  case USART1_LINE_LF_ONLY:
  case USART1_LINE_CRLF:
    return STM_OK;
  default:
    return STM_ERR_INVALID_ARG;
  }
}

/*
 * 初始化 USART1 基础收发能力（不直接开启接收中断）。
 *
 * 流程：
 *   - 依据当前 PCLK2 与过采样方式计算并写入 BRR；
 *   - 初始化接收环形缓冲区；
 *   - 配置 PA9/PA10 复用功能（TX=复用推挽输出，RX=浮空输入）；
 *   - 配置 OVER8 并使能 UE/TE/RE。
 */
stm_status_t usart1_init_with_config(const usart1_config_t *config) {
  uint16_t brr = 0U;
  stm_status_t st = usart1_validate_config(config);
  if (st != STM_OK) {
    return st;
  }

  st = usart1_compute_brr(bsp_clock_get_pclk2_hz(), config->baudrate,
                          config->oversampling, &brr);
  if (st != STM_OK) {
    return st;
  }

  (void)ring_buffer_init(&g_usart1_rx_rb, g_usart1_rx_storage,
                         USART1_RX_BUF_SIZE);

  /* PA9(TX)/PA10(RX) 先清配置位，再写入目标模式。 */
  BOARD_USART1_GPIO_CRH_REG &=
      ~(BOARD_GPIO_PA9_CRH_MASK | BOARD_GPIO_PA10_CRH_MASK);
  BOARD_USART1_GPIO_CRH_REG |=
      (BOARD_GPIO_PA9_AF_PP_50MHZ | BOARD_GPIO_PA10_INPUT_FLOATING);

  USART1_BRR = (uint32_t)brr;

  if (config->oversampling == USART_OVERSAMPLING_8) {
    USART1_CR1 |= USART_CR1_OVER8_BIT;
  } else {
    USART1_CR1 &= ~USART_CR1_OVER8_BIT;
  }

  USART1_CR1 |= (USART_CR1_UE_BIT | USART_CR1_TE_BIT | USART_CR1_RE_BIT);
  g_line_policy = config->line_policy;
  g_usart1_rx_overflow = 0U;
  g_usart1_initialized = 1U;

  if (config->enable_rx_interrupt != 0U) {
    return usart1_enable_rx_interrupt();
  }
  return STM_OK;
}

stm_status_t usart1_init(uint32_t baudrate, usart_oversampling_t oversampling) {
  const usart1_config_t config = {
      .baudrate = baudrate,
      .oversampling = oversampling,
      .line_policy = USART1_LINE_CR_OR_LF,
      .enable_rx_interrupt = 0U,
  };
  return usart1_init_with_config(&config);
}

/*
 * 开启 USART1 接收中断路径。
 *
 * 两步都必须完成：
 *   1) 外设侧放行：RXNEIE=1，接收数据寄存器非空时可向 NVIC 发起中断请求；
 *   2) NVIC 侧放行：使能 USART1 IRQ 通道（IRQn=37，对应 ISER1 bit5）。
 *
 * 本函数只负责「开中断通路」，不负责 USART 基础初始化（波特率、GPIO、UE/TE/RE）。
 */
stm_status_t usart1_enable_rx_interrupt(void) {
  if (g_usart1_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  USART1_CR1 |= USART_CR1_RXNEIE_BIT;
  NVIC_ISER1 |= NVIC_USART1_IRQ_BIT;
  return STM_OK;
}

void usart1_irq_handler(void) {
  if ((USART1_SR & USART_SR_RXNE_BIT) == 0U) {
    return;
  }
  uint8_t data = (uint8_t)USART1_DR;
  if (ring_buffer_push_byte(&g_usart1_rx_rb, data) == STM_ERR_OVERFLOW) {
    g_usart1_rx_overflow = 1U;
  }
}

stm_status_t usart1_write_byte_blocking(uint8_t data) {
  if (g_usart1_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  while ((USART1_SR & USART_SR_TXE_BIT) == 0U) {
  }
  USART1_DR = data;
  return STM_OK;
}

stm_status_t usart1_write_string_blocking(const char *str) {
  if (str == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (g_usart1_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  while (*str != '\0') {
    (void)usart1_write_byte_blocking((uint8_t)*str);
    str++;
  }
  return STM_OK;
}

stm_status_t usart1_read_byte_try(uint8_t *out) {
  if (g_usart1_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  return ring_buffer_pop_byte(&g_usart1_rx_rb, out);
}

stm_status_t usart1_set_line_policy(usart1_line_policy_t policy) {
  switch (policy) {
  case USART1_LINE_CR_OR_LF:
  case USART1_LINE_CR_ONLY:
  case USART1_LINE_LF_ONLY:
  case USART1_LINE_CRLF:
    break;
  default:
    return STM_ERR_INVALID_ARG;
  }
  g_line_policy = policy;
  return STM_OK;
}

stm_status_t usart1_read_line_try(char *out, uint16_t out_size) {
  static char s_line_buf[USART1_RX_BUF_SIZE];
  static uint16_t s_line_len = 0U;
  static uint8_t s_consume_next_lf = 0U;
  static uint8_t s_line_overflow = 0U;

  uint8_t ch = 0U;
  stm_status_t st = STM_OK;

  if ((out == NULL) || (out_size < 2U)) {
    return STM_ERR_INVALID_ARG;
  }
  if (g_usart1_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }

  while ((st = usart1_read_byte_try(&ch)) == STM_OK) {
    if ((s_consume_next_lf != 0U) && (ch == '\n')) {
      s_consume_next_lf = 0U;
      continue;
    }

    if (usart1_is_line_end(ch, &s_consume_next_lf) != 0U) {
      uint16_t copy_len = s_line_len;
      if ((s_line_len == 0U) && (s_line_overflow == 0U)) {
        continue;
      }
      if (s_line_overflow != 0U) {
        s_line_len = 0U;
        s_line_overflow = 0U;
        return STM_ERR_OVERFLOW;
      }
      if (copy_len > (out_size - 1U)) {
        copy_len = (uint16_t)(out_size - 1U);
      }
      for (uint16_t i = 0U; i < copy_len; ++i) {
        out[i] = s_line_buf[i];
      }
      out[copy_len] = '\0';
      s_line_len = 0U;
      return STM_OK;
    }

    /* 退格（BS=0x08）/删除（DEL=0x7F）：在行缓冲里回退一个字符。 */
    if ((ch == 0x08U) || (ch == 0x7FU)) {
      if (s_line_len > 0U) {
        s_line_len--;
      }
      continue;
    }

    if (s_line_len < (uint16_t)(USART1_RX_BUF_SIZE - 1U)) {
      s_line_buf[s_line_len] = (char)ch;
      s_line_len++;
    } else {
      s_line_overflow = 1U;
    }
  }

  if (st == STM_ERR_BUSY) {
    if (g_usart1_rx_overflow != 0U) {
      g_usart1_rx_overflow = 0U;
      return STM_ERR_OVERFLOW;
    }
    return STM_ERR_BUSY;
  }
  return st;
}

void usart1_send_byte(uint8_t data) { (void)usart1_write_byte_blocking(data); }

void usart1_send_string(const char *str) {
  (void)usart1_write_string_blocking(str);
}

uint8_t usart1_try_read_byte(uint8_t *out) {
  return (usart1_read_byte_try(out) == STM_OK) ? 1U : 0U;
}

uint8_t usart1_try_read_string(char *out, uint16_t out_size) {
  return (usart1_read_line_try(out, out_size) == STM_OK) ? 1U : 0U;
}
