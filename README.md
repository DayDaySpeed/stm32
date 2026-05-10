# STM32F103C8T6 Baremetal (CMake)

## 索引 | Index

- [中文文档](#中文文档)
  - [1. 项目简介](#1-项目简介)
  - [2. 本分支做了什么](#2-本分支做了什么)
  - [2.1 近期更新](#21-近期更新)
  - [2.3 TIM 秒级定时（1 字节计数）](#23-tim-秒级定时1-字节计数)
  - [2.4 PWM 模块与项目规范化重构（v4）](#24-pwm-模块与项目规范化重构v4)
  - [3. 依赖与构建](#3-依赖与构建)
  - [4. 烧录与串口工具](#4-烧录与串口工具)
  - [5. 目录结构](#5-目录结构)
- [English Documentation](#english-documentation)
  - [1. Overview](#1-overview)
  - [2. What This Branch Changed](#2-what-this-branch-changed)
  - [2.1 Recent Updates](#21-recent-updates)
  - [2.2 TIM Second Tick (1-Byte Counter)](#22-tim-second-tick-1-byte-counter)
  - [2.3 PWM Module & Project-wide Refactor (v4)](#23-pwm-module--project-wide-refactor-v4)
  - [3. Build](#3-build)
  - [4. Flash and Monitor](#4-flash-and-monitor)
  - [5. Project Layout](#5-project-layout)

## 中文文档

### 1. 项目简介

这是一个基于 `STM32F103C8T6` 的裸机寄存器工程，使用 `CMake + arm-none-eabi` 工具链。当前采用 **分层工业化写法**：`bsp + common + hal + drivers + app`，其中 `I2C1` 与 `USART1` 已重构为可复用驱动路径。

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
  - `ssd1306_oled.c` 使用 **硬件 I2C1**（100kHz 标准模式，`bsp_clock_get_pclk1_hz()` 用于 CCR/TRISE）；NACK/超时发 STOP 中止。
  - **刷新策略**：普通 `putc` 仅通过 **`0x21`/`0x22` 窗口** 推送 **6 列×当前页** 到 GDDRAM；**滚屏**（`memmove` 整帧）或 `clear` / 显式 `refresh` 时仍 **全屏 1024 字节**，减轻总线占用。
  - `ssd1306_oled_putc()` 内部完成上述刷新；`app` 中不再每字调用全屏 `refresh`。`ssd1306_oled_clear()` 清缓冲后 **自动全屏 refresh**。
- **`burning.sh`**
  - 修正 `-m`/`--monitor` 与 `--minicom-only` 语义；脚本先 `cd` 到仓库根，`-d`/`-b` 缺参时报错。
- **应用与串口回显**
  - 修复 `recv:` 后误用 `usart1_send_string(&ch)` 导致尾随乱码，改为 **`usart1_send_byte(ch)`**。
- **文档与资料**
  - `docs/CODING_STYLE.md`、`USART.md` 路径说明随 `bsp` 调整。
  - 可选：英文原版数据手册可置于 `Document/ssd1306/`（例如 `SSD1306_Solomon_Rev1.1_EN.pdf`）。

### 2.2 工业级重构（v3）

- **架构分层升级**
  - 新增 `include/common` / `src/common`：公共状态码、宏、通用环形缓冲。
  - 新增 `include/hal` / `src/hal`：`i2c1_master` 事务层，屏蔽设备驱动对寄存器细节依赖。
  - `drivers/ssd1306_oled.c` 升级为“**设备对象 + 总线回调**”写法（`ssd1306_t`）。
- **USART 驱动工业化**
  - `usart1` 接收链路改为复用型 `ring_buffer_t`。
  - 新增行策略配置：`CR/LF/CRLF`（`usart1_set_line_policy`）。
- **应用层迁移**
  - `app` 回到“按行读取并显示到 OLED”的业务流。
  - 移除临时逐字节十六进制调试逻辑。
- **构建质量门禁**
  - CMake 新增选项：`DEBUG_LOG`、`ASSERT_LEVEL`、`OLED_REFRESH_MODE`。
  - 新增 `format` / `lint` 目标（自动探测 `clang-format` / `cppcheck`）。

### 2.3 TIM 秒级定时（1 字节计数）

本分支已将 TIM2 改为“上层可配置秒周期”的接口，并在应用层实现了一个 `uint8_t` 秒计数器，用于按秒刷新 OLED 文本。

- **驱动接口（可配置周期）**
  - 使用 `tim2_init_periodic_interrupt_seconds(period_seconds)` 初始化 TIM2 更新中断。
  - `period_seconds=1` 时即每秒触发一次中断；当前 `app_init()` 传入 `1U`。
  - 中断入口链路：`TIM2_IRQHandler -> tim2_irq_handler() -> tim2_on_second_interrupt()`。
- **应用层 1 字节计时实现**
  - 在 `src/app/app.c` 的 `tim2_on_second_interrupt()` 中，使用 `static uint8_t tim` 作为秒计数。
  - `tim < 60` 时显示 `TIM=%usec`；`tim >= 60` 时按 `min = tim / 60`、`sec = tim % 60` 显示 `TIM=%umin%usec`。
  - 每次中断后 `++tim`，实现“每秒自增”。
- **为何用 1 字节**
  - RAM 占用极小，演示定时/中断/OLED 联动足够直观。
  - 便于在早期裸机调试阶段快速观察中断是否稳定触发。
- **边界与注意事项**
  - `uint8_t` 范围为 `0~255`，计数到 `255` 后会回绕到 `0`（约 `4 分 15 秒` 后循环）。
  - 回调中调用 `ssd1306_oled_write_text_atf()` 属于 ISR 内执行业务，若后续业务变重，建议改为“ISR 只置标志，主循环再刷新显示”。
  - 当前 TIM2 分频策略按 10kHz 基准 + 16 位 ARR 计算，可配置秒数存在上限（现实现可用于较小秒周期场景）。

### 2.4 PWM 模块与项目规范化重构（v4）

本节记录把 TIM2 从「秒级中断」改造成「PWM 输出 + 呼吸灯」的过程，以及对全项目代码风格的一轮规范化整理。

详细的 PWM 使用文档见 [pwm.md](./pwm.md)。

#### PWM 模块（TIM2_CH1，PA0 输出）

- **新模块替换旧模块**：删除 `src/drivers/tim2.c` / `include/drivers/tim2.h`，新增 `src/drivers/pwm.c` / `include/drivers/pwm.h`，专做边沿对齐 PWM 模式 1 输出。
- **接口（详见 `pwm.h`）**：
  - `tim2_ch1_pwm_init_hz(pwm_hz, duty_permille)`：按目标频率自动反推 (PSC, ARR)，配置 GPIO + 寄存器，启动输出。
  - `tim2_ch1_pwm_set_duty_permille(duty)`：仅改 CCR1，运行中无毛刺。
  - `tim2_ch1_pwm_set_hz(pwm_hz, duty)`：连频率一起改。
  - `tim2_ch1_pwm_stop()`：停 CNT 并关 CC1E，PA0 回到 GPIO 低电平。
- **占空比单位**：千分比（0..1000），在无 FPU 的 MCU 上比百分比有更高分辨率，且整数运算不丢精度。
- **频率分辨率**：从 PWM 频率反推 (PSC, ARR) 时，先尝试 ARR 上限内的精确解，无精确解再回退到向上取整的近似解；近似情况下会损失少量频率精度但保证 ARR 不溢出。
- **应用：呼吸灯**：`src/app/app.c` 在主循环里以 12ms 节流推进相位 0..399，三角波映射到占空比 0..1000，整圈 ~4.8 秒。

#### 应用层 bug 修复

- **OLED 写入节流**：原来主循环每轮都 `ssd1306_oled_write_text_atf(...)` 重绘 OLED，I2C 总线接近 100% 占用，呼吸灯肉眼可见卡顿。改为「仅当串口收到完整一行才更新 OLED」。
- **page 越界**：`page_count` 超过 8 后底层 `ssd1306_write_text_at` 静默失败、新行不显示。改为「写满 8 页就 `ssd1306_oled_clear()` + 回 0」。
- **变量命名**：`s_next_ms` → `s_last_step_ms`（变量名和实际语义对齐）。

#### 项目命名/风格规范化

- **变量前缀**：文件级 static 用 `g_`，函数级 static 用 `s_`。把 `pwm.c` 内的 `s_ticks_per_period` 改为 `g_ticks_per_period`；把 `usart1.c` 的函数级 static 加 `s_` 前缀。
- **空指针比较**：全项目 `== 0` / `!= 0` 改为 `== NULL` / `!= NULL`（约 30 处）。
- **错误码统一**：`usart1_init()` 从 `uint8_t 0/1` 改为返回 `stm_status_t`，与项目其他 init 函数一致；`usart1_compute_brr()` 同步调整。
- **寄存器位常量**：补全 `TIM_CCMR1_CC1S_MASK` / `CC1S_OUT`、`FLASH_ACR_LATENCY_MASK`，并把 `pwm.c` 里 `~(3U << 0)` / `TIM2_SR = 0U` 这种 magic number 替换为命名常量。
- **大括号风格**：`bsp/clock.c` 由 Allman 改为项目统一的 K&R。
- **轮询超时常量**：`bsp/clock.c` 里两处 `2000000UL` 抽为 `BSP_CLOCK_WAIT_MAX_ITER`；`hal/i2c1_master.c` 抽为 `I2C1_MASTER_DEFAULT_TIMEOUT_ITER`。
- **板级引脚宏**：`hal/i2c1_master.c` 直接写 `0xFFFFFF00UL | 0x000000FFUL` 配 PB8/PB9，提取到 `bsp/board_pins.h` 的 `BOARD_GPIO_PB8_PB9_*` 宏。
- **未用宏清理**：`bsp/clock.h` 删除未被代码引用的 `SYSCLK_HZ` / `BSP_PCLK1_HZ` / `BSP_PCLK2_HZ` 宏，调用方改为直接调 `bsp_clock_get_pclk*_hz()`。
- **include 风格**：`ssd1306_oled.c` 错写的 `#include "stm_status.h"` 修正为 `#include "common/stm_status.h"`；统一 include 顺序为「自身头 → 标准库 → 项目内 bsp/common/hal/drivers」。
- **`stm_status.h`**：删除未使用的 `<stdint.h>`，消除 clangd 警告。

#### 构建/工具

- **`CMakeLists.txt`**：`format` target 改用 `file(GLOB_RECURSE)` 自动收集 `src/*.c` 和 `include/*.h`，新增源文件不再需要手工维护清单。
- **`burning.sh`**：长选项 `--monitor` 改名 `--minicom`（短选项 `-m` 不变），更直观。

#### 编译指标

| 指标 | v3 末 | v4 末 | Δ |
|---|---:|---:|---:|
| `.text` | 8824 B | 8740 B | -84 B |
| `.data` | 44 B | 44 B | 0 |
| `.bss`  | 3236 B | 3236 B | 0 |
| 编译警告 | 0 | 0 | - |

代码体积小幅下降的原因是合并了 `pwm.c` 的两次 `&=` 与 `clock.c` 的 `wait_reg_bit` 简化分支。

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
- `src/app/app.c`：应用初始化与主循环（串口按行输入 + OLED 文本输出）
- `src/drivers/systick.c`：SysTick 1ms 节拍与延时
- `src/common/ring_buffer.c`：可复用环形缓冲实现
- `src/hal/i2c1_master.c`：I2C1 主机事务层（轮询 + 超时 + NACK 收敛）
- `src/drivers/usart1.c`：USART1 初始化、发送、RX 中断、行策略读取
- `src/drivers/pwm.c`：TIM2_CH1 边沿对齐 PWM 输出（PA0），见 [pwm.md](./pwm.md)
- `src/drivers/ssd1306_oled.c` / `oled_font5x7.c`：SSD1306 设备对象驱动、局部/全屏刷新与字库
- `Document/ssd1306/`：SSD1306 英文数据手册等（可选，自管下载）
- `include/bsp`：寄存器映射、`clock.h`、`board_pins.h`、`rcc_board.h`、`board_init.h`
- `include/common`：状态码、通用宏、基础容器接口
- `include/hal`：硬件事务层接口
- `include/drivers`：设备驱动接口声明（含 `ssd1306_t` 对象 API）
- `include/app`：应用接口声明
- `cmake/stm32_sources.cmake`：固件源文件清单（供 `CMakeLists.txt` 引用）
- `startup/startup_stm32f103c8tx.s`：启动与向量表
- `linker/STM32F103C8TX_FLASH.ld`：链接脚本
- `docs/CODING_STYLE.md`：编码规范

## English Documentation

### 1. Overview

This is a bare-metal register-level project for `STM32F103C8T6`, built with `CMake + arm-none-eabi`. It now uses an industrialized layered layout: **bsp + common + hal + drivers + app**, with reusable USART and I2C transaction paths.

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
  - More Chinese comments in `ssd1306_oled.c`; hardware I2C1 **stops** the transaction on NACK/timeout.
  - **Refresh**: normal `putc` updates only **6 columns × current page** via column/page window commands; **full 1024-byte** push after **scroll** (`memmove`), on **`ssd1306_oled_clear()`**, or explicit **`ssd1306_oled_refresh()`**.
  - `ssd1306_oled_putc()` performs the appropriate refresh internally; `app` no longer calls full refresh per character.
- **`burning.sh`**
  - Fixed `-m` / `--monitor` vs `--minicom-only`; `cd` to repo root; `-d` / `-b` require an argument.
- **App / UART echo**
  - Fixed `recv:` line: use **`usart1_send_byte(ch)`** instead of treating `&ch` as a C string.
- **Docs**
  - `docs/CODING_STYLE.md` and `USART.md` updated for new `bsp` paths.
  - Optional English datasheet under `Document/ssd1306/` (e.g. `SSD1306_Solomon_Rev1.1_EN.pdf`).

### 2.2 TIM Second Tick (1-Byte Counter)

This branch upgrades TIM2 init to a configurable-second API and uses an app-level `uint8_t` second counter to refresh OLED content once per tick.

- **Driver API (configurable period)**
  - Use `tim2_init_periodic_interrupt_seconds(period_seconds)` to configure TIM2 update IRQ.
  - `period_seconds=1` means one interrupt per second (currently used in `app_init()`).
  - IRQ chain: `TIM2_IRQHandler -> tim2_irq_handler() -> tim2_on_second_interrupt()`.
- **App-level 1-byte timer**
  - `src/app/app.c` implements `tim2_on_second_interrupt()` with `static uint8_t tim`.
  - For `tim < 60`, OLED prints `TIM=%usec`; for `tim >= 60`, it prints `TIM=%umin%usec` with `min = tim / 60`, `sec = tim % 60`.
  - `tim` increments once per interrupt (`++tim`).
- **Why 1 byte**
  - Minimal RAM footprint while keeping timer/IRQ/OLED behavior easy to observe.
  - Suitable for early bare-metal bring-up and interrupt path validation.
- **Limitations / notes**
  - `uint8_t` wraps at 255 back to 0 (loop period ~4m15s at 1Hz).
  - OLED writes are currently done inside ISR; for heavier workloads, prefer ISR-sets-flag and render in main loop.
  - Current TIM2 setup uses a 10kHz base with a 16-bit ARR budget, so valid second periods are limited to small values.

### 2.3 PWM Module & Project-wide Refactor (v4)

This section captures the migration of TIM2 from a "second-tick interrupt" into a "PWM output for breathing LED", plus a project-wide code style cleanup. Detailed PWM usage is in [pwm.md](./pwm.md).

#### PWM module (TIM2_CH1, output on PA0)

- **Module replacement**: removed `src/drivers/tim2.{c,h}`, added `src/drivers/pwm.{c,h}` for edge-aligned PWM mode 1 only.
- **API**:
  - `tim2_ch1_pwm_init_hz(pwm_hz, duty_permille)`: solves (PSC, ARR) from target frequency, configures GPIO + registers, and starts output.
  - `tim2_ch1_pwm_set_duty_permille(duty)`: changes only CCR1 (glitch-free at runtime).
  - `tim2_ch1_pwm_set_hz(pwm_hz, duty)`: changes both frequency and duty.
  - `tim2_ch1_pwm_stop()`: stops CNT and clears CC1E.
- **Duty unit**: per-mille (0..1000), better resolution than percent on FPU-less MCU and integer-friendly.
- **Frequency resolver**: tries an exact (PSC, ARR) within ARR-16-bit limit first; falls back to a ceiling-rounded approximation if no exact factor exists.
- **App: breathing LED**: `src/app/app.c` advances a phase counter every 12 ms over [0..399], maps a triangle wave to duty [0..1000]; full breath cycle ≈ 4.8 s.

#### App-layer bug fixes

- **OLED throttling**: previous `app_run_forever` redrew the OLED every loop, saturating I²C and visibly stuttering the breathing LED. Now the OLED is updated only when a full UART line arrives.
- **Page wraparound**: `page_count` past 8 silently failed downstream. Now it triggers `ssd1306_oled_clear()` + reset to page 0.
- **Naming**: `s_next_ms` → `s_last_step_ms` (matches actual semantics).

#### Project-wide style normalization

- **Static variable prefixes**: file-scope statics use `g_`, function-scope statics use `s_`. Renamed `s_ticks_per_period` → `g_ticks_per_period` in `pwm.c`; added `s_` prefixes to `usart1.c` function-level statics.
- **Null pointer comparison**: `== 0` / `!= 0` → `== NULL` / `!= NULL` across the project (~30 occurrences).
- **Status code unification**: `usart1_init()` now returns `stm_status_t` instead of `uint8_t 0/1`, matching other init functions; `usart1_compute_brr()` updated likewise.
- **Register bit constants**: added `TIM_CCMR1_CC1S_MASK` / `CC1S_OUT`, `FLASH_ACR_LATENCY_MASK`; replaced magic numbers like `~(3U << 0)` and `TIM2_SR = 0U` with named constants (`~TIM_SR_UIF_BIT`).
- **Brace style**: `bsp/clock.c` switched from Allman to project-standard K&R.
- **Polling timeout constants**: `bsp/clock.c` extracted `BSP_CLOCK_WAIT_MAX_ITER`; `hal/i2c1_master.c` extracted `I2C1_MASTER_DEFAULT_TIMEOUT_ITER`.
- **Pin macros**: PB8/PB9 raw masks moved into `bsp/board_pins.h` as `BOARD_GPIO_PB8_PB9_*`.
- **Dead macro removal**: `bsp/clock.h` dropped unused `SYSCLK_HZ` / `BSP_PCLK1_HZ` / `BSP_PCLK2_HZ`; callers use `bsp_clock_get_pclk*_hz()` directly.
- **Includes**: fixed `ssd1306_oled.c` wrong-path `#include "stm_status.h"` → `"common/stm_status.h"`; standardized include ordering (own header → stdlib → project layers).
- **`stm_status.h`**: removed unused `<stdint.h>` (silences clangd warning).

#### Build/tooling

- **`CMakeLists.txt`**: `format` target now uses `file(GLOB_RECURSE)` over `src/*.c` and `include/*.h`; no more manual file list maintenance.
- **`burning.sh`**: long option `--monitor` renamed to `--minicom` (short `-m` unchanged).

#### Footprint

| Metric | end of v3 | end of v4 | Δ |
|---|---:|---:|---:|
| `.text` | 8824 B | 8740 B | -84 B |
| `.data` | 44 B | 44 B | 0 |
| `.bss`  | 3236 B | 3236 B | 0 |
| Warnings | 0 | 0 | - |

The size reduction comes from merging the two `&=` writes in `pwm.c` and simplifying the polling helper in `clock.c`.

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
- `src/drivers/pwm.c`: TIM2_CH1 edge-aligned PWM output on PA0; see [pwm.md](./pwm.md)
- `src/drivers/ssd1306_oled.c`, `oled_font5x7.c`: SSD1306 hardware I2C1, partial/full refresh, font
- `Document/ssd1306/`: optional SSD1306 datasheet PDFs
- `include/bsp`: register map, `clock.h`, `board_pins.h`, `rcc_board.h`, `board_init.h`
- `include/drivers`: driver headers
- `include/app`: app headers
- `cmake/stm32_sources.cmake`: firmware source list for CMake
- `startup/startup_stm32f103c8tx.s`: startup and vector table
- `linker/STM32F103C8TX_FLASH.ld`: linker script
- `docs/CODING_STYLE.md`: coding style rules
