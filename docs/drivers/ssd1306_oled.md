# SSD1306 OLED 驱动（`ssd1306_oled` + `oled_font5x7`）

## 作用

驱动 **128×64 SSD1306** OLED（I2C）：帧缓冲、5×7 字体、按页/列局部刷新、简易 `printf` 子集。默认实例绑定 `I2C1` + 地址 `0x3C`。

## 硬件

| 项目 | 说明 |
|------|------|
| 接口 | I2C1 PB8(SCL) / PB9(SDA) |
| 地址 | 7-bit `0x3C` |
| 分辨率 | 128×64，8 个 page（每 page 8 像素高） |

## 核心对象

```c
typedef struct {
  uint16_t width, page_count;
  uint8_t addr7;
  ssd1306_bus_write_fn bus_write;  /* 默认 i2c1_master_write_frame */
  uint8_t *framebuffer;
  uint16_t col_px, row_page;
  uint8_t initialized;
} ssd1306_t;
```

## API 参考（对象 API）

| 函数 | 说明 |
|------|------|
| `ssd1306_init(dev)` | 发初始化命令序列 |
| `ssd1306_clear(dev)` | 清帧缓冲 |
| `ssd1306_putc(dev, c)` | 写字符到光标，`\n` 滚屏 |
| `ssd1306_flush(dev)` | 把脏区域推送到屏 |
| `ssd1306_write_text_at(dev, page, col, text)` | 定点字符串 |
| `ssd1306_write_text_atf(dev, page, col, fmt, ...)` | 格式化（子集） |

**默认实例**：`ssd1306_default_*()` — 本板推荐入口。

**兼容包装**：`ssd1306_oled_*()` 吞掉错误码，新代码勿用。

## 字库 `oled_font5x7.c`

- 95 个 ASCII 可打印字符 × 5 列 × 7 行 = 常量数组
- 无独立 `.h`，由 `ssd1306_oled.c` `extern` 引用
- 占用 Flash ~1280 字节，换字体只改此文件

## 实现说明

### 总线与 SSD1306 分离

驱动通过 `bus_write(addr7, ctrl, payload, len)` 回调发 I2C：

- `ctrl=0x00`：命令流
- `ctrl=0x40`：GDDRAM 数据

便于以后换 SPI 或 mock 测试，SSD1306 协议不绑死 I2C 寄存器。

### 刷新策略

CMake 选项 `OLED_REFRESH_MODE`：

- **AUTO**：改动的 page/列窗口刷新（省 I2C 时间）
- **FULL**：全屏 1024 字节
- **REGION**：固定区域

局部刷新失败多次时，`bsp_display_recover()` 做 I2C 总线恢复 + 重 init。

### 为什么帧缓冲在驱动内

128×64 = 1KB RAM，F103 有 20KB，可接受。先改 RAM 再一次 I2C 推送，避免每个像素一次事务。

## 使用示例

```c
ssd1306_default_init();
ssd1306_default_write_text_atf(0U, 0U, "ENC=%d", enc);
ssd1306_default_refresh();
```

应用层：`bsp_display_write_text_atf(page, col, fmt, ...)`。

I2C 与协议细节见 [topics/I2C.md](../topics/I2C.md)。

## 常见坑

- I2C 上拉、地址 0x3C/0x3D 跳线
- 只 write 不 refresh → 屏不更新
- `page >= 8` 或 `col >= 128` → 静默裁剪或失败
- 串口行与调试 page 争用同一 8 行 → app 满 8 行清屏

---

# English

# SSD1306 OLED Driver (`ssd1306_oled` + `oled_font5x7`)

## Purpose

Drives **128×64 SSD1306** OLED (I2C): framebuffer, 5×7 font, page/column partial refresh, minimal `printf` subset. Default instance bound to `I2C1` + address `0x3C`.

## Hardware

| Item | Description |
|------|------|
| Interface | I2C1 PB8(SCL) / PB9(SDA) |
| Address | 7-bit `0x3C` |
| Resolution | 128×64, 8 pages (8 pixels high per page) |

## Core Object

```c
typedef struct {
  uint16_t width, page_count;
  uint8_t addr7;
  ssd1306_bus_write_fn bus_write;  /* default i2c1_master_write_frame */
  uint8_t *framebuffer;
  uint16_t col_px, row_page;
  uint8_t initialized;
} ssd1306_t;
```

## API Reference (Object API)

| Function | Description |
|------|------|
| `ssd1306_init(dev)` | Send initialization command sequence |
| `ssd1306_clear(dev)` | Clear framebuffer |
| `ssd1306_putc(dev, c)` | Write character at cursor; `\n` scrolls |
| `ssd1306_flush(dev)` | Push dirty regions to display |
| `ssd1306_write_text_at(dev, page, col, text)` | Fixed-position string |
| `ssd1306_write_text_atf(dev, page, col, fmt, ...)` | Formatted output (subset) |

**Default instance**: `ssd1306_default_*()` — recommended entry point on this board.

**Legacy wrappers**: `ssd1306_oled_*()` swallow error codes; do not use in new code.

## Font Library `oled_font5x7.c`

- 95 printable ASCII characters × 5 columns × 7 rows = constant array
- No separate `.h`; referenced via `extern` in `ssd1306_oled.c`
- ~1280 bytes Flash; change font by editing this file only

## Implementation Notes

### Bus and SSD1306 Decoupling

Driver sends I2C via `bus_write(addr7, ctrl, payload, len)` callback:

- `ctrl=0x00`: command stream
- `ctrl=0x40`: GDDRAM data

Enables future SPI or mock testing; SSD1306 protocol is not tied to I2C registers.

### Refresh Strategy

CMake option `OLED_REFRESH_MODE`:

- **AUTO**: refresh modified page/column window (saves I2C time)
- **FULL**: full screen 1024 bytes
- **REGION**: fixed region

After repeated partial refresh failures, `bsp_display_recover()` performs I2C bus recovery + re-init.

### Why Framebuffer Lives in Driver

128×64 = 1 KB RAM; F103 has 20 KB, acceptable. Modify RAM first, then one I2C push, avoiding one transaction per pixel.

## Usage Example

```c
ssd1306_default_init();
ssd1306_default_write_text_atf(0U, 0U, "ENC=%d", enc);
ssd1306_default_refresh();
```

Application layer: `bsp_display_write_text_atf(page, col, fmt, ...)`.

I2C and protocol details: [topics/I2C.md](../topics/I2C.md).

## Common Pitfalls

- I2C pull-ups, address 0x3C/0x3D jumper
- Write without refresh → display does not update
- `page >= 8` or `col >= 128` → silent clip or failure
- Serial lines and debug pages share 8 rows → app clears screen when full
