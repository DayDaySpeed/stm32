# 编码规范（Baremetal C）

## 目标

- 保持代码可读、可维护、可扩展。
- 新模块接入时，不破坏现有目录分层。

## 目录分层

- `src/app`：应用层逻辑（状态机、业务流程、任务调度）。
- `src/bsp`：板级初始化（时钟门控、与具体 PCB 相关的默认配置）。
- `src/drivers`：外设驱动层（USART、SysTick、SSD1306 等）。
- `include/bsp`：芯片寄存器、板级引脚宏、RCC 组合掩码、`board_init` 声明。
- `include/drivers`：驱动公开接口（`.h`）。
- `include/app`：应用层公开接口（`.h`）。
- `cmake/stm32_sources.cmake`：固件 `.c/.s` 源文件清单，增删模块时集中维护。

## 层间依赖

- `app` 优先依赖 `bsp/board_devices.h` 这样的逻辑设备入口，不直接依赖具体 `USART1/TIM2/ADC1` 绑定。
- `drivers` 负责外设语义，不负责决定“本板默认用哪个实例”。
- `bsp` 负责板级默认绑定、时钟门控、默认设备组合。
- `common` 提供状态码、断言、日志、故障处理等横切基础设施。

## 命名规则

- 函数：`module_action_target`，例如 `usart1_send_byte`。
- 文件：小写下划线命名，例如 `usart1.c`。
- 宏常量：全大写下划线命名，例如 `USART_CR1_UE_BIT`。
- 静态私有变量：`g_` 前缀，例如 `g_ms_ticks`。

## 代码风格

- 统一使用 C11。
- 缩进使用 2 个空格。
- 一个函数只做一件事，优先保持短小。
- 注释写“为什么”，避免解释显而易见的“做什么”。
- 裸寄存器读写集中在驱动层与 `bsp`，应用层不直接操作寄存器。
- 驱动依赖的板级时钟门控由 `bsp_board_init()` 完成；驱动头文件中注明前置条件。

## 驱动接口约定

- 公开 API 统一优先返回 `stm_status_t`。
- `*_init_with_config()` 是主入口；`*_init()` 仅作为默认参数包装。
- `*_read_*_try()` 表示非阻塞；无数据/条件未满足时返回 `STM_ERR_BUSY`。
- `*_write_*_blocking()` / `*_read_*_blocking()` 表示会忙等硬件状态。
- 若保留旧接口，必须在头文件中标明“兼容包装”，新代码不再推荐直接使用。
- 详细约定见 [`docs/DRIVER_API_GUIDE.md`](./DRIVER_API_GUIDE.md)。

## 提交流程建议

- 每次新增模块，至少包含 `.c + .h`。
- 每次重构后执行：
  - `cmake --build build`
  - 串口/板载 LED 最小冒烟测试
