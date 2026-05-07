#ifndef HAL_I2C1_MASTER_H
#define HAL_I2C1_MASTER_H

#include <stddef.h>
#include <stdint.h>

#include "common/stm_status.h"

typedef struct {
  uint32_t pclk_hz;
  uint32_t bus_hz;
  uint32_t timeout_iter;
} i2c1_master_config_t;

stm_status_t i2c1_master_init(const i2c1_master_config_t *cfg);
stm_status_t i2c1_master_write_frame(uint8_t addr7, uint8_t ctrl,
                                     const uint8_t *payload,
                                     size_t payload_len);

#endif
