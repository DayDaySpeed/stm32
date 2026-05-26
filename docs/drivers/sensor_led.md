# 传感器指示 LED 驱动（`sensor_led`）

## 作用

用 **TIM1 双通道 PWM** 驱动两颗指示 LED：

- **PA8 / TIM1_CH1**：光敏越亮 → LED 越亮
- **PA11 / TIM1_CH4**：NTC 温度越高 → LED 越亮

## 硬件

引脚 → 限流电阻（330Ω~1kΩ）→ LED+ → LED- → GND

TIM1 是 **高级定时器**，须设 `BDTR.MOE=1` 才输出到引脚。

## 配置结构体

```c
typedef struct {
  uint32_t pwm_hz;  /* 默认 1000，见 BOARD_SENSOR_LED_PWM_HZ */
} sensor_led_config_t;
```

## API 参考

| 函数 | 说明 |
|------|------|
| `sensor_led_init_with_config(config)` | GPIO + TIM1 双 PWM |
| `sensor_led_init()` | 1kHz 默认 |
| `sensor_led_set_ldr_permille(duty)` | CH1 占空 0~1000 |
| `sensor_led_set_ntc_permille(duty)` | CH4 占空 0~1000 |

## 实现说明

### 时钟源

TIM1 在 APB2，使用 `bsp_clock_get_apb2_timer_hz()`（含 F1 定时器 ×2 规则）。此前直接用 `pclk2` 在 HSI 8MHz 全 /1 时正确；统一 helper 后换时钟方案更安全。

### 双通道同一 ARR

LDR 与 NTC 共用 PSC/ARR，频率一致；运行时只改 CCR1/CCR4，互不影响。

### 与 BSP 的配合

`bsp_sensor_led_update_from_sensors()`：

1. `read_pair_average` 得 ldr/ntc raw
2. ldr → 线性映射 0~1000 permille
3. ntc raw → `thermistor_read_temperature_from_raw` → 按 `BOARD_NTC_LED_FULL_TEMP_X10` 映射

**为什么映射在 BSP 不在驱动**：驱动只管 PWM 输出；传感器语义属于板级策略。

## 使用示例

```c
sensor_led_init();
sensor_led_set_ldr_permille(500U);
sensor_led_set_ntc_permille(800U);
```

## 常见坑

- 忘记 MOE → 寄存器有波形但引脚无输出
- duty > 1000 → 参数错误
- LED 极性接反 → 占空越高越暗（硬件问题）

---

# English

# Sensor Indicator LED Driver (`sensor_led`)

## Purpose

Drives two indicator LEDs with **TIM1 dual-channel PWM**:

- **PA8 / TIM1_CH1**: brighter ambient light → brighter LED
- **PA11 / TIM1_CH4**: higher NTC temperature → brighter LED

## Hardware

Pin → current-limit resistor (330Ω~1kΩ) → LED+ → LED- → GND

TIM1 is an **advanced timer**; `BDTR.MOE=1` is required for pin output.

## Configuration Structure

```c
typedef struct {
  uint32_t pwm_hz;  /* default 1000, see BOARD_SENSOR_LED_PWM_HZ */
} sensor_led_config_t;
```

## API Reference

| Function | Description |
|------|------|
| `sensor_led_init_with_config(config)` | GPIO + TIM1 dual PWM |
| `sensor_led_init()` | 1 kHz default |
| `sensor_led_set_ldr_permille(duty)` | CH1 duty 0~1000 |
| `sensor_led_set_ntc_permille(duty)` | CH4 duty 0~1000 |

## Implementation Notes

### Clock Source

TIM1 is on APB2; uses `bsp_clock_get_apb2_timer_hz()` (includes F1 timer ×2 rule). Previously using `pclk2` directly was correct at HSI 8 MHz full /1; unified helper is safer when clock scheme changes.

### Dual Channel, Same ARR

LDR and NTC share PSC/ARR for consistent frequency; runtime updates only CCR1/CCR4, independent of each other.

### BSP Integration

`bsp_sensor_led_update_from_sensors()`:

1. `read_pair_average` for ldr/ntc raw
2. ldr → linear map to 0~1000 permille
3. ntc raw → `thermistor_read_temperature_from_raw` → map by `BOARD_NTC_LED_FULL_TEMP_X10`

**Why mapping is in BSP, not driver**: driver handles PWM output only; sensor semantics belong to board-level policy.

## Usage Example

```c
sensor_led_init();
sensor_led_set_ldr_permille(500U);
sensor_led_set_ntc_permille(800U);
```

## Common Pitfalls

- Forgetting MOE → registers show waveform but no pin output
- duty > 1000 → parameter error
- LED polarity reversed → higher duty appears dimmer (hardware issue)
