# 文档索引

> 各文档均为**中文在前**、以 `---` 分隔的 **English** 章节在后。`MIGRATION_GUIDE.md` 为中文概述 + 英文详情。

## 工程规范

- [编码规范](./CODING_STYLE.md)
- [驱动接口约定](./DRIVER_API_GUIDE.md)
- [迁移指南](./MIGRATION_GUIDE.md)

## 分层文档

| 层 | 入口 | 说明 |
|----|------|------|
| BSP | [bsp/README.md](./bsp/README.md) | 板级引脚、时钟门控、逻辑设备 |
| HAL | [hal/README.md](./hal/README.md) | I2C 等总线事务 |
| drivers | [drivers/README.md](./drivers/README.md) | 外设驱动 API 与实现 |
| topics | [topics/README.md](./topics/README.md) | 寄存器原理专题 |

## 模块专题（寄存器原理）

长文原理文档位于 [`topics/`](./topics/README.md)，与 `drivers/` API 文档互补。

| 主题 | 原理文档 | 对应 API / BSP |
|------|----------|----------------|
| 时钟树 / RCC | [topics/clock.md](./topics/clock.md) | [bsp/clock.md](./bsp/clock.md) |
| I2C / SSD1306 | [topics/I2C.md](./topics/I2C.md) | [drivers/i2c1_master.md](./drivers/i2c1_master.md)、[ssd1306_oled.md](./drivers/ssd1306_oled.md) |
| USART / NVIC | [topics/USART.md](./topics/USART.md)、[topics/NVIC.md](./topics/NVIC.md) | [drivers/usart1.md](./drivers/usart1.md) |
| PWM | [topics/pwm.md](./topics/pwm.md) | [drivers/breathing_led.md](./drivers/breathing_led.md) 等 |
| 编码器 | [topics/encoder.md](./topics/encoder.md) | [drivers/encoder.md](./drivers/encoder.md) |
| ADC | [topics/adc.md](./topics/adc.md) | [drivers/adc1_dual_scan_dma.md](./drivers/adc1_dual_scan_dma.md) |
| GCC 扩展 | [topics/gcc_CompilerExtensions.md](./topics/gcc_CompilerExtensions.md) | — |

## 阅读顺序建议

1. [编码规范](./CODING_STYLE.md)
2. [驱动接口约定](./DRIVER_API_GUIDE.md)
3. [BSP 层](./bsp/README.md) → [board_devices](./bsp/board_devices.md)
4. 按需阅读 [drivers/](./drivers/README.md) 或 [topics/](./topics/README.md)
5. 对照 `src/app/app.c` 看实际用法

---

# English

# Documentation Index

> All documents use **Chinese first**, then `---`, then an **English** section. `MIGRATION_GUIDE.md` uses Chinese overview + English details.

## Project Conventions

- [Coding Style](./CODING_STYLE.md)
- [Driver Interface Conventions](./DRIVER_API_GUIDE.md)
- [Migration Guide](./MIGRATION_GUIDE.md)

## Layered Documentation

| Layer | Entry | Description |
|----|------|------|
| BSP | [bsp/README.md](./bsp/README.md) | Board pins, clock gating, logical devices |
| HAL | [hal/README.md](./hal/README.md) | I2C and other bus transactions |
| drivers | [drivers/README.md](./drivers/README.md) | Peripheral driver APIs and implementations |
| topics | [topics/README.md](./topics/README.md) | Register-level topic guides |

## Module Topics (Register-Level Theory)

In-depth theory documents live under [`topics/`](./topics/README.md), complementing the `drivers/` API documentation.

| Topic | Theory Document | Related API / BSP |
|------|----------|----------------|
| Clock tree / RCC | [topics/clock.md](./topics/clock.md) | [bsp/clock.md](./bsp/clock.md) |
| I2C / SSD1306 | [topics/I2C.md](./topics/I2C.md) | [drivers/i2c1_master.md](./drivers/i2c1_master.md), [ssd1306_oled.md](./drivers/ssd1306_oled.md) |
| USART / NVIC | [topics/USART.md](./topics/USART.md), [topics/NVIC.md](./topics/NVIC.md) | [drivers/usart1.md](./drivers/usart1.md) |
| PWM | [topics/pwm.md](./topics/pwm.md) | [drivers/breathing_led.md](./drivers/breathing_led.md), etc. |
| Encoder | [topics/encoder.md](./topics/encoder.md) | [drivers/encoder.md](./drivers/encoder.md) |
| ADC | [topics/adc.md](./topics/adc.md) | [drivers/adc1_dual_scan_dma.md](./drivers/adc1_dual_scan_dma.md) |
| GCC extensions | [topics/gcc_CompilerExtensions.md](./topics/gcc_CompilerExtensions.md) | — |

## Suggested Reading Order

1. [Coding Style](./CODING_STYLE.md)
2. [Driver Interface Conventions](./DRIVER_API_GUIDE.md)
3. [BSP layer](./bsp/README.md) → [board_devices](./bsp/board_devices.md)
4. Read [drivers/](./drivers/README.md) or [topics/](./topics/README.md) as needed
5. Cross-reference `src/app/app.c` for practical usage
