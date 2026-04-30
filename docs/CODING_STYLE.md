# 编码规范（Baremetal C）

## 目标

- 保持代码可读、可维护、可扩展。
- 新模块接入时，不破坏现有目录分层。

## 目录分层

- `src/app`：应用层逻辑（状态机、业务流程、任务调度）。
- `src/drivers`：外设驱动层（USART、GPIO、SysTick 等）。
- `include/bsp`：芯片寄存器定义与时钟常量。
- `include/drivers`：驱动公开接口（`.h`）。
- `include/app`：应用层公开接口（`.h`）。
- `include/GPIO.h`、`include/RCC.h`、`include/SYS.h`：对外稳定兼容入口。

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
- 裸寄存器读写集中在驱动层，应用层不直接操作寄存器。
- 驱动优先包含稳定入口头（`GPIO.h/RCC.h/SYS.h`），避免直接依赖底层路径。

## 提交流程建议

- 每次新增模块，至少包含 `.c + .h`。
- 每次重构后执行：
  - `cmake --build build`
  - 串口/板载 LED 最小冒烟测试
