# I2C 整理（面向 STM32F103 + SSD1306 实战）

---

## 1. I2C 是什么

I2C（Inter-Integrated Circuit）是两根线的同步串行总线：

- `SCL`：时钟线（通常由主机输出）
- `SDA`：数据线（双向）

核心特点：

- 多设备并联在同一总线上（共享 SCL/SDA）
- 通过地址区分设备
- 线与线（open-drain/open-collector）结构，依赖上拉电阻
- 半双工按位传输，字节为 8bit，后跟 1bit ACK/NACK

---

## 2. 电气与硬件基础

## 2.1 开漏 + 上拉

I2C 引脚必须用开漏（OD）方式：

- 输出 0：主动拉低
- 输出 1：释放总线，由上拉电阻拉高

为什么不能推挽：

- 多设备共享总线，若一个推高一个推低会硬冲突

## 2.2 上拉电阻

常见 3.3V 系统上拉范围：

- 2.2k ~ 10k（常见 4.7k）

线长越长、电容越大，上升沿越慢，越难跑高频。

## 2.3 速率模式

- Standard-mode：100kHz（最稳，初学优先）
- Fast-mode：400kHz（对布线/上拉要求更高）

---

## 3. I2C 协议帧结构

一次主机写事务典型流程：

1. `START`
2. `7位地址 + W(0)`（或 10 位地址流程）
3. 从机 `ACK`
4. 数据字节 1
5. 从机 `ACK`
6. 数据字节 2 ...
7. `STOP`

读事务类似，只是地址阶段是 `7位地址 + R(1)`，最后由主机回 `NACK` 结束读取。

## 3.1 START/STOP 定义

- START：`SCL` 高电平期间，`SDA` 从高到低
- STOP：`SCL` 高电平期间，`SDA` 从低到高

## 3.2 ACK/NACK

每发完 8bit，第 9 个时钟周期：

- `ACK`：接收方拉低 SDA
- `NACK`：接收方不拉低（SDA 保持高）

NACK 常见原因：地址错、从机忙、时序问题、连线/上拉异常。

---

## 4. 7 位地址、8 位地址到底怎么回事

很多资料混着写，建议统一心智：

- **设备地址本体是 7 位**（例如 SSD1306 常见 `0x3C`）
- 总线上实际发的是 **8 位地址字节**：`addr7 << 1 | R/W`

例子：

- `addr7 = 0x3C`
  - 写地址字节：`0x78`
  - 读地址字节：`0x79`

若模块丝印写了 `0x78/0x7A`，通常是“8 位写地址”标法，不是 7 位地址。

---

## 5. STM32F103 的 I2C（本项目关注 I2C1）

## 5.1 时钟与引脚

- 外设时钟：`RCC_APB1ENR` 使能 `I2C1`
- 默认引脚：`PB6(SCL)/PB7(SDA)`
- 重映射后：`PB8(SCL)/PB9(SDA)`（本项目使用）

重映射依赖：

- `AFIO_MAPR.I2C1_REMAP = 1`

GPIO 模式：

- 复用开漏输出（AF_OD）

## 5.2 关键寄存器

- `I2C_CR1`：外设使能、START、STOP 等
- `I2C_CR2`：`FREQ`（PCLK1 频率 MHz）
- `I2C_CCR`：SCL 时钟控制
- `I2C_TRISE`：最大上升时间配置
- `I2C_SR1/SR2`：状态标志（`SB/ADDR/TXE/BTF/AF/BUSY` 等）
- `I2C_DR`：数据寄存器

---

## 6. F103 常用时序参数计算（100kHz）

假设：

- `PCLK1 = 8MHz`
- 目标 `SCL = 100kHz`

常用配置：

- `CR2.FREQ = 8`（单位 MHz）
- `CCR = PCLK1 / (2 * SCL) = 8000000 / 200000 = 40`
- `TRISE = FREQ_MHz + 1 = 9`（Sm 模式常用写法）

> `TRISE = FREQ+1`是工程上常用简化写法，本质来自手册“标准模式上升时间上限 1000ns”换算。

---

## 7. 轮询发送状态机（本项目当前实现思路）

一次写事务（对应 EV5/EV6/EV8 路线）：

1. 等 `BUSY=0`
2. 置 `START`
3. 等 `SR1.SB=1`（EV5）
4. 写地址字节（7 位地址左移 + 写位）
5. 等 `SR1.ADDR=1`（EV6）
6. 读 `SR1` 再读 `SR2`（清 `ADDR`）
7. 等 `TXE=1`，写控制字节
8. 每个 payload 字节都“等 `TXE` -> 写 `DR`”
9. 等 `BTF=1`（最后字节传输完成）
10. 置 `STOP`

错误处理：

- 若 `AF=1`（NACK），清标志并 `STOP`
- 若超时，`STOP` 并返回失败

---

## 8. 为什么有 `(void)I2C1_SR1; (void)I2C1_SR2;`

这是“读 SR1 后读 SR2”的手册规定动作，用于清除 `ADDR` 事件锁存，推进状态机。

- 不是清空整个寄存器
- 是对特定事件位的清除序列
- `(void)` 仅表示“故意忽略读取值”，但读动作会真实发生（`volatile`）

---

## 9. SSD1306 在 I2C 上怎么组织数据

SSD1306 一般 7 位地址 `0x3C`（部分板是 `0x3D`）。

I2C 数据阶段先发一个“控制字节”：

- `0x00`：后续是命令流
- `0x40`：后续是 GDDRAM 数据流

所以常见帧：

- 发命令：`START -> addr+w -> 0x00 -> cmd... -> STOP`
- 发显存：`START -> addr+w -> 0x40 -> data... -> STOP`

---

## 10. SSD1306 显存模型（128x64）

- 总像素：128 x 64
- 组织成 8 页（page），每页 8 像素高
- 一页 128 字节，总计 1024 字节

本地帧缓冲通常也是 1024 字节（本项目 `g_fb`），再整包或局部写到 GDDRAM。

---

## 11. 局部刷新 vs 整屏刷新

整屏刷新：

- 一次发 1024 字节，逻辑简单、显示一致
- 总线占用长

局部刷新：

- 先发窗口命令（`0x21` 列范围 + `0x22` 页范围）
- 只发变化区域的数据
- 速度快、占用低

本项目策略：

- 普通字符显示：局部刷新 6 列（5x7 字模 + 1 列间距）
- 发生滚屏后：整屏刷新（因为大量内存搬移）

---

## 12. 轮询、中断、DMA 怎么选

轮询：

- 最容易写和调
- CPU 会忙等状态位

中断：

- CPU 利用率更好
- 状态机复杂度明显上升

DMA：

- 大块数据搬运最省 CPU
- 初始化和联调成本最高

建议路径（学习 + 工程平衡）：

1. 先轮询跑通（必做）
2. 再做中断异步发送
3. 最后做 DMA 优化

---

## 13. 常见故障与排查清单（高频）

1. **无 ACK**
  - 地址错（`0x3C`/`0x3D`）
  - SCL/SDA 接反
  - 没上拉或上拉过大
  - 从机未上电
2. **总线一直 BUSY**
  - 某设备把 SDA 拉低（异常中断导致未释放）
  - 上电时序异常
  - 需软复位 I2C 或 GPIO 模拟时钟恢复
3. **偶发乱码/丢字节**
  - 时钟太快（先降到 100k）
  - 线太长/干扰大
  - 缓冲或状态机边界处理有误
4. **初始化后不亮**
  - SSD1306 初始化序列缺关键命令（如 charge pump）
  - 对比度/扫描方向配置不匹配
  - 刷新函数未调用

---

## 14. 本仓库 I2C 实现对照

关键文件：

- `include/bsp/stm32f103_regs.h`：I2C 寄存器与位定义
- `include/bsp/clock.h`：`SYSCLK_HZ` / `BSP_PCLK1_HZ`
- `src/bsp/board_init.c`：RCC 时钟使能（含 I2C1）
- `src/drivers/ssd1306_oled.c`：I2C1 配置、事务发送、OLED 刷新

关键点：

- 使用 `I2C1` 硬件主机，不再 bit-bang
- 使用 `AFIO` 重映射到 `PB8/PB9`
- 当前发送路径为轮询 + 超时 + AF 错误收尾

### 14.1 工业化重构后的模块映射

- `src/hal/i2c1_master.c`
  - 提供 `i2c1_master_init()` 与 `i2c1_master_write_frame()`
  - 负责 EV5/EV6/EV8 的事务推进、超时与 NACK 收敛
- `src/drivers/ssd1306_oled.c`
  - 提供 `ssd1306_t` 设备对象
  - 通过 `ssd1306_bus_write_fn` 注入总线发送实现（默认接到 I2C1 HAL）
  - 上层 API：`ssd1306_init/clear/putc/flush/write_text`

这样做的价值：

- I2C 总线逻辑与 SSD1306 设备协议解耦
- 后续切换 I2C2 或软 I2C 仅需替换 `bus_write` 实现
- 设备驱动可复用于不同板级实现

---

## 15. 面试/实战常问（速答）

**Q1: 为什么 I2C 要开漏？**  
A: 多设备共享总线，避免推挽冲突；高电平靠上拉。

**Q2: 为什么要设置 CR2.FREQ？**  
A: 告诉 I2C 外设 APB1 时钟频率（MHz），供内部时序计算使用。

**Q3: 清 ADDR 为什么要读 SR1 再读 SR2？**  
A: 手册规定的硬件清标志序列，缺一步状态机会卡住。

**Q4: 100k 与 400k 怎么选？**  
A: 先 100k 保稳定；链路确认无误再提升到 400k。

**Q5: 什么时候必须整屏刷新？**  
A: 显存做了全局搬移（如滚屏）时，局部刷新会不一致，需整屏。

---

## 16. 下一步实战

1. 把 I2C 发送改成“异步中断状态机”并保留轮询 fallback
2. 新增错误统计计数（AF/timeout 次数）串口可读
3. 增加 I2C 总线恢复函数（SDA 卡低时 SCL 人工脉冲）
4. 做一个 `100k/400k` 可配置宏并验证波形

---

## 17. 一页总结

I2C 真正要掌握的不是“背寄存器名”，而是这条主线：

**电气（开漏上拉） -> 协议（START/ADDR/ACK） -> 外设状态机（SB/ADDR/TXE/BTF） -> 设备协议（SSD1306 控制字节） -> 错误收敛（AF/超时/恢复）**

这条链通了，换任意 I2C 传感器/EEPROM/屏，方法都一样。

---

# English

# I2C Overview (STM32F103 + SSD1306 Practice)

---

## 1. What I2C Is

I2C (Inter-Integrated Circuit) is a two-wire synchronous serial bus:

- `SCL`: clock (usually master-driven)
- `SDA`: data (bidirectional)

Key properties:

- Multiple devices on one bus (shared SCL/SDA)
- Address distinguishes devices
- Open-drain wiring with pull-ups
- Half-duplex bit transfer; 8 data bits + 1 ACK/NACK bit per byte

---

## 2. Electrical and Hardware Basics

### 2.1 Open-drain + pull-up

I2C pins must be open-drain (OD):

- Drive 0: actively pull low
- Drive 1: release line; pull-up raises SDA/SCL

Why not push-pull:

- Shared bus—push-pull high vs low causes hard contention

### 2.2 Pull-up resistors

Typical 3.3 V systems: 2.2k–10k (often 4.7k).

Longer wires / more capacitance → slower edges → harder to run high speed.

### 2.3 Speed modes

- Standard-mode: 100 kHz (most stable; good for learning)
- Fast-mode: 400 kHz (stricter wiring/pull-ups)

---

## 3. I2C Protocol Frame

Typical master **write** transaction:

1. `START`
2. `7-bit address + W(0)` (or 10-bit address sequence)
3. Slave `ACK`
4. Data byte(s) + slave `ACK` each
5. `STOP`

**Read** is similar with `R(1)`; host sends `NACK` on last byte.

### 3.1 START/STOP

- START: `SCL` high, `SDA` high→low
- STOP: `SCL` high, `SDA` low→high

### 3.2 ACK/NACK

After 8 bits, 9th clock:

- `ACK`: receiver pulls SDA low
- `NACK`: receiver leaves SDA high

NACK causes: wrong address, busy slave, bad timing, wiring/pull-up issues.

---

## 4. 7-Bit vs 8-Bit Address

Unified model:

- **Device address is 7 bits** (SSD1306 often `0x3C`)
- On wire you send **8-bit address byte**: `addr7 << 1 | R/W`

Example `addr7 = 0x3C`:

- Write byte: `0x78`
- Read byte: `0x79`

Silkscreen `0x78/0x7A` is often **8-bit write address**, not 7-bit.

---

## 5. STM32F103 I2C (I2C1 in This Project)

### 5.1 Clock and pins

- Clock: `RCC_APB1ENR` enable `I2C1`
- Default: `PB6(SCL)/PB7(SDA)`
- Remapped: `PB8(SCL)/PB9(SDA)` (**this project**)

Remap requires:

- `AFIO_MAPR.I2C1_REMAP = 1`

GPIO: alternate function open-drain (AF_OD).

### 5.2 Key registers

- `I2C_CR1`: enable, START, STOP
- `I2C_CR2`: `FREQ` (PCLK1 in MHz)
- `I2C_CCR`: SCL timing
- `I2C_TRISE`: max rise time
- `I2C_SR1/SR2`: `SB/ADDR/TXE/BTF/AF/BUSY`, etc.
- `I2C_DR`: data

---

## 6. F103 Timing at 100 kHz (Example)

Assume:

- `PCLK1 = 8 MHz`
- Target `SCL = 100 kHz`

Typical:

- `CR2.FREQ = 8` (MHz)
- `CCR = PCLK1 / (2 * SCL) = 40`
- `TRISE = FREQ_MHz + 1 = 9` (common Sm-mode shortcut)

> `TRISE = FREQ+1` is a practical rule from the 1000 ns rise-time limit in the manual.

---

## 7. Polling Transmit State Machine (This Project)

Write transaction (EV5/EV6/EV8 path):

1. Wait `BUSY=0`
2. Set `START`
3. Wait `SR1.SB=1` (EV5)
4. Write address byte (`addr7 << 1 | write`)
5. Wait `SR1.ADDR=1` (EV6)
6. Read `SR1` then `SR2` (clear `ADDR`)
7. Wait `TXE=1`, write control byte
8. Each payload: wait `TXE` → write `DR`
9. Wait `BTF=1` (last byte complete)
10. Set `STOP`

Errors:

- `AF=1` (NACK): clear flag, `STOP`, fail
- Timeout: `STOP`, fail

---

## 8. Why `(void)I2C1_SR1; (void)I2C1_SR2;`

Manual sequence to clear `ADDR` event—advances state machine.

- Not clearing whole registers
- `(void)` means intentional read; `volatile` makes read real

---

## 9. SSD1306 Data on I2C

7-bit address usually `0x3C` (some boards `0x3D`).

Control byte after address:

- `0x00`: command stream
- `0x40`: GDDRAM data stream

Frames:

- Command: `START → addr+w → 0x00 → cmd... → STOP`
- GRAM: `START → addr+w → 0x40 → data... → STOP`

---

## 10. SSD1306 Frame Buffer (128×64)

- 128×64 pixels
- 8 pages × 128 bytes = 1024 bytes (`g_fb` in project)

---

## 11. Partial vs Full Refresh

Full refresh: 1024 bytes, simple, consistent, long bus time.

Partial: set window (`0x21` columns, `0x22` pages), send changed region only.

This project:

- Normal char: partial 6 columns
- After scroll: full refresh (large memory move)

---

## 12. Polling vs Interrupt vs DMA

| Mode | Pros | Cons |
|------|------|------|
| Polling | Easiest debug | CPU busy-waits |
| Interrupt | Better CPU use | Complex state machine |
| DMA | Best for bulk | Highest bring-up cost |

Path: polling first → interrupt → DMA.

---

## 13. Common Faults

1. **No ACK** — wrong address, swapped SCL/SDA, no pull-up, slave off
2. **BUSY stuck** — SDA held low; bus recovery / soft reset
3. **Garbled data** — clock too fast, long wires, state machine bugs
4. **Blank after init** — missing charge pump, wrong contrast/scan, no flush

---

## 14. This Repo Mapping

| File | Role |
|------|------|
| `stm32f103_regs.h` | I2C register bits |
| `clock.h` | `SYSCLK_HZ` / `BSP_PCLK1_HZ` |
| `board_init.c` | RCC I2C1 |
| `ssd1306_oled.c` | I2C1 + OLED refresh |

- Hardware I2C1 master, not bit-bang
- AFIO remap to `PB8/PB9`
- Polling + timeout + AF cleanup

### 14.1 Refactored modules

- `hal/i2c1_master.c`: `i2c1_master_init()`, `i2c1_master_write_frame()`, EV5/6/8, timeout, NACK
- `ssd1306_oled.c`: `ssd1306_t`, `ssd1306_bus_write_fn`, `init/clear/putc/flush/write_text`

Benefit: bus logic decoupled from SSD1306; swap I2C2 or bit-bang via `bus_write`.

---

## 15. Interview / Practice Q&A

**Q1: Why open-drain?**  
A: Shared bus; high level from pull-up, no push-pull fight.

**Q2: Why `CR2.FREQ`?**  
A: Tells peripheral PCLK1 MHz for internal timing.

**Q3: Clear ADDR — read SR1 then SR2?**  
A: Required hardware sequence; missing step stalls FSM.

**Q4: 100k vs 400k?**  
A: Start 100k; raise after stable waveforms.

**Q5: When full refresh?**  
A: Global framebuffer moves (e.g. scroll).

---

## 16. Next Steps

1. Async interrupt TX + polling fallback
2. Error counters (AF/timeout) on UART
3. Bus recovery (SCL pulses when SDA stuck)
4. Configurable 100k/400k macro

---

## 17. One-Page Summary

Master the chain:

**Electrical (OD + pull-up) → Protocol (START/ADDR/ACK) → Peripheral FSM (SB/ADDR/TXE/BTF) → Device protocol (SSD1306 control byte) → Error handling (AF/timeout/recovery)**

Same method for any I2C sensor/EEPROM/display.