# NVIC 说明（本项目 + Cortex-M3 通用）

本文说明 **嵌套向量中断控制器（NVIC）** 在本 STM32F103 工程里**实际用到的寄存器与位**、**推荐开启顺序**，并补齐与 NVIC 相关的常用概念（与向量表、系统异常、优先级的关系）。

---

## 1. NVIC 在 Cortex-M3 里做什么

- **外部中断（IRQ）**：来自片上外设（如 TIM、USART）的中断请求，经 NVIC 仲裁后进入内核，CPU 从**向量表**取出 `IRQHandler` 地址并跳转执行。
- **系统异常**：如 `NMI`、`HardFault`、`SysTick` 等，**不经过 NVIC 的 `ISER` 使能位**；其中 **SysTick** 由 **SysTick 自己的 `SYST_CSR`** 打开，向量表里仍有独立入口（见下文第 8 节）。
- **嵌套**：若多个中断同时 pending，NVIC 按**优先级**决定谁先执行；高优先级可打断低优先级（默认配置下）。

本项目在 `include/bsp/stm32f103_regs.h` 中把 NVIC 映射到固定地址：

| 符号 | 地址 | 含义 |
|------|------|------|
| `NVIC_BASE` | `0xE000E100` | NVIC 寄存器区起始 |
| `NVIC_ISER0` | `NVIC_BASE + 0x00` | Interrupt Set-Enable 寄存器 0（IRQ 0～31） |
| `NVIC_ISER1` | `NVIC_BASE + 0x04` | Interrupt Set-Enable 寄存器 1（IRQ 32～63） |

> **读改写注意**：对 `ISER` 写 `1` 表示**置位使能**该 IRQ；写 `0` 无意义（不是“关闭”）。关闭中断要用 **`ICER`**（本项目当前未封装）。

---

## 2. 本项目用到的 NVIC：只有 `ISER`（使能位）

### 2.1 TIM2 更新中断

- **STM32F103 向量号**：`TIM2` 对应 **IRQn = 28**（与 `startup_stm32f103c8tx.s` 中 “IRQ28 = TIM2” 一致）。
- **寄存器**：`NVIC_ISER0`（因为 `28 / 32 = 0`）。
- **位**：`NVIC_TIM2_IRQ_BIT = (1U << 28)`，即 **ISER0 的 bit 28**。
- **代码位置**：`src/drivers/tim2.c` 在配置好 `TIM2_DIER`（更新中断允许）后执行 `NVIC_ISER0 |= NVIC_TIM2_IRQ_BIT`，再启动计数器。

### 2.2 USART1 接收中断

- **IRQn = 37**。
- **寄存器**：`NVIC_ISER1`（因为 `37 / 32 = 1`）。
- **位**：`NVIC_USART1_IRQ_BIT = (1U << 5)`，即 **ISER1 的 bit 5**（注意是 **`37 % 32 = 5`**，不是“USART1 外设里的某个位 5”）。
- **代码位置**：`src/drivers/usart1.c` 的 `usart1_enable_rx_interrupt()` 在置位 `USART_CR1_RXNEIE` 后执行 `NVIC_ISER1 |= NVIC_USART1_IRQ_BIT`。

### 2.3 小结表

| 外设 | IRQn | ISER 下标 | 使用的寄存器 | 置 1 的位 |
|------|------|-----------|--------------|-----------|
| TIM2 | 28 | 0 | `NVIC_ISER0` | bit 28 |
| USART1 | 37 | 1 | `NVIC_ISER1` | bit 5 |

通用公式：

- `ISER 索引 = IRQn / 32`
- `位序号 = IRQn % 32`

---

## 3. 推荐的中断/NVIC 开启流程（与本项目一致）

对**外设 IRQ**，建议顺序为：

1. **时钟与 GPIO**：`RCC` 打开外设时钟，引脚复用/模式正确（在 `bsp_board_init` / 各驱动里完成）。
2. **配置外设本身**：波特率、定时器分频与 ARR、DMA 等；**先不要**或**谨慎**打开会立即产生事件的路径。
3. **清除外设悬挂标志**（若需要）：例如定时器可先清 `UIF`，避免一开 NVIC 就进一次异常中断。
4. **打开外设“中断源”**：如 `TIM2_DIER` 的 `UIE`、`USART1_CR1` 的 `RXNEIE` —— 表示“外设允许向 NVIC 发请求”。
5. **再使能 NVIC 通道**：`NVIC_ISERx |= (1 << (IRQn % 32))`。  
   这样可避免外设尚未就绪时 IRQ 已 pending，减少竞态。
6. **最后启动会持续产生请求的动作**：例如 `TIM2_CR1` 的 `CEN`（本项目在 `NVIC_ISER0` 之后才 `CEN`）。

USART1 的 NVIC 在 `usart1_enable_rx_interrupt()` 里打开：**先 `RXNEIE`，再 `NVIC_ISER1`**，与上述原则一致。

---

## 4. 向量表与本项目中的 Handler 对应关系

向量表在 `startup/startup_stm32f103c8tx.s` 的 `g_pfnVectors` 中。与 NVIC 外设 IRQ 直接相关的条目包括：

- 索引 **44**（从 0 数：第 45 个字）：`TIM2_IRQHandler` → 对应 **IRQ28**。
- 索引 **53**：`USART1_IRQHandler` → 对应 **IRQ37**。

C 侧强符号在 `src/main.c`：

- `TIM2_IRQHandler` → `tim2_irq_handler()`
- `USART1_IRQHandler` → `usart1_irq_handler()`

若某 IRQ 在 NVIC 已使能但向量仍指向 `Default_Handler`，会进入死循环 `b .`，排障时需检查向量表是否链接正确、符号名是否与启动文件一致。

---

## 5. NVIC 完整寄存器地图（Cortex-M3，常用）

基址 **`0xE000E100`** 起，除 `ISER` 外，固件库/手册中常见寄存器如下（**本项目代码里目前仅使用 ISER**）：

| 偏移 | 名称 | 作用简述 |
|------|------|----------|
| `0x000` | `ISER[0]` | 写 1 使能 IRQ 0～31 |
| `0x004` | `ISER[1]` | 写 1 使能 IRQ 32～63 |
| `0x080` | `ICER[0]` | 写 1 **禁用** IRQ 0～31 |
| `0x084` | `ICER[1]` | 写 1 **禁用** IRQ 32～63 |
| `0x100` | `ISPR[0]` | 写 1 **软件置** pending（一般少用） |
| `0x180` | `ICPR[0]` | 写 1 **清除** pending |
| `0x200` | `IABR[0]` | **只读**：是否 active（正在执行） |
| `0x300` 起 | `IPR[n]` | **优先级**，每个 IRQ 占 **1 字节**（STM32F103 通常只用高 4 位） |

**位宽**：每个 IRQ 在 `ISER/ICER/...` 中占 **1 bit**，编号 = `IRQn % 32`，寄存器索引 = `IRQn / 32`。

---

## 6. 优先级（IPR）与抢占（本项目未改默认值）

- **每个 IRQ 在 `IPR` 里有一个字节**；STM32F103 常实现 **4 位有效优先级**（具体以参考手册为准），其余位读为 0。
- **复位后**多数 IRQ 优先级相同；相同优先级时，**同优先级不嵌套**（谁先 pending 先处理，或按硬件仲裁顺序）。
- **优先级分组**由 **SCB 的 `AIRCR`**（应用中断与复位控制寄存器，`0xE000ED0C`）中的 **PRIGROUP** 字段决定：把 4 位分成“抢占优先级”和“子优先级”。本项目**未在代码中设置** `AIRCR`，即使用芯片默认分组。
- 若以后要设置 TIM2 高于 USART1，需要写 `NVIC_IPR` 相应字节，并理解当前 PRIGROUP 下抢占位与子优先级的划分。

---

## 7. 与 NVIC 协同的 CPU 特殊寄存器（概念）

| 名称 | 作用 |
|------|------|
| **PRIMASK** | 置 1 可屏蔽**除 NMI 与 HardFault 外**所有可配置优先级异常（总开关效果）。 |
| **FAULTMASK** | 更激进地屏蔽更多 fault 类异常（一般少用）。 |
| **BASEPRI** | 屏蔽**优先级数值大于等于**某阈值的中断（用于临界区精细控制）。 |

C 里常见 `__enable_irq()` / `__disable_irq()` 与 PRIMASK 相关。本项目驱动层未显式调用，默认上电后全局中断为允许状态（具体以启动与运行时库为准）。

---

## 8. SysTick 与 NVIC 的关系（本项目）

- **SysTick** 在向量表里是 **异常 15**（`SysTick_Handler`），**不是**某个 `IRQn` 去写 `ISER`。
- 本项目在 `systick_init_1ms()` 里配置 `SYST_RVR` / `SYST_CVR` / `SYST_CSR` 打开 SysTick **定时与 TICKINT**，见 `src/drivers/systick.c`。
- SysTick 的**优先级**在 **系统处理器优先级寄存器 `SHPR3`**（内核区域）里配置，**不是** NVIC 的 `IPR`；本项目未改 `SHPR`，使用默认优先级。

因此：**“开 NVIC”一词在本项目中主要指对 TIM2/USART1 写 `ISER`；SysTick 属于另一条路径。**

---

## 9. 排障清单（与本项目相关）

1. **IRQn 与 ISER 位算错**：USART1 是 **ISER1 bit 5**，不是 ISER0。
2. **只开了 NVIC 没开外设中断源**：无 `UIE` / `RXNEIE` 则不会来中断。
3. **pending 未清**：定时器更新标志未清可能一进中断就连续触发。
4. **向量名拼写错误**：必须与启动文件中的 `.word XXX_IRQHandler` 一致。
5. **总中断被关**：若某处长期 `__disable_irq()` 未配对打开，NVIC 使能也进不了 ISR。

---

## 10. 代码引用索引

- NVIC 宏与位定义：`include/bsp/stm32f103_regs.h`（`NVIC_BASE`、`NVIC_ISER0/1`、`NVIC_TIM2_IRQ_BIT`、`NVIC_USART1_IRQ_BIT`）。
- TIM2：`src/drivers/tim2.c`。
- USART1：`src/drivers/usart1.c` 中 `usart1_enable_rx_interrupt()`。
- 向量表与弱符号：`startup/startup_stm32f103c8tx.s`。
- Handler 转发：`src/main.c`。
