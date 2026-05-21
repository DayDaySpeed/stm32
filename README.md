# STM32F103C8T6 裸机固件工程

基于 **STM32F103C8T6**（64 KB Flash / 20 KB RAM）的寄存器级裸机项目，使用 **CMake + arm-none-eabi** 工具链构建，无 HAL/Cube 依赖。代码采用分层架构，接口统一、可测试、可移植，适用于产品原型开发与嵌入式教学。

完整文档入口：[docs/INDEX.md](docs/INDEX.md)

---

## 特性概览

| 类别 | 能力 |
|------|------|
| 人机交互 | USART1 控制台（115200，RX 中断 + 行读取）；SSD1306 128×64 OLED（I2C1） |
| 执行器 | TB6612 直流电机（PWM + 正反转）；有源蜂鸣器 |
| 传感与指示 | ADC1 三路 SCAN+DMA（光敏 / NTC / 反射红外）；TIM1 双路传感器指示 LED |
| 输入 | TIM3 正交编码器（旋钮调速） |
| 状态 | TIM2 状态灯 PWM 呼吸效果 |
| 系统 | HSE+PLL 72 MHz；SysTick 1 ms；统一 `stm_status_t` 错误码；故障停机与可选日志 |

应用层采用 **合作式任务调度**（`app.c`）：呼吸灯、编码器调速、红外接近检测、OLED 调试页、串口行输入显示等，均基于 `systick_get_ms()` 非阻塞运行。

---

## 软件架构

```
app/          业务逻辑、任务调度
  ↓
bsp/          板级引脚、时钟门控、逻辑设备（board_devices）
  ↓
drivers/      外设驱动（USART、PWM、ADC、OLED…）
hal/          总线事务（I2C1 主机写帧）
  ↓
common/       状态码、环形缓冲、日志、断言
bsp/regs      寄存器映射（stm32f103_regs.h）
```

**设计原则**

- 应用层通过 `bsp/board_devices.h` 访问硬件，不直接绑定 `USART1` / `TIM2` 等实例名。
- 驱动返回 `stm_status_t`；初始化使用 `*_init_with_config()` + 配置结构体。
- 板级可调参数集中在 `include/bsp/board_config.h`（编码器方向、红外阈值、蜂鸣器极性等）。

详见 [docs/bsp/README.md](docs/bsp/README.md)、[docs/DRIVER_API_GUIDE.md](docs/DRIVER_API_GUIDE.md)。

---

## 硬件连接（默认）

| 功能 | 引脚 | 说明 |
|------|------|------|
| 状态 LED | PA0 | TIM2_CH1 PWM |
| 光敏 / 热敏 / 红外 | PA1 / PA2 / PA3 | ADC1 IN1–IN3 |
| 蜂鸣器 | PA4 | GPIO |
| 编码器 A/B | PA6 / PA7 | TIM3 |
| LDR / NTC 指示 LED | PA8 / PA11 | TIM1 CH1 / CH4 |
| 串口 TX / RX | PA9 / PA10 | USART1 → USB 串口模块 |
| OLED I2C | PB8 / PB9 | I2C1 |
| 电机 PWM / 方向 | PB6 / PB5 / PB7 | TIM4 + TB6612 |

串口模块（如 CH340）：**PA9 → RXD，PA10 → TXD，GND 共地**，115200 8N1。

引脚图与模块接线见 `Document/` 目录。

---

## 快速开始

### 依赖（Arch Linux 示例）

```bash
sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-binutils \
  arm-none-eabi-newlib openocd stlink minicom
```

### 构建

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
cmake --build build
```

产物：`build/stm32f103_baremetal.{elf,hex,bin}`

可选 CMake 选项：`DEBUG_LOG`、`ASSERT_LEVEL`、`OLED_REFRESH_MODE`（见 `CMakeLists.txt`）。

### 烧录与串口

```bash
./burning.sh --flash-only              # 编译并烧录
./burning.sh -m -d /dev/ttyUSB0        # 烧录后打开 minicom
./burning.sh --minicom-only -d /dev/ttyUSB0 -b 115200
```

或通过 CMake：`cmake --build build --target flash`（需 OpenOCD + ST-Link）。

上电后串口输出 `board console ready`；输入一行文本回车可在 OLED 对应行显示。

---

## 启动流程

```
main
 ├─ bsp_clock_apply_profile(HSE_PLL_72MHZ)
 ├─ bsp_board_init()              // 外设时钟门控
 ├─ bsp_dc_motor_gpio_safe_early() // 电机安全态
 ├─ app_init()
 │    ├─ systick_init_1ms()
 │    └─ bsp_default_devices_init()
 └─ app_run_forever()             // 合作式任务 + 串口行处理
```

中断：`SysTick_Handler` → 毫秒节拍；`USART1_IRQHandler` → 接收环形缓冲。

---

## 目录结构

```
├── src/
│   ├── main.c              入口与中断转发
│   ├── app/                应用任务
│   ├── bsp/                板级 init、时钟、board_devices
│   ├── drivers/            外设驱动
│   ├── hal/                I2C1 主机
│   └── common/             状态码、缓冲、日志
├── include/                与 src 对应的分层头文件
├── startup/                启动汇编与向量表
├── linker/                 链接脚本（64K/20K）
├── cmake/                  工具链与源文件清单
├── docs/                   全部项目文档（见 INDEX.md）
├── Document/               引脚图、数据手册等
├── burning.sh              烧录与串口脚本
└── CMakeLists.txt
```

增删源文件：编辑 `cmake/stm32_sources.cmake`。

---

## 文档

| 文档 | 说明 |
|------|------|
| [docs/INDEX.md](docs/INDEX.md) | **总索引** |
| [docs/CODING_STYLE.md](docs/CODING_STYLE.md) | 编码与分层规范 |
| [docs/DRIVER_API_GUIDE.md](docs/DRIVER_API_GUIDE.md) | 驱动接口约定 |
| [docs/bsp/](docs/bsp/README.md) | 板级逻辑设备与配置 |
| [docs/drivers/](docs/drivers/README.md) | 各驱动 API 说明 |
| [docs/hal/](docs/hal/README.md) | HAL 层（I2C） |
| [docs/topics/](docs/topics/README.md) | 寄存器原理专题（时钟、PWM、ADC…） |

---

## 质量与维护

```bash
cmake --build build --target format   # clang-format（若已安装）
cmake --build build --target lint     # cppcheck（若已安装）
```

固件体积参考（Debug 构建）：运行 `cmake --build build` 后查看 `arm-none-eabi-size` 输出。

---

## 许可证

见 [LICENSE](LICENSE)。第三方数据手册见 `Document/`。
