#ifndef DRIVERS_ENCODER_H
#define DRIVERS_ENCODER_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 正交编码器驱动（TIM3 编码器模式，A=PA6 / B=PA7）
 *
 * 工作原理：
 *   - TIM3 设为「Encoder Mode 3」：TI1/TI2 双相位双边沿计数（4x 分辨率）。
 *   - 计数和测向完全由硬件完成，CPU 只读 TIM3_CNT 寄存器即可，零中断开销。
 *   - 旋转方向由 CR1.DIR 反映：0=向上计数（正转），1=向下计数（反转）。
 *
 * 前置条件：
 *   - bsp_clock_apply_profile() 已配好时钟；
 *   - bsp_board_init() 已使能 GPIOA + TIM3 时钟（rcc_board.h 中加了 TIM3EN）。
 *
 * 典型使用：
 *   tim3_encoder_init(TIM3_ENCODER_DIR_NORMAL);   // 上电只调一次
 *
 *   // 控制环里（如 SysTick 中断或主循环 N ms 一次）：
 *   int16_t now   = tim3_encoder_get_count();
 *   int16_t delta = (int16_t)(now - prev);        // 自动处理 16 位回绕
 *   prev = now;
 *   // 速度 = delta / dt
 */

/*
 * 方向约定：
 *   TIM3_ENCODER_DIR_NORMAL   = A 相超前 B 相 -> 顺时针正转 CNT++
 *   TIM3_ENCODER_DIR_INVERTED = 硬件翻转 A 相极性 -> 方向反过来
 *
 * 方向与接线：在 include/bsp/board_config.h 里改 BOARD_WHEEL_ENCODER_DIRECTION；
 * 应用 init 时勿写死 NORMAL/INVERTED。
 */
typedef enum {
  TIM3_ENCODER_DIR_NORMAL = 0,
  TIM3_ENCODER_DIR_INVERTED = 1,
} tim3_encoder_dir_t;

typedef struct {
  tim3_encoder_dir_t direction;
} tim3_encoder_config_t;

/* 初始化 TIM3 编码器模式，并配 PA6/PA7 为输入上拉。 */
stm_status_t tim3_encoder_init_with_config(const tim3_encoder_config_t *config);
stm_status_t tim3_encoder_init(tim3_encoder_dir_t direction);

/* 当前累计计数（带符号 16 位）。 */
stm_status_t tim3_encoder_read_count(int16_t *out_count);
int16_t tim3_encoder_get_count(void);

/* 重置计数为 0。一般只在初始化或「校零」时调用。 */
stm_status_t tim3_encoder_reset_count(void);

/* 当前旋转方向。0 = 向上计数，1 = 向下计数。 */
stm_status_t tim3_encoder_read_direction(uint8_t *out_direction);
uint8_t tim3_encoder_get_direction(void);

#endif
