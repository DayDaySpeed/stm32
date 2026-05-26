# 反射红外驱动（`ir_reflect`）

## 作用

读取 **TCRT5000 类反射红外** 模块的 ADC 原始值/电压。共享 ADC1 IN3（PA3）。靠近/远离判定逻辑在 **应用层**（`app.c` + `board_config.h`）。

## 硬件

- 模块 AO → **PA3（ADC1_IN3）**
- 本板实测：远离 ~4000，靠近 ~100（raw **越低越近**）

## 配置结构体

```c
typedef struct {
  ir_reflect_adc_clock_source_t clock_source;
  ir_reflect_adc_prescaler_t adc_prescaler;
} ir_reflect_config_t;
```

## API 参考

| 函数 | 说明 |
|------|------|
| `ir_reflect_init_with_config(config)` | init 共享 ADC |
| `ir_reflect_init()` | 默认 AUTO |
| `ir_reflect_read_raw_blocking(out)` | 单次原始值 |
| `ir_reflect_read_raw_average_blocking(out, n)` | n 次平均 |
| `ir_reflect_read_millivolts_blocking(out)` | mV 估算 |

## 实现说明

### 为什么读 raw 用 `read_all` 而光敏用 `read_pair`

红外在 SCAN 序列第 3 槽；`read_all_blocking` 最直接。光敏/热敏薄封装历史上用 pair API，功能等价（都会扫完三路）。

### 应用层去抖

驱动只提供原始 ADC；`app_ir_proximity_buzzer_task` 实现：

- 上电须先见到「远离」才 **武装**
- 连续 N 次低于 `BOARD_IR_NEAR_RAW_LOW` 才判靠近
- 蜂鸣冷却时间 `BOARD_IR_BEEP_COOLDOWN_MS`

阈值集中在 `board_config.h`，改板子只需改宏。

## 使用示例

```c
ir_reflect_init();
uint16_t ir;
ir_reflect_read_raw_average_blocking(&ir, 4U);
```

或一次读三路：

```c
uint16_t s[ADC1_DUAL_SLOT_COUNT];
bsp_analog_sensors_read_all_average(s, 4U);
uint16_t ir = s[ADC1_DUAL_SLOT_IR_REFLECT];
```

## 常见坑

- 环境光/模块高度影响 raw，阈值需实测标定
- OLED 与 IR 任务若各读一次 ADC → 双倍 SCAN 负载；合并为 `read_all_average`
- 有源蜂鸣器阻塞 `beep_blocking` 会占主循环 80ms

---

# English

# Reflective IR Driver (`ir_reflect`)

## Purpose

Reads **TCRT5000-class reflective IR** module ADC raw value/voltage. Shares ADC1 IN3 (PA3). Near/far detection logic is in the **application layer** (`app.c` + `board_config.h`).

## Hardware

- Module AO → **PA3 (ADC1_IN3)**
- Measured on this board: far ~4000, near ~100 (lower raw = closer)

## Configuration Structure

```c
typedef struct {
  ir_reflect_adc_clock_source_t clock_source;
  ir_reflect_adc_prescaler_t adc_prescaler;
} ir_reflect_config_t;
```

## API Reference

| Function | Description |
|------|------|
| `ir_reflect_init_with_config(config)` | Init shared ADC |
| `ir_reflect_init()` | Default AUTO |
| `ir_reflect_read_raw_blocking(out)` | Single raw value |
| `ir_reflect_read_raw_average_blocking(out, n)` | Mean of n samples |
| `ir_reflect_read_millivolts_blocking(out)` | mV estimate |

## Implementation Notes

### Why Raw Read Uses `read_all` While Photoresistor Uses `read_pair`

IR is slot 3 in the SCAN sequence; `read_all_blocking` is most direct. Photoresistor/thermistor thin wrappers historically use the pair API; functionally equivalent (all three channels are scanned).

### Application-Layer Debouncing

Driver provides raw ADC only; `app_ir_proximity_buzzer_task` implements:

- Must observe "far" after power-up before **arming**
- N consecutive samples below `BOARD_IR_NEAR_RAW_LOW` before declaring near
- Beep cooldown `BOARD_IR_BEEP_COOLDOWN_MS`

Thresholds centralized in `board_config.h`; board changes require macro updates only.

## Usage Example

```c
ir_reflect_init();
uint16_t ir;
ir_reflect_read_raw_average_blocking(&ir, 4U);
```

Or read all three channels at once:

```c
uint16_t s[ADC1_DUAL_SLOT_COUNT];
bsp_analog_sensors_read_all_average(s, 4U);
uint16_t ir = s[ADC1_DUAL_SLOT_IR_REFLECT];
```

## Common Pitfalls

- Ambient light and module height affect raw values; thresholds need field calibration
- OLED and IR tasks each reading ADC separately → double SCAN load; merge into `read_all_average`
- Active buzzer blocking `beep_blocking` holds main loop for 80 ms
