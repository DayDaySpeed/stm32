#ifndef COMMON_RING_BUFFER_H
#define COMMON_RING_BUFFER_H

#include <stdint.h>

#include "common/stm_status.h"

/*
 * 定长字节环形缓冲（head 写、tail 读）。
 * 容量须 ≥ 2；满时 push 返回 STM_ERR_OVERFLOW，空时 pop 返回 STM_ERR_BUSY。
 */

typedef struct {
  uint8_t *data;
  uint16_t capacity;
  volatile uint16_t head; /* 下一写入位置 */
  volatile uint16_t tail; /* 下一读出位置 */
} ring_buffer_t;

stm_status_t ring_buffer_init(ring_buffer_t *rb, uint8_t *backing,
                              uint16_t capacity);
stm_status_t ring_buffer_push_byte(ring_buffer_t *rb, uint8_t value);
stm_status_t ring_buffer_pop_byte(ring_buffer_t *rb, uint8_t *out);

#endif
