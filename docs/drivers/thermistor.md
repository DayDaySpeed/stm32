# 热敏电阻 NTC 驱动（`thermistor`）

## 作用

读取 **10k NTC（B3950）** 模块的 ADC 值，换算为电阻与 **温度（0.1°C 整数）**。共享 ADC1 IN2（PA2）。

## 硬件

- 常见模块：3.3V 供电，AO → **PA2（ADC1_IN2）**
- 默认分压：**上拉 10k 固定电阻 + 下拉 NTC**（`FIXED_UP_NTC_DOWN`）

## 配置结构体

```c
typedef struct {
  thermistor_adc_clock_source_t clock_source;
  thermistor_adc_prescaler_t adc_prescaler;
  thermistor_divider_topology_t divider_topology;
  uint32_t fixed_resistor_ohms;   /* 默认 10000 */
  uint32_t vdda_mv;               /* 默认 3300 */
} thermistor_config_t;
```

`divider_topology`：

- `FIXED_UP_NTC_DOWN`：温度↑ → AO 电压通常↓
- `NTC_UP_FIXED_DOWN`：温度↑ → AO 电压通常↑

## API 参考

| 函数 | 说明 |
|------|------|
| `thermistor_init_with_config(config)` | 保存分压参数 + init 共享 ADC |
| `thermistor_init()` | 默认 10k/B3950/3.3V |
| `thermistor_read_raw_blocking` / `_average_blocking` | ADC 原始值 |
| `thermistor_read_millivolts_blocking` | 输入电压 mV |
| `thermistor_read_resistance_ohms_blocking` | 反算 NTC 电阻 |
| `thermistor_read_temperature_celsius_x10_blocking` | 采样 + 查表温度 |
| `thermistor_read_temperature_from_raw_blocking(raw, out)` | **不再触发 ADC**，由已有 raw 换算 |

返回值：`STM_ERR_INVALID_ARG` 当 raw 在分压边界（0 或 4095）无法算电阻。

## 实现说明

### 温度怎么算

1. raw → 分压公式 → NTC 电阻（Ω）
2. 内置 `g_ntc_10k_b3950_table[]`（-40°C~125°C 稀疏表）
3. 电阻落在两表项之间 → **线性插值** → `celsius_x10`

### 为什么提供 `from_raw`

`bsp_sensor_led_update` 与 OLED 任务都会读光敏+热敏。一次 `read_pair_average` 后，温度应用 `thermistor_read_temperature_from_raw_blocking(ntc_raw, &t)`，避免第二次 SCAN。

### 为什么用查表而非 Steinhart-Hart

裸机无 `log()`，查表 + 插值 **确定性、无浮点、代码小**。精度对指示 LED/OLED 足够。

## 使用示例

```c
thermistor_init();
int16_t temp_x10;
thermistor_read_temperature_celsius_x10_blocking(&temp_x10);
/* 253 → 25.3°C */

uint16_t ntc_raw;
bsp_analog_sensors_read_pair_average(&ldr, &ntc_raw, 2U);
bsp_temperature_read_celsius_x10_from_raw(ntc_raw, &temp_x10);
```

## 常见坑

- 分压拓扑与模块不符 → 温度趋势反了，改 `divider_topology`
- raw 贴 0 或 4095 → 电阻计算除零，返回错误
- 表外极端温度 → 钳在表端点值
