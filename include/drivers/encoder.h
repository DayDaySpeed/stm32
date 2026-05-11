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
 * 实测时如果发现「向右转」反而 CNT 减少，把入参换成 INVERTED 即可，
 * 等价于硬件层面交换 A/B，应用层无需再处理符号。
 */
typedef enum {
  TIM3_ENCODER_DIR_NORMAL = 0,
  TIM3_ENCODER_DIR_INVERTED = 1,
} tim3_encoder_dir_t;

/* 初始化 TIM3 编码器模式，并配 PA6/PA7 为输入上拉。 */
stm_status_t tim3_encoder_init(tim3_encoder_dir_t direction);

/* 当前累计计数（带符号 16 位）。
 * 注意：TIM3_CNT 是 16 位寄存器，会自然回绕（0xFFFF -> 0），
 *      调用方应**短周期采样**并用 (int16_t) 强制截断来计算增量，
 *      让硬件回绕和软件回绕匹配上。 */
int16_t tim3_encoder_get_count(void);

/* 重置计数为 0。一般只在初始化或「校零」时调用。 */
void tim3_encoder_reset_count(void);

/* 当前旋转方向。返回值：
 *   0 = 向上计数（正转 / forward）
 *   1 = 向下计数（反转 / reverse）
 * 注意：编码器停转时方向位保持上一次的值，所以单看方向不可靠，
 *      实际控制还是要靠 delta 的正负号。 */
uint8_t tim3_encoder_get_direction(void);

#endif
