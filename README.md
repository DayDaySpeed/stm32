# STM32F103C8T6 Baremetal (CMake)

## 索引 | Index

- [中文文档](#中文文档)
  - [1. 项目简介](#1-项目简介)
  - [2. 本分支做了什么](#2-本分支做了什么)
  - [2.1 近期更新](#21-近期更新)
  - [3. 依赖与构建](#3-依赖与构建)
  - [4. 烧录与串口工具](#4-烧录与串口工具)
  - [5. 目录结构](#5-目录结构)
- [English Documentation](#english-documentation)
  - [1. Overview](#1-overview)
  - [2. What This Branch Changed](#2-what-this-branch-changed)
  - [2.1 Recent Updates](#21-recent-updates)
  - [3. Build](#3-build)
  - [4. Flash and Monitor](#4-flash-and-monitor)
  - [5. Project Layout](#5-project-layout)

## 中文文档

### 1. 项目简介

这是一个基于 `STM32F103C8T6` 的裸机寄存器工程，使用 `CMake + arm-none-eabi` 工具链。功能上包含 **`USART1` 串口**（中断接收 + 环形缓冲）与 **`SSD1306` 128×64 OLED**（`PB8/PB9` 软件 I2C）。

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
    - `-m` / `--monitor`（编译+烧录后打开 minicom）
    - `--minicom-only`（仅打开串口，不编译不烧录）
    - `-d/--device` 与 `-b/--baud` 参数化
  - `CMake` 增强裸机场景稳定性（toolchain 检测与 size 工具处理）。

### 2.1 近期更新

本节记录在 OLED 与工程维护上的一轮改动（与上文「本分支」内容互补）：

- **板级与构建**
  - 新增 `src/bsp/board_init.c`：`bsp_board_init()` 统一打开 `AFIO` / `GPIOA` / `GPIOB` / `USART1` 的 APB2 时钟；`main` 最先调用。
  - 根目录 `GPIO.h`、`RCC.h`、`SYS.h` 迁入 `include/bsp/`（`board_pins.h`、`rcc_board.h`，SysTick 重装载并入 `clock.h`）。
  - `cmake/stm32_sources.cmake` 集中列出固件源，`CMakeLists.txt` 引用之。
- **SSD1306 驱动**
  - `ssd1306_oled.c` 补充模块级与关键流程中文注释；软件 I2C 写字节时 **采样并校验 ACK**，NACK 时发 STOP 中止。
  - **刷新策略**：普通 `putc` 仅通过 **`0x21`/`0x22` 窗口** 推送 **6 列×当前页** 到 GDDRAM；**滚屏**（`memmove` 整帧）或 `clear` / 显式 `refresh` 时仍 **全屏 1024 字节**，显著减轻位带 I2C 负载。
  - `ssd1306_oled_putc()` 内部完成上述刷新；`app` 中不再每字调用全屏 `refresh`。`ssd1306_oled_clear()` 清缓冲后 **自动全屏 refresh**。
- **`burning.sh`**
  - 修正 `-m`/`--monitor` 与 `--minicom-only` 语义；脚本先 `cd` 到仓库根，`-d`/`-b` 缺参时报错。
- **应用与串口回显**
  - 修复 `recv:` 后误用 `usart1_send_string(&ch)` 导致尾随乱码，改为 **`usart1_send_byte(ch)`**。
- **文档与资料**
  - `docs/CODING_STYLE.md`、`USART.md` 路径说明随 `bsp` 调整。
  - 可选：英文原版数据手册可置于 `Document/ssd1306/`（例如 `SSD1306_Solomon_Rev1.1_EN.pdf`）。

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
./burning.sh --minicom-only -d /dev/ttyUSB0 -b 115200
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

- `src/main.c`：程序入口；先 `bsp_board_init()`，再 `app_init()` / 中断转发
- `src/bsp/board_init.c`：板级时钟门控（APB2 上本板外设）
- `src/app/app.c`：应用初始化与主循环（串口 + OLED）
- `src/drivers/systick.c`：SysTick 1ms 节拍与延时
- `src/drivers/usart1.c`：USART1 初始化、发送、RX 中断与环形缓冲
- `src/drivers/ssd1306_oled.c` / `oled_font5x7.c`：SSD1306 软件 I2C、局部/全屏刷新与字库
- `Document/ssd1306/`：SSD1306 英文数据手册等（可选，自管下载）
- `include/bsp`：寄存器映射、`clock.h`、`board_pins.h`、`rcc_board.h`、`board_init.h`
- `include/drivers`：驱动接口声明
- `include/app`：应用接口声明
- `cmake/stm32_sources.cmake`：固件源文件清单（供 `CMakeLists.txt` 引用）
- `startup/startup_stm32f103c8tx.s`：启动与向量表
- `linker/STM32F103C8TX_FLASH.ld`：链接脚本
- `docs/CODING_STYLE.md`：编码规范

## English Documentation

### 1. Overview

This is a bare-metal register-level project for `STM32F103C8T6`, built with `CMake + arm-none-eabi`. It includes **USART1** (interrupt RX + ring buffer) and **SSD1306 128×64 OLED** (bit-bang I2C on `PB8`/`PB9`).

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
    - `-m` / `--monitor` (build + flash, then minicom)
    - `--minicom-only` (minicom only, no build/flash)
    - parameterized `-d/--device`, `-b/--baud`
  - `CMake` improved for bare-metal toolchain stability.

### 2.1 Recent updates

Follow-up work on OLED and tooling (adds to section 2 above):

- **Board / build**
  - `src/bsp/board_init.c`: `bsp_board_init()` enables `AFIO` / `GPIOA` / `GPIOB` / `USART1` on APB2; called first from `main`.
  - Legacy `GPIO.h` / `RCC.h` / `SYS.h` folded into `include/bsp/` (`board_pins.h`, `rcc_board.h`; SysTick reload in `clock.h`).
  - `cmake/stm32_sources.cmake` lists firmware sources; top `CMakeLists.txt` includes it.
- **SSD1306**
  - More Chinese comments in `ssd1306_oled.c`; bit-bang I2C **checks ACK** and **STOPS** the transaction on NACK.
  - **Refresh**: normal `putc` updates only **6 columns × current page** via column/page window commands; **full 1024-byte** push after **scroll** (`memmove`), on **`ssd1306_oled_clear()`**, or explicit **`ssd1306_oled_refresh()`**.
  - `ssd1306_oled_putc()` performs the appropriate refresh internally; `app` no longer calls full refresh per character.
- **`burning.sh`**
  - Fixed `-m` / `--monitor` vs `--minicom-only`; `cd` to repo root; `-d` / `-b` require an argument.
- **App / UART echo**
  - Fixed `recv:` line: use **`usart1_send_byte(ch)`** instead of treating `&ch` as a C string.
- **Docs**
  - `docs/CODING_STYLE.md` and `USART.md` updated for new `bsp` paths.
  - Optional English datasheet under `Document/ssd1306/` (e.g. `SSD1306_Solomon_Rev1.1_EN.pdf`).

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
./burning.sh --minicom-only -d /dev/ttyUSB0 -b 115200
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

- `src/main.c`: entry; calls `bsp_board_init()` before `app_init()`; ISR forwarding
- `src/bsp/board_init.c`: board-level APB2 clock enables
- `src/app/app.c`: app init and main loop (UART + OLED)
- `src/drivers/systick.c`: SysTick 1 ms tick and delay
- `src/drivers/usart1.c`: USART1 init, TX, RX IRQ and ring buffer
- `src/drivers/ssd1306_oled.c`, `oled_font5x7.c`: SSD1306 bit-bang I2C, partial/full refresh, font
- `Document/ssd1306/`: optional SSD1306 datasheet PDFs
- `include/bsp`: register map, `clock.h`, `board_pins.h`, `rcc_board.h`, `board_init.h`
- `include/drivers`: driver headers
- `include/app`: app headers
- `cmake/stm32_sources.cmake`: firmware source list for CMake
- `startup/startup_stm32f103c8tx.s`: startup and vector table
- `linker/STM32F103C8TX_FLASH.ld`: linker script
- `docs/CODING_STYLE.md`: coding style rules
