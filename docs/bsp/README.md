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
| [clock.md](./clock.md) | BSP 时钟 API 与启动顺序（原理见根目录专题） |

## 源文件速查

| 文件 | 职责 |
|------|------|
| `include/bsp/board_devices.h` + `src/bsp/board_devices.c` | 逻辑设备公开 API |
| `include/bsp/board_config.h` | 板级可调参数（**改板子优先改这里**） |
| `include/bsp/board_pins.h` | GPIO CRL/CRH 位域宏、引脚号常量 |
| `include/bsp/board_init.h` + `src/bsp/board_init.c` | 一次性打开本板外设时钟 |
| `include/bsp/rcc_board.h` | APB1/APB2/AHB 时钟使能掩码组合 |
| `include/bsp/clock.h` + `src/bsp/clock.c` | 系统时钟 profile + 频率 getter |
| `include/bsp/stm32f103_regs.h` | 寄存器地址与位定义 |

## 本板引脚总览

| 功能 | 引脚 | 外设/模式 |
|------|------|-----------|
| 状态 LED PWM | PA0 | TIM2_CH1 |
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

引脚宏定义与 CRL/CRH 字段值见 `include/bsp/board_pins.h`（含注释说明 MODE/CNF 编码）。

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

换板子时：改 `board_pins.h`、`board_config.h`、`board_devices.c` 里的静态 config；驱动层尽量不动。

## 相关文档

- [驱动文档](../drivers/README.md)
- [HAL 层](../hal/README.md)
- [驱动接口约定](../DRIVER_API_GUIDE.md)
- 时钟原理专题：[clock.md](../../clock.md)
