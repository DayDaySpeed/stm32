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

协议与波形细节见根目录 [I2C.md](../../I2C.md)。

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

- [I2C 与 SSD1306 原理](../../I2C.md)  
- [drivers/ssd1306_oled.md](../drivers/ssd1306_oled.md)  
- [bsp/board_devices.md](../bsp/board_devices.md) — `bsp_display_*`  
- [bsp/clock.md](../bsp/clock.md) — PCLK1 频率
