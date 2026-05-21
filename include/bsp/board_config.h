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

/*
 * 有源蜂鸣器触发极性（三脚模块 VCC/GND/S）：
 *   0 = 低电平响（常见：S 接 PA4，空闲须为高电平）← 默认
 *   1 = 高电平响
 * 若上电后一直叫，把 0/1 对调试一次。
 */
#define BOARD_BUZZER_ACTIVE_HIGH      (0U)
/* 1=手靠近红外时鸣叫；0=关闭（仅排查硬件用） */
#define BOARD_IR_PROXIMITY_BEEP_ENABLE (1U)
/* 单次鸣叫时长（毫秒） */
#define BOARD_BUZZER_BEEP_MS          (80U)
/* 手靠近红外后再次允许鸣叫的最小间隔（毫秒） */
#define BOARD_IR_BEEP_COOLDOWN_MS     (800U)

/*
 * 反射红外「靠近」判定（本板 TCRT5000 类：远离 ~4000，靠近 ~100，故用「低于」判靠近）：
 *   raw <= NEAR_LOW  → 视为手靠近，可触发蜂鸣
 *   raw >= LEAVE_HIGH → 视为手离开，允许下次再响
 */
#define BOARD_IR_NEAR_RAW_LOW         (500U)
#define BOARD_IR_LEAVE_RAW_HIGH       (3500U)
/* 连续多少次判定为「靠近/远离」才动作（滤 ADC 毛刺） */
#define BOARD_IR_NEAR_STREAK_COUNT    (3U)
#define BOARD_IR_LEAVE_STREAK_COUNT   (2U)
/* 上电后须先见到「远离」读数，才允许靠近鸣叫（避免启动误响） */
#define BOARD_IR_ARM_RAW_HIGH         (3000U)
#define BOARD_IR_ARM_STREAK_COUNT     (5U)

#endif
