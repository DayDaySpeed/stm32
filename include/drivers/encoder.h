#ifndef DRIVERS_ENCODER_H
#define DRIVERS_ENCODER_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * TIM3 正交编码器（4x 硬件计数）。
 * 方向见 board_config.h 的 BOARD_WHEEL_ENCODER_DIRECTION。
 */

typedef enum {
  TIM3_ENCODER_DIR_NORMAL = 0,
  TIM3_ENCODER_DIR_INVERTED = 1,
} tim3_encoder_dir_t;

typedef struct {
  tim3_encoder_dir_t direction;
} tim3_encoder_config_t;

/* config->direction：是否翻转 A 相极性。 */
stm_status_t tim3_encoder_init_with_config(const tim3_encoder_config_t *config);
/* out_count：TIM3 CNT 转 int16_t；未 init 返回 STM_ERR_NOT_INITIALIZED。 */
stm_status_t tim3_encoder_read_count(int16_t *out_count);

#endif
