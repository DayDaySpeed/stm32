# ADC1 三路 SCAN + DMA 驱动（`adc1_dual_scan_dma`）

## 作用

**一次 SWSTART** 按硬件序列扫描三路模拟输入，DMA1 通道 1 自动把结果写入 RAM。保证光敏/热敏/红外 **同一时刻对齐采样**，比软件轮流改 SQR 更省时、更一致。

> 文件名保留 `dual` 历史；当前实现为 **3 通道**（光敏 + 热敏 + 红外）。

## 硬件绑定

| 槽位 | 枚举 | 引脚 | 通道 |
|------|------|------|------|
| 0 | `ADC1_DUAL_SLOT_PHOTO` | PA1 | ADC1_IN1 光敏 |
| 1 | `ADC1_DUAL_SLOT_THERM` | PA2 | ADC1_IN2 热敏 |
| 2 | `ADC1_DUAL_SLOT_IR_REFLECT` | PA3 | ADC1_IN3 红外 |

## 配置结构体

```c
typedef struct {
  adc1_dual_clock_source_t clock_source;  /* 固定 PCLK2 */
  adc1_dual_prescaler_t adc_prescaler;  /* AUTO 或 /2 /4 /6 /8 */
} adc1_dual_config_t;
```

ADC 时钟 = PCLK2 / ADCPRE，**不得超过 14MHz**（F103 硬性限制）。

## API 参考

| 函数 | 说明 |
|------|------|
| `adc1_dual_init_with_config(config)` | RCC、GPIO 模拟、SCAN 序列、校准 |
| `adc1_dual_init()` | AUTO 分频默认 init |
| `adc1_dual_is_initialized()` | 是否已 init |
| `adc1_dual_read_all_blocking(out[3])` | 单次三路扫描 |
| `adc1_dual_read_all_average_blocking(out[3], n)` | n 次扫描算术平均 |
| `adc1_dual_read_pair_blocking(out[2])` | 只取光敏+热敏（仍扫三路） |
| `adc1_dual_read_pair_average_blocking(out[2], n)` | 同上 + 平均 |

所有读接口均为 **阻塞**：轮询 DMA `TCIF1`，超时返回 `STM_ERR_TIMEOUT`。

## 实现说明

### 数据通路

```
PA1/2/3 模拟输入 → ADC1 SCAN(SQ1→SQ2→SQ3) → 每完成一路 DR → DMA1 Ch1 → buffer[]
```

关键寄存器：`CR1.SCAN=1`，`SQR1.L=2`（长度 3），`CR2.DMA=1`，`CR2.EXTSEL=软件触发`。

### 为什么用 DMA 仍阻塞

教学/裸机阶段优先 **行为简单**：CPU 等 TCIF 再返回，调用方无需管理半完成状态。以后可改为 DMA 中断 + 双缓冲。

### 为什么 `read_pair` 仍扫三路

硬件 SCAN 序列固定；丢弃红外槽位比动态改 SQR 更安全（避免与并发读冲突）。应用层应优先 `read_all_*` 一次取齐三路（本工程 OLED 任务已优化为此方式）。

### 校准流程

`ADON` → 延时 → `RSTCAL` → 等清零 → `CAL` → 等清零。跳过校准会导致绝对值偏差。

## 使用示例

```c
adc1_dual_init();

uint16_t s[ADC1_DUAL_SLOT_COUNT];
adc1_dual_read_all_average_blocking(s, 4U);
uint16_t ldr = s[ADC1_DUAL_SLOT_PHOTO];
uint16_t ntc = s[ADC1_DUAL_SLOT_THERM];
uint16_t ir  = s[ADC1_DUAL_SLOT_IR_REFLECT];
```

更完整 ADC 原理见 [topics/adc.md](../topics/adc.md)。

## 常见坑

- ADCPRE 配太大 → ADC 时钟 >14MHz，读数不稳定。
- GPIO 未配模拟输入 → 漏电或读数漂。
- 多次 init 不同 prescaler → 会重配时钟，需理解 `s_initialized` 分支行为。

---

# English

# ADC1 Triple-Channel SCAN + DMA Driver (`adc1_dual_scan_dma`)

## Purpose

**One SWSTART** scans three analog inputs in hardware sequence; DMA1 channel 1 writes results to RAM automatically. Ensures photoresistor, thermistor, and IR **sample at the same instant**, faster and more consistent than software channel switching via SQR.

> Filename retains historical `dual`; current implementation is **3 channels** (photoresistor + thermistor + IR).

## Hardware Binding

| Slot | Enum | Pin | Channel |
|------|------|------|------|
| 0 | `ADC1_DUAL_SLOT_PHOTO` | PA1 | ADC1_IN1 photoresistor |
| 1 | `ADC1_DUAL_SLOT_THERM` | PA2 | ADC1_IN2 thermistor |
| 2 | `ADC1_DUAL_SLOT_IR_REFLECT` | PA3 | ADC1_IN3 IR |

## Configuration Structure

```c
typedef struct {
  adc1_dual_clock_source_t clock_source;  /* fixed PCLK2 */
  adc1_dual_prescaler_t adc_prescaler;  /* AUTO or /2 /4 /6 /8 */
} adc1_dual_config_t;
```

ADC clock = PCLK2 / ADCPRE, **must not exceed 14 MHz** (F103 hard limit).

## API Reference

| Function | Description |
|------|------|
| `adc1_dual_init_with_config(config)` | RCC, analog GPIO, SCAN sequence, calibration |
| `adc1_dual_init()` | Default init with AUTO prescaler |
| `adc1_dual_is_initialized()` | Whether init completed |
| `adc1_dual_read_all_blocking(out[3])` | Single triple-channel scan |
| `adc1_dual_read_all_average_blocking(out[3], n)` | Arithmetic mean of n scans |
| `adc1_dual_read_pair_blocking(out[2])` | Photoresistor + thermistor only (still scans all three) |
| `adc1_dual_read_pair_average_blocking(out[2], n)` | Same as above + averaging |

All read APIs are **blocking**: poll DMA `TCIF1`; timeout returns `STM_ERR_TIMEOUT`.

## Implementation Notes

### Data Path

```
PA1/2/3 analog input → ADC1 SCAN(SQ1→SQ2→SQ3) → per-channel DR complete → DMA1 Ch1 → buffer[]
```

Key registers: `CR1.SCAN=1`, `SQR1.L=2` (length 3), `CR2.DMA=1`, `CR2.EXTSEL=software trigger`.

### Why DMA Is Still Blocking

Bare-metal / teaching phase prioritizes **simple behavior**: CPU waits for TCIF before return; caller need not manage half-complete state. Can be upgraded to DMA interrupt + double buffering later.

### Why `read_pair` Still Scans Three Channels

Hardware SCAN sequence is fixed; discarding the IR slot is safer than dynamically changing SQR (avoids conflicts with concurrent reads). Application layer should prefer `read_all_*` to fetch all three at once (OLED task in this project is already optimized this way).

### Calibration Sequence

`ADON` → delay → `RSTCAL` → wait clear → `CAL` → wait clear. Skipping calibration causes absolute value offset.

## Usage Example

```c
adc1_dual_init();

uint16_t s[ADC1_DUAL_SLOT_COUNT];
adc1_dual_read_all_average_blocking(s, 4U);
uint16_t ldr = s[ADC1_DUAL_SLOT_PHOTO];
uint16_t ntc = s[ADC1_DUAL_SLOT_THERM];
uint16_t ir  = s[ADC1_DUAL_SLOT_IR_REFLECT];
```

Full ADC theory: [topics/adc.md](../topics/adc.md).

## Common Pitfalls

- ADCPRE too small → ADC clock >14 MHz, unstable readings.
- GPIO not configured as analog input → leakage or drifting readings.
- Multiple init with different prescaler → reconfigures clock; understand `s_initialized` branch behavior.
