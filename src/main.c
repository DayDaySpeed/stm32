#include "GPIO.h"
#include "RCC.h"
#include "SYS.h"




static volatile uint32_t g_ms_ticks = 0;            // 全局毫秒计数器（在中断中递增）



void SysTick_Handler(void)
{
  // 每 1ms 进入一次中断，维护系统节拍
  g_ms_ticks++;
}

static void systick_init(void)
{
  // 8MHz/1000 = 8000，每 1ms 产生一次中断
  SYST_RVR = (SYSCLK_HZ / 1000UL) - 1UL;
  // 清当前计数器，确保从重装载值开始计数
  SYST_CVR = 0UL;
  // 选择处理器时钟 + 开中断 + 使能计数器
  SYST_CSR = SYSTICK_CLKSRC_BIT | SYSTICK_TICKINT_BIT | SYSTICK_ENABLE_BIT;
}

static void delay_ms(uint32_t ms)
{
  // 通过比较节拍差值实现阻塞延时（可自然处理计数器溢出）
  uint32_t start = g_ms_ticks;
  while ((g_ms_ticks - start) < ms) {
  }
}

int main(void)
{
  // 1) 打开 GPIOC 外设时钟
  RCC_APB2ENR |= RCC_IOPCEN_BIT;
  // 2) 初始化 1ms 系统节拍
  systick_init();

  // 3) 配置 PC13 为 2MHz 推挽输出
  GPIOC_CRH &= ~GPIO13_MODE_MSK;
  GPIOC_CRH |= GPIO13_OUT_2M_PP;

  // 4) 每 500ms 翻转一次 PC13（板载 LED）
  while (1) {
    GPIOC_ODR ^= GPIO13_ODR_BIT;
    delay_ms(500U);
  }
}
