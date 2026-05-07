#include "drivers/usart1.h"

#include "bsp/board_pins.h"
#include "bsp/clock.h"
#include "common/ring_buffer.h"

#define USART1_RX_BUF_SIZE 64U

static uint8_t g_usart1_rx_storage[USART1_RX_BUF_SIZE];
static ring_buffer_t g_usart1_rx_rb;

static usart1_line_policy_t g_line_policy = USART1_LINE_CR_OR_LF;

static uint8_t usart1_compute_brr(uint32_t pclk_hz, uint32_t baudrate,
                                  usart_oversampling_t oversampling,
                                  uint16_t *out_brr) {
  uint32_t brr = 0U;
  uint32_t div = 0U;
  uint32_t mantissa = 0U;
  uint32_t fraction = 0U;

  if ((out_brr == 0) || (baudrate == 0U) ||
      ((oversampling != USART_OVERSAMPLING_16) &&
       (oversampling != USART_OVERSAMPLING_8))) {
    return 0U;
  }

  if (oversampling == USART_OVERSAMPLING_16) {
    brr = (pclk_hz + (baudrate / 2U)) / baudrate;
    if ((brr < 16U) || (brr > 0xFFFFU)) {
      return 0U;
    }
  } else {
    div = (pclk_hz + (baudrate / 2U)) / baudrate;
    if (div < 8U) {
      return 0U;
    }

    mantissa = div / 8U;
    fraction = div % 8U;
    if (mantissa > 0x0FFFU) {
      return 0U;
    }
    brr = (mantissa << 4U) | fraction;
  }

  *out_brr = (uint16_t)brr;
  return 1U;
}

static uint8_t usart1_is_line_end(uint8_t ch, uint8_t *consume_next_lf) {
  if (consume_next_lf != 0) {
    *consume_next_lf = 0U;
  }

  switch (g_line_policy) {
  case USART1_LINE_CR_ONLY:
    return (ch == '\r') ? 1U : 0U;

  case USART1_LINE_LF_ONLY:
    return (ch == '\n') ? 1U : 0U;

  case USART1_LINE_CRLF:
    if (ch == '\r') {
      if (consume_next_lf != 0) {
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

void usart1_enable_rx_interrupt(void) {
  USART1_CR1 |= USART_CR1_RXNEIE_BIT;
  NVIC_ISER1 |= NVIC_USART1_IRQ_BIT;
}

void usart1_irq_handler(void) {
  uint8_t data = 0U;

  if ((USART1_SR & USART_SR_RXNE_BIT) == 0U) {
    return;
  }

  data = (uint8_t)USART1_DR;
  (void)ring_buffer_push_byte(&g_usart1_rx_rb, data);
}

uint8_t usart1_init(uint32_t baudrate, usart_oversampling_t oversampling) {
  uint16_t brr = 0U;

  if (usart1_compute_brr(SYSCLK_HZ, baudrate, oversampling, &brr) == 0U) {
    return 0U;
  }

  (void)ring_buffer_init(&g_usart1_rx_rb, g_usart1_rx_storage,
                         USART1_RX_BUF_SIZE);

  BOARD_USART1_GPIO_CRH_REG &=
      ~(BOARD_GPIO_PA9_CRH_MASK | BOARD_GPIO_PA10_CRH_MASK);
  BOARD_USART1_GPIO_CRH_REG |=
      (BOARD_GPIO_PA9_AF_PP_50MHZ | BOARD_GPIO_PA10_INPUT_FLOATING);

  USART1_BRR = (uint32_t)brr;

  if (oversampling == USART_OVERSAMPLING_8) {
    USART1_CR1 |= USART_CR1_OVER8_BIT;
  } else {
    USART1_CR1 &= ~USART_CR1_OVER8_BIT;
  }

  USART1_CR1 |= (USART_CR1_UE_BIT | USART_CR1_TE_BIT | USART_CR1_RE_BIT);
  return 1U;
}

void usart1_send_byte(uint8_t data) {
  while ((USART1_SR & USART_SR_TXE_BIT) == 0U) {
  }
  USART1_DR = data;
}

void usart1_send_string(const char *str) {
  while (*str != '\0') {
    usart1_send_byte((uint8_t)*str);
    str++;
  }
}

uint8_t usart1_try_read_byte(uint8_t *out) {
  if (ring_buffer_pop_byte(&g_usart1_rx_rb, out) != STM_OK) {
    return 0U;
  }
  return 1U;
}

void usart1_set_line_policy(usart1_line_policy_t policy) {
  g_line_policy = policy;
}

uint8_t usart1_try_read_string(char *out, uint16_t out_size) {
  static char line_buf[USART1_RX_BUF_SIZE];
  static uint16_t line_len = 0U;
  static uint8_t consume_next_lf = 0U;

  uint8_t ch = 0U;

  if ((out == 0) || (out_size < 2U)) {
    return 0U;
  }

  while (usart1_try_read_byte(&ch) != 0U) {
    if ((consume_next_lf != 0U) && (ch == '\n')) {
      consume_next_lf = 0U;
      continue;
    }

    if (usart1_is_line_end(ch, &consume_next_lf) != 0U) {
      uint16_t copy_len = line_len;
      if (line_len == 0U) {
        continue;
      }
      if (copy_len > (out_size - 1U)) {
        copy_len = (uint16_t)(out_size - 1U);
      }
      for (uint16_t i = 0U; i < copy_len; ++i) {
        out[i] = line_buf[i];
      }
      out[copy_len] = '\0';
      line_len = 0U;
      return 1U;
    }

    if ((ch == 0x08U) || (ch == 0x7FU)) {
      if (line_len > 0U) {
        line_len--;
      }
      continue;
    }

    if (line_len < (uint16_t)(USART1_RX_BUF_SIZE - 1U)) {
      line_buf[line_len] = (char)ch;
      line_len++;
    }
  }

  return 0U;
}
