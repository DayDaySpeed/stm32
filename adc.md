# STM32F103 ADC 全整理（单通道 vs 双通道 SCAN+DMA）

本文结合本工程，把 ADC1 的两种用法一次讲清：

- **单通道轮询**：一次 `SWSTART` 只采一路，CPU 等 `EOC` 后读 `DR`（早期光敏/热敏各自独立驱动的思路）
- **双通道 SCAN + DMA**：一次 `SWSTART` 按序列扫多路，每路转换结束由 DMA 自动把 `DR` 搬进 RAM（当前 `adc1_dual_scan_dma`）

并说明相关寄存器怎么配、本工程代码在哪、常见踩坑。

---

## 1. 先建立 ADC 心智模型

模拟传感器（光敏分压、NTC 分压）输出的是 **0 ~ VDDA 的电压**。  
STM32 的 ADC 把它量化成 **12 位无符号整数** `0 ~ 4095`：

```
Vout = raw / 4095 * VDDA
```

本板默认：

| 引脚 | ADC 通道 | 模块 |
|------|----------|------|
| PA1 | ADC1_IN1 | 光敏电阻 AO |
| PA2 | ADC1_IN2 | 热敏电阻 AO |

数据通路（概念）：

```
传感器分压 ──► GPIO 模拟输入 ──► ADC 采样保持 ──► 12bit 数字量 ──► DR / DMA / 应用换算
```

**F103 上 ADC 时钟** 固定来自 `PCLK2`，再经 `RCC_CFGR.ADCPRE` 做 `/2 /4 /6 /8`，**不能超过 14MHz**，且 **没有 /1**。

---

## 2. 两种方案对比（本工程为什么改成 SCAN+DMA）

| 维度 | 单通道轮询 | 双通道 SCAN + DMA（当前） |
|------|------------|---------------------------|
| 一次触发采几路 | 1 路 | N 路（本工程 N=2） |
| `CR1.SCAN` | 0 | 1 |
| `SQR1.L` | 0（长度 1） | 1（长度 2，L 表示 N-1） |
| `CR2.DMA` | 0 | 1 |
| 结果怎么取 | 等 `EOC` → 读 `ADC1_DR` | DMA 写入 `uint16_t buffer[]` |
| 两路传感器 | 两个驱动轮流改 `SQR/SMPR`，**软件分时** | 硬件按 SQ1→SQ2 扫完，**时间对齐** |
| CPU 占用 | 轮询等待 EOC | 轮询等待 DMA `TCIF1`（也可改中断） |
| 代码位置 | 已收敛到 `adc1_dual_scan_dma.c`；光敏/热敏为薄封装 | `src/drivers/adc1_dual_scan_dma.c` |

```
单通道（软件复用 ADC1）：

  读光敏：SQR3=IN1 → SWSTART → EOC → DR
  读热敏：SQR3=IN2 → SWSTART → EOC → DR   （两次触发，时刻不对齐）

双通道 SCAN+DMA：

  buffer[0]=IN1 ─┐
                 ├─ 一次 SWSTART，硬件 SQ1→SQ2，DMA 依次写入
  buffer[1]=IN2 ─┘
```

---

## 3. 相关寄存器速查（按配置顺序）

### 3.1 时钟与 GPIO

| 寄存器 | 作用 | 本工程写法 |
|--------|------|------------|
| `RCC_APB2ENR.ADC1EN` | ADC1 外设时钟 | `bsp_board_init()` 里 `RCC_BOARD_APB2_ENABLE_MASK` |
| `RCC_AHBENR.DMA1EN` | DMA1 时钟（SCAN+DMA 需要） | `RCC_BOARD_AHB_ENABLE_MASK` |
| `RCC_CFGR.ADCPRE` | `ADCCLK = PCLK2 / 分频` | `adc1_dual_configure_adc_clock_*()`，≤14MHz |
| `GPIOx_CRL/CRH` | 引脚模式 | PA1/PA2：**模拟输入** `CNF=00 MODE=00` |

模拟输入要关数字输入缓冲，减小漏电；本工程宏见 `include/bsp/board_pins.h` 的 `BOARD_GPIO_PA1_ANALOG`、`BOARD_GPIO_PA2_ANALOG`。

### 3.2 ADC1 控制寄存器

#### `ADC1_SR`（状态）

| 位 | 名 | 含义 |
|----|-----|------|
| 1 | EOC | 规则组一次转换结束；读 `DR` 会清除 |

单通道：等 `EOC=1` 再读 `DR`。  
SCAN+DMA：通常等 **DMA 传完**；仍可在启动前 `ADC1_SR &= ~EOC` 清残留。

#### `ADC1_CR1`

| 位/字段 | 名 | 单通道 | 双通道 SCAN |
|---------|-----|--------|-------------|
| 8 | SCAN | 0 | **1** |
| [23:20] | L（在 `SQR1` 里） | 0 → 1 次转换 | **1** → 2 次转换 |

本工程双通道：

```c
ADC1_CR1 = ADC_CR1_SCAN_BIT;
ADC1_SQR1 = (ADC1_SQR1 & ~ADC_CR1_L_MASK) | ADC_CR1_L_2_CONV;  // L=1
```

#### `ADC1_CR2`

| 位 | 名 | 作用 |
|----|-----|------|
| 0 | ADON | ADC 上电；校准前也要置 1 |
| 2 | CAL | 启动校准，硬件完成后自动清零 |
| 3 | RSTCAL | 复位校准寄存器 |
| 8 | DMA | 1：每完成一次规则转换，请求 DMA 搬 `DR` |
| [19:17] | EXTSEL | 规则组触发源；`111` = SWSTART |
| 20 | EXTTRIG | 允许规则组触发（**软件触发也要先置 1**） |
| 22 | SWSTART | 写 1 启动一次转换/一轮扫描 |

本工程：

```c
ADC1_CR2 = ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG_BIT | ADC_CR2_DMA_BIT;
// 采样时：ADC1_CR2 |= ADC_CR2_SWSTART_BIT;
```

> **踩坑**：只写 `SWSTART` 而不配 `EXTSEL=111` + `EXTTRIG=1`，F103 上规则组可能根本不启动，一直等不到 EOC/DMA。

#### `ADC1_SMPR2`（采样时间，通道 0~9）

每个通道 3 bit，值越大采样时间越长，**高阻分压节点**（光敏/NTC）建议最长档 `111` = 239.5 周期。

| 通道 | 字段 | 本工程 |
|------|------|--------|
| IN1 | SMP1[5:3] | `ADC_SMPR2_SMP1_MAX` |
| IN2 | SMP2[8:6] | `ADC_SMPR2_SMP2_MAX` |

写法：**先清字段再写入**，避免误改其它通道的 SMP。

#### `ADC1_SQR1` / `ADC1_SQR3`（规则序列）

- `SQR1.L[3:0]`：序列长度 **减 1**（0→1 次，1→2 次，…）
- `SQR3`：SQ1~SQ6 的通道号（每个 5 bit）

本工程双通道顺序：

```c
// SQ1 = 通道 1 (PA1 光敏)，SQ2 = 通道 2 (PA2 热敏)
ADC1_SQR3 = ADC_SQR3_SQ1(1) | ADC_SQR3_SQ2(2);
```

DMA 缓冲区顺序与 SQ 顺序一致：

```c
out_pair[ADC1_DUAL_SLOT_PHOTO]  // 先转换 IN1
out_pair[ADC1_DUAL_SLOT_THERM]   // 后转换 IN2
```

#### `ADC1_DR`（数据）

- 12 位结果，右对齐，读低 12 位：`ADC1_DR & 0xFFF`
- 单通道：CPU 直接读
- SCAN+DMA：DMA 从 `DR` 读到内存；**每完成一路转换触发一次 DMA 请求**

### 3.3 DMA1 通道 1（仅 ADC1 规则组）

| 寄存器 | 作用 |
|--------|------|
| `DMA1_CPAR1` | 外设地址 = `&ADC1_DR`（固定） |
| `DMA1_CMAR1` | 内存缓冲区地址 |
| `DMA1_CNDTR1` | 传输次数（本工程 2 次 = 两路各一个半字） |
| `DMA1_CCR1` | 通道配置：EN、MINC、PSIZE/MSIZE=16bit 等 |
| `DMA1_ISR.TCIF1` | 通道 1 传输完成 |
| `DMA1_IFCR.CTCIF1` | 写 1 清除 TC 标志 |

本工程 `DMA1_CCR1` 配置要点：

- **DIR**：默认 0 = 外设 → 内存
- **MINC=1**：每传一个半字，内存地址 +2
- **PSIZE=01、MSIZE=01**：16 位（与 12 位 ADC 结果一致）
- **不开循环模式**：每次采样前重新设 `CNDTR`、清标志、再 `EN`

流程：

```
1) 配 CPAR/CMAR/CNDTR/CCR
2) DMA1_CCR1.EN = 1
3) ADC1_CR2.SWSTART = 1
4) 硬件：IN1 转换 → DMA 写 buffer[0] → IN2 转换 → DMA 写 buffer[1]
5) 轮询 DMA1_ISR.TCIF1
6) DMA1_CCR1.EN = 0
```

---

## 4. 单通道轮询：完整步骤（教学用）

适合 **只有一路模拟量** 或临时调试。本工程已不再在光敏/热敏里各自配一套，但思路如下。

```
1. 开时钟：ADC1、GPIOA
2. ADCPRE：ADCCLK ≤ 14MHz
3. PAx 模拟输入
4. ADC1_CR1 = 0          // 不扫描
5. ADC1_CR2：EXTSEL+EXTTRIG，暂不开 DMA
6. SMPRx：该通道最长采样时间
7. SQR1.L = 0，SQR3.SQ1 = 通道号
8. 校准：ADON → RSTCAL → CAL
9. 采样：清 EOC → SWSTART → 等 EOC → 读 DR
```

伪代码：

```c
ADC1_SQR1 = 0;
ADC1_SQR3 = channel_id;
ADC1_SR &= ~ADC_SR_EOC_BIT;
ADC1_CR2 |= ADC_CR2_SWSTART_BIT;
while (!(ADC1_SR & ADC_SR_EOC_BIT)) { }
raw = ADC1_DR & 0xFFF;
```

**多路怎么办？** 每次采样前改 `SQR3`（和 SMP），本质是 **多次单通道**，不是硬件 SCAN。

---

## 5. 双通道 SCAN+DMA：本工程实现路径

### 5.1 模块与文件

| 文件 | 职责 |
|------|------|
| `src/drivers/adc1_dual_scan_dma.c` | ADC+DMA 硬件配置与采样 |
| `include/drivers/adc1_dual_scan_dma.h` | `adc1_dual_read_pair_*` API |
| `src/drivers/photoresistor.c` | 取 `SLOT_PHOTO` + 电压换算 |
| `src/drivers/thermistor.c` | 取 `SLOT_THERM` + 电阻/温度查表 |
| `src/bsp/board_devices.c` | `bsp_analog_sensors_read_pair_average()` |
| `src/app/app.c` | OLED 第 4/6 行显示光敏与热敏 |

### 5.2 初始化（`adc1_dual_init_with_config`）

1. `RCC_AHBENR` / `RCC_CFGR.ADCPRE`
2. PA1+PA2 模拟输入
3. `CR1.SCAN=1`，`CR2` 开 `DMA` + 软件触发链
4. `SQR` + `SMPR2` 双通道
5. `RSTCAL` + `CAL`

### 5.3 一次采样（`adc1_dual_start_scan_dma_blocking`）

1. `dma_setup(buffer, 2)`
2. `DMA EN` + `SWSTART`
3. 等 `TCIF1`
4. `DMA DIS`

### 5.4 平均

`adc1_dual_read_pair_average_blocking()` = 重复上述过程 `scan_count` 次，对两路分别累加再除。

应用层推荐 **一次读两路**，避免重复触发 ADC：

```c
uint16_t photo, therm;
bsp_analog_sensors_read_pair_average(&photo, &therm, 4);
thermistor_read_temperature_from_raw_blocking(therm, &temp_x10);
```

---

## 6. 校准与转换时间（为什么有时会很慢）

### 6.1 上电校准（必须做一次）

```
ADON = 1 → 短延时 → RSTCAL = 1（等清零）→ CAL = 1（等清零）
```

未校准会导致零点漂移。代码见 `adc1_dual_adc_calibrate()`。

### 6.2 单次转换时间（估算）

```
T_conv ≈ (采样周期 + 12.5) / ADCCLK
```

采样周期由 `SMP` 决定；本工程两路都用 **239.5 cycles**（高阻源更稳）。

双通道 SCAN **一次触发** 的总时间 ≈ **两路 T_conv 之和**（再加 DMA 开销，很小）。

---

## 7. 常见踩坑

| 现象 | 原因 | 处理 |
|------|------|------|
| `SWSTART` 后一直超时 | 未配 `EXTSEL=SWSTART` + `EXTTRIG` | 见 3.2 |
| 读数全 0 或卡死 | GPIO 仍是数字模式 | 改为模拟输入 |
| 读数乱跳 | 采样时间太短 / 无平均 | 加长 SMP；应用层多次平均 |
| 双驱动抢 ADC | 两模块各写 `SQR` | 合并为一个 SCAN 驱动 |
| DMA 永不完成 | `CNDTR` 与通道数不一致；未开 `CR2.DMA` | `CNDTR=通道数`；`DMA=1` |
| 链路 `.data` 与 exidx 重叠 | 链接脚本未收 `.ARM.exidx*` | 见 `linker/STM32F103C8TX_FLASH.ld` |
| 温度/电压公式不对 | `VDDA` 不是精确 3.3V | 配置 `thermistor_config.vdda_mv` 或实测标定 |

---

## 8. 与 OLED 显示相关的说明

`bsp_display_write_text_atf()` 使用 **精简版格式化**（`ssd1306_vformat`），**不是**完整 `printf`。

支持的占位符：

| 格式 | 含义 |
|------|------|
| `%u` | `uint32_t` 无符号十进制 |
| `%d` | `int32_t` 有符号十进制 |
| `%c` | `char` 字符 |
| `%s` | 字符串 |
| `%x` / `%X` | 十六进制 |
| `%%` | 字面量 `%` |

**不支持**宽度/精度（如 `%02d`、`%5s`）。  
`app.c` 里 `"NTC=%c%d.%dC raw=%u"` 合法：`%c` 输出正负号，`%d` 输出整数部分与小数一位。

---

## 9. 快速对照：寄存器 ↔ 本工程宏

| 寄存器/位 | 宏（`stm32f103_regs.h`） |
|-----------|-------------------------|
| `CR1.SCAN` | `ADC_CR1_SCAN_BIT` |
| `SQR1.L=1` | `ADC_CR1_L_2_CONV` |
| `CR2.DMA` | `ADC_CR2_DMA_BIT` |
| `CR2.SWSTART` | `ADC_CR2_SWSTART_BIT` |
| `CR2.EXTSEL` | `ADC_CR2_EXTSEL_SWSTART` |
| `SR.EOC` | `ADC_SR_EOC_BIT` |
| `SQR3.SQ1/SQ2` | `ADC_SQR3_SQ1(ch)` / `ADC_SQR3_SQ2(ch)` |
| `SMPR2` IN1/IN2 | `ADC_SMPR2_SMP1_MAX` / `ADC_SMPR2_SMP2_MAX` |
| DMA TC | `DMA_ISR_TCIF1_BIT` / `DMA_IFCR_CTCIF1_BIT` |

---

## 10. 阅读顺序建议

1. 本文第 2、3 节：弄清单通道 vs SCAN+DMA 差异  
2. `src/drivers/adc1_dual_scan_dma.c`：对照寄存器写入顺序  
3. `src/drivers/thermistor.c`：看 **原始值 → 电阻 → 查表温度**  
4. `src/app/app.c` 的 `app_analog_sensors_oled_task()`：看应用层如何一次读两路  

若只加第三路模拟传感器，需要扩展 `SQR` 序列长度、`DMA CNDTR` 和缓冲区槽位，而不是再复制一套单通道驱动。
