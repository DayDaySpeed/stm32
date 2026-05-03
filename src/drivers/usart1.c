#include "drivers/usart1.h"

#include "bsp/clock.h"
#include "GPIO.h"
#include "RCC.h"

#define USART1_RX_BUF_SIZE 64U

static volatile uint8_t g_usart1_rx_buf[USART1_RX_BUF_SIZE];
static volatile uint8_t g_usart1_rx_head = 0U;
static volatile uint8_t g_usart1_rx_tail = 0U;

static uint8_t usart1_compute_brr(
    uint32_t pclk_hz,
    uint32_t baudrate,
    usart_oversampling_t oversampling,
    uint16_t *out_brr)
{
  uint32_t brr = 0U;
  uint32_t div = 0U;
  uint32_t mantissa = 0U;
  uint32_t fraction = 0U;

  if ((out_brr == 0) || (baudrate == 0U) ||
      ((oversampling != USART_OVERSAMPLING_16) && (oversampling != USART_OVERSAMPLING_8))) {
    return 0U;
  }

  if (oversampling == USART_OVERSAMPLING_16) {
    /* OVER8=0: BRR 约等于 round(PCLK / baud). */
    brr = (pclk_hz + (baudrate / 2U)) / baudrate;
    if ((brr < 16U) || (brr > 0xFFFFU)) {
      return 0U;
    }
  } else {
    /*
     * OVER8=1: USARTDIV = PCLK / (8 * baud)
     * div = round(USARTDIV * 8) = round(PCLK / baud)
     */
    div = (pclk_hz + (baudrate / 2U)) / baudrate;
    if (div < 8U) {
      return 0U;
    }

    mantissa = div / 8U;
    fraction = div % 8U; /* 3bit 小数 */
    if (mantissa > 0x0FFFU) {
      return 0U;
    }
    brr = (mantissa << 4U) | fraction;
  }

  *out_brr = (uint16_t)brr;
  return 1U;
}

void usart1_enable_rx_interrupt(void)
{
  /* 开启 USART1 RXNE 中断，并在 NVIC 中使能 USART1 中断通道 */
  USART1_CR1 |= USART_CR1_RXNEIE_BIT;
  NVIC_ISER1 |= NVIC_USART1_IRQ_BIT;
}

void usart1_irq_handler(void)
{
  uint8_t data = 0U;
  uint8_t next = 0U;
  //接受数据寄存器是否有数据
  if ((USART1_SR & USART_SR_RXNE_BIT) == 0U) {
    return;
  }

  data = (uint8_t)USART1_DR; /* 读 DR 清 RXNE */
  next = (uint8_t)((g_usart1_rx_head + 1U) % USART1_RX_BUF_SIZE);
  if (next == g_usart1_rx_tail) {
    return; /* 缓冲区满时丢弃最新字节 */
  }

  g_usart1_rx_buf[g_usart1_rx_head] = data;
  g_usart1_rx_head = next;
}

/**
@brief 初始化USART1
@param baudrate 波特率
@param oversampling 过采样模式
@return 0: 失败, 1: 成功
*/
uint8_t usart1_init(uint32_t baudrate, usart_oversampling_t oversampling)
{
  uint16_t brr = 0U;

  if (usart1_compute_brr(SYSCLK_HZ, baudrate, oversampling, &brr) == 0U) {
    return 0U;
  }
  //开启USART时钟
  RCC_APB2ENR |= RCC_APB2_USART1_REQUIRED_BITS;
  //配置PA9为推挽输出|50MHz，PA10为浮空输入
  GPIO_USART1_PINS_CRH_REG &= ~(GPIO_PA9_CRH_MASK | GPIO_PA10_CRH_MASK);
  GPIO_USART1_PINS_CRH_REG |= (GPIO_PA9_AF_PP_50M | GPIO_PA10_INPUT_FLOATING);

  USART1_BRR = (uint32_t)brr;

  if (oversampling == USART_OVERSAMPLING_8) {
    USART1_CR1 |= USART_CR1_OVER8_BIT;
  } else {
    USART1_CR1 &= ~USART_CR1_OVER8_BIT;
  }

  /* 8N1: 8 data bits, no parity, 1 stop bit (默认配置) */
  /* 开启USART外设 | 发送使能 | 接收使能 */
  USART1_CR1 |= (USART_CR1_UE_BIT | USART_CR1_TE_BIT | USART_CR1_RE_BIT);
  return 1U;
}

void usart1_send_byte(uint8_t data)
{
  while ((USART1_SR & USART_SR_TXE_BIT) == 0U) {
  }
  USART1_DR = data;
}

void usart1_send_string(const char *str)
{
  while (*str != '\0') {
    usart1_send_byte((uint8_t)*str);
    str++;
  }
}

uint8_t usart1_try_read_byte(uint8_t *out)
{
  /* out 为空指针；head==tail 表示环形缓冲无未读数据 */
  if ((out == 0) || (g_usart1_rx_head == g_usart1_rx_tail)) {
    return 0U; /* 无数据可读 */
  }

  *out = g_usart1_rx_buf[g_usart1_rx_tail];
  g_usart1_rx_tail = (uint8_t)((g_usart1_rx_tail + 1U) % USART1_RX_BUF_SIZE);
  return 1U;
}
