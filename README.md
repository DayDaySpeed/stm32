# STM32F103C8T6 Baremetal (CMake)

这是一个最小可用的 STM32F103C8T6 裸机寄存器工程，使用 CMake + arm-none-eabi 工具链，不依赖 Keil。当前示例包含：

- `SysTick` 1ms 节拍与阻塞延时
- `USART1`（PA9/PA10）115200 回显

## 依赖

```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib openocd stlink
```

## 构建

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

构建后会生成：

- `build/stm32f103_baremetal.elf`
- `build/stm32f103_baremetal.hex`
- `build/stm32f103_baremetal.bin`

## 烧录（ST-Link）

```bash
cmake --build build --target flash
```

如果需要手动指定：

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/stm32f103_baremetal.elf verify reset exit"
```

## 目录结构

- `src/main.c`：程序入口与中断入口绑定。
- `src/app/app.c`：应用层逻辑（初始化和主循环）。
- `src/drivers/*.c`：外设驱动实现（`systick`、`usart1`）。
- `include/bsp`：芯片寄存器定义与时钟常量。
- `include/drivers`：驱动接口声明。
- `include/app`：应用接口声明。
- `startup/startup_stm32f103c8tx.s`：向量表 + 数据段拷贝 + BSS 清零。
- `linker/STM32F103C8TX_FLASH.ld`：64KB Flash / 20KB RAM 链接脚本。
- `docs/CODING_STYLE.md`：项目编码规范。

## 串口验证（CH340）

- 接线：`PA9 -> RXD`，`PA10 -> TXD`，`GND -> GND`
- 参数：`115200 8N1`
- 终端示例：

```bash
minicom -D /dev/ttyUSB0 -b 115200
```
