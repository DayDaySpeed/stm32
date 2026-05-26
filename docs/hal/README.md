# HAL 层文档

HAL（Hardware Abstraction Layer）在本工程中指 **总线级、与具体从设备无关** 的薄封装。当前仅实现 **I2C1 主机写帧**。

```
drivers/ssd1306_oled.c
    ↓  bus_write 回调
hal/i2c1_master.c
    ↓  GPIO + I2C1 寄存器
bsp/board_pins.h + bsp/clock.h + bsp/board_init()
```

---

## 与 drivers / bsp 的区别

| 层 | 抽象级别 | 本工程示例 |
|----|----------|------------|
| **bsp** | 本板引脚 + 默认绑定 | PA9=USART1，`bsp_console_*` |
| **hal** | 片上总线能力 | I2C START/地址/数据/STOP |
| **drivers** | 具体芯片协议 | SSD1306 命令序列、帧缓冲 |

新增 I2C 从设备（如 MPU6050）时：复用 `i2c1_master_write_frame`，新建 `drivers/mpu6050.c`，**不必**改 HAL。

---

## 模块列表

| 模块 | 文档 | 源文件 |
|------|------|--------|
| I2C1 主机 | [../drivers/i2c1_master.md](../drivers/i2c1_master.md) | `src/hal/i2c1_master.c` |

> I2C1 文档放在 `docs/drivers/` 是为与 SSD1306 文档相邻；逻辑上仍属 HAL 层。

---

## I2C1 要点摘要

- **引脚**：PB8 SCL / PB9 SDA（I2C1 重映射，开漏 + 外部上拉）
- **init 参数**：`pclk1_hz`（`bsp_clock_get_pclk1_hz()`）、`bus_hz`（通常 100000）
- **核心 API**：`i2c1_master_write_frame(addr7, ctrl, payload, len)`
- **恢复**：`i2c1_master_bus_recover()` — 9 脉冲释放卡死 SDA

SSD1306 使用方式：

- `ctrl=0x00`：命令流  
- `ctrl=0x40`：GDDRAM 数据  

协议与波形细节见 [topics/I2C.md](../topics/I2C.md)。

---

## 初始化谁负责

| 步骤 | 调用方 |
|------|--------|
| 开 I2C1/GPIOB/AFIO 时钟 | `bsp_board_init()` |
| `i2c1_master_init(...)` | `ssd1306_default_init()` 内部 |
| 总线恢复 | `bsp_display_recover()` → `i2c1_master_bus_recover()` |

应用层 **不应** 直接 init I2C，除非新增不经过 SSD1306 的 I2C 设备。

---

## 扩展 HAL 的约定

若增加 SPI 或 I2C 读：

1. 源文件放 `src/hal/`，头文件放 `include/hal/`  
2. 在 `cmake/stm32_sources.cmake` 登记  
3. 依赖 `bsp/clock.h` 做时序计算，引脚用 `board_pins.h`  
4. 返回 `stm_status_t`，错误路径释放总线  
5. 在 `docs/hal/README.md` 与本目录交叉链接  

---

## 相关文档

- [topics/I2C.md](../topics/I2C.md)  
- [drivers/ssd1306_oled.md](../drivers/ssd1306_oled.md)  
- [bsp/board_devices.md](../bsp/board_devices.md) — `bsp_display_*`  
- [bsp/clock.md](../bsp/clock.md) — PCLK1 频率

---

# English

# HAL Layer Documentation

HAL (Hardware Abstraction Layer) in this project refers to **bus-level, slave-device-agnostic** thin wrappers. Currently only **I2C1 master write frames** are implemented.

```
drivers/ssd1306_oled.c
    ↓  bus_write callback
hal/i2c1_master.c
    ↓  GPIO + I2C1 registers
bsp/board_pins.h + bsp/clock.h + bsp/board_init()
```

---

## Difference from drivers / bsp

| Layer | Abstraction Level | Example in This Project |
|-------|-------------------|-------------------------|
| **bsp** | This board's pins + default bindings | PA9=USART1, `bsp_console_*` |
| **hal** | On-chip bus capability | I2C START/address/data/STOP |
| **drivers** | Specific chip protocol | SSD1306 command sequence, frame buffer |

When adding a new I2C slave (e.g. MPU6050): reuse `i2c1_master_write_frame`, add `drivers/mpu6050.c`, **no HAL changes required**.

---

## Module List

| Module | Documentation | Source File |
|--------|---------------|-------------|
| I2C1 master | [../drivers/i2c1_master.md](../drivers/i2c1_master.md) | `src/hal/i2c1_master.c` |

> I2C1 documentation lives under `docs/drivers/` to sit next to SSD1306 docs; logically it still belongs to the HAL layer.

---

## I2C1 Key Points

- **Pins**: PB8 SCL / PB9 SDA (I2C1 remapped, open-drain + external pull-ups)
- **Init parameters**: `pclk1_hz` (`bsp_clock_get_pclk1_hz()`), `bus_hz` (typically 100000)
- **Core API**: `i2c1_master_write_frame(addr7, ctrl, payload, len)`
- **Recovery**: `i2c1_master_bus_recover()` — 9 pulses to release stuck SDA

SSD1306 usage:

- `ctrl=0x00`: command stream  
- `ctrl=0x40`: GDDRAM data  

Protocol and waveform details in [topics/I2C.md](../topics/I2C.md).

---

## Who Initializes What

| Step | Caller |
|------|--------|
| Enable I2C1/GPIOB/AFIO clocks | `bsp_board_init()` |
| `i2c1_master_init(...)` | Inside `ssd1306_default_init()` |
| Bus recovery | `bsp_display_recover()` → `i2c1_master_bus_recover()` |

Application layer **should not** init I2C directly unless adding an I2C device that does not go through SSD1306.

---

## Conventions for Extending HAL

If adding SPI or I2C read:

1. Source under `src/hal/`, headers under `include/hal/`  
2. Register in `cmake/stm32_sources.cmake`  
3. Depend on `bsp/clock.h` for timing calculation; pins from `board_pins.h`  
4. Return `stm_status_t`; release bus on error paths  
5. Cross-link in `docs/hal/README.md` and this directory  

---

## Related Documentation

- [topics/I2C.md](../topics/I2C.md)  
- [drivers/ssd1306_oled.md](../drivers/ssd1306_oled.md)  
- [bsp/board_devices.md](../bsp/board_devices.md) — `bsp_display_*`  
- [bsp/clock.md](../bsp/clock.md) — PCLK1 frequency
