#include "KEY.h"
#include "GPIO.h"
#include "RCC.h"
#include "SYS.h"

static uint8_t key_released[KEY_COUNT] = {1U, 1U, 1U};

static uint8_t key_scan_single(uint32_t key_bit, uint8_t *released)
{
  //按下
  if ((*released == 1U) && ((GPIOB_IDR & key_bit) == 0U)) {
    delay_ms(KEY_DEBOUNCE_MS);
    if ((GPIOB_IDR & key_bit) == 0U) {
      *released = 0U;
      return 1U;
    }
  }
  //松开
  if ((GPIOB_IDR & key_bit) != 0U) {
    *released = 1U;
  }

  return 0U;
}

void key_init(void)
{
  RCC_APB2ENR |= RCC_IOPBEN_BIT;

  //设置上拉输入
  GPIOB_CRL &= ~PB0_MODE_MSK;
  GPIOB_CRL |= PB0_IN_PUPD;
  GPIOB_CRL &= ~PB1_MODE_MSK;
  GPIOB_CRL |= PB1_IN_PUPD;
  GPIOB_CRH &= ~PB10_CRH_MSK;
  GPIOB_CRH |= PB10_IN_PUPD;

  //初始为高电平
  GPIOB_ODR |= (PB0_ODR_BIT | PB1_ODR_BIT | PB10_ODR_BIT);
}

uint8_t key_scan_event(key_id_t key)
{
  switch (key) {
    case KEY_PB0:
      return key_scan_single(PB0_ODR_BIT, &key_released[KEY_PB0]);
    case KEY_PB1:
      return key_scan_single(PB1_ODR_BIT, &key_released[KEY_PB1]);
    case KEY_PB10:
      return key_scan_single(PB10_ODR_BIT, &key_released[KEY_PB10]);
    default:
      return 0U;
  }
}