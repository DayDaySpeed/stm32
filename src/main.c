#include "GPIO.h"
#include "RCC.h"
#include "SYS.h"
#include <stdint.h>

static volatile uint32_t g_ms_ticks = 0;

void SysTick_Handler(void)
{
  g_ms_ticks++;
}

static void systick_init(void)
{
  // 8MHz/1000 = 8000，每 1ms 产生一次中断
  SYST_RVR = (SYSCLK_HZ / 1000UL) - 1UL;
  SYST_CVR = 0UL;
  SYST_CSR = SYSTICK_CLKSRC_BIT | SYSTICK_TICKINT_BIT | SYSTICK_ENABLE_BIT;
}

static void delay_ms(uint32_t ms)
{
  uint32_t start = g_ms_ticks;
  while ((g_ms_ticks - start) < ms) {
  }
}

int main(void)
{
  // 开启 GPIOA/GPIOB/GPIOC 外设时钟（APB2 位 2~4）
  for (uint32_t i = 2U; i <= 4U; ++i) {
    RCC_APB2ENR |= (1UL << i);
  }

  systick_init();

  // 配置 GPIOA/B/C 的所有引脚为 2MHz 推挽输出（每个引脚字段为 0b0010）
  for (uint32_t offset = 0U; offset <= 0x0800UL; offset += 0x0400UL) {
    uint32_t base = GPIO_BASE + offset;
    volatile uint32_t *crl = (volatile uint32_t *)(base + 0x00UL);
    volatile uint32_t *crh = (volatile uint32_t *)(base + 0x04UL);
    *crl = 0x22222222UL; // pin0~7
    *crh = 0x22222222UL; // pin8~15
  }

  // 所有引脚循环闪烁：按端口 A->B->C，再按 pin0->pin15
  while (1) {
    for (uint32_t offset = 0U; offset <= 0x0800UL; offset += 0x0400UL) {
      uint32_t base = GPIO_BASE + offset;
      volatile uint32_t *odr = (volatile uint32_t *)(base + GPIO_ODR_OFFSET);

      for (uint32_t pin = 0U; pin < 16U; ++pin) {
        *odr ^= (1UL << pin);
        delay_ms(150U);
        *odr ^= (1UL << pin);
        delay_ms(150U);
      }
    }
  }
}
