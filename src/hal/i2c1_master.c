#include "hal/i2c1_master.h"

#include "bsp/stm32f103_regs.h"

static uint32_t g_timeout_iter = 100000UL;

static stm_status_t i2c1_wait_sr1(uint32_t mask, uint32_t expect) {
  for (volatile uint32_t i = 0U; i < g_timeout_iter; ++i) {
    uint32_t sr1 = I2C1_SR1;
    if ((sr1 & I2C_SR1_AF_BIT) != 0U) {
      (void)I2C1_SR1;
      (void)I2C1_SR2;
      I2C1_SR1 &= (uint32_t)~I2C_SR1_AF_BIT;
      I2C1_CR1 |= I2C_CR1_STOP_BIT;
      return STM_ERR_NACK;
    }
    if ((sr1 & mask) == expect) {
      return STM_OK;
    }
  }
  I2C1_CR1 |= I2C_CR1_STOP_BIT;
  return STM_ERR_TIMEOUT;
}

static stm_status_t i2c1_wait_not_busy(void) {
  for (volatile uint32_t i = 0U; i < g_timeout_iter; ++i) {
    if ((I2C1_SR2 & I2C_SR2_BUSY_BIT) == 0U) {
      return STM_OK;
    }
  }
  return STM_ERR_BUSY;
}

stm_status_t i2c1_master_init(const i2c1_master_config_t *cfg) {
  uint32_t freq_mhz = 0U;
  uint32_t ccr = 0U;
  uint32_t trise = 0U;

  if ((cfg == 0) || (cfg->pclk_hz < 2000000UL) || (cfg->bus_hz == 0U)) {
    return STM_ERR_INVALID_ARG;
  }
  freq_mhz = cfg->pclk_hz / 1000000UL;
  if ((freq_mhz == 0U) || (freq_mhz > 63U)) {
    return STM_ERR_INVALID_ARG;
  }

  ccr = cfg->pclk_hz / (cfg->bus_hz * 2UL);
  if (ccr == 0U) {
    ccr = 1U;
  }
  if (ccr > 0xFFFUL) {
    return STM_ERR_INVALID_ARG;
  }

  trise = freq_mhz + 1UL;
  if (trise > 0x3FUL) {
    trise = 0x3FUL;
  }

  g_timeout_iter = (cfg->timeout_iter == 0U) ? 100000UL : cfg->timeout_iter;

  AFIO_MAPR |= AFIO_MAPR_I2C1_REMAP_BIT;
  GPIOB_CRH = (GPIOB_CRH & 0xFFFFFF00UL) | 0x000000FFUL;

  I2C1_CR1 &= (uint32_t)~I2C_CR1_PE_BIT;
  I2C1_CR2 = freq_mhz & 0x3FUL;
  I2C1_TRISE = trise & 0x3FUL;
  I2C1_CCR = ccr & 0xFFFUL;
  I2C1_CR1 = I2C_CR1_PE_BIT;
  return STM_OK;
}

stm_status_t i2c1_master_write_frame(uint8_t addr7, uint8_t ctrl,
                                     const uint8_t *payload,
                                     size_t payload_len) {
  stm_status_t st = STM_OK;
  size_t i = 0U;

  if ((payload_len > 0U) && (payload == 0)) {
    return STM_ERR_INVALID_ARG;
  }

  st = i2c1_wait_not_busy();
  if (st != STM_OK) {
    return st;
  }

  I2C1_CR1 |= I2C_CR1_START_BIT;
  st = i2c1_wait_sr1(I2C_SR1_SB_BIT, I2C_SR1_SB_BIT);
  if (st != STM_OK) {
    return st;
  }

  I2C1_DR = (uint32_t)(addr7 << 1);
  st = i2c1_wait_sr1(I2C_SR1_ADDR_BIT, I2C_SR1_ADDR_BIT);
  if (st != STM_OK) {
    return st;
  }
  (void)I2C1_SR1;
  (void)I2C1_SR2;

  st = i2c1_wait_sr1(I2C_SR1_TXE_BIT, I2C_SR1_TXE_BIT);
  if (st != STM_OK) {
    return st;
  }
  I2C1_DR = ctrl;

  for (i = 0U; i < payload_len; ++i) {
    st = i2c1_wait_sr1(I2C_SR1_TXE_BIT, I2C_SR1_TXE_BIT);
    if (st != STM_OK) {
      return st;
    }
    I2C1_DR = payload[i];
  }

  st = i2c1_wait_sr1(I2C_SR1_BTF_BIT, I2C_SR1_BTF_BIT);
  if (st != STM_OK) {
    return st;
  }
  I2C1_CR1 |= I2C_CR1_STOP_BIT;
  return STM_OK;
}
