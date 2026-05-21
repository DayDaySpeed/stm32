#ifndef BSP_BOARD_CONFIG_H
#define BSP_BOARD_CONFIG_H

#include "drivers/encoder.h"

/*
 * 板级可调参数（用户改这里即可，勿改驱动 / app 源码）
 *
 * 编码器方向：
 *   TIM3_ENCODER_DIR_NORMAL   — 默认：A 超前 B 时 CNT 增加
 *   TIM3_ENCODER_DIR_INVERTED — 若拧旋钮时 ENC 减少、或电机加减速反了，换这个
 *
 * 也可对调编码器 A/B 接线（PA6↔PA7），效果与 INVERTED 类似。
 */
#define BOARD_WHEEL_ENCODER_DIRECTION TIM3_ENCODER_DIR_INVERTED

#endif
