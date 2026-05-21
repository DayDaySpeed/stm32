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
