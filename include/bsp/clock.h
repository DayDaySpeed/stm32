#ifndef BSP_CLOCK_H
#define BSP_CLOCK_H

#include <stdint.h>

#include "common/stm_status.h"

typedef enum {
  BSP_CLOCK_PROFILE_HSI_8MHZ = 0,
  BSP_CLOCK_PROFILE_HSE_PLL_72MHZ = 1
} bsp_clock_profile_t;

/* 配置系统时钟方案（当前支持 HSI 8MHz / HSE+PLL 72MHz）。 */
stm_status_t bsp_clock_apply_profile(bsp_clock_profile_t profile);

/* 运行时查询时钟频率，避免业务层依赖 RCC 细节。 */
uint32_t bsp_clock_get_sysclk_hz(void);
uint32_t bsp_clock_get_hclk_hz(void);
uint32_t bsp_clock_get_pclk1_hz(void);
uint32_t bsp_clock_get_pclk2_hz(void);

#define SYSCLK_HZ (bsp_clock_get_sysclk_hz())
#define BSP_PCLK1_HZ (bsp_clock_get_pclk1_hz())
#define BSP_PCLK2_HZ (bsp_clock_get_pclk2_hz())
#define BSP_SYSTICK_RELOAD_1MS ((bsp_clock_get_hclk_hz() / 1000UL) - 1UL)

#endif
