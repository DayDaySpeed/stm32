# 文档索引

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
| PWM | [topics/pwm.md](./topics/pwm.md) | [drivers/pwm.md](./drivers/pwm.md) 等 |
| 编码器 | [topics/encoder.md](./topics/encoder.md) | [drivers/encoder.md](./drivers/encoder.md) |
| ADC | [topics/adc.md](./topics/adc.md) | [drivers/adc1_dual_scan_dma.md](./drivers/adc1_dual_scan_dma.md) |
| GCC 扩展 | [topics/gcc_CompilerExtensions.md](./topics/gcc_CompilerExtensions.md) | — |

## 阅读顺序建议

1. [编码规范](./CODING_STYLE.md)
2. [驱动接口约定](./DRIVER_API_GUIDE.md)
3. [BSP 层](./bsp/README.md) → [board_devices](./bsp/board_devices.md)
4. 按需阅读 [drivers/](./drivers/README.md) 或 [topics/](./topics/README.md)
5. 对照 `src/app/app.c` 看实际用法
