#ifndef COMMON_STM_STATUS_H
#define COMMON_STM_STATUS_H

#include <stdint.h>

typedef enum {
  STM_OK = 0,
  STM_ERR_INVALID_ARG = 1,
  STM_ERR_BUSY = 2,
  STM_ERR_TIMEOUT = 3,
  STM_ERR_NACK = 4,
  STM_ERR_OVERFLOW = 5,
  STM_ERR_IO = 6
} stm_status_t;

#endif
