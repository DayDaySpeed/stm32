#ifndef BSP_CLOCK_H
#define BSP_CLOCK_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 系统时钟配置与运行时频率查询。
 * 驱动算波特率/PWM/ADC 分频时统一用 getter，避免硬编码 72MHz。
 */

typedef enum {
  BSP_CLOCK_PROFILE_HSI_8MHZ = 0,       /* 内部 8MHz，调试用 */
  BSP_CLOCK_PROFILE_HSE_PLL_72MHZ = 1   /* 外部 8MHz × PLL9 → 72MHz */
} bsp_clock_profile_t;

stm_status_t bsp_clock_apply_profile(bsp_clock_profile_t profile);

uint32_t bsp_clock_get_hclk_hz(void);
uint32_t bsp_clock_get_pclk1_hz(void);
uint32_t bsp_clock_get_pclk2_hz(void);

/* F1 规则：APB 预分频≠1 时，该总线上定时器时钟 = PCLK×2 */
uint32_t bsp_clock_get_apb1_timer_hz(void);
uint32_t bsp_clock_get_apb2_timer_hz(void);

#define BSP_SYSTICK_RELOAD_1MS ((bsp_clock_get_hclk_hz() / 1000UL) - 1UL)

#endif
