#include "drivers/usart1.h"

#include "GPIO.h"
#include "RCC.h"

void usart1_init_115200_8n1(void)
{
  RCC_APB2ENR |= RCC_APB2_USART1_REQUIRED_BITS;

  GPIO_USART1_PINS_CRH_REG &= ~(GPIO_PA9_CRH_MASK | GPIO_PA10_CRH_MASK);
  GPIO_USART1_PINS_CRH_REG |= (GPIO_PA9_AF_PP_50M | GPIO_PA10_INPUT_FLOATING);

  USART1_BRR = 0x45UL;
  //开启USART外设 | 发送使能 | 接收使能
  USART1_CR1 = USART_CR1_UE_BIT | USART_CR1_TE_BIT | USART_CR1_RE_BIT;
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
  if ((USART1_SR & USART_SR_RXNE_BIT) == 0U) {
    return 0U;
  }

  *out = (uint8_t)USART1_DR;
  return 1U;
}
