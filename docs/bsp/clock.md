# BSP 时钟 API

本工程时钟分两层：

- **BSP API**（本文）：`bsp/clock.h` — profile 切换与频率查询  
- **原理专题**（推荐阅读）：[topics/clock.md](../topics/clock.md) — 时钟树、RCC/FLASH/SysTick/TIM 寄存器细节

---

## 文件

| 文件 | 职责 |
|------|------|
| `include/bsp/clock.h` | 公开 API 与 `BSP_SYSTICK_RELOAD_1MS` 宏 |
| `src/bsp/clock.c` | HSI 8MHz / HSE+PLL 72MHz 切换实现 |

---

## Profile API

```c
typedef enum {
  BSP_CLOCK_PROFILE_HSI_8MHZ = 0,
  BSP_CLOCK_PROFILE_HSE_PLL_72MHZ = 1
} bsp_clock_profile_t;

stm_status_t bsp_clock_apply_profile(bsp_clock_profile_t profile);
```

| Profile | 结果 | 典型用途 |
|---------|------|----------|
| `HSI_8MHZ` | 内部 8MHz，无 PLL | 无晶振调试、最低功耗 |
| `HSE_PLL_72MHZ` | 外部 8MHz × PLL9 → 72MHz | **本板默认**（`main.c`） |

失败返回 `STM_ERR_TIMEOUT`（HSE/PLL 未就绪）或 `STM_ERR_INVALID_ARG`。

`main.c` 在失败时调用 `stm_fault_halt("clock", st)` 停机。

---

## 频率 Getter

切换成功后，`clock.c` 内静态缓存更新，驱动通过 getter 查询，**禁止硬编码 72000000**。

| 函数 | 含义 |
|------|------|
| `bsp_clock_get_sysclk_hz()` | 系统时钟 |
| `bsp_clock_get_hclk_hz()` | AHB / CPU / SysTick |
| `bsp_clock_get_pclk1_hz()` | APB1（I2C1、TIM2/3/4） |
| `bsp_clock_get_pclk2_hz()` | APB2（USART1、TIM1、ADC） |
| `bsp_clock_get_apb1_timer_hz()` | APB1 定时器实际时钟（预分频≠1 时 ×2） |
| `bsp_clock_get_apb2_timer_hz()` | APB2 定时器实际时钟（同上） |

### 为什么有 timer_hz

STM32F1 规定：当 APB 预分频不为 1 时，挂在该总线上的定时器时钟 **自动 ×2**。

本板 72MHz 配置下：

```
HCLK = 72MHz
PCLK1 = 36MHz  →  TIM2/3/4 时钟 = 72MHz
PCLK2 = 72MHz  →  TIM1 时钟 = 72MHz
```

PWM 相关驱动（`breathing_led`、`dc_motor`、`sensor_led`）和 `common/tim_timebase.c` 依赖上述 getter 反推 PSC/ARR。

---

## SysTick 宏

```c
#define BSP_SYSTICK_RELOAD_1MS ((bsp_clock_get_hclk_hz() / 1000UL) - 1UL)
```

`systick_init_1ms()` 使用此宏。须在 `bsp_clock_apply_profile()` **之后**调用。

---

## 与 board_init 的关系

```
bsp_clock_apply_profile()   // 先定频率
bsp_board_init()            // 再开外设时钟门控（rcc_board.h 掩码）
驱动 init                   // 用 getter 算 BRR/PSC/ARR
```

`bsp_board_init()` **不**改系统频率，只写 `RCC_*ENR` 使能位：

- AHB：`DMA1`（ADC DMA）
- APB2：AFIO、GPIOA/B、USART1、ADC1、TIM1
- APB1：I2C1、TIM2、TIM3、TIM4

详见 `include/bsp/rcc_board.h`。

---

## 驱动依赖示例

| 驱动 | 使用的时钟 API |
|------|----------------|
| usart1 | `bsp_clock_get_pclk2_hz()` → BRR |
| breathing_led / dc_motor | `bsp_clock_get_apb1_timer_hz()` |
| sensor_led | `bsp_clock_get_apb2_timer_hz()` |
| adc1_dual | `bsp_clock_get_pclk2_hz()` → ADCPRE |
| i2c1_master | `bsp_clock_get_pclk1_hz()` → CCR |
| systick | `bsp_clock_get_hclk_hz()` via 宏 |

---

## 常见坑

- **顺序错误**：未 `apply_profile` 就 init USART/TIM → 波特率/PWM 频率全错。  
- **HSI 下调试 HSE 参数**：无 8MHz 晶振时 `HSE_PLL_72MHZ` 会 timeout。  
- **改 PLL 后忘记改 Flash LATENCY**：72MHz 需 `FLASH_ACR_LATENCY_2`（已在 `apply_hse_pll_72mhz` 内处理）。  

## 延伸阅读

- [topics/clock.md](../topics/clock.md) — 完整时钟树、寄存器逐步配置、排障  
- [bsp/README.md](./README.md) — 上电 init 顺序  
- [drivers/systick.md](../drivers/systick.md) — 毫秒节拍

---

# English

# BSP Clock API

Clock handling in this project is split into two layers:

- **BSP API** (this document): `bsp/clock.h` — profile switching and frequency queries  
- **Theory topic** (recommended reading): [topics/clock.md](../topics/clock.md) — clock tree, RCC/FLASH/SysTick/TIM register details

---

## Files

| File | Responsibility |
|------|----------------|
| `include/bsp/clock.h` | Public API and `BSP_SYSTICK_RELOAD_1MS` macro |
| `src/bsp/clock.c` | HSI 8 MHz / HSE+PLL 72 MHz profile switching implementation |

---

## Profile API

```c
typedef enum {
  BSP_CLOCK_PROFILE_HSI_8MHZ = 0,
  BSP_CLOCK_PROFILE_HSE_PLL_72MHZ = 1
} bsp_clock_profile_t;

stm_status_t bsp_clock_apply_profile(bsp_clock_profile_t profile);
```

| Profile | Result | Typical Use |
|---------|--------|-------------|
| `HSI_8MHZ` | Internal 8 MHz, no PLL | Debug without crystal, lowest power |
| `HSE_PLL_72MHZ` | External 8 MHz × PLL9 → 72 MHz | **This board default** (`main.c`) |

On failure returns `STM_ERR_TIMEOUT` (HSE/PLL not ready) or `STM_ERR_INVALID_ARG`.

`main.c` calls `stm_fault_halt("clock", st)` on failure.

---

## Frequency Getters

After a successful switch, static cache in `clock.c` is updated; drivers query via getters — **do not hard-code 72000000**.

| Function | Meaning |
|----------|---------|
| `bsp_clock_get_sysclk_hz()` | System clock |
| `bsp_clock_get_hclk_hz()` | AHB / CPU / SysTick |
| `bsp_clock_get_pclk1_hz()` | APB1 (I2C1, TIM2/3/4) |
| `bsp_clock_get_pclk2_hz()` | APB2 (USART1, TIM1, ADC) |
| `bsp_clock_get_apb1_timer_hz()` | Actual APB1 timer clock (×2 when prescaler ≠ 1) |
| `bsp_clock_get_apb2_timer_hz()` | Actual APB2 timer clock (same rule) |

### Why timer_hz Exists

On STM32F1, when the APB prescaler is not 1, timer clocks on that bus are **automatically doubled**.

Under this board's 72 MHz configuration:

```
HCLK = 72 MHz
PCLK1 = 36 MHz  →  TIM2/3/4 clock = 72 MHz
PCLK2 = 72 MHz  →  TIM1 clock = 72 MHz
```

PWM drivers (`breathing_led`, `dc_motor`, `sensor_led`) and `common/tim_timebase.c` use these getters to derive PSC/ARR.

---

## SysTick Macro

```c
#define BSP_SYSTICK_RELOAD_1MS ((bsp_clock_get_hclk_hz() / 1000UL) - 1UL)
```

Used by `systick_init_1ms()`. Must be called **after** `bsp_clock_apply_profile()`.

---

## Relationship with board_init

```
bsp_clock_apply_profile()   // set frequency first
bsp_board_init()            // then enable peripheral clock gating (rcc_board.h masks)
driver init                 // use getters to compute BRR/PSC/ARR
```

`bsp_board_init()` does **not** change system frequency; it only writes `RCC_*ENR` enable bits:

- AHB: `DMA1` (ADC DMA)
- APB2: AFIO, GPIOA/B, USART1, ADC1, TIM1
- APB1: I2C1, TIM2, TIM3, TIM4

See `include/bsp/rcc_board.h` for details.

---

## Driver Dependency Examples

| Driver | Clock API Used |
|--------|----------------|
| usart1 | `bsp_clock_get_pclk2_hz()` → BRR |
| breathing_led / dc_motor | `bsp_clock_get_apb1_timer_hz()` |
| sensor_led | `bsp_clock_get_apb2_timer_hz()` |
| adc1_dual | `bsp_clock_get_pclk2_hz()` → ADCPRE |
| i2c1_master | `bsp_clock_get_pclk1_hz()` → CCR |
| systick | `bsp_clock_get_hclk_hz()` via macro |

---

## Common Pitfalls

- **Wrong order**: Init USART/TIM before `apply_profile` → baud rate/PWM frequency all wrong.  
- **Debugging HSE parameters on HSI**: Without 8 MHz crystal, `HSE_PLL_72MHZ` will timeout.  
- **Forgot Flash LATENCY after PLL change**: 72 MHz requires `FLASH_ACR_LATENCY_2` (already handled inside `apply_hse_pll_72mhz`).  

## Further Reading

- [topics/clock.md](../topics/clock.md) — full clock tree, step-by-step register configuration, troubleshooting  
- [bsp/README.md](./README.md) — power-on init order  
- [drivers/systick.md](../drivers/systick.md) — millisecond tick
