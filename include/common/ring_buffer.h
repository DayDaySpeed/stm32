#ifndef COMMON_RING_BUFFER_H
#define COMMON_RING_BUFFER_H

#include <stdint.h>

#include "common/stm_status.h"

typedef struct {
  uint8_t *data;
  uint16_t capacity;
  volatile uint16_t head;
  volatile uint16_t tail;
} ring_buffer_t;

stm_status_t ring_buffer_init(ring_buffer_t *rb, uint8_t *backing,
                              uint16_t capacity);
stm_status_t ring_buffer_push_byte(ring_buffer_t *rb, uint8_t value);
stm_status_t ring_buffer_pop_byte(ring_buffer_t *rb, uint8_t *out);
uint16_t ring_buffer_count(const ring_buffer_t *rb);

#endif
