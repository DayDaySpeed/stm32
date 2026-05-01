# STM32F103C8T6 Baremetal (CMake)

## 索引 | Index

- [中文文档](#中文文档)
  - [1. 项目简介](#1-项目简介)
  - [2. 本分支做了什么](#2-本分支做了什么)
  - [3. 依赖与构建](#3-依赖与构建)
  - [4. 烧录与串口工具](#4-烧录与串口工具)
  - [5. 目录结构](#5-目录结构)
- [English Documentation](#english-documentation)
  - [1. Overview](#1-overview)
  - [2. What This Branch Changed](#2-what-this-branch-changed)
  - [3. Build](#3-build)
  - [4. Flash and Monitor](#4-flash-and-monitor)
  - [5. Project Layout](#5-project-layout)

## 中文文档

### 1. 项目简介

这是一个基于 `STM32F103C8T6` 的裸机寄存器工程，使用 `CMake + arm-none-eabi` 工具链。当前学习重点是 `USART1` 串口通信（轮询与中断）。

### 2. 本分支做了什么

本分支已经完成以下工程化与功能升级：

- **工程结构重构**
  - 将代码拆分为 `app`、`drivers`、`bsp` 分层。
  - `main` 只保留入口与中断转发，业务逻辑下沉到 `app` / `drivers`。
- **USART1 驱动工程化**
  - 引入统一初始化接口：`usart1_init(baudrate, oversampling)`。
  - 支持 `8N1`、`16x/8x` 过采样参数化配置。
  - `BRR` 由函数计算，移除硬编码波特率常量。
- **接收路径升级为中断**
  - 启用 `RXNEIE` 和 NVIC 对应中断通道。
  - 新增 `USART1_IRQHandler -> usart1_irq_handler` 链路。
  - 在驱动内加入环形缓冲区，主循环改为从缓冲区取字节回显。
- **启动文件完善**
  - `startup/startup_stm32f103c8tx.s` 增加详细注释。
  - 明确向量表、`Reset_Handler`、`Default_Handler` 与 weak 覆盖机制。
- **脚本与构建流程增强**
  - `burning.sh` 支持：
    - `--flash-only`（仅编译+烧录）
    - `--monitor-only`（仅打开串口工具）
    - `-d/--device` 与 `-b/--baud` 参数化
  - `CMake` 增强裸机场景稳定性（toolchain 检测与 size 工具处理）。

### 3. 依赖与构建

依赖：
```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib openocd stlink minicom
```

构建：
```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

构建产物：
- `build/stm32f103_baremetal.elf`
- `build/stm32f103_baremetal.hex`
- `build/stm32f103_baremetal.bin`

### 4. 烧录与串口工具

仅编译+烧录：
```bash
./burning.sh --flash-only
```

仅打开串口工具：
```bash
./burning.sh --monitor-only -d /dev/ttyUSB0 -b 115200
```

手动打开 `minicom`：
```bash
minicom -D /dev/ttyUSB0 -b 115200
```

CH340 接线：
- `PA9  -> RXD`
- `PA10 -> TXD`
- `GND  -> GND`（必须共地）

### 5. 目录结构

- `src/main.c`：程序入口与中断转发
- `src/app/app.c`：应用初始化与主循环回显逻辑
- `src/drivers/systick.c`：系统节拍与延时
- `src/drivers/usart1.c`：串口初始化、发送、接收中断与缓冲
- `include/bsp`：寄存器与时钟常量
- `include/drivers`：驱动接口声明
- `include/app`：应用接口声明
- `startup/startup_stm32f103c8tx.s`：启动与向量表
- `linker/STM32F103C8TX_FLASH.ld`：链接脚本
- `docs/CODING_STYLE.md`：编码规范

## English Documentation

### 1. Overview

This is a bare-metal register-level project for `STM32F103C8T6`, built with `CMake + arm-none-eabi`. The current focus is UART learning with both polling and interrupt-based receive paths.

### 2. What This Branch Changed

This branch includes the following engineering and feature updates:

- **Project restructuring**
  - Code is split into `app`, `drivers`, and `bsp` layers.
  - `main` is reduced to entry and ISR forwarding only.
- **USART1 driver engineering**
  - Unified init API: `usart1_init(baudrate, oversampling)`.
  - Supports `8N1` with configurable `16x/8x` oversampling.
  - `BRR` is computed by function (no hard-coded baud constant).
- **Interrupt-based RX path**
  - Enables `RXNEIE` and corresponding NVIC interrupt line.
  - Adds `USART1_IRQHandler -> usart1_irq_handler` flow.
  - Introduces ring buffer in driver; app loop reads from buffer for echo.
- **Startup file improvements**
  - `startup/startup_stm32f103c8tx.s` now has detailed comments.
  - Documents vector table, `Reset_Handler`, `Default_Handler`, and weak override behavior.
- **Script and build enhancements**
  - `burning.sh` now supports:
    - `--flash-only` (build + flash only)
    - `--monitor-only` (monitor only)
    - parameterized `-d/--device`, `-b/--baud`
  - `CMake` improved for bare-metal toolchain stability.

### 3. Build

Dependencies:
```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib openocd stlink minicom
```

Build:
```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

Outputs:
- `build/stm32f103_baremetal.elf`
- `build/stm32f103_baremetal.hex`
- `build/stm32f103_baremetal.bin`

### 4. Flash and Monitor

Build + flash only:
```bash
./burning.sh --flash-only
```

Monitor only:
```bash
./burning.sh --monitor-only -d /dev/ttyUSB0 -b 115200
```

Manual `minicom`:
```bash
minicom -D /dev/ttyUSB0 -b 115200
```

CH340 wiring:
- `PA9  -> RXD`
- `PA10 -> TXD`
- `GND  -> GND` (common ground is required)

### 5. Project Layout

- `src/main.c`: program entry and ISR forwarding
- `src/app/app.c`: app initialization and echo loop
- `src/drivers/systick.c`: system tick and delay
- `src/drivers/usart1.c`: UART init, TX, RX interrupt and buffer
- `include/bsp`: registers and clock constants
- `include/drivers`: driver interfaces
- `include/app`: app interfaces
- `startup/startup_stm32f103c8tx.s`: startup and vector table
- `linker/STM32F103C8TX_FLASH.ld`: linker script
- `docs/CODING_STYLE.md`: coding style rules
