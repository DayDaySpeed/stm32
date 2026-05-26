# 驱动接口约定（教学型工业级模板）

## 目标

- 保留寄存器级可见性，方便学习底层原理。
- 让模块接口可预测、可组合、可诊断，避免“每个驱动一套脾气”。

## 统一约定

### 1. 返回值

- 所有**可能失败**的公开接口统一返回 `stm_status_t`。
- 常用语义：
  - `STM_OK`：成功
  - `STM_ERR_INVALID_ARG`：参数非法
  - `STM_ERR_BUSY`：非阻塞接口当前没有数据/资源不可用
  - `STM_ERR_TIMEOUT`：轮询硬件超时
  - `STM_ERR_OVERFLOW`：缓冲区或内部行缓冲溢出
  - `STM_ERR_IO`：总线/外设操作失败
  - `STM_ERR_NOT_INITIALIZED`：模块尚未初始化或已停止

### 2. 命名

- 初始化：
  - `module_init_with_config(const module_config_t *config)`
  - `module_init(...)` 仅作为默认/便捷包装
- 非阻塞轮询：
  - `*_read_*_try(...)`
  - 无数据时返回 `STM_ERR_BUSY`
- 阻塞式发送/接收：
  - `*_write_*_blocking(...)`
  - `*_read_*_blocking(...)`
- 裸寄存器实例名只保留在驱动/BSP 层；应用层尽量使用逻辑角色名。

### 3. 配置方式

- 新驱动优先使用 `config struct`，便于以后扩展参数而不破坏函数签名。
- 简单场景可保留便捷包装，但包装最终都应落到 `*_init_with_config()`。

### 4. 板级绑定

- `app` 层不直接依赖 `USART1`、`TIM2_CH1`、`ADC1_IN1` 这类绑定。
- 默认板级绑定统一收敛到 `bsp/board_devices`：
  - `console`
  - `display`
  - `status_led`
  - `wheel_encoder`
  - `ambient_light`

### 5. 兼容接口

- 为了迁移平滑，旧接口可以暂时保留。
- 但旧接口应明确标注为“兼容包装”，不再作为新代码的推荐入口。

## 推荐模板

```c
typedef struct {
  uint32_t option_a;
  uint8_t option_b;
} demo_config_t;

stm_status_t demo_init_with_config(const demo_config_t *config);
stm_status_t demo_init(void);

stm_status_t demo_read_try(uint16_t *out_value);
stm_status_t demo_write_blocking(const uint8_t *data, uint16_t len);
```

## 应用层推荐用法

```c
stm_status_t st = bsp_console_read_line_try(line, sizeof(line));
if (st == STM_OK) {
  /* 处理一整行 */
} else if (st == STM_ERR_BUSY) {
  /* 当前无数据，主循环继续跑其他任务 */
} else {
  /* 记录或上报故障 */
}
```

---

# English

# Driver Interface Conventions (Teaching-Oriented Industrial Template)

## Goals

- Preserve register-level visibility for learning underlying hardware behavior.
- Make module interfaces predictable, composable, and diagnosable—avoiding "every driver has its own quirks."

## Unified Conventions

### 1. Return Values

- All public interfaces that **can fail** return `stm_status_t`.
- Common semantics:
  - `STM_OK`: Success
  - `STM_ERR_INVALID_ARG`: Invalid parameter
  - `STM_ERR_BUSY`: Non-blocking interface has no data / resource currently unavailable
  - `STM_ERR_TIMEOUT`: Hardware polling timed out
  - `STM_ERR_OVERFLOW`: Buffer or internal line buffer overflow
  - `STM_ERR_IO`: Bus/peripheral operation failed
  - `STM_ERR_NOT_INITIALIZED`: Module not initialized or already stopped

### 2. Naming

- Initialization:
  - `module_init_with_config(const module_config_t *config)`
  - `module_init(...)` as default/convenience wrapper only
- Non-blocking polling:
  - `*_read_*_try(...)`
  - Returns `STM_ERR_BUSY` when no data is available
- Blocking send/receive:
  - `*_write_*_blocking(...)`
  - `*_read_*_blocking(...)`
- Raw peripheral instance names stay in the driver/BSP layer; the application layer should use logical role names where possible.

### 3. Configuration

- New drivers should prefer `config struct` patterns to extend parameters without breaking function signatures.
- Simple cases may keep convenience wrappers, but wrappers must ultimately call `*_init_with_config()`.

### 4. Board-Level Binding

- The `app` layer must not depend directly on bindings such as `USART1`, `TIM2_CH1`, or `ADC1_IN1`.
- Default board bindings are centralized in `bsp/board_devices`:
  - `console`
  - `display`
  - `status_led`
  - `wheel_encoder`
  - `ambient_light`

### 5. Compatibility Interfaces

- Legacy interfaces may be retained temporarily for smooth migration.
- Legacy interfaces must be clearly marked as compatibility wrappers and are not the recommended entry point for new code.

## Recommended Template

```c
typedef struct {
  uint32_t option_a;
  uint8_t option_b;
} demo_config_t;

stm_status_t demo_init_with_config(const demo_config_t *config);
stm_status_t demo_init(void);

stm_status_t demo_read_try(uint16_t *out_value);
stm_status_t demo_write_blocking(const uint8_t *data, uint16_t len);
```

## Recommended Application Usage

```c
stm_status_t st = bsp_console_read_line_try(line, sizeof(line));
if (st == STM_OK) {
  /* Process a complete line */
} else if (st == STM_ERR_BUSY) {
  /* No data yet; main loop continues other tasks */
} else {
  /* Log or report fault */
}
```
