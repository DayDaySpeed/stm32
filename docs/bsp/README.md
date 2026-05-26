# BSP 层文档

BSP（Board Support Package）负责 **本板 PCB 相关的默认绑定**：引脚宏、外设时钟门控、逻辑设备名 → 具体驱动实例。应用层（`app`）应优先依赖本层，而非直接调用 `usart1_*`、`tim2_*` 等。

```
app/app.c
    ↓  bsp/board_devices.h（逻辑设备）
bsp/board_devices.c
    ↓  drivers/* + hal/*
bsp/board_pins.h / board_config.h / clock.c / board_init.c
    ↓
stm32f103_regs.h（寄存器）
```

## 文档列表

| 文档 | 内容 |
|------|------|
| [board_devices.md](./board_devices.md) | 逻辑设备 API、init 顺序、驱动映射表 |
| [board_config.md](./board_config.md) | 用户可调宏（编码器、红外、蜂鸣器等） |
| [clock.md](./clock.md) | BSP 时钟 API 与启动顺序（原理见 [topics/clock.md](../topics/clock.md)） |

## 源文件速查

| 文件 | 职责 |
|------|------|
| `include/bsp/board_devices.h` + `src/bsp/board_devices.c` | 逻辑设备公开 API |
| `include/bsp/board_pin_mux.h` | **GPIO 复用选项**（换引脚/解决冲突时改这里） |
| `include/bsp/board_pins.h` | 由 mux 解析出的语义化引脚宏（驱动消费） |
| `include/bsp/board_gpio.h` | GPIO 配置辅助宏（CRL/CRH/BSRR） |
| `include/bsp/board_config.h` | 板级可调参数（**改行为优先改这里**） |
| `include/bsp/board_init.h` + `src/bsp/board_init.c` | 一次性打开本板外设时钟 |
| `include/bsp/rcc_board.h` | APB1/APB2/AHB 时钟使能掩码组合 |
| `include/bsp/clock.h` + `src/bsp/clock.c` | 系统时钟 profile + 频率 getter |
| `include/bsp/stm32f103_regs.h` | 寄存器地址与位定义 |

## 本板引脚总览

| 功能 | 引脚 | 外设/模式 |
|------|------|-----------|
| 呼吸灯 | PA0 | TIM2_CH1 |
| 光敏 ADC | PA1 | ADC1_IN1 模拟 |
| 热敏 ADC | PA2 | ADC1_IN2 模拟 |
| 红外 ADC | PA3 | ADC1_IN3 模拟 |
| 蜂鸣器 | PA4 | GPIO 推挽 |
| 编码器 A/B | PA6/PA7 | TIM3 输入上拉 |
| LDR 指示 LED | PA8 | TIM1_CH1 |
| USART TX/RX | PA9/PA10 | USART1 |
| NTC 指示 LED | PA11 | TIM1_CH4 |
| OLED I2C | PB8/PB9 | I2C1 复用开漏 |
| 电机 AIN2/PWM/AIN1 | PB5/PB6/PB7 | GPIO + TIM4_CH1 |

引脚宏定义见 `include/bsp/board_pin_mux.h`（用户配置）→ `include/bsp/board_pins.h`（语义绑定）。

## 上电初始化顺序

`main.c` 中的推荐顺序（**顺序有意义，勿随意调换**）：

```
1. bsp_clock_apply_profile(...)     // 系统时钟
2. bsp_board_init()                 // 外设时钟门控
3. bsp_dc_motor_gpio_safe_early()   // 电机 GPIO 安全态（防误转）
4. app_init()
   ├─ systick_init_1ms()
   └─ bsp_default_devices_init()    // 全部逻辑设备
5. app_run_forever()
```

中断向量（同文件）：

- `SysTick_Handler` → `systick_on_interrupt()`
- `USART1_IRQHandler` → `bsp_console_irq_handler()`

## 与 drivers / hal 的分工

| 层 | 知道什么 | 不知道什么 |
|----|----------|------------|
| **drivers** | 外设寄存器怎么配 | 本板默认用哪个引脚/实例 |
| **hal** | 总线事务（I2C 写帧） | 具体从设备协议 |
| **bsp** | 本板引脚、默认参数、逻辑名 | 应用业务状态机 |
| **app** | 任务调度、用户交互 | 寄存器细节 |

换板子时：改 `board_pin_mux.h`、`board_config.h`、`board_devices.c` 里的静态 config；驱动层尽量不动。

## 相关文档

- [驱动文档](../drivers/README.md)
- [HAL 层](../hal/README.md)
- [驱动接口约定](../DRIVER_API_GUIDE.md)
- 时钟原理专题：[topics/clock.md](../topics/clock.md)

---

# English

# BSP Layer Documentation

The BSP (Board Support Package) handles **default bindings specific to this PCB**: pin macros, peripheral clock gating, and logical device names mapped to concrete driver instances. The application layer (`app`) should depend on this layer first, rather than calling `usart1_*`, `tim2_*`, and similar APIs directly.

```
app/app.c
    ↓  bsp/board_devices.h (logical devices)
bsp/board_devices.c
    ↓  drivers/* + hal/*
bsp/board_pins.h / board_config.h / clock.c / board_init.c
    ↓
stm32f103_regs.h (registers)
```

## Document Index

| Document | Contents |
|----------|----------|
| [board_devices.md](./board_devices.md) | Logical device API, init order, driver mapping table |
| [board_config.md](./board_config.md) | User-tunable macros (encoder, IR, buzzer, etc.) |
| [clock.md](./clock.md) | BSP clock API and startup order (theory in [topics/clock.md](../topics/clock.md)) |

## Source File Quick Reference

| File | Responsibility |
|------|----------------|
| `include/bsp/board_devices.h` + `src/bsp/board_devices.c` | Logical device public API |
| `include/bsp/board_pin_mux.h` | **GPIO mux options** (change pins here when retargeting) |
| `include/bsp/board_pins.h` | Semantic pin macros resolved from mux (consumed by drivers) |
| `include/bsp/board_gpio.h` | GPIO helper macros (CRL/CRH/BSRR) |
| `include/bsp/board_config.h` | Board-level tunable parameters (**change behavior here first**) |
| `include/bsp/board_init.h` + `src/bsp/board_init.c` | One-shot enable of this board's peripheral clocks |
| `include/bsp/rcc_board.h` | APB1/APB2/AHB clock enable mask combinations |
| `include/bsp/clock.h` + `src/bsp/clock.c` | System clock profile + frequency getters |
| `include/bsp/stm32f103_regs.h` | Register addresses and bit definitions |

## Board Pin Overview

| Function | Pin | Peripheral / Mode |
|----------|-----|-------------------|
| Breathing LED | PA0 | TIM2_CH1 |
| Light sensor ADC | PA1 | ADC1_IN1 analog |
| Thermistor ADC | PA2 | ADC1_IN2 analog |
| IR ADC | PA3 | ADC1_IN3 analog |
| Buzzer | PA4 | GPIO push-pull |
| Encoder A/B | PA6/PA7 | TIM3 input pull-up |
| LDR indicator LED | PA8 | TIM1_CH1 |
| USART TX/RX | PA9/PA10 | USART1 |
| NTC indicator LED | PA11 | TIM1_CH4 |
| OLED I2C | PB8/PB9 | I2C1 alternate-function open-drain |
| Motor AIN2/PWM/AIN1 | PB5/PB6/PB7 | GPIO + TIM4_CH1 |

Pin macros: configure in `include/bsp/board_pin_mux.h` (user settings) → resolved in `include/bsp/board_pins.h` (semantic bindings).

## Power-On Initialization Order

Recommended sequence in `main.c` (**order matters; do not rearrange casually**):

```
1. bsp_clock_apply_profile(...)     // system clock
2. bsp_board_init()                 // peripheral clock gating
3. bsp_dc_motor_gpio_safe_early()   // motor GPIO safe state (prevent unintended rotation)
4. app_init()
   ├─ systick_init_1ms()
   └─ bsp_default_devices_init()    // all logical devices
5. app_run_forever()
```

Interrupt vectors (same file):

- `SysTick_Handler` → `systick_on_interrupt()`
- `USART1_IRQHandler` → `bsp_console_irq_handler()`

## Division of Responsibility: drivers / hal / bsp

| Layer | Knows | Does Not Know |
|-------|-------|---------------|
| **drivers** | How to configure peripheral registers | Which pin/instance this board uses by default |
| **hal** | Bus transactions (I2C write frames) | Specific slave device protocols |
| **bsp** | This board's pins, default parameters, logical names | Application business state machines |
| **app** | Task scheduling, user interaction | Register details |

When retargeting the board: change `board_pin_mux.h`, `board_config.h`, and static configs in `board_devices.c`; keep the driver layer unchanged where possible.

## Related Documentation

- [Driver Documentation](../drivers/README.md)
- [HAL Layer](../hal/README.md)
- [Driver Interface Conventions](../DRIVER_API_GUIDE.md)
- Clock theory topic: [topics/clock.md](../topics/clock.md)
