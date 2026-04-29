# STM32F103C8T6 Bare-Metal Demo (CMake)

> Chinese & English documentation for the same project.

- [跳转到中文](#中文说明)
- [Jump to English](#english)

---

## 中文说明

这是一个基于 `STM32F103C8T6` 的裸机寄存器工程，使用 `CMake + arm-none-eabi` 工具链构建，使用 `OpenOCD + ST-Link` 烧录。

项目特性：

- 无 HAL、无标准外设库，直接操作寄存器。
- 自定义启动文件（向量表、`.data` 拷贝、`.bss` 清零）。
- 自定义链接脚本（`64KB FLASH / 20KB RAM`）。
- 通过 `SysTick` 生成 1ms 节拍，实现毫秒延时。
- 示例程序将 `GPIOA/GPIOB/GPIOC` 全部 48 个引脚配置为 `2MHz 推挽输出`，并按端口与引脚顺序轮流闪烁。

### 目录结构

```text
.
├── CMakeLists.txt
├── cmake/arm-gcc-toolchain.cmake
├── include/
│   ├── GPIO.h
│   ├── RCC.h
│   └── SYS.h
├── linker/STM32F103C8TX_FLASH.ld
├── openocd.cfg
├── startup/startup_stm32f103c8tx.s
└── src/main.c
```

### 环境依赖

以 Arch Linux 为例：

```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib openocd stlink
```

### 构建

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

构建完成后会生成：

- `build/stm32f103_baremetal.elf`
- `build/stm32f103_baremetal.hex`
- `build/stm32f103_baremetal.bin`
- `build/stm32f103_baremetal.map`

### 烧录（ST-Link + OpenOCD）

推荐方式（使用 CMake 自定义目标）：

```bash
cmake --build build --target flash
```

等价手动命令：

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/stm32f103_baremetal.elf verify reset exit"
```

你也可以参考脚本：

```bash
./burning.sh
```

### 实现要点

- `src/main.c`
  - 使能 `RCC_APB2ENR` 中 `GPIOA/GPIOB/GPIOC` 时钟。
  - 初始化 `SysTick`（8MHz 时钟下每 1ms 中断一次）。
  - 将 A/B/C 端口 `CRL/CRH` 全部写为 `0x22222222`（2MHz 推挽输出）。
  - 在主循环中遍历每个引脚，翻转 `ODR` 位实现闪烁。
- `startup/startup_stm32f103c8tx.s`
  - 定义中断向量表与 `Reset_Handler`。
  - 在复位后完成 `.data` 拷贝和 `.bss` 清零，再跳转 `main`。
- `linker/STM32F103C8TX_FLASH.ld`
  - 内存布局：`FLASH @ 0x08000000 (64K)`，`RAM @ 0x20000000 (20K)`。

### 注意事项

- 该示例会切换多个 GPIO 引脚电平。若接有外设，请确认不会引发冲突。
- `PC13` 在 Blue Pill 上连接板载 LED（低电平点亮），但本工程并不只操作 `PC13`。
- 默认 `SYSCLK_HZ = 8000000`，如果系统时钟源变更，请同步修改 `include/SYS.h`。

---

## English

This is a bare-metal register-level project for `STM32F103C8T6`, built with `CMake + arm-none-eabi`, and flashed via `OpenOCD + ST-Link`.

Highlights:

- No HAL and no StdPeriph library; direct register programming only.
- Custom startup code (vector table, `.data` copy, `.bss` zeroing).
- Custom linker script (`64KB FLASH / 20KB RAM`).
- `SysTick` configured for a 1 ms tick, used for millisecond delays.
- Demo configures all pins on `GPIOA/GPIOB/GPIOC` (48 pins total) as `2 MHz push-pull outputs` and blinks them sequentially.

### Project Layout

```text
.
├── CMakeLists.txt
├── cmake/arm-gcc-toolchain.cmake
├── include/
│   ├── GPIO.h
│   ├── RCC.h
│   └── SYS.h
├── linker/STM32F103C8TX_FLASH.ld
├── openocd.cfg
├── startup/startup_stm32f103c8tx.s
└── src/main.c
```

### Prerequisites

Example on Arch Linux:

```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib openocd stlink
```

### Build

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

Generated artifacts:

- `build/stm32f103_baremetal.elf`
- `build/stm32f103_baremetal.hex`
- `build/stm32f103_baremetal.bin`
- `build/stm32f103_baremetal.map`

### Flash (ST-Link + OpenOCD)

Recommended:

```bash
cmake --build build --target flash
```

Equivalent manual command:

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/stm32f103_baremetal.elf verify reset exit"
```

Or use:

```bash
./burning.sh
```

### Implementation Notes

- `src/main.c`
  - Enables clocks for `GPIOA/GPIOB/GPIOC` via `RCC_APB2ENR`.
  - Initializes `SysTick` for 1 ms interrupts at an 8 MHz system clock.
  - Sets all GPIO config fields (`CRL/CRH`) to `0x22222222` (2 MHz push-pull output).
  - Iterates through ports and pins, toggling `ODR` bits to blink.
- `startup/startup_stm32f103c8tx.s`
  - Defines the vector table and `Reset_Handler`.
  - Copies `.data`, clears `.bss`, then branches to `main`.
- `linker/STM32F103C8TX_FLASH.ld`
  - Memory map: `FLASH @ 0x08000000 (64K)`, `RAM @ 0x20000000 (20K)`.

### Notes

- The demo toggles many GPIO pins. If external peripherals are connected, verify there is no conflict.
- On Blue Pill, `PC13` is connected to the onboard LED (active low), but this project is not limited to `PC13`.
- Default `SYSCLK_HZ` is `8000000`; if your clock setup changes, update `include/SYS.h` accordingly.
