# STM32F103C8T6 裸机按键控制分支说明

[English](#english-version)

本分支流程

```bash
1.配置systick
2.初始化按键
3.初始化led
4.按键按下熄灭,再按下亮起
```

本分支基于 STM32F103C8T6（Blue Pill）实现了一个纯寄存器、无 HAL 的双按键控制示例：

- `PB0` 按下翻转 `PC13`（板载 LED）
- `PB1` 按下翻转 `PA1`（外接辅助输出）

工程使用 CMake + arm-none-eabi 工具链，适合学习 GPIO 输入输出、按键消抖与模块化代码组织。

## 功能特性

- 纯寄存器开发，便于理解底层配置流程
- 按键输入采用上拉输入（低电平按下）
- 软件消抖，单次按下只触发一次事件
- 支持双通道映射：
  - `PB0 -> PC13`
  - `PB1 -> PA1`
- 代码按模块拆分：`SYS` / `KEY` / `LED(output)` / `main`

## 当前引脚映射

- `PB0`：按键输入（上拉输入，按下为低）
- `PB1`：按键输入（上拉输入，按下为低）
- `PC13`：板载 LED 输出（翻转控制）
- `PA1`：外接 LED/负载输出（翻转控制）

> 说明：`PA1` 一般需要外接 LED + 限流电阻（220R~1k）后接地进行观察。

## 目录结构（本分支）

- `src/main.c`：主循环与按键-输出映射逻辑
- `src/KEY.c` + `include/KEY.h`：按键初始化、消抖扫描、事件接口
- `src/LED.c` + `include/LED.h`：输出初始化与翻转接口
- `src/SYS.c` + `include/SYS.h`：SysTick 1ms 时基与阻塞延时
- `startup/startup_stm32f103c8tx.s`：向量表、数据段拷贝、BSS 清零
- `linker/STM32F103C8TX_FLASH.ld`：链接脚本（64KB Flash / 20KB RAM）

## 可调参数

- `KEY_DEBOUNCE_MS`（`include/KEY.h`）  
按键消抖时间，默认 `20ms`。  
如果想提高响应速度可尝试 `10~15ms`，如有误触发可适当调大。

## 依赖安装（Arch Linux）

```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib openocd stlink
```

## 构建

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

构建成功后将生成：

- `build/stm32f103_baremetal.elf`
- `build/stm32f103_baremetal.hex`
- `build/stm32f103_baremetal.bin`

## 烧录

```bash
cmake --build build --target flash
```

手动烧录命令（可选）：

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/stm32f103_baremetal.elf verify reset exit"
```

## 常见问题

- `PC13` 亮但 `PA1` 不亮：优先检查外接 LED 极性与限流电阻
- `flash` 报 `open failed`：通常是 ST-Link 连接、权限或占用问题
- 按键触发不稳定：调整 `KEY_DEBOUNCE_MS`，并确认按键接地触发逻辑

---

# English Version

This branch implements a dual-key control example on STM32F103C8T6 (Blue Pill), using pure register programming without HAL.

Branch flow:

```text
1. Configure SysTick
2. Initialize keys
3. Initialize LEDs/outputs
4. Press key: toggle output state
```

- Press `PB0` to toggle `PC13` (on-board LED)
- Press `PB1` to toggle `PA1` (external auxiliary output)

The project uses CMake + arm-none-eabi toolchain, suitable for learning GPIO input/output, key debounce, and modular bare-metal design.

## Features

- Pure register-level implementation for low-level understanding
- Pull-up key input (active-low when pressed)
- Software debounce, one event per key press
- Dual-channel mapping:
  - `PB0 -> PC13`
  - `PB1 -> PA1`
- Modular structure: `SYS` / `KEY` / `LED(output)` / `main`

## Current Pin Mapping

- `PB0`: key input (pull-up, active-low)
- `PB1`: key input (pull-up, active-low)
- `PC13`: on-board LED output (toggle control)
- `PA1`: external LED/load output (toggle control)

> Note: `PA1` usually needs an external LED with a series resistor (220R~1k) to GND for observation.

## Project Structure (This Branch)

- `src/main.c`: main loop and key-output mapping
- `src/KEY.c` + `include/KEY.h`: key init, debounce scan, event API
- `src/LED.c` + `include/LED.h`: output init and toggle API
- `src/SYS.c` + `include/SYS.h`: SysTick 1ms time base and blocking delay
- `startup/startup_stm32f103c8tx.s`: vector table, data copy, BSS clear
- `linker/STM32F103C8TX_FLASH.ld`: linker script (64KB Flash / 20KB RAM)

## Tunable Parameter

- `KEY_DEBOUNCE_MS` (`include/KEY.h`)  
Key debounce time, default is `20ms`.  
You can try `10~15ms` for faster response if hardware is stable.

## Dependencies (Arch Linux)

```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib openocd stlink
```

## Build

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

Generated artifacts:

- `build/stm32f103_baremetal.elf`
- `build/stm32f103_baremetal.hex`
- `build/stm32f103_baremetal.bin`

## Flash

```bash
cmake --build build --target flash
```

Manual flash command (optional):

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/stm32f103_baremetal.elf verify reset exit"
```

## FAQ

- `PC13` works but `PA1` does not: check LED polarity and series resistor first
- `flash` reports `open failed`: usually ST-Link connection/permission/device-busy issue
- Unstable key triggering: tune `KEY_DEBOUNCE_MS` and ensure active-low wiring to GND

