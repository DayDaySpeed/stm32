# TIM3 正交编码器驱动（`encoder`）

## 作用

读取旋钮/电机编码器的 **累计计数** 与 **旋转方向**。硬件 Encoder Mode 3（4× 分辨率），CPU 零中断开销，主循环或周期任务里读 `TIM3_CNT` 即可。

## 硬件与依赖

| 项目 | 说明 |
|------|------|
| 引脚 | PA6 = TIM3_CH1（A 相），PA7 = TIM3_CH2（B 相），输入上拉 |
| 定时器 | TIM3 |
| 方向默认值 | `BOARD_WHEEL_ENCODER_DIRECTION`（`board_config.h`） |
| 前置 | `bsp_clock_apply_profile()` → `bsp_board_init()` |

寄存器细节见 [topics/encoder.md](../topics/encoder.md)。

## 配置与方向

```c
typedef enum {
  TIM3_ENCODER_DIR_NORMAL = 0,    /* A 超前 B → CNT++ */
  TIM3_ENCODER_DIR_INVERTED = 1,  /* 翻转 CC1P，等价交换 A/B 极性 */
} tim3_encoder_dir_t;

typedef struct {
  tim3_encoder_dir_t direction;
} tim3_encoder_config_t;
```

接线反了可在 `board_config.h` 改方向，或对调 PA6/PA7。

## API 参考

| 函数 | 说明 |
|------|------|
| `tim3_encoder_init_with_config(config)` | 配置 GPIO + TIM3 编码器模式 |
| `tim3_encoder_init(direction)` | 便捷包装 |
| `tim3_encoder_read_count(out)` | 读 CNT（带初始化检查） |
| `tim3_encoder_get_count()` | 直接读 CNT（兼容，不检查 init） |
| `tim3_encoder_reset_count()` | CNT 清零 |
| `tim3_encoder_read_direction(out)` | 读 CR1.DIR：0=向上，1=向下 |
| `tim3_encoder_get_direction()` | 兼容包装 |

## 实现说明

### 关键寄存器选择

- **PSC=0**：编码器模式禁止预分频，否则丢脉冲。
- **ARR=0xFFFF**：16 位自由计数，应用层用 `(int16_t)(now - prev)` 算带符号增量。
- **IC1F/IC2F=0xF**：最大数字滤波，挡 EMI 毛刺；正常编码器边沿周期远大于滤波窗口。
- **SMCR.SMS=011**：Encoder Mode 3，TI1+TI2 双边沿，4× 分辨率。

### 为什么只翻 CC1P 而不翻 CC2P

只翻转其中一个通道极性等价于交换 A/B；两个都翻则方向还原。便于软件修正接线而不动硬件。

## 使用示例

```c
tim3_encoder_init(BOARD_WHEEL_ENCODER_DIRECTION);

int16_t prev = tim3_encoder_get_count();
/* 每 20ms */
int16_t now = tim3_encoder_get_count();
int16_t delta = (int16_t)(now - prev);
prev = now;
/* delta 正/负 → 顺/逆时针 */
```

本工程 `app_motor_encoder_task` 用 delta × 步进系数驱动电机速度。

## 常见坑

- 未上拉或 A/B 悬空 → 计数乱跳。
- 把 CNT 当 16 位无符号做差 → 过零点增量错误，应转 `int16_t`。
- `get_count()` 不检查 init → 调试时若忘记 init 仍「有读数」但不可信。

---

# English

# TIM3 Quadrature Encoder Driver (`encoder`)

## Purpose

Reads **accumulated count** and **rotation direction** from a knob or motor encoder. Hardware Encoder Mode 3 (4× resolution), zero CPU interrupt overhead; read `TIM3_CNT` from the main loop or periodic task.

## Hardware and Dependencies

| Item | Description |
|------|------|
| Pins | PA6 = TIM3_CH1 (A phase), PA7 = TIM3_CH2 (B phase), input pull-up |
| Timer | TIM3 |
| Default direction | `BOARD_WHEEL_ENCODER_DIRECTION` (`board_config.h`) |
| Prerequisite | `bsp_clock_apply_profile()` → `bsp_board_init()` |

Register details: [topics/encoder.md](../topics/encoder.md).

## Configuration and Direction

```c
typedef enum {
  TIM3_ENCODER_DIR_NORMAL = 0,    /* A leads B → CNT++ */
  TIM3_ENCODER_DIR_INVERTED = 1,  /* Flip CC1P, equivalent to swapping A/B polarity */
} tim3_encoder_dir_t;

typedef struct {
  tim3_encoder_dir_t direction;
} tim3_encoder_config_t;
```

If wiring is reversed, change direction in `board_config.h` or swap PA6/PA7.

## API Reference

| Function | Description |
|------|------|
| `tim3_encoder_init_with_config(config)` | Configure GPIO + TIM3 encoder mode |
| `tim3_encoder_init(direction)` | Convenience wrapper |
| `tim3_encoder_read_count(out)` | Read CNT (with init check) |
| `tim3_encoder_get_count()` | Direct CNT read (legacy, no init check) |
| `tim3_encoder_reset_count()` | Clear CNT |
| `tim3_encoder_read_direction(out)` | Read CR1.DIR: 0=up, 1=down |
| `tim3_encoder_get_direction()` | Legacy wrapper |

## Implementation Notes

### Key Register Choices

- **PSC=0**: encoder mode forbids prescaler; otherwise pulses are lost.
- **ARR=0xFFFF**: 16-bit free-running count; application uses `(int16_t)(now - prev)` for signed delta.
- **IC1F/IC2F=0xF**: maximum digital filter for EMI glitches; normal encoder edge period far exceeds filter window.
- **SMCR.SMS=011**: Encoder Mode 3, TI1+TI2 both edges, 4× resolution.

### Why Only CC1P Is Inverted, Not CC2P

Inverting one channel polarity is equivalent to swapping A/B; inverting both restores original direction. Enables software correction without hardware changes.

## Usage Example

```c
tim3_encoder_init(BOARD_WHEEL_ENCODER_DIRECTION);

int16_t prev = tim3_encoder_get_count();
/* every 20 ms */
int16_t now = tim3_encoder_get_count();
int16_t delta = (int16_t)(now - prev);
prev = now;
/* positive/negative delta → clockwise/counter-clockwise */
```

This project uses delta × step factor in `app_motor_encoder_task` to drive motor speed.

## Common Pitfalls

- No pull-up or floating A/B → erratic counts.
- Treating CNT as unsigned 16-bit for delta → wrong increment at wrap; cast to `int16_t`.
- `get_count()` skips init check → debugging without init still returns a value, but it is unreliable.
