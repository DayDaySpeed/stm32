#ifndef DRIVERS_BUZZER_H
#define DRIVERS_BUZZER_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 有源蜂鸣器 —— GPIO 推挽输出，极性由 board_config.h 配置。
 * active_high：1=高电平响，0=低电平响（常见三脚模块）。
 */

typedef struct {
  uint8_t active_high;
} buzzer_config_t;

stm_status_t buzzer_init_with_config(const buzzer_config_t *config);
stm_status_t buzzer_on(void);
stm_status_t buzzer_off(void);
stm_status_t buzzer_beep_blocking(uint32_t duration_ms);

#endif
