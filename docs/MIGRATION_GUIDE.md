# 迁移指南

## 概述

本项目从单体驱动风格迁移为：

- `common/` 共享基础设施
- `hal/` 外设事务层
- `drivers/` 带面向对象 C 结构体的设备级逻辑

## API 映射

### SSD1306

- 旧接口：
  - `ssd1306_oled_init()`
  - `ssd1306_oled_putc()`
  - `ssd1306_oled_refresh()`
- 新接口：
  - `ssd1306_t *dev = ssd1306_default();`
  - `ssd1306_init(dev)`
  - `ssd1306_putc(dev, ch)`
  - `ssd1306_flush(dev)`

`ssd1306_oled.h` 中仍提供兼容包装函数。

### USART1

- 旧接口：
  - `usart1_try_read_byte()`
  - `usart1_try_read_string()`
- 新增：
  - `usart1_set_line_policy()` 用于选择 `CR/LF/CRLF`
  - 内部 RX 路径现使用可复用的 `ring_buffer_t`

## 新增构建选项

- `DEBUG_LOG`（ON/OFF）
- `ASSERT_LEVEL`（`0..2`）
- `OLED_REFRESH_MODE`（`AUTO|FULL|REGION`，预留用于刷新策略切换）

## 新模块迁移建议步骤

1. 将寄存器级事务流程放在 `hal/`
2. 将协议/设备逻辑放在 `drivers/`
3. 对外仅暴露类型化状态返回值（`stm_status_t`）
4. 迁移 `app/` 期间保留可选的兼容包装函数

---

# Migration Guide (Industrial Refactor)

## Overview

This project moved from a monolithic driver style to:

- `common/` shared foundations
- `hal/` peripheral transaction layer
- `drivers/` device-level logic with object-oriented C structs

## API Mapping

### SSD1306

- Old:
  - `ssd1306_oled_init()`
  - `ssd1306_oled_putc()`
  - `ssd1306_oled_refresh()`
- New:
  - `ssd1306_t *dev = ssd1306_default();`
  - `ssd1306_init(dev)`
  - `ssd1306_putc(dev, ch)`
  - `ssd1306_flush(dev)`

Compatibility wrappers are still provided in `ssd1306_oled.h`.

### USART1

- Old:
  - `usart1_try_read_byte()`
  - `usart1_try_read_string()`
- New additions:
  - `usart1_set_line_policy()` to select `CR/LF/CRLF`
  - internal RX path now uses reusable `ring_buffer_t`

## New Build Options

- `DEBUG_LOG` (ON/OFF)
- `ASSERT_LEVEL` (`0..2`)
- `OLED_REFRESH_MODE` (`AUTO|FULL|REGION`, reserved for strategy switching)

## Suggested Migration Steps for New Modules

1. put register-level transaction flow in `hal/`
2. put protocol/device logic in `drivers/`
3. expose only typed status returns (`stm_status_t`)
4. keep optional compatibility wrappers while migrating `app/`
