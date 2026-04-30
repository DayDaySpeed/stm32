# STM32F103C8T6 Baremetal (CMake)

## 中文文档

### 项目简介

这是一个基于 `STM32F103C8T6` 的裸机寄存器工程，使用 `CMake + arm-none-eabi` 工具链。当前目标是用工程化结构学习串口基础收发。

### 当前实现（轮询方案）

你在当前阶段完成了以下内容（Polling UART）：
- 使用 `USART1`，引脚为 `PA9(TX)` 和 `PA10(RX)`。
- 初始化为 `8N1`，支持 `16x/8x` 过采样参数配置。
- 波特率 `BRR` 通过函数计算，不再写死魔法数。
- 发送路径使用 `TXE` 轮询：寄存器可写后再写 `DR`。
- 接收路径使用 `RXNE` 轮询：有数据时读 `DR` 并回显。
- 应用层在 `app_run_forever()` 中实现 `"recv: <char>"` 回显逻辑。

### 依赖

```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib openocd stlink minicom
```

### 构建

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

构建产物：
- `build/stm32f103_baremetal.elf`
- `build/stm32f103_baremetal.hex`
- `build/stm32f103_baremetal.bin`

### 烧录

推荐（脚本）：
```bash
./burning.sh --flash-only
```

CMake 目标：
```bash
cmake --build build --target flash
```

### 串口监视

```bash
./burning.sh --monitor-only -d /dev/ttyUSB0 -b 115200
```

或：
```bash
minicom -D /dev/ttyUSB0 -b 115200
```

### 接线说明（CH340）

- `PA9  -> RXD`
- `PA10 -> TXD`
- `GND  -> GND`（必须共地）

### 目录结构

- `src/main.c`：程序入口与中断入口绑定。
- `src/app/app.c`：应用层主流程。
- `src/drivers/*.c`：驱动实现（`systick`, `usart1`）。
- `include/bsp`：芯片寄存器与时钟常量。
- `include/drivers`：驱动接口声明。
- `include/app`：应用层接口声明。
- `startup/startup_stm32f103c8tx.s`：启动文件。
- `linker/STM32F103C8TX_FLASH.ld`：链接脚本。
- `docs/CODING_STYLE.md`：编码规范。

## English Documentation

### Project Overview

This is a bare-metal register-level project for `STM32F103C8T6`, built with `CMake + arm-none-eabi`. The current goal is to learn UART Tx/Rx with an engineering-oriented project structure.

### Current Implementation (Polling)

At this stage, the project implements the following polling-based UART workflow:
- `USART1` is used on `PA9 (TX)` and `PA10 (RX)`.
- UART is configured as `8N1` with configurable `16x/8x` oversampling.
- `BRR` is computed by function (no hard-coded magic baud constant).
- TX path uses `TXE` polling: write `DR` only when transmit register is ready.
- RX path uses `RXNE` polling: read `DR` when a byte is available.
- Echo logic (`"recv: <char>"`) is implemented in `app_run_forever()`.

### Dependencies

```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib openocd stlink minicom
```

### Build

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

Build outputs:
- `build/stm32f103_baremetal.elf`
- `build/stm32f103_baremetal.hex`
- `build/stm32f103_baremetal.bin`

### Flash

Recommended (script):
```bash
./burning.sh --flash-only
```

CMake target:
```bash
cmake --build build --target flash
```

### UART Monitor

```bash
./burning.sh --monitor-only -d /dev/ttyUSB0 -b 115200
```

Or:
```bash
minicom -D /dev/ttyUSB0 -b 115200
```

### Wiring (CH340)

- `PA9  -> RXD`
- `PA10 -> TXD`
- `GND  -> GND` (common ground is required)

### Project Layout

- `src/main.c`: entry point and ISR binding.
- `src/app/app.c`: application flow.
- `src/drivers/*.c`: driver implementations (`systick`, `usart1`).
- `include/bsp`: MCU registers and clock constants.
- `include/drivers`: driver interfaces.
- `include/app`: app interfaces.
- `startup/startup_stm32f103c8tx.s`: startup code.
- `linker/STM32F103C8TX_FLASH.ld`: linker script.
- `docs/CODING_STYLE.md`: coding standard.
