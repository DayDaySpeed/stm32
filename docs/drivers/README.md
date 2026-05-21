# 驱动模块文档

本目录为 `src/drivers/` 与 `src/hal/i2c1_master.c` 的专题说明。每篇文档包含：驱动作用、API 与参数、实现思路、使用示例与常见坑。

更深入的寄存器/原理说明见根目录专题（如 [pwm.md](../../pwm.md)、[adc.md](../../adc.md)）。

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

1. [驱动接口约定](../DRIVER_API_GUIDE.md)
2. [编码规范](../CODING_STYLE.md)
3. 按外设选读上表对应文档
4. 应用层用法见 `bsp/board_devices.h` 与 `src/app/app.c`
