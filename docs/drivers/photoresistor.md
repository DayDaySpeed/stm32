# 光敏电阻驱动（`photoresistor`）

## 作用

读取 **光敏电阻分压模块** 的 ADC 原始值（0~4095）或估算电压（mV）。是对 `adc1_dual_scan_dma` 的 **薄封装**，只取 IN1（PA1）槽位。

## 硬件

- 模块 AO → **PA1（ADC1_IN1）**
- 与热敏、红外共享一次 SCAN+DMA

## 配置结构体

```c
typedef struct {
  photoresistor_adc_clock_source_t clock_source;  /* 固定 PCLK2 */
  photoresistor_adc_prescaler_t adc_prescaler;    /* 传给共享 ADC */
} photoresistor_config_t;
```

## API 参考

| 函数 | 说明 |
|------|------|
| `photoresistor_init_with_config(config)` | 初始化共享 ADC + 标记本模块可用 |
| `photoresistor_init()` | AUTO 分频默认 |
| `photoresistor_read_raw_blocking(out)` | 单次 12 位原始值 |
| `photoresistor_read_raw_average_blocking(out, n)` | n 次平均 |
| `photoresistor_read_millivolts_blocking(out)` | 按 VDDA=3.3V 估算 mV |

兼容旧名：`photoresistor_read_raw` 等（无 `_blocking` 后缀）。

## 实现说明

### 为什么是薄封装

光敏、热敏、红外 **必须共用同一 ADC1 实例**。底层 `adc1_dual` 负责寄存器/DMA；本驱动只：

1. 把 `photoresistor_config_t` 转成 `adc1_dual_config_t`；
2. 调 `adc1_dual_read_pair_*` 取 slot0；
3. 做 mV 线性换算。

### 为什么 `read_raw` 走 `read_pair` 而非 `read_all`

历史原因：光敏+热敏成对使用较多。单独读光敏仍会触发 **完整三路 SCAN**（红外槽位被丢弃）。若同时需要红外，应直接用 `adc1_dual_read_all_*` 或 `bsp_analog_sensors_read_all_average`。

### 电压换算

```
mV = raw * 3300 / 4095  （四舍五入）
```

VDDA 实际随 LDO 略有偏差，精确测量需校准或外部基准。

## 使用示例

```c
photoresistor_init();
uint16_t raw;
photoresistor_read_raw_average_blocking(&raw, 4U);
```

应用层推荐 `bsp_ambient_light_read_raw_average()` 或 `bsp_analog_sensors_read_pair_average()`。

## 常见坑

- 未 init → `STM_ERR_NOT_INITIALIZED`
- 模块分压接法不同 → raw 与「亮度」可能反相关，需在应用层标定
- 与 `thermistor_init` 重复 init 无害（共享 ADC 单例）
