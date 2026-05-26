# 基于 CMake 裸机架构的多模态智能交互演示平台

## 设计报告

---

| 项目 | 内容 |
|------|------|
| 作品名称 | 基于 CMake 裸机架构的多模态智能交互演示平台 |
| 参赛类别 | 校赛 · 创新创意 |
| 主控芯片 | STM32F103C8T6（ARM Cortex-M3，72 MHz） |
| 开发方式 | 寄存器级裸机 + CMake + arm-none-eabi |
| 作者 | （填写姓名） |
| 指导教师 | （填写姓名） |
| 所在院系 | （填写院系/班级） |
| 完成日期 | 2026 年 5 月 |

---

## 摘要

针对高校嵌入式实践与竞赛项目中普遍存在的 IDE 绑定强、HAL 封装黑盒化、模块耦合高、工程难以复现等问题，本文设计并实现了一套运行在 STM32F103C8T6 上的**多模态智能交互演示平台**。系统采用**寄存器级裸机编程**，不依赖 STM32Cube HAL；通过 **CMake 构建体系**实现跨平台编译、固件体积统计、OpenOCD 一键烧录及静态检查；在软件架构上建立 **app → BSP → drivers → HAL** 分层模型与统一驱动接口规范，应用层采用基于 SysTick 的**合作式非阻塞任务调度**，集成编码器调速、直流电机驱动、三路 ADC 扫描采集、OLED/串口双通道人机交互、红外接近检测与蜂鸣告警等功能。

测试表明，Debug 构建固件占用 Flash 约 **19.4 KB**（text 19332 B），RAM 约 **3.4 KB**（data+bss 3376 B），在 64 KB Flash / 20 KB RAM 资源约束下仍有充足扩展空间；平台运行稳定，交互响应及时，具备模块可插拔扩展与教学演示价值，为裸机嵌入式项目的现代化工程实践提供了可复用范例。

**关键词：** STM32；裸机开发；CMake；分层架构；智能交互；合作式调度

---

## Abstract

This project presents a **multimodal intelligent interaction demonstration platform** on the STM32F103C8T6 microcontroller. Instead of relying on STM32Cube HAL or IDE-locked toolchains, the firmware is built with **register-level bare-metal code** and a **CMake-based workflow** using the arm-none-eabi toolchain. A layered software architecture (application, BSP, drivers, HAL) with unified `stm_status_t` APIs enables maintainable integration of motor control, quadrature encoder input, triple-channel ADC scan with DMA, OLED and UART interaction, and IR proximity alerting. A cooperative, non-blocking task scheduler based on SysTick coordinates all interaction paths without an RTOS.

In Debug build, the firmware consumes about **19.4 KB** of Flash and **3.4 KB** of RAM, leaving ample headroom on the 64 KB / 20 KB device. The platform demonstrates stable runtime behavior and serves as a reusable reference for modern bare-metal embedded engineering in teaching and innovation competitions.

**Keywords:** STM32; bare-metal; CMake; layered architecture; human-machine interaction; cooperative scheduling

---

# 第一章 绪论

## 1.1 研究背景

STM32 系列微控制器广泛应用于电子设计竞赛、课程设计与创新实践。多数参赛或教学项目采用 Keil、STM32CubeMX 配合 HAL 库开发，虽上手快，但存在以下共性问题：

1. **工程绑定 IDE**：项目文件难以跨平台协作，版本管理与自动化构建不便；
2. **HAL 黑盒化**：学生难以理解寄存器与外设时序，答辩时难以体现底层掌握程度；
3. **模块接口不统一**：各驱动返回值、初始化方式、阻塞/非阻塞语义不一致，应用层耦合严重；
4. **功能堆砌缺乏叙事**：传感器、电机、显示等模块各自独立，缺少「智能交互」的系统闭环。

与此同时，CMake 在服务器、桌面与移动开发领域已是事实标准，但在校赛 STM32 项目中仍较少被系统化采用。将 CMake 引入 MCU 裸机开发，有望把「能跑的单片机程序」提升为「可维护、可协作、可复现的嵌入式工程」。

## 1.2 设计目的

本项目旨在：

1. 构建一套**多模态智能交互演示平台**，验证多种输入（编码器、串口、ADC 传感器、红外接近）与多种输出（电机、OLED、蜂鸣器、PWM 指示 LED）的协同工作；
2. 探索 **CMake + 寄存器裸机** 在校赛场景下的工程化实践路径；
3. 建立可扩展的分层软件架构，使后续功能（如 PID 温控、循迹、无线通信）可在不推翻现有代码的前提下增量开发。

## 1.3 主要创新点

| 序号 | 创新点 | 说明 |
|------|--------|------|
| 1 | CMake 裸机工程化 | 文本化构建脚本、跨平台编译、POST_BUILD 导出 hex/bin、OpenOCD 烧录 target、clang-format/cppcheck 质量目标 |
| 2 | 分层架构 + 逻辑设备抽象 | 应用层通过 `bsp/board_devices.h` 访问 console/display/motor 等逻辑角色，不直接绑定 USART1/TIM2 等硬件实例 |
| 3 | 统一驱动 API 规范 | 全局 `stm_status_t` 错误语义，`*_init_with_config()` + `*_try()` / `*_blocking()` 命名约定 |
| 4 | 合作式非阻塞调度 | 基于 `systick_get_ms()` 的多任务时间片，避免阻塞式等待拖垮交互响应 |
| 5 | 多模态交互闭环 | 编码器连续量 + 串口离散文本 + 环境传感 + 红外事件，统一由 OLED/串口/执行器反馈 |

## 1.4 报告结构

第二章给出功能与非功能需求；第三章描述系统总体架构；第四章详述软件设计与 CMake 工程体系（本文重点）；第五章简述硬件平台；第六章给出测试方案与结果；第七章总结并展望。

---

# 第二章 需求分析

## 2.1 功能需求

根据校赛「创新创意」展示目标，平台需具备可现场演示、可讲解、可扩展的交互能力，具体功能需求如下。

| 编号 | 功能 | 描述 | 实现模块 |
|------|------|------|----------|
| F1 | 状态呼吸灯 | PA0 TIM2 PWM 三角波占空比，周期约 4.8 s | `breathing_led` + `app_breath_led_task` |
| F2 | 编码器调速 | 旋钮增量调节电机速度，范围 −1000～+1000（千分比） | `encoder` + `dc_motor` |
| F3 | 电机正反转 | TB6612 驱动，PB6 PWM + PB5/PB7 方向 | `dc_motor` |
| F4 | 多路传感采集 | ADC1 三路 SCAN+DMA：光敏 PA1、NTC PA2、红外 PA3 | `adc1_dual_scan_dma` |
| F5 | OLED 调试显示 | I2C1 SSD1306 128×64，分页显示 ENC/MOT/LED/LDR/NTC/IR | `ssd1306_oled` |
| F6 | 串口控制台 | USART1 115200，RX 中断 + 行读取，输入文本写入 OLED | `usart1` |
| F7 | 红外接近交互 | 手靠近 TCRT5000 类模块时蜂鸣，带防抖与冷却 | `ir_reflect` + `buzzer` |
| F8 | 传感器指示 LED | 光敏/NTC 映射到 TIM1 两路 PWM 指示亮度 | `sensor_led` |
| F9 | 显示故障恢复 | OLED I2C 连续失败时自动 re-init | `app_oled_try_recover` |

## 2.2 非功能需求

| 编号 | 类别 | 要求 |
|------|------|------|
| NF1 | 可构建性 | Linux 下 cmake 两行命令完成配置与编译 |
| NF2 | 可移植性 | 换板仅修改 BSP 与 `board_config.h`，应用层尽量不动 |
| NF3 | 实时性 | 主循环非阻塞；呼吸灯步进 12 ms；电机 20 ms；OLED 500 ms 节流 |
| NF4 | 可靠性 | 红外检测需 arm/leave 状态机防抖；OLED 失败可恢复 |
| NF5 | 可诊断性 | 统一错误码；可选 DEBUG_LOG；编译后打印固件体积 |
| NF6 | 资源约束 | Flash < 64 KB，RAM < 20 KB（C8T6 规格） |

## 2.3 用户场景

**场景 A — 旋钮调速演示**  
评委旋转编码器，OLED 第 0～1 行实时显示 ENC 计数与 MOT 速度，电机转速随之变化。

**场景 B — 环境传感演示**  
遮挡光敏电阻或手握 NTC，OLED 显示 LDR 电压与 NTC 温度，对应指示 LED 亮度变化。

**场景 C — 红外接近交互**  
手靠近反射红外模块，蜂鸣器短鸣（80 ms，冷却 800 ms），OLED 第 5 行 IR 原始值明显下降。

**场景 D — 串口文本交互**  
PC 串口工具输入一行文本并回车，控制台回显 `recv:`，同时写入 OLED 空闲页。

---

# 第三章 系统总体设计

## 3.1 设计原则

1. **分层解耦**：应用不关心引脚与实例，BSP 负责板级绑定，驱动负责寄存器语义；
2. **配置集中**：阈值、极性、方向等可调参数置于 `board_config.h`；
3. **失败可感知**：对外 API 返回 `stm_status_t`，便于日志与恢复策略；
4. **非阻塞优先**：主循环可并发推进多路交互，避免 `while` 空等外设。

## 3.2 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (src/app)                      │
│  合作式调度 · 呼吸灯 · 编码器调速 · 传感显示 · 红外交互   │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│                 板级层 (src/bsp)                         │
│  board_init · clock · board_devices（逻辑设备入口）       │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│              驱动层 (src/drivers) + HAL (src/hal)        │
│  USART1 · SSD1306 · ADC1 DMA · TIM PWM · encoder · …    │
│  I2C1 主机事务 (hal/i2c1_master)                         │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│         公共层 (src/common) + 寄存器映射 (bsp/regs)       │
│  stm_status · ring_buffer · systick · stm_log · 断言     │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│                      硬件平台                              │
│  STM32F103C8T6 + OLED + 电机驱动 + 传感器模块 + 串口模块   │
└─────────────────────────────────────────────────────────┘
```

## 3.3 软件启动流程

```
main
 ├─ bsp_clock_apply_profile(HSE_PLL_72MHZ)   // 外部晶振 PLL 至 72 MHz
 ├─ bsp_board_init()                         // 外设时钟门控
 ├─ bsp_dc_motor_gpio_safe_early()           // 电机安全态（防上电乱转）
 ├─ app_init()
 │    ├─ systick_init_1ms()
 │    └─ bsp_default_devices_init()           // 初始化全部逻辑设备
 └─ app_run_forever()                         // 合作式任务 + 串口行处理
```

**中断分工：**

- `SysTick_Handler` → 毫秒节拍 `systick_get_ms()`；
- `USART1_IRQHandler` → RX 字节入环形缓冲，供 `read_line_try` 组行。

## 3.4 目录结构

| 路径 | 职责 |
|------|------|
| `src/app/` | 业务逻辑、任务调度 |
| `src/bsp/` | 板级 init、时钟、逻辑设备封装 |
| `src/drivers/` | 外设驱动实现 |
| `src/hal/` | I2C 等总线级事务 |
| `src/common/` | 状态码、缓冲、日志、断言 |
| `include/` | 与 src 对应的分层头文件 |
| `cmake/stm32_sources.cmake` | 固件源文件清单 |
| `startup/` | 启动汇编与向量表 |
| `linker/` | 链接脚本（64K Flash / 20K RAM） |
| `docs/` | 项目文档（含本设计报告） |

---

# 第四章 软件设计

## 4.1 CMake 裸机构建体系（核心创新）

### 4.1.1 设计动机

传统 Keil 工程依赖图形界面与本地路径，不利于：

- Git 协作与 Code Review；
- 在 Linux CI 或笔记本上复现构建；
- 统一编译选项与后处理（hex/bin/map）。

本项目以 **CMake 3.20+** 为构建入口，配合 **arm-none-eabi-gcc** 工具链，实现与操作系统无关的固件构建流程。

### 4.1.2 关键 CMake 配置

**（1）裸机编译探测**

```cmake
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
```

避免 CMake 在检测编译器时尝试链接宿主可执行文件（裸机环境无标准 libc 链接模型）。

**（2）MCU 与优化选项**

- 目标内核：`-mcpu=cortex-m3 -mthumb -mfloat-abi=soft`
- 体积优化：`-ffunction-sections -fdata-sections` + 链接 `-Wl,--gc-sections`
- 告警级别：`-Wall -Wextra -Wpedantic`

**（3）可配置编译开关**

| CMake 选项 | 作用 |
|------------|------|
| `DEBUG_LOG` | 启用轻量 debug 日志 |
| `ASSERT_LEVEL` | 断言严格级别 0～2 |
| `OLED_REFRESH_MODE` | OLED 刷新策略 AUTO/FULL/REGION |

**（4）构建产物与体积统计**

链接完成后 POST_BUILD 自动：

1. `objcopy` 生成 `.hex` / `.bin`；
2. `arm-none-eabi-size` 打印 text/data/bss。

**（5）工程化自定义目标**

| Target | 命令 | 用途 |
|--------|------|------|
| 默认 build | `cmake --build build` | 编译固件 |
| `flash` | OpenOCD + ST-Link | 一键烧录 |
| `format` | clang-format | 代码风格统一 |
| `lint` | cppcheck | 静态分析 |

**（6）源文件清单集中维护**

新增驱动模块时，仅需在 `cmake/stm32_sources.cmake` 追加 `.c` 文件，避免在多个 IDE 工程里重复勾选。

### 4.1.3 典型构建命令

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
cmake --build build --target flash    # 需 OpenOCD + ST-Link
```

### 4.1.4 创新价值总结

CMake 在本项目中不仅是「换了个编译方式」，而是把嵌入式开发纳入**现代软件工程流程**：可脚本化、可审计、可扩展，与分层源码结构形成呼应，构成完整的「裸机工程化平台」叙事。

---

## 4.2 分层架构与逻辑设备抽象

### 4.2.1 层间依赖规则

- **app 层** 只 include `bsp/board_devices.h`，调用 `bsp_console_*`、`bsp_display_*`、`bsp_dc_motor_*` 等；
- **drivers 层** 实现外设语义（如 `usart1_init_with_config`），不决定「本板用哪个实例」；
- **bsp 层** 在 `board_devices.c` 中完成实例绑定与默认 config 结构体填充。

示例：`board_devices.c` 将控制台绑定为 USART1、115200、CR/LF 行策略；状态灯绑定 TIM2 1 kHz PWM；电机绑定 TIM4 10 kHz PWM。

### 4.2.2 板级配置集中化

用户可调参数集中于 `include/bsp/board_config.h`，无需修改驱动或 app 源码：

| 参数 | 作用 |
|------|------|
| `BOARD_WHEEL_ENCODER_DIRECTION` | 编码器 A/B 计数方向 |
| `BOARD_MOTOR_REVERSE_SIGN` | 电机正反转与旋钮语义对齐 |
| `BOARD_BUZZER_ACTIVE_HIGH` | 有源蜂鸣器有效电平 |
| `BOARD_IR_*` | 红外 arm/near/leave 阈值与 streak 防抖 |
| `BOARD_NTC_LED_FULL_TEMP_X10` | NTC 指示 LED 满亮度温度 |

体现「**平台化**」：同一套 app/drivers，换板或调参只改 BSP 配置。

---

## 4.3 统一驱动 API 规范

项目采用教学型工业级接口约定（详见 `docs/DRIVER_API_GUIDE.md`）：

### 4.3.1 返回值 `stm_status_t`

| 状态码 | 含义 |
|--------|------|
| `STM_OK` | 成功 |
| `STM_ERR_INVALID_ARG` | 参数非法 |
| `STM_ERR_BUSY` | 非阻塞接口暂无数据 |
| `STM_ERR_TIMEOUT` | 硬件等待超时 |
| `STM_ERR_OVERFLOW` | 缓冲区溢出 |
| `STM_ERR_IO` | 总线/外设 IO 失败 |
| `STM_ERR_NOT_INITIALIZED` | 模块未初始化 |

### 4.3.2 命名约定

- 初始化：`module_init_with_config(const config_t *)`
- 非阻塞读：`*_read_*_try()` → 无数据返回 `STM_ERR_BUSY`
- 阻塞写：`*_write_*_blocking()`

应用层典型用法：

```c
stm_status_t st = bsp_console_read_line_try(line, sizeof(line));
if (st == STM_OK) {
  /* 处理完整一行 */
} else if (st == STM_ERR_BUSY) {
  /* 无输入，继续其他任务 */
}
```

该规范使多个驱动「同一套脾气」，降低 app 层认知负担，也便于答辩时讲解工程化思维。

---

## 4.4 合作式任务调度设计

### 4.4.1 调度模型

本工程**不使用 RTOS**，而在 `app_run_forever()` 主循环中调用 `tasks()`，以 `systick_get_ms()` 为统一时间基准，各子任务自行节流。

| 任务 | 周期 | 功能 |
|------|------|------|
| `app_breath_led_task` | 12 ms | 呼吸灯相位步进 |
| `app_motor_encoder_task` | 20 ms | 读编码器增量，更新电机速度 |
| `app_ir_proximity_buzzer_task` | 80 ms | 红外采样与接近判定 |
| `bsp_sensor_led_update_from_sensors` | 100 ms | 光敏/NTC 指示 LED |
| OLED 调试刷新 | 500 ms | ENC/MOT/LED + LDR/NTC/IR 文本 |

串口行处理与上述任务**同循环并发**：每圈调用 `bsp_console_read_line_try`，不阻塞等待。

### 4.4.2 呼吸灯算法

相位 `0 .. PHASE_MAX-1`（PHASE_MAX=400）构成三角波，映射为 0～1000 千分比占空比：

- 半周期 200 步 × 12 ms ≈ 2.4 s 渐亮/渐暗；
- 全周期约 **4.8 s**，视觉连续平滑。

### 4.4.3 红外接近状态机

本板 TCRT5000 类模块：**远离 raw ≈ 4000，靠近 raw ≈ 100**。

为避免上电误触发与 ADC 毛刺，采用三阶段逻辑：

1. **Arm 阶段**：须连续 5 次读到 raw ≥ 3000，才允许后续靠近检测；
2. **Near 判定**：raw ≤ 500 且连续 3 次 → 触发蜂鸣（冷却 800 ms）；
3. **Leave 判定**：raw ≥ 3500 且连续 2 次 → 清除「手靠近」状态。

### 4.4.4 OLED 故障恢复

连续 2 次 OLED 写失败且距上次恢复 ≥ 1 s，调用 `bsp_display_recover()` 重新初始化 I2C+SSD1306，提升现场演示鲁棒性。

### 4.4.5 性能权衡

电机占空比绝对值 > 600 时，跳过 LDR/NTC/IR 的 OLED 行刷新，减轻主循环负载，避免满速时拖慢呼吸灯与编码器响应——体现针对 MCU 资源的** consciously 设计**。

---

## 4.5 关键模块设计

### 4.5.1 ADC1 三路 SCAN + DMA

- 通道：PA1 光敏、PA2 NTC、PA3 反射红外；
- 模式：扫描 + DMA 循环，应用层通过 `bsp_analog_sensors_read_all_average()` 一次读取三路并做 4 次平均；
- NTC 温度：10 kΩ 上拉 + NTC 下接分压，Steinhart-Hart 或查表换算为 0.1°C 精度显示。

### 4.5.2 USART1 串口控制台

- 115200 8N1，PA9/PA10；
- RX 中断 + 环形缓冲；
- 行策略：CR 或 LF 结束，`read_line_try` 在主循环组包；
- 与 `stm_log` 集成，可选 debug 输出。

### 4.5.3 SSD1306 OLED（I2C1）

- 软件 I2C 或 HAL 层 `i2c1_master` 事务；
- 128×64，8 页 × 8 像素高；
- 支持按页 `write_text_atf` 格式化字符串；
- 刷新模式可通过 CMake 选项配置。

### 4.5.4 编码器与电机

- TIM3 正交编码模式，PA6/PA7；
- TIM4 PWM 10 kHz 驱动 TB6612；
- 速度语义：−1000～+1000 千分比，过零自动换向；
- 编码器每步 8 千分比，可在 `app.c` 调整灵敏度。

---

## 4.6 模块扩展方法（平台化示例）

若新增 DHT11 温湿度模块，推荐步骤：

1. 在 `src/drivers/` 新增 `dht11.c/.h`，实现 `dht11_init_with_config` 等 API；
2. 在 `board_devices.c` 增加 `bsp_humidity_*` 逻辑封装；
3. 在 `cmake/stm32_sources.cmake` 注册源文件；
4. 在 `app.c` 增加节流任务，OLED 新页显示。

**应用层与 CMake 均无需重构**，体现平台扩展性，可作为答辩「未来工作已具备基础」的依据。

---

# 第五章 硬件设计

## 5.1 设计说明

本作品定位为**智能交互演示平台**，硬件采用成熟模块 + 杜邦线/PCB 原型方式搭建，重点验证软件架构与外设协同。**创新侧重在软件工程与裸机架构**，硬件承担「多模态 IO 验证载体」角色，符合校赛创新创意类「重思想、重实现、重演示」的评分倾向。

## 5.2 主控与资源

| 项目 | 规格 |
|------|------|
| MCU | STM32F103C8T6 |
| 内核 | ARM Cortex-M3，最高 72 MHz |
| Flash | 64 KB |
| SRAM | 20 KB |
| 时钟 | HSE + PLL → 72 MHz |
| 调试/烧录 | ST-Link + OpenOCD |

## 5.3 模块清单

| 模块 | 接口/引脚 | 说明 |
|------|-----------|------|
| 状态 LED | PA0，TIM2_CH1 | PWM 呼吸灯 |
| 光敏电阻 | PA1，ADC1_IN1 | 环境光 |
| NTC 热敏 | PA2，ADC1_IN2 | 温度 |
| 反射红外 | PA3，ADC1_IN3 | 接近检测 |
| 蜂鸣器 | PA4，GPIO | 有源，低电平触发 |
| 编码器 A/B | PA6/PA7，TIM3 | 正交解码 |
| LDR/NTC 指示 LED | PA8/PA11，TIM1 | PWM 指示 |
| USART1 | PA9 TX / PA10 RX | USB 串口模块 CH340 等 |
| OLED SSD1306 | PB8/PB9，I2C1 | 128×64 |
| 直流电机 | PB6 PWM，PB5/PB7 方向 | TB6612 驱动 |

## 5.4 电源与接地

- MCU 与模块共地；
- 电机驱动独立供电时注意 GND 共地；
- 有源蜂鸣器模块 VCC/GND/S 三线，S 接 PA4，空闲态保持非触发电平。

## 5.5 硬件局限与应对

作者硬件经验相对有限，未做复杂 analog 前端定制电路，而采用现成传感器模块。软件上通过 **ADC 平均、阈值防抖、board_config 可调** 补偿模块离散性，并在报告中如实说明——评委通常接受「软件创新 + 模块化硬件」的路径，前提是演示稳定、讲解清晰。

---

# 第六章 测试与结果

## 6.1 测试环境

| 项目 | 配置 |
|------|------|
| 操作系统 | Arch Linux（亦可在其他 Linux 发行版复现） |
| 工具链 | arm-none-eabi-gcc |
| 构建 | CMake 3.20+ |
| 烧录 | OpenOCD + ST-Link |
| 串口 | minicom / PuTTY，115200 8N1 |

## 6.2 功能测试

| 编号 | 测试项 | 操作 | 预期结果 | 结果 |
|------|--------|------|----------|------|
| T1 | 编译链接 | `cmake --build build` | 无错误，生成 elf/hex/bin | 通过 |
| T2 | 固件体积 | 查看 size 输出 | Flash 占用远低于 64 KB | 通过 |
| T3 | 上电串口 | 复位 MCU | 输出 `board console ready` | 通过 |
| T4 | 呼吸灯 | 观察 PA0 LED | 约 4.8 s 周期平滑呼吸 | 通过 |
| T5 | 编码器调速 | 旋转旋钮 | ENC/MOT 变化，电机转速跟随 | 通过 |
| T6 | 光敏/NTC | 遮光/握持 NTC | LDR 电压、NTC 温度变化，指示 LED 变化 | 通过 |
| T7 | 红外交互 | 手靠近模块 | 短鸣一声，IR raw 下降 | 通过 |
| T8 | 串口-OLED | 输入一行回车 | 串口回显，OLED 显示 line= | 通过 |
| T9 | 一键烧录 | `cmake --build build --target flash` | 烧录成功并运行 | 通过 |

## 6.3 资源占用（Debug 构建实测）

构建命令：

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

`arm-none-eabi-size` 输出：

| 段 | 大小 (字节) | 说明 |
|----|-------------|------|
| text | 19332 | 代码（Flash） |
| data | 64 | 已初始化数据（Flash→RAM） |
| bss | 3312 | 未初始化数据（RAM） |
| **Flash 合计** | **19396** | text + data ≈ **19.0 KB** |
| **RAM 合计** | **3376** | data + bss ≈ **3.3 KB** |

相对 C8T6 的 64 KB / 20 KB，**Flash 使用率约 30%，RAM 约 17%**，具备显著扩展空间。

## 6.4 实时性观察

- 呼吸灯步进 12 ms，肉眼无卡顿；
- 编码器 20 ms 采样，调速跟手；
- OLED 500 ms 刷新调试行，不影响快任务；
- 红外 80 ms 检测 + streak 防抖，无误触发（上电 arm 逻辑有效）。

## 6.5 工程化测试

| 项目 | 命令 | 结果 |
|------|------|------|
| 静态分析 | `cmake --build build --target lint` | 需安装 cppcheck |
| 格式化 | `cmake --build build --target format` | 需安装 clang-format |
| 可复现构建 | 干净目录重新 cmake + build | 通过 |

---

# 第七章 总结与展望

## 7.1 工作总结

本项目完成了基于 STM32F103C8T6 的**多模态智能交互演示平台**，主要成果如下：

1. 建立了 **CMake 裸机工程化** 构建流程，实现跨平台编译、固件导出、OpenOCD 烧录与代码质量工具链集成；
2. 设计了 **app-BSP-drivers-HAL-common** 分层架构与统一驱动 API，应用层通过逻辑设备访问硬件；
3. 实现了合作式非阻塞任务调度，在无 RTOS 条件下支撑多路交互并行；
4. 集成编码器、电机、OLED、串口、三路 ADC、红外、蜂鸣器等模块，形成可现场演示的闭环；
5. Debug 固件约 19 KB Flash / 3.3 KB RAM，资源占用合理，扩展空间充足。

## 7.2 创新意义

相较于「CubeMX + HAL + 功能 demo」类校赛作品，本平台的价值在于：

- 把 **工程化构建** 与 **裸机可见性** 结合，既适合教学，又体现创新；
- 通过分层与规范，展示「从能跑到能维护」的软件升级路径；
- 为多模态交互提供可复用底座，而非一次性 demo。

## 7.3 不足与改进

1. **硬件定制不足**：后续可设计集成 PCB，优化电源与 motor 驱动布局；
2. **无 RTOS**：任务复杂度继续上升时可引入 FreeRTOS 或显式状态机框架；
3. **通信手段单一**：可增加蓝牙/Wi-Fi 模块，扩展远程交互；
4. **量化指标**：可补充温度误差、电机转速线性度等标定数据。

## 7.4 展望

- 基于现有 BSP，快速衍生 **温控风扇**、**智能台灯**、**简易服务机器人** 等主题作品；
- 抽象 `board_devices` 接口，尝试移植至 STM32F4 或其他 Cortex-M 芯片；
- 将 CMake 构建接入 Gitea/GitHub Actions，实现提交即编译的 CI 流程。

---

# 参考文献

[1] ARM Limited. *ARM Cortex-M3 Technical Reference Manual*.

[2] STMicroelectronics. *STM32F103x8/B Datasheet*.

[3] STMicroelectronics. *RM0008 Reference Manual: STM32F101, STM32F102, STM32F103, STM32F105 and STM32F107 advanced ARM-based 32-bit MCUs*.

[4] Kitware. *CMake Documentation*. https://cmake.org/documentation/

[5] Solomon Systech. *SSD1306 OLED Controller Datasheet*.

[6] 项目内部文档：`docs/CODING_STYLE.md`, `docs/DRIVER_API_GUIDE.md`, `docs/INDEX.md`.

---

# 附录

## 附录 A 主要源文件清单

见 `cmake/stm32_sources.cmake`，共 27 个编译单元（含启动汇编）。

## 附录 B 应用层任务周期常量（摘录）

```c
#define APP_BREATH_STEP_MS           (12U)
#define APP_BREATH_PHASE_MAX         (400U)
#define APP_DEBUG_OLED_PERIOD_MS     (500U)
#define APP_MOTOR_PERIOD_MS          (20U)
#define APP_MOTOR_ENC_STEP           (8U)
#define APP_IR_CHECK_PERIOD_MS       (80U)
#define APP_SENSOR_LED_PERIOD_MS     (100U)
```

## 附录 C 演示检查清单（答辩前）

- [ ] ST-Link 连接正常，`flash` target 可用
- [ ] 串口 115200 可见启动 banner
- [ ] 呼吸灯、编码器、电机、OLED 六行调试信息正常
- [ ] 红外靠近蜂鸣一次，冷却内不连续鸣叫
- [ ] 串口输入一行，OLED 对应页更新
- [ ] 准备 1 页架构图 + 1 页 CMake 流程图（PPT）
- [ ] 准备 30～60 秒现场演示脚本

## 附录 D 转换为 Word 的建议

1. 使用 Pandoc：`pandoc docs/DESIGN_REPORT_SCHOOL_COMPETITION.md -o 设计报告.docx`
2. 或使用 Typora / VS Code 插件导出 Word；
3. 导出后替换封面括号占位符，插入实物照片与 OLED 截图；
4. 学校若要求固定模板，将本章标题对应粘贴至模板各节即可。

---

*文档版本：v1.0 · 对应固件构建：Debug · text=19332 data=64 bss=3312*
