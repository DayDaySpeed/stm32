#ifndef COMMON_RING_BUFFER_H
#define COMMON_RING_BUFFER_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 定长字节环形缓冲（单生产者单消费者）。
 */

typedef struct {
  uint8_t *data;
  uint16_t capacity;
  volatile uint16_t head;
  volatile uint16_t tail;
} ring_buffer_t;

/* rb：对象；backing：外部存储；capacity：容量，须 ≥2。 */
stm_status_t ring_buffer_init(ring_buffer_t *rb, uint8_t *backing,
                              uint16_t capacity);
/* 满返回 STM_ERR_OVERFLOW。 */
stm_status_t ring_buffer_push_byte(ring_buffer_t *rb, uint8_t value);
/* 空返回 STM_ERR_BUSY。 */
stm_status_t ring_buffer_pop_byte(ring_buffer_t *rb, uint8_t *out);

#endif
