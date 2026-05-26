# USART1 控制台驱动（`usart1`）

## 作用

**115200 8N1** 串口控制台：阻塞发送、**RX 中断 + 环形缓冲**、按行非阻塞读取（支持退格与多种换行策略）。

## 硬件

| 信号 | 引脚 |
|------|------|
| TX | PA9 复用推挽 |
| RX | PA10 浮空输入 |

USART1 在 APB2，BRR 由 `bsp_clock_get_pclk2_hz()` 计算。

## 配置结构体

```c
typedef struct {
  uint32_t baudrate;
  usart_oversampling_t oversampling;   /* 16 或 8 倍 */
  usart1_line_policy_t line_policy;    /* CR/LF/CRLF/CR或LF */
  uint8_t enable_rx_interrupt;         /* init 时是否开 RXNE 中断 */
} usart1_config_t;
```

## API 参考

| 函数 | 说明 |
|------|------|
| `usart1_init_with_config(config)` | BRR、GPIO、UE/TE/RE |
| `usart1_init(baud, oversampling)` | 便捷包装 |
| `usart1_enable_rx_interrupt()` | RXNEIE + NVIC |
| `usart1_irq_handler()` | 在 `USART1_IRQHandler` 调用 |
| `usart1_write_byte_blocking` / `_string_blocking` | 等 TXE 发送 |
| `usart1_read_byte_try(out)` | 无数据 → `STM_ERR_BUSY` |
| `usart1_read_line_try(out, size)` | 读到换行 → `STM_OK`；缓冲满 → `STM_ERR_OVERFLOW` |
| `usart1_set_line_policy(policy)` | 运行时改换行策略 |

兼容旧 API：`usart1_send_byte` 等（void/uint8_t 返回）。

## 实现说明

### RX 路径

```
RXNE 中断 → 读 DR → ring_buffer_push(64 字节)
主循环 → read_line_try → 从 ring 取字节，组行，支持退格 0x08
```

溢出时丢弃新字节并置 `g_usart1_rx_overflow`（可扩展诊断）。

### BRR 计算

16 倍：`BRR = round(PCLK/baud)`  
8 倍：mantissa/fraction 拆分写入 BRR 低 4 位（F103 要求 BRR[3]=0）。

### 为什么 line_policy 可配

不同终端（minicom / screen / Windows）换行符不一致；`CR_OR_LF` 最宽松。

## 使用示例

```c
usart1_init(115200UL, USART_OVERSAMPLING_16);
usart1_enable_rx_interrupt();

char line[64];
if (usart1_read_line_try(line, sizeof(line)) == STM_OK) {
  usart1_write_string_blocking("echo: ");
  usart1_write_string_blocking(line);
}
```

应用层用 `bsp_console_*`；中断向量调 `bsp_console_irq_handler()`。

更完整 USART/NVIC 说明见 [topics/USART.md](../topics/USART.md)、[topics/NVIC.md](../topics/NVIC.md)。

## 常见坑

- 未开 USART1/GPIOA 时钟 → 无输出
- 未 enable RX 中断 → `read_line_try` 永远 BUSY
- 行缓冲小于输入 → `STM_ERR_OVERFLOW`

---

# English

# USART1 Console Driver (`usart1`)

## Purpose

**115200 8N1** serial console: blocking transmit, **RX interrupt + ring buffer**, non-blocking line read (supports backspace and multiple newline policies).

## Hardware

| Signal | Pin |
|------|------|
| TX | PA9 alternate-function push-pull |
| RX | PA10 floating input |

USART1 is on APB2; BRR computed from `bsp_clock_get_pclk2_hz()`.

## Configuration Structure

```c
typedef struct {
  uint32_t baudrate;
  usart_oversampling_t oversampling;   /* 16× or 8× */
  usart1_line_policy_t line_policy;    /* CR/LF/CRLF/CR or LF */
  uint8_t enable_rx_interrupt;         /* enable RXNE interrupt at init */
} usart1_config_t;
```

## API Reference

| Function | Description |
|------|------|
| `usart1_init_with_config(config)` | BRR, GPIO, UE/TE/RE |
| `usart1_init(baud, oversampling)` | Convenience wrapper |
| `usart1_enable_rx_interrupt()` | RXNEIE + NVIC |
| `usart1_irq_handler()` | Call from `USART1_IRQHandler` |
| `usart1_write_byte_blocking` / `_string_blocking` | Wait for TXE, then transmit |
| `usart1_read_byte_try(out)` | No data → `STM_ERR_BUSY` |
| `usart1_read_line_try(out, size)` | Line complete → `STM_OK`; buffer full → `STM_ERR_OVERFLOW` |
| `usart1_set_line_policy(policy)` | Change newline policy at runtime |

Legacy API: `usart1_send_byte`, etc. (void/uint8_t return).

## Implementation Notes

### RX Path

```
RXNE interrupt → read DR → ring_buffer_push(64 bytes)
Main loop → read_line_try → dequeue bytes, assemble line, support backspace 0x08
```

On overflow, new bytes are discarded and `g_usart1_rx_overflow` is set (extensible for diagnostics).

### BRR Calculation

16×: `BRR = round(PCLK/baud)`  
8×: mantissa/fraction split into BRR low 4 bits (F103 requires BRR[3]=0).

### Why line_policy Is Configurable

Different terminals (minicom / screen / Windows) use different line endings; `CR_OR_LF` is most permissive.

## Usage Example

```c
usart1_init(115200UL, USART_OVERSAMPLING_16);
usart1_enable_rx_interrupt();

char line[64];
if (usart1_read_line_try(line, sizeof(line)) == STM_OK) {
  usart1_write_string_blocking("echo: ");
  usart1_write_string_blocking(line);
}
```

Application layer uses `bsp_console_*`; interrupt vector calls `bsp_console_irq_handler()`.

Full USART/NVIC details: [topics/USART.md](../topics/USART.md), [topics/NVIC.md](../topics/NVIC.md).

## Common Pitfalls

- USART1/GPIOA clock not enabled → no output
- RX interrupt not enabled → `read_line_try` always BUSY
- Line buffer smaller than input → `STM_ERR_OVERFLOW`
