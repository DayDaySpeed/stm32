#ifndef DRIVERS_BUZZER_H
#define DRIVERS_BUZZER_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 有源蜂鸣器 GPIO 驱动；极性由 config.active_high 决定。
 */

typedef struct {
  uint8_t active_high; /* 1=高电平响，0=低电平响 */
} buzzer_config_t;

/* config->active_high：1=高电平响。 */
stm_status_t buzzer_init_with_config(const buzzer_config_t *config);
stm_status_t buzzer_on(void);
stm_status_t buzzer_off(void);
/* duration_ms：响铃时长，内部阻塞 systick_delay_ms。 */
stm_status_t buzzer_beep_blocking(uint32_t duration_ms);

#endif
