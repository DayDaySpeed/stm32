/*
 * TIM3 正交编码器驱动（A=PA6 / B=PA7，Encoder Mode 3 = 4x 分辨率）
 *
 * 寄存器配置流程（init 一次性完成，之后纯硬件运行）：
 *   1) 停 CNT、清状态
 *   2) PSC=0（编码器模式禁止预分频，否则脉冲会被滤掉）
 *      ARR=0xFFFF（让 16 位计数器循环到顶才回绕）
 *   3) CCMR1：CC1S/CC2S=01 把 CH1/CH2 配成输入连到 TI1/TI2；
 *            IC1F/IC2F=0xF 开最大数字滤波（挡毛刺/抖动）；
 *            IC1PSC/IC2PSC 清零（不预分频，4x 分辨率必须）
 *   4) CCER：CC1E/CC2E=1 启用输入通道；
 *            CC1P 按入参 direction 选择极性（INVERTED 时置 1，等价交换 A/B）
 *   5) SMCR.SMS=011（编码器模式 3：TI1+TI2 双相双边沿）
 *   6) CR1.CEN=1 启动计数器
 */

#include "drivers/encoder.h"

#include "bsp/board_pins.h"
#include "bsp/stm32f103_regs.h"

#include <stddef.h>
#include <stdint.h>

static uint8_t g_tim3_encoder_initialized;

stm_status_t tim3_encoder_init_with_config(const tim3_encoder_config_t *config) {
  if (config == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if ((config->direction != TIM3_ENCODER_DIR_NORMAL) &&
      (config->direction != TIM3_ENCODER_DIR_INVERTED)) {
    return STM_ERR_INVALID_ARG;
  }

  /* ---------- 1) GPIO PA6/PA7 配成输入上拉 ---------- */
  GPIOA_CRL = (GPIOA_CRL & ~(BOARD_GPIO_PA6_CRL_MASK | BOARD_GPIO_PA7_CRL_MASK)) |
              BOARD_GPIO_PA6_INPUT_PULL | BOARD_GPIO_PA7_INPUT_PULL;
  /* 输入模式下，ODR 的对应 bit 决定上拉(1) 还是下拉(0)；编码器接 VCC 用上拉。 */
  GPIOA_ODR |= BOARD_GPIO_PA6_PA7_ODR_PULLUP;

  /* ---------- 2) 停 CNT，确保配置过程中不会跑飞 ---------- */
  TIM3_CR1 &= ~TIM_CR1_CEN_BIT;

  /* ---------- 3) 时基：PSC=0（编码器模式必须）、ARR=最大 ---------- */
  TIM3_PSC = 0U;
  TIM3_ARR = 0xFFFFU;

  /* ---------- 4) CCMR1：CH1/CH2 配为输入，开滤波、无预分频 ----------
   * - CC1S/CC2S = 01：把 CH1/CH2 接到 TI1/TI2 输入；
   * - IC1F/IC2F = 0xF：最大数字滤波（fDTS/32, N=8，约 3.5μs 窗口）。
   *   电机编码器脉冲周期通常几十~几百 μs，不会被滤掉；
   *   但能挡住 EMI 毛刺和机械触点抖动的高频成分。
   * - IC1PSC/IC2PSC = 00：不分频，每个边沿都参与计数（4x 分辨率必须）。 */
  TIM3_CCMR1 &= ~(TIM_CCMR1_CC1S_MASK | TIM_CCMR1_CC2S_MASK |
                  TIM_CCMR1_IC1F_MASK | TIM_CCMR1_IC2F_MASK |
                  TIM_CCMR1_IC1PSC_MASK | TIM_CCMR1_IC2PSC_MASK);
  TIM3_CCMR1 |= TIM_CCMR1_CC1S_TI1 | TIM_CCMR1_CC2S_TI2 |
                TIM_CCMR1_IC1F_MAX | TIM_CCMR1_IC2F_MAX;

  /* ---------- 5) CCER：极性配置 + 通道使能 ----------
   * 关键事实：只翻转其中一个 (CC1P 或 CC2P) 等价于交换 A/B，方向反转；
   *           两个都翻转又恢复原方向。所以这里只按入参动 CC1P。 */
  TIM3_CCER &= ~(TIM_CCER_CC1P_BIT | TIM_CCER_CC2P_BIT);
  TIM3_CCER |= TIM_CCER_CC1E_BIT | TIM_CCER_CC2E_BIT;
  if (config->direction == TIM3_ENCODER_DIR_INVERTED) {
    TIM3_CCER |= TIM_CCER_CC1P_BIT; /* A 相反相 -> 整体计数方向反转 */
  }

  /* ---------- 6) SMCR：从模式 = 编码器模式 3（4x 分辨率）---------- */
  TIM3_SMCR = (TIM3_SMCR & ~TIM_SMCR_SMS_MASK) | TIM_SMCR_SMS_ENC3;

  /* ---------- 7) 清状态位、CNT 归零、启动计数器 ---------- */
  TIM3_CNT = 0U;
  TIM3_SR = 0U;
  TIM3_CR1 |= TIM_CR1_CEN_BIT;
  g_tim3_encoder_initialized = 1U;

  return STM_OK;
}

stm_status_t tim3_encoder_init(tim3_encoder_dir_t direction) {
  const tim3_encoder_config_t config = {.direction = direction};
  return tim3_encoder_init_with_config(&config);
}

stm_status_t tim3_encoder_read_count(int16_t *out_count) {
  if (out_count == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (g_tim3_encoder_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  *out_count = (int16_t)(uint16_t)TIM3_CNT;
  return STM_OK;
}

int16_t tim3_encoder_get_count(void) {
  /* 直接强转为 int16_t：让 0xFFFF 显示为 -1，0x8000 显示为 -32768，
   * 这样 (now - prev) 在两端附近回绕时也能给出正确的「带符号增量」。 */
  return (int16_t)(uint16_t)TIM3_CNT;
}

stm_status_t tim3_encoder_reset_count(void) {
  if (g_tim3_encoder_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  TIM3_CNT = 0U;
  return STM_OK;
}

stm_status_t tim3_encoder_read_direction(uint8_t *out_direction) {
  if (out_direction == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (g_tim3_encoder_initialized == 0U) {
    return STM_ERR_NOT_INITIALIZED;
  }
  *out_direction = ((TIM3_CR1 & TIM_CR1_DIR_BIT) != 0U) ? 1U : 0U;
  return STM_OK;
}

uint8_t tim3_encoder_get_direction(void) {
  return ((TIM3_CR1 & TIM_CR1_DIR_BIT) != 0U) ? 1U : 0U;
}
