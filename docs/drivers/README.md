# 驱动模块文档

本目录为 `src/drivers/` 的专题说明。HAL 层（I2C）见 [../hal/README.md](../hal/README.md)。每篇文档包含：驱动作用、API 与参数、实现思路、使用示例与常见坑。

## 原理专题（根目录，API 文档互补）

| 原理 | 根目录专题 | 本目录 API 文档 |
|------|------------|-----------------|
| 时钟 / RCC | [clock.md](../../clock.md) | [../bsp/clock.md](../bsp/clock.md) |
| I2C / SSD1306 | [I2C.md](../../I2C.md) | [i2c1_master.md](./i2c1_master.md)、[ssd1306_oled.md](./ssd1306_oled.md) |
| USART / NVIC | [USART.md](../../USART.md)、[NVIC.md](../../NVIC.md) | [usart1.md](./usart1.md) |
| PWM | [pwm.md](../../pwm.md) | [pwm.md](./pwm.md)、[dc_motor.md](./dc_motor.md)、[sensor_led.md](./sensor_led.md) |
| 编码器 | [encoder.md](../../encoder.md) | [encoder.md](./encoder.md) |
| ADC | [adc.md](../../adc.md) | [adc1_dual_scan_dma.md](./adc1_dual_scan_dma.md) 及传感器薄封装 |

## 文档列表

| 模块 | 文档 | 源文件 |
|------|------|--------|
| SysTick 毫秒节拍 | [systick.md](./systick.md) | `systick.c` |
| TIM2 状态灯 PWM | [pwm.md](./pwm.md) | `pwm.c` |
| TIM3 正交编码器 | [encoder.md](./encoder.md) | `encoder.c` |
| TB6612 直流电机 | [dc_motor.md](./dc_motor.md) | `dc_motor.c` |
| ADC1 三路 SCAN+DMA | [adc1_dual_scan_dma.md](./adc1_dual_scan_dma.md) | `adc1_dual_scan_dma.c` |
| 光敏电阻 | [photoresistor.md](./photoresistor.md) | `photoresistor.c` |
| 热敏电阻 NTC | [thermistor.md](./thermistor.md) | `thermistor.c` |
| 反射红外 | [ir_reflect.md](./ir_reflect.md) | `ir_reflect.c` |
| 有源蜂鸣器 | [buzzer.md](./buzzer.md) | `buzzer.c` |
| 传感器指示 LED | [sensor_led.md](./sensor_led.md) | `sensor_led.c` |
| USART1 控制台 | [usart1.md](./usart1.md) | `usart1.c` |
| SSD1306 OLED | [ssd1306_oled.md](./ssd1306_oled.md) | `ssd1306_oled.c` + `oled_font5x7.c` |
| I2C1 主机 HAL | [i2c1_master.md](./i2c1_master.md) | `i2c1_master.c` |

## 阅读顺序

1. [BSP 层](../bsp/README.md) 与 [驱动接口约定](../DRIVER_API_GUIDE.md)
2. [编码规范](../CODING_STYLE.md)
3. 按外设选读上表对应文档
4. 应用层用法见 [board_devices](../bsp/board_devices.md) 与 `src/app/app.c`
