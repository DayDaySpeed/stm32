# STM32F103C8T6 Baremetal (CMake)

这是一个最小可用的 STM32F103C8T6 裸机寄存器工程，使用 CMake + arm-none-eabi 工具链，不依赖 Keil。

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

## 工程说明

- `src/main.c`：纯寄存器方式点亮 Blue Pill 板载 LED（PC13，低电平点亮，代码里做翻转）。
- `startup/startup_stm32f103c8tx.s`：向量表 + 数据段拷贝 + BSS 清零。
- `linker/STM32F103C8TX_FLASH.ld`：64KB Flash / 20KB RAM 链接脚本。
