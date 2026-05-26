#include "hal/i2c1_master.h"

#include <stddef.h>

#include "bsp/board_pins.h"
#include "bsp/board_gpio.h"
#include "bsp/stm32f103_regs.h"

/*
 * 轮询超时上限（循环次数，非毫秒）。
 * 72MHz 下约 1ms/次等待；SSD1306 失败时应快速返回，避免拖死主循环。
 */
#define I2C1_MASTER_DEFAULT_TIMEOUT_ITER (3000UL)

#define I2C1_SCL_PIN BOARD_GPIO_I2C1_SCL_PIN
#define I2C1_SDA_PIN BOARD_GPIO_I2C1_SDA_PIN

static uint32_t g_timeout_iter = I2C1_MASTER_DEFAULT_TIMEOUT_ITER;

static void i2c1_gpio_apply_mode(uint32_t mode) {
  board_gpio_apply_crl(BOARD_GPIO_I2C1_CR_REG, BOARD_GPIO_I2C1_CR_MASK, mode);
}

static void i2c1_gpio_set_lines(uint8_t scl_high, uint8_t sda_high) {
  board_gpio_write(BOARD_GPIO_I2C1_BSRR_REG, I2C1_SCL_PIN, scl_high);
  board_gpio_write(BOARD_GPIO_I2C1_BSRR_REG, I2C1_SDA_PIN, sda_high);
}

static void i2c1_delay_short(void) {
  for (volatile uint32_t d = 0U; d < 64U; ++d) {
  }
}

/*
 * 总线卡死时：关 PE → GPIO 模拟 9 个 SCL 脉冲 → 恢复 AF 开漏 → 再开 PE。
 */
void i2c1_master_bus_recover(void) {
  I2C1_CR1 &= (uint32_t)~I2C_CR1_PE_BIT;

  i2c1_gpio_apply_mode(BOARD_GPIO_I2C1_MODE_GPIO_OD);
  i2c1_gpio_set_lines(1U, 1U);
  i2c1_delay_short();

  for (uint8_t pulse = 0U; pulse < 9U; pulse++) {
    board_gpio_write(BOARD_GPIO_I2C1_BSRR_REG, I2C1_SCL_PIN, 0U);
    i2c1_delay_short();
    board_gpio_write(BOARD_GPIO_I2C1_BSRR_REG, I2C1_SCL_PIN, 1U);
    i2c1_delay_short();
  }

  board_gpio_write(BOARD_GPIO_I2C1_BSRR_REG, I2C1_SCL_PIN, 1U);
  i2c1_delay_short();
  board_gpio_write(BOARD_GPIO_I2C1_BSRR_REG, I2C1_SDA_PIN, 0U);
  i2c1_delay_short();
  board_gpio_write(BOARD_GPIO_I2C1_BSRR_REG, I2C1_SDA_PIN, 1U);
  i2c1_delay_short();

  i2c1_gpio_apply_mode(BOARD_GPIO_I2C1_MODE_AF_OD);
  I2C1_CR1 |= I2C_CR1_PE_BIT;
}

/*
 * 等待 SR1 的 mask 字段达到 expect 值；同时拦截 NACK（AF=1）。
 * 出错路径会发 STOP，避免总线悬挂。
 */
static stm_status_t i2c1_wait_sr1(uint32_t mask, uint32_t expect) {
  for (volatile uint32_t i = 0U; i < g_timeout_iter; ++i) {
    uint32_t sr1 = I2C1_SR1;
    if ((sr1 & I2C_SR1_AF_BIT) != 0U) {
      (void)I2C1_SR1;
      (void)I2C1_SR2;
      I2C1_SR1 &= (uint32_t)~I2C_SR1_AF_BIT;
      I2C1_CR1 |= I2C_CR1_STOP_BIT;
      i2c1_master_bus_recover();
      return STM_ERR_NACK;
    }
    if ((sr1 & mask) == expect) {
      return STM_OK;
    }
  }
  I2C1_CR1 |= I2C_CR1_STOP_BIT;
  i2c1_master_bus_recover();
  return STM_ERR_TIMEOUT;
}

static stm_status_t i2c1_wait_not_busy(void) {
  for (volatile uint32_t i = 0U; i < g_timeout_iter; ++i) {
    if ((I2C1_SR2 & I2C_SR2_BUSY_BIT) == 0U) {
      return STM_OK;
    }
  }
  i2c1_master_bus_recover();
  return STM_ERR_BUSY;
}

/*
 * 初始化 I2C1 主机。
 *
 * 完整流程：
 *   [1] 校验入参（cfg 非空、PCLK1 ≥ 2MHz、目标总线频率 > 0）
 *   [2] 计算 CR2.FREQ：以 MHz 为单位的 PCLK1 值（让 I2C 外设知道自己的输入时钟）
 *   [3] 计算 CCR    ：标准模式下决定 SCL 的高/低电平时长
 *   [4] 计算 TRISE  ：SDA/SCL 允许的最大上升时间（防止上拉电阻太弱时误采样）
 *   [5] 设置轮询超时上限（用户可覆盖默认值）
 *   [6] 重映射 + 配 GPIO：I2C1 SCL/SDA 接到 PB8/PB9，复用开漏 50MHz
 *   [7] 关 PE → 写时序寄存器 → 开 PE（PE=1 时硬件不允许改时序参数）
 */
stm_status_t i2c1_master_init(const i2c1_master_config_t *cfg) {
  uint32_t freq_mhz = 0U;
  uint32_t ccr = 0U;
  uint32_t trise = 0U;

  /* ---------- [1] 入参校验 ----------
   * - cfg == NULL：调用方传错
   * - pclk1_hz < 2MHz：STM32F1 I2C 标准模式硬性要求（手册规定 FREQ ≥ 2）
   * - bus_hz == 0：除零保护（后面 CCR 计算会用到） */
  if ((cfg == NULL) || (cfg->pclk1_hz < 2000000UL) || (cfg->bus_hz == 0U)) {
    return STM_ERR_INVALID_ARG;
  }

  /* ---------- [2] 计算 CR2.FREQ（PCLK1 的 MHz 值）----------
   * CR2.FREQ[5:0] 字段告诉 I2C 外设「我的输入时钟是多少 MHz」，
   * 硬件内部据此推算 1μs 等基本时间单位。
   *
   * 取值范围：
   *   - 字段宽度 6 bit，理论 0..63
   *   - 实际硬件标准模式要求 ≥ 2，快速模式 ≥ 4
   *   - F1 上 PCLK1 不会超过 36MHz，所以 freq_mhz ∈ [2, 36] 是常态 */
  freq_mhz = cfg->pclk1_hz / 1000000UL;
  if ((freq_mhz == 0U) || (freq_mhz > 63U)) {
    return STM_ERR_INVALID_ARG;
  }

  /* ---------- [3] 计算 CCR（SCL 周期）----------
   * 标准模式（100kHz 及以下）公式：
   *   T_high = T_low = CCR × T_pclk1
   *   T_scl  = T_high + T_low = 2 × CCR × T_pclk1
   *   => CCR = PCLK1 / (2 × bus_hz)
   *
   * 例：PCLK1=36MHz、bus_hz=100kHz → CCR = 36e6 / 200e3 = 180
   *
   * 边界：
   *   - CCR=0 不合法，硬件直接忽略 → 钳到 1（极端情况，比如 PCLK1=2MHz、bus_hz=1MHz）
   *   - CCR 字段 12 bit，最大 0xFFF（4095），超了说明总线频率配得太低 */
  ccr = cfg->pclk1_hz / (cfg->bus_hz * 2UL);
  if (ccr == 0U) {
    ccr = 1U;
  }
  if (ccr > 0xFFFUL) {
    return STM_ERR_INVALID_ARG;
  }

  /* ---------- [4] 计算 TRISE（最大上升时间）----------
   * I2C 协议规定 SCL/SDA 上升时间不能超过：
   *   - 标准模式：1000ns
   *   - 快速模式：300ns
   *
   * TRISE 字段填的是「以 PCLK1 周期为单位 + 1」：
   *   TRISE = (max_rise_ns × PCLK1_hz / 1e9) + 1
   *
   * 标准模式 1μs 上升时间 → TRISE = freq_mhz + 1
   *   例：PCLK1=36MHz → TRISE = 37
   *
   * 字段 6 bit，上限 0x3F=63，超了夹住即可（实际工程上不会超）。 */
  trise = freq_mhz + 1UL;
  if (trise > 0x3FUL) {
    trise = 0x3FUL;
  }

  /* ---------- [5] 超时窗口 ----------
   * 0 表示用默认值；非 0 直接采纳（慢设备如 EEPROM 写周期需要调大）。 */
  g_timeout_iter =
      (cfg->timeout_iter == 0U) ? I2C1_MASTER_DEFAULT_TIMEOUT_ITER
                                : cfg->timeout_iter;

  /* ---------- [6] GPIO 配置 ----------
   * I2C1 默认引脚是 PB6(SCL)/PB7(SDA)，重映射后挪到 PB8(SCL)/PB9(SDA)，
   * 本板用的是重映射版本。复用开漏（AF_OD）+ 外部上拉电阻是 I2C 标准接法。 */
  board_gpio_afio_apply(AFIO_MAPR_I2C1_REMAP_BIT, BOARD_AFIO_I2C1_REMAP);
  i2c1_gpio_apply_mode(BOARD_GPIO_I2C1_MODE_AF_OD);

  /* ---------- [7] 写时序寄存器并启动 ----------
   * 顺序关键：
   *   - 先 PE=0：CR1.PE=1 时硬件锁定 CCR/TRISE/FREQ，写入无效，必须先关
   *   - 写 FREQ/TRISE/CCR 顺序无所谓，但都得在 PE=1 之前
   *   - 最后 PE=1 启动外设
   *
   * 这里没用 |=，直接整写 CR1：清掉所有别的位（如可能残留的 START/STOP/ACK 配置），
   * 保证从干净状态启动。 */
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

  if ((payload_len > 0U) && (payload == NULL)) {
    return STM_ERR_INVALID_ARG;
  }

  st = i2c1_wait_not_busy();
  if (st != STM_OK) {
    return st;
  }

  /* 1) 起始条件 -> EV5（SB=1）。 */
  I2C1_CR1 |= I2C_CR1_START_BIT;
  st = i2c1_wait_sr1(I2C_SR1_SB_BIT, I2C_SR1_SB_BIT);
  if (st != STM_OK) {
    return st;
  }

  /* 2) 写入 7 位地址并左移加上写位 -> EV6（ADDR=1，读 SR1+SR2 清掉）。 */
  I2C1_DR = (uint32_t)(addr7 << 1);
  st = i2c1_wait_sr1(I2C_SR1_ADDR_BIT, I2C_SR1_ADDR_BIT);
  if (st != STM_OK) {
    return st;
  }
  (void)I2C1_SR1;
  (void)I2C1_SR2;

  /* 3) 控制字节（ctrl）。 */
  st = i2c1_wait_sr1(I2C_SR1_TXE_BIT, I2C_SR1_TXE_BIT);
  if (st != STM_OK) {
    return st;
  }
  I2C1_DR = ctrl;

  /* 4) 数据负载。 */
  for (i = 0U; i < payload_len; ++i) {
    st = i2c1_wait_sr1(I2C_SR1_TXE_BIT, I2C_SR1_TXE_BIT);
    if (st != STM_OK) {
      return st;
    }
    I2C1_DR = payload[i];
  }

  /* 5) BTF 表示数据寄存器和移位寄存器都空 -> 发 STOP 收尾。 */
  st = i2c1_wait_sr1(I2C_SR1_BTF_BIT, I2C_SR1_BTF_BIT);
  if (st != STM_OK) {
    return st;
  }
  I2C1_CR1 |= I2C_CR1_STOP_BIT;
  return STM_OK;
}
