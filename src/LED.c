#include "GPIO.h"
#include "LED.h"
#include "RCC.h"

void output_init(void)
{
  //开启时钟
  RCC_APB2ENR |= RCC_IOPAEN_BIT | RCC_IOPCEN_BIT;

  //设置推挽输出 | 2MHZ
  GPIOA_CRL &= ~PA1_MODE_MSK;
  GPIOA_CRL |= PA1_OUT_2M_PP;

  GPIOC_CRH &= ~PC13_MODE_MSK;
  GPIOC_CRH |= PC13_OUT_2M_PP;
}

void output_toggle_pc13(void)
{
  GPIOC_ODR ^= PC13_ODR_BIT;
}

void output_toggle_aux(void)
{
  GPIOA_ODR ^= PA1_ODR_BIT;
}
