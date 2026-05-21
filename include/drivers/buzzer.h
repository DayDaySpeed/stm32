#ifndef DRIVERS_BUZZER_H
#define DRIVERS_BUZZER_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 有源蜂鸣器（电平驱动，非 PWM 音调）
 *
 * 本板默认：PA4 推挽输出，低电平响（BOARD_BUZZER_ACTIVE_HIGH=0，常见三脚模块）。
 * 上电一直叫 → 在 board_config.h 把 ACTIVE_HIGH 改为 1 试一次。
 *
 * 前置：bsp_board_init()（GPIOA 时钟）。
 */

typedef struct {
  uint8_t active_high;
} buzzer_config_t;

stm_status_t buzzer_init_with_config(const buzzer_config_t *config);
stm_status_t buzzer_init(void);
stm_status_t buzzer_on(void);
stm_status_t buzzer_off(void);

/* 阻塞鸣叫 duration_ms 毫秒后关闭。 */
stm_status_t buzzer_beep_blocking(uint32_t duration_ms);

#endif
