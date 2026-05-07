#include "common/ring_buffer.h"

static uint16_t ring_buffer_next(const ring_buffer_t *rb, uint16_t idx) {
  uint16_t next = (uint16_t)(idx + 1U);
  if (next >= rb->capacity) {
    next = 0U;
  }
  return next;
}

stm_status_t ring_buffer_init(ring_buffer_t *rb, uint8_t *backing,
                              uint16_t capacity) {
  if ((rb == 0) || (backing == 0) || (capacity < 2U)) {
    return STM_ERR_INVALID_ARG;
  }
  rb->data = backing;
  rb->capacity = capacity;
  rb->head = 0U;
  rb->tail = 0U;
  return STM_OK;
}

stm_status_t ring_buffer_push_byte(ring_buffer_t *rb, uint8_t value) {
  uint16_t next = 0U;
  if (rb == 0) {
    return STM_ERR_INVALID_ARG;
  }
  next = ring_buffer_next(rb, rb->head);
  if (next == rb->tail) {
    return STM_ERR_OVERFLOW;
  }
  rb->data[rb->head] = value;
  rb->head = next;
  return STM_OK;
}

stm_status_t ring_buffer_pop_byte(ring_buffer_t *rb, uint8_t *out) {
  if ((rb == 0) || (out == 0)) {
    return STM_ERR_INVALID_ARG;
  }
  if (rb->head == rb->tail) {
    return STM_ERR_BUSY;
  }
  *out = rb->data[rb->tail];
  rb->tail = ring_buffer_next(rb, rb->tail);
  return STM_OK;
}

uint16_t ring_buffer_count(const ring_buffer_t *rb) {
  if (rb == 0) {
    return 0U;
  }
  if (rb->head >= rb->tail) {
    return (uint16_t)(rb->head - rb->tail);
  }
  return (uint16_t)(rb->capacity - (rb->tail - rb->head));
}
