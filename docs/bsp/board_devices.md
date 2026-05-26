# board_devices — 板级逻辑设备

`bsp/board_devices.h` 是 **应用层访问硬件的唯一推荐入口**。把 `USART1`、`TIM2_CH1`、`ADC1_IN1` 等具体绑定收敛成语义化名字：`console`、`display`、`wheel_encoder` 等。

实现文件：`src/bsp/board_devices.c`（内部持有各驱动的 `*_config_t` 静态默认值）。

---

## 设计目标

1. **app 不碰引脚/外设实例名**，方便换板或换定时器通道。
2. **默认参数集中**在 `board_devices.c` 的 `g_board_*_config` 结构体；用户向的阈值在 `board_config.h`。
3. **统一 `stm_status_t`**，与 [驱动接口约定](../DRIVER_API_GUIDE.md) 一致。

---

## 默认初始化：`bsp_default_devices_init()`

`app_init()` 调用此函数一次性 init 本板全部外设。顺序如下：

| 步骤 | 函数 | 底层驱动 |
|------|------|----------|
| 1 | `bsp_dc_motor_init()` | `dc_motor`（TIM4 10kHz，速度 0） |
| 2 | `bsp_console_init()` | `usart1` 115200 + 绑定 `stm_log` |
| 3 | `bsp_console_enable_rx_interrupt()` | USART1 RXNE 中断 |
| 4 | `bsp_display_init()` | `ssd1306_default_init()`（含 I2C） |
| 5 | `bsp_status_led_init()` | `breathing_led` 1kHz，占空 0 |
| 6 | `bsp_wheel_encoder_init()` | `tim3_encoder` + `BOARD_WHEEL_ENCODER_DIRECTION` |
| 7 | `bsp_analog_sensors_init()` | ADC 三路 + 光敏/热敏/红外薄封装 |
| 8 | `bsp_buzzer_init()` | `buzzer` + `BOARD_BUZZER_ACTIVE_HIGH` |
| 9 | `bsp_sensor_led_init()` | `sensor_led` TIM1 双 PWM |

任一步失败则返回对应 `stm_status_t`，`main` 会 `stm_fault_halt`。

**注意**：`bsp_dc_motor_gpio_safe_early()` 不在此函数内，须在 `bsp_board_init()` 之后、`app_init()` 之前由 `main` 单独调用。

---

## API 分组参考

### 控制台 `bsp_console_*`

| API | 说明 |
|-----|------|
| `bsp_console_init()` | USART1 115200，16 倍过采样，CR/LF 均可作行结束 |
| `bsp_console_enable_rx_interrupt()` | 开 RX 中断（init 后单独调，便于分步调试） |
| `bsp_console_write_string_blocking(text)` | 阻塞发字符串 |
| `bsp_console_read_line_try(out, size)` | 非阻塞读一行；`STM_ERR_BUSY` 无数据 |
| `bsp_console_irq_handler()` | 在 `USART1_IRQHandler` 调用 |

### 显示 `bsp_display_*`

| API | 说明 |
|-----|------|
| `bsp_display_init()` | SSD1306 + I2C |
| `bsp_display_recover()` | I2C 总线恢复 + 重 init（OLED 卡死） |
| `bsp_display_clear()` | 清屏 |
| `bsp_display_write_text_atf(page, col, fmt, ...)` | 定点格式化输出 |

### 状态灯 `bsp_status_led_*`

| API | 说明 |
|-----|------|
| `bsp_status_led_init()` | `breathing_led` 1kHz |
| `bsp_status_led_set_duty_permille(duty)` | 0~1000，呼吸灯任务调用 |

### 编码器 `bsp_wheel_encoder_*`

| API | 说明 |
|-----|------|
| `bsp_wheel_encoder_init()` | TIM3 编码器模式 |
| `bsp_wheel_encoder_read_count(out)` | 16 位带符号累计值 |
| `bsp_wheel_encoder_read_direction(out)` | 0=向上计数，1=向下 |

### 直流电机 `bsp_dc_motor_*`

| API | 说明 |
|-----|------|
| `bsp_dc_motor_gpio_safe_early()` | 上电最早安全 GPIO（见 main） |
| `bsp_dc_motor_init()` | TIM4 PWM + TB6612 方向脚 |
| `bsp_dc_motor_set_speed_signed(speed)` | -1000~+1000；受 `BOARD_MOTOR_REVERSE_SIGN` 影响 |
| `bsp_dc_motor_get_speed_signed(out)` | 读缓存速度（含符号反转） |
| `bsp_dc_motor_set_speed_permille(duty)` | 仅正转 0~1000 |
| `bsp_dc_motor_get_speed_permille(out)` | 仅当速度≥0 |
| `bsp_dc_motor_stop()` | 速度归零 |

BSP 层在 set/get 时应用 `BOARD_MOTOR_REVERSE_SIGN`，驱动层保持物理 AIN1/AIN2 真值表不变。

### 模拟传感器 `bsp_analog_sensors_*` / 环境光 / 红外 / 温度

| API | 说明 |
|-----|------|
| `bsp_analog_sensors_init()` | ADC1 三路 SCAN+DMA + 三个薄驱动 init |
| `bsp_analog_sensors_read_pair_average(photo, therm, n)` | 一次 SCAN 平均，取光敏+热敏 |
| `bsp_analog_sensors_read_all_average(samples[3], n)` | 一次 SCAN 平均，取光敏+热敏+红外 |
| `bsp_ambient_light_init()` / `_read_raw_average()` | 仅光敏（通常用 pair/all 即可） |
| `bsp_ir_reflect_read_raw_average(raw, n)` | 仅红外槽位 |
| `bsp_temperature_read_celsius_x10_from_raw(raw, out)` | 由 NTC raw 算温度，**不再触发 ADC** |

**性能提示**：需要三路数据时用 `read_all_average`，避免 pair + ir 各读一次造成双倍 SCAN。

### 蜂鸣器 / 传感器 LED

| API | 说明 |
|-----|------|
| `bsp_buzzer_init()` / `bsp_buzzer_beep_blocking(ms)` | 有源蜂鸣器 |
| `bsp_sensor_led_init()` | TIM1 双通道指示 LED |
| `bsp_sensor_led_update_from_sensors()` | 读 LDR/NTC → 映射 PWM 占空比 |

`update_from_sensors` 内部策略：

- LDR raw 线性映射 0~1000 permille
- NTC raw → 温度 → 按 `BOARD_NTC_LED_FULL_TEMP_X10` 映射 NTC 灯亮度

---

## 逻辑名 → 驱动映射表

| BSP 逻辑名 | 驱动模块 | 硬件实例 |
|------------|----------|----------|
| console | usart1 | PA9/PA10 |
| display | ssd1306_oled + i2c1_master | PB8/PB9, 0x3C |
| status_led | breathing_led (TIM2_CH1) | PA0 |
| wheel_encoder | encoder (tim3) | PA6/PA7 |
| dc_motor | dc_motor | PB5/6/7, TIM4 |
| ambient_light / analog | photoresistor + adc1_dual | PA1 |
| temperature | thermistor + adc1_dual | PA2 |
| ir_reflect | ir_reflect + adc1_dual | PA3 |
| buzzer | buzzer | PA4 |
| sensor_led | sensor_led | PA8/PA11, TIM1 |

---

## 使用示例（app 层）

```c
#include "bsp/board_devices.h"

stm_status_t st = bsp_default_devices_init();
if (st != STM_OK) { /* 处理 */ }

uint16_t samples[ADC1_DUAL_SLOT_COUNT];
bsp_analog_sensors_read_all_average(samples, 4U);

bsp_status_led_set_duty_permille(500U);
bsp_dc_motor_set_speed_signed(300);

char line[64];
if (bsp_console_read_line_try(line, sizeof(line)) == STM_OK) {
  bsp_display_write_text_atf(0U, 0U, "%s", line);
}
```

完整任务调度见 `src/app/app.c`。

---

## 为什么这么分层

- **drivers** 可复用到其它 STM32 板，只要改 BSP 绑定。
- **app** 表达「做什么」（呼吸灯、旋钮调速），不关心「PA0 还是 PB0」。
- 板级策略（电机符号反转、红外阈值）放在 config 或 BSP，不散落在多个驱动里。

## 相关文档

- [board_config.md](./board_config.md) — 可调宏
- [drivers/usart1.md](../drivers/usart1.md) 等底层 API 细节

---

# English

# board_devices — Board-Level Logical Devices

`bsp/board_devices.h` is the **recommended sole entry point for application-layer hardware access**. It consolidates concrete bindings such as `USART1`, `TIM2_CH1`, and `ADC1_IN1` into semantic names: `console`, `display`, `wheel_encoder`, and so on.

Implementation file: `src/bsp/board_devices.c` (holds static default values for each driver's `*_config_t` internally).

---

## Design Goals

1. **Applications do not touch pin/peripheral instance names**, simplifying board retargeting or timer channel changes.
2. **Default parameters are centralized** in the `g_board_*_config` structures in `board_devices.c`; user-facing thresholds are in `board_config.h`.
3. **Unified `stm_status_t`**, consistent with [Driver Interface Conventions](../DRIVER_API_GUIDE.md).

---

## Default Initialization: `bsp_default_devices_init()`

`app_init()` calls this function to initialize all on-board peripherals in one pass. Order:

| Step | Function | Underlying Driver |
|------|----------|-------------------|
| 1 | `bsp_dc_motor_init()` | `dc_motor` (TIM4 10 kHz, speed 0) |
| 2 | `bsp_console_init()` | `usart1` 115200 + bind `stm_log` |
| 3 | `bsp_console_enable_rx_interrupt()` | USART1 RXNE interrupt |
| 4 | `bsp_display_init()` | `ssd1306_default_init()` (includes I2C) |
| 5 | `bsp_status_led_init()` | `breathing_led` 1 kHz, duty 0 |
| 6 | `bsp_wheel_encoder_init()` | `tim3_encoder` + `BOARD_WHEEL_ENCODER_DIRECTION` |
| 7 | `bsp_analog_sensors_init()` | ADC three channels + light/thermistor/IR thin wrappers |
| 8 | `bsp_buzzer_init()` | `buzzer` + `BOARD_BUZZER_ACTIVE_HIGH` |
| 9 | `bsp_sensor_led_init()` | `sensor_led` TIM1 dual PWM |

If any step fails, the corresponding `stm_status_t` is returned and `main` calls `stm_fault_halt`.

**Note**: `bsp_dc_motor_gpio_safe_early()` is not inside this function; `main` must call it separately after `bsp_board_init()` and before `app_init()`.

---

## API Group Reference

### Console `bsp_console_*`

| API | Description |
|-----|-------------|
| `bsp_console_init()` | USART1 115200, 16× oversampling, CR/LF both accepted as line terminators |
| `bsp_console_enable_rx_interrupt()` | Enable RX interrupt (called separately after init for stepwise debugging) |
| `bsp_console_write_string_blocking(text)` | Blocking string transmit |
| `bsp_console_read_line_try(out, size)` | Non-blocking line read; `STM_ERR_BUSY` when no data |
| `bsp_console_irq_handler()` | Call from `USART1_IRQHandler` |

### Display `bsp_display_*`

| API | Description |
|-----|-------------|
| `bsp_display_init()` | SSD1306 + I2C |
| `bsp_display_recover()` | I2C bus recovery + re-init (stuck OLED) |
| `bsp_display_clear()` | Clear screen |
| `bsp_display_write_text_atf(page, col, fmt, ...)` | Formatted output at fixed position |

### Status LED `bsp_status_led_*`

| API | Description |
|-----|-------------|
| `bsp_status_led_init()` | `breathing_led` 1 kHz |
| `bsp_status_led_set_duty_permille(duty)` | 0–1000; called by breathing-LED task |

### Encoder `bsp_wheel_encoder_*`

| API | Description |
|-----|-------------|
| `bsp_wheel_encoder_init()` | TIM3 encoder mode |
| `bsp_wheel_encoder_read_count(out)` | 16-bit signed accumulated count |
| `bsp_wheel_encoder_read_direction(out)` | 0 = counting up, 1 = counting down |

### DC Motor `bsp_dc_motor_*`

| API | Description |
|-----|-------------|
| `bsp_dc_motor_gpio_safe_early()` | Earliest safe GPIO at power-on (see main) |
| `bsp_dc_motor_init()` | TIM4 PWM + TB6612 direction pins |
| `bsp_dc_motor_set_speed_signed(speed)` | -1000 to +1000; affected by `BOARD_MOTOR_REVERSE_SIGN` |
| `bsp_dc_motor_get_speed_signed(out)` | Read cached speed (including sign inversion) |
| `bsp_dc_motor_set_speed_permille(duty)` | Forward only 0–1000 |
| `bsp_dc_motor_get_speed_permille(out)` | Only when speed ≥ 0 |
| `bsp_dc_motor_stop()` | Zero speed |

The BSP layer applies `BOARD_MOTOR_REVERSE_SIGN` on set/get; the driver layer keeps the physical AIN1/AIN2 truth table unchanged.

### Analog Sensors `bsp_analog_sensors_*` / Ambient Light / IR / Temperature

| API | Description |
|-----|-------------|
| `bsp_analog_sensors_init()` | ADC1 three-channel SCAN+DMA + three thin driver inits |
| `bsp_analog_sensors_read_pair_average(photo, therm, n)` | One SCAN average for light + thermistor |
| `bsp_analog_sensors_read_all_average(samples[3], n)` | One SCAN average for light + thermistor + IR |
| `bsp_ambient_light_init()` / `_read_raw_average()` | Light sensor only (pair/all usually sufficient) |
| `bsp_ir_reflect_read_raw_average(raw, n)` | IR slot only |
| `bsp_temperature_read_celsius_x10_from_raw(raw, out)` | Compute temperature from NTC raw; **does not trigger ADC** |

**Performance tip**: When all three channels are needed, use `read_all_average` to avoid double SCAN from pair + ir separately.

### Buzzer / Sensor LED

| API | Description |
|-----|-------------|
| `bsp_buzzer_init()` / `bsp_buzzer_beep_blocking(ms)` | Active buzzer |
| `bsp_sensor_led_init()` | TIM1 dual-channel indicator LEDs |
| `bsp_sensor_led_update_from_sensors()` | Read LDR/NTC → map PWM duty cycle |

`update_from_sensors` internal strategy:

- LDR raw linearly mapped 0–1000 permille
- NTC raw → temperature → map NTC LED brightness per `BOARD_NTC_LED_FULL_TEMP_X10`

---

## Logical Name → Driver Mapping Table

| BSP Logical Name | Driver Module | Hardware Instance |
|------------------|---------------|-------------------|
| console | usart1 | PA9/PA10 |
| display | ssd1306_oled + i2c1_master | PB8/PB9, 0x3C |
| status_led | breathing_led (TIM2_CH1) | PA0 |
| wheel_encoder | encoder (tim3) | PA6/PA7 |
| dc_motor | dc_motor | PB5/6/7, TIM4 |
| ambient_light / analog | photoresistor + adc1_dual | PA1 |
| temperature | thermistor + adc1_dual | PA2 |
| ir_reflect | ir_reflect + adc1_dual | PA3 |
| buzzer | buzzer | PA4 |
| sensor_led | sensor_led | PA8/PA11, TIM1 |

---

## Usage Example (Application Layer)

```c
#include "bsp/board_devices.h"

stm_status_t st = bsp_default_devices_init();
if (st != STM_OK) { /* handle */ }

uint16_t samples[ADC1_DUAL_SLOT_COUNT];
bsp_analog_sensors_read_all_average(samples, 4U);

bsp_status_led_set_duty_permille(500U);
bsp_dc_motor_set_speed_signed(300);

char line[64];
if (bsp_console_read_line_try(line, sizeof(line)) == STM_OK) {
  bsp_display_write_text_atf(0U, 0U, "%s", line);
}
```

See `src/app/app.c` for full task scheduling.

---

## Why This Layering

- **drivers** can be reused on other STM32 boards by changing BSP bindings only.
- **app** expresses *what to do* (breathing LED, knob speed control), not *PA0 vs PB0*.
- Board-level policy (motor sign inversion, IR thresholds) lives in config or BSP, not scattered across drivers.

## Related Documentation

- [board_config.md](./board_config.md) — tunable macros
- [drivers/usart1.md](../drivers/usart1.md) and other low-level API details
