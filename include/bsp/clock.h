#ifndef BSP_CLOCK_H
#define BSP_CLOCK_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 系统时钟配置与频率查询。
 */

typedef enum {
  BSP_CLOCK_PROFILE_HSI_8MHZ = 0,
  BSP_CLOCK_PROFILE_HSE_PLL_72MHZ = 1
} bsp_clock_profile_t;

/* profile：切换 HSI 8MHz 或 HSE+PLL 72MHz，并更新内部频率缓存。 */
stm_status_t bsp_clock_apply_profile(bsp_clock_profile_t profile);

/* 返回当前 HCLK / PCLK1 / PCLK2 频率（Hz），由 apply_profile 更新。 */
uint32_t bsp_clock_get_hclk_hz(void);
uint32_t bsp_clock_get_pclk1_hz(void);
uint32_t bsp_clock_get_pclk2_hz(void);
/* F1：APB 预分频≠1 时定时器时钟 = PCLK×2。 */
uint32_t bsp_clock_get_apb1_timer_hz(void);
uint32_t bsp_clock_get_apb2_timer_hz(void);

#define BSP_SYSTICK_RELOAD_1MS ((bsp_clock_get_hclk_hz() / 1000UL) - 1UL)

#endif
