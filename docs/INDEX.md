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

## 模块专题（寄存器原理）

根目录长文，与 `docs/drivers/` API 文档互补，**不重复搬运正文**。

| 主题 | 文档 | 对应驱动文档 |
|------|------|--------------|
| 时钟树 / RCC | [clock.md](../clock.md) | [bsp/clock.md](./bsp/clock.md) |
| I2C / SSD1306 协议 | [I2C.md](../I2C.md) | [drivers/i2c1_master.md](./drivers/i2c1_master.md)、[ssd1306_oled.md](./drivers/ssd1306_oled.md) |
| USART / NVIC | [USART.md](../USART.md)、[NVIC.md](../NVIC.md) | [drivers/usart1.md](./drivers/usart1.md) |
| PWM 原理 | [pwm.md](../pwm.md) | [drivers/pwm.md](./drivers/pwm.md)、[dc_motor.md](./drivers/dc_motor.md)、[sensor_led.md](./drivers/sensor_led.md) |
| 编码器原理 | [encoder.md](../encoder.md) | [drivers/encoder.md](./drivers/encoder.md) |
| ADC SCAN+DMA | [adc.md](../adc.md) | [drivers/adc1_dual_scan_dma.md](./drivers/adc1_dual_scan_dma.md) |

## 阅读顺序建议

1. [编码规范](./CODING_STYLE.md)
2. [驱动接口约定](./DRIVER_API_GUIDE.md)
3. [BSP 层](./bsp/README.md) → [board_devices](./bsp/board_devices.md)
4. 按需阅读 [drivers/](./drivers/README.md) 或根目录原理专题
5. 对照 `src/app/app.c` 看实际用法
