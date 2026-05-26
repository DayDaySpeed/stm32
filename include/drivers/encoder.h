#ifndef DRIVERS_ENCODER_H
#define DRIVERS_ENCODER_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * TIM3 正交编码器（4x 分辨率，硬件计数）。
 *
 * 方向由 board_config.h 的 BOARD_WHEEL_ENCODER_DIRECTION 决定；
 * 也可对调 A/B 接线达到类似效果。
 */

typedef enum {
  TIM3_ENCODER_DIR_NORMAL = 0,    /* A 超前 B 时 CNT 增加 */
  TIM3_ENCODER_DIR_INVERTED = 1,  /* 翻转 A 相极性 */
} tim3_encoder_dir_t;

typedef struct {
  tim3_encoder_dir_t direction;
} tim3_encoder_config_t;

stm_status_t tim3_encoder_init_with_config(const tim3_encoder_config_t *config);
stm_status_t tim3_encoder_read_count(int16_t *out_count);

#endif
