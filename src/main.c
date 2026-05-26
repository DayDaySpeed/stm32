/*
 * 固件入口：配置时钟 → 板级初始化 → 应用主循环。
 *
 * 中断向量：
 *   SysTick_Handler  — 1ms 系统节拍（systick）
 *   USART1_IRQHandler — 串口接收字节入环形缓冲
 *
 * 上电顺序不可随意调换：电机 GPIO 须在 app_init 之前拉安全态。
 */
#include "app/app.h"
#include "bsp/board_init.h"
#include "bsp/board_devices.h"
#include "bsp/clock.h"
#include "common/stm_fault.h"
#include "drivers/systick.h"

void SysTick_Handler(void) { systick_on_interrupt(); }

/* 转发至 bsp_console_irq_handler → usart1_irq_handler。 */
void USART1_IRQHandler(void) { bsp_console_irq_handler(); }

/* 上电入口：时钟 → 板级 init → 电机安全态 → app。失败则 stm_fault_halt。 */
int main(void) {
  stm_status_t st = STM_OK;

  /* 可切换时钟方案：HSI 8MHz / HSE+PLL 72MHz */
  st = bsp_clock_apply_profile(BSP_CLOCK_PROFILE_HSE_PLL_72MHZ);
  if (st != STM_OK) {
    stm_fault_halt("clock", st);
  }
  bsp_board_init();
  bsp_dc_motor_gpio_safe_early();
  st = app_init();
  if (st != STM_OK) {
    stm_fault_halt("app_init", st);
  }
  app_run_forever();
}
