# I2C1 主机 HAL（`i2c1_master`）

> **分层**：本模块属 [HAL 层](../hal/README.md)，非 `src/drivers/`。文档放在此目录是为与 SSD1306 相邻。

## 作用

提供 **I2C1 主机** 阻塞式写帧事务，供 SSD1306 等从设备使用。位于 `src/hal/`，介于 BSP 时钟/引脚与具体器件驱动之间。

## 硬件

| 信号 | 引脚 | 说明 |
|------|------|------|
| SCL | PB8 | 复用开漏（AFIO 重映射 I2C1） |
| SDA | PB9 | 复用开漏 |

须外接上拉电阻（通常 4.7kΩ~10kΩ 到 3.3V）。

## 配置结构体

```c
typedef struct {
  uint32_t pclk1_hz;      /* bsp_clock_get_pclk1_hz()，≥ 2MHz */
  uint32_t bus_hz;        /* 目标 SCL，如 100000 */
  uint32_t timeout_iter;  /* 轮询超时循环次数，0=默认 3000 */
} i2c1_master_config_t;
```

## API 参考

| 函数 | 说明 |
|------|------|
| `i2c1_master_init(cfg)` | GPIO 重映射 + 时序寄存器 + PE |
| `i2c1_master_bus_recover()` | 9 脉冲 + STOP，释放卡死总线 |
| `i2c1_master_write_frame(addr7, ctrl, payload, len)` | START→地址(W)→ctrl→数据→STOP |

### `write_frame` 返回值

| 状态 | 含义 |
|------|------|
| `STM_OK` | 整帧成功 |
| `STM_ERR_BUSY` | 起始前总线一直 BUSY |
| `STM_ERR_NACK` | 从机无应答 |
| `STM_ERR_TIMEOUT` | 事件位等待超时 |

错误路径均发 STOP，避免 SDA 被拉低挂死。

## 实现说明

### 为什么在 HAL 层而不是 drivers

I2C 是 **总线能力**，SSD1306、MPU6050 等都可复用。SSD1306 驱动只关心 `0x00/0x40` 控制字节，通过函数指针注入 `bus_write`。

### 标准模式时序

由 `pclk1_hz` 和 `bus_hz` 算 `CR2.FREQ`、`CCR`、`TRISE`。当前实现面向 **100kHz 标准模式**；400kHz 需另配 Fast mode 位。

### 总线恢复

OLED 复位或热插拔可能导致 SDA 低电平锁死。`bus_recover` 切 GPIO 模式对 SCL 打 9 脉冲，再发 STOP，然后可 `ssd1306_init` 重试。

## 使用示例

```c
const i2c1_master_config_t cfg = {
  .pclk1_hz = bsp_clock_get_pclk1_hz(),
  .bus_hz = 100000UL,
  .timeout_iter = 0U,
};
i2c1_master_init(&cfg);

uint8_t cmd[] = {0xAF};  /* display on */
i2c1_master_write_frame(0x3CU, 0x00U, cmd, 1U);
```

SSD1306 默认在 `ssd1306_default_init` 内部完成 I2C init。

协议与 SSD1306 帧格式见 [topics/I2C.md](../topics/I2C.md)。HAL 层总览见 [hal/README.md](../hal/README.md)。

## 常见坑

- `pclk1_hz` 传错 → 波特率偏离，NACK 或偶发失败
- 无上拉 → 波形不起振
- 长 payload 阻塞主循环 → OLED 全刷时配合任务节流
- 多主机：本 HAL 仅主机写，不支持仲裁读

---

# English

# I2C1 Master HAL (`i2c1_master`)

> **Layering**: This module belongs to the [HAL layer](../hal/README.md), not `src/drivers/`. Documentation is placed here for proximity to SSD1306.

## Purpose

Provides **I2C1 master** blocking write-frame transactions for slave devices such as SSD1306. Located in `src/hal/`, between BSP clock/pin setup and device-specific drivers.

## Hardware

| Signal | Pin | Description |
|------|------|------|
| SCL | PB8 | Alternate-function open-drain (AFIO remap for I2C1) |
| SDA | PB9 | Alternate-function open-drain |

External pull-up resistors required (typically 4.7kΩ~10kΩ to 3.3V).

## Configuration Structure

```c
typedef struct {
  uint32_t pclk1_hz;      /* bsp_clock_get_pclk1_hz(), ≥ 2MHz */
  uint32_t bus_hz;        /* target SCL, e.g. 100000 */
  uint32_t timeout_iter;  /* poll timeout loop count, 0=default 3000 */
} i2c1_master_config_t;
```

## API Reference

| Function | Description |
|------|------|
| `i2c1_master_init(cfg)` | GPIO remap + timing registers + PE |
| `i2c1_master_bus_recover()` | 9 pulses + STOP to release stuck bus |
| `i2c1_master_write_frame(addr7, ctrl, payload, len)` | START→address(W)→ctrl→data→STOP |

### `write_frame` Return Values

| Status | Meaning |
|------|------|
| `STM_OK` | Full frame success |
| `STM_ERR_BUSY` | Bus remained BUSY before START |
| `STM_ERR_NACK` | No slave acknowledge |
| `STM_ERR_TIMEOUT` | Event flag wait timeout |

Error paths always send STOP to avoid SDA stuck low.

## Implementation Notes

### Why HAL Layer, Not drivers

I2C is a **bus capability** reusable by SSD1306, MPU6050, etc. SSD1306 driver only cares about `0x00/0x40` control bytes, injected via function pointer as `bus_write`.

### Standard-Mode Timing

`CR2.FREQ`, `CCR`, and `TRISE` derived from `pclk1_hz` and `bus_hz`. Current implementation targets **100 kHz standard mode**; 400 kHz requires separate Fast mode configuration.

### Bus Recovery

OLED reset or hot-plug may lock SDA low. `bus_recover` switches to GPIO mode, clocks 9 pulses on SCL, sends STOP, then `ssd1306_init` can retry.

## Usage Example

```c
const i2c1_master_config_t cfg = {
  .pclk1_hz = bsp_clock_get_pclk1_hz(),
  .bus_hz = 100000UL,
  .timeout_iter = 0U,
};
i2c1_master_init(&cfg);

uint8_t cmd[] = {0xAF};  /* display on */
i2c1_master_write_frame(0x3CU, 0x00U, cmd, 1U);
```

SSD1306 default init performs I2C init internally in `ssd1306_default_init`.

Protocol and SSD1306 frame format: [topics/I2C.md](../topics/I2C.md). HAL overview: [hal/README.md](../hal/README.md).

## Common Pitfalls

- Wrong `pclk1_hz` → baud rate off, NACK or intermittent failure
- No pull-ups → waveform does not rise
- Long payload blocks main loop → throttle tasks during full OLED refresh
- Multi-master: this HAL is master-write only, no arbitration or read support
