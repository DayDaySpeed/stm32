# 呼吸灯驱动（`breathing_led`）

## 作用

驱动板载 **状态 LED 呼吸效果**：在 PA0（TIM2_CH1）输出 PWM，应用层周期调节占空比即可实现亮度渐变。占空比用 **千分比** 0~1000 表示。

## 硬件与依赖

| 项目 | 说明 |
|------|------|
| 引脚 | PA0 = TIM2_CH1，复用推挽 50MHz |
| 定时器 | TIM2（APB1，本板专用于状态灯） |
| 时钟 | `bsp_clock_get_apb1_timer_hz()` |
| 前置 | `bsp_clock_apply_profile()` → `bsp_board_init()` |

PWM 寄存器原理见 [topics/pwm.md](../topics/pwm.md)。

## 配置结构体

```c
typedef struct {
  uint32_t carrier_hz;       /* PWM 载波频率，如 1000 */
  uint16_t duty_permille;    /* 0=全低，1000=100% */
} breathing_led_config_t;
```

## API 参考

| 函数 | 说明 |
|------|------|
| `breathing_led_init_with_config(config)` | 初始化 GPIO + TIM2 PWM |
| `breathing_led_init_hz(hz, duty)` | 便捷包装 |
| `breathing_led_set_duty_permille(duty)` | **仅改 CCR1**（呼吸灯主循环常用） |
| `breathing_led_set_config(config)` | 改载波频率 + 占空比 |
| `breathing_led_set_hz(hz, duty)` | 便捷包装 |
| `breathing_led_stop()` | 停止输出 |

## 使用示例

```c
const breathing_led_config_t cfg = { .carrier_hz = 1000U, .duty_permille = 0U };
breathing_led_init_with_config(&cfg);

/* app 呼吸任务里 */
breathing_led_set_duty_permille(new_duty);
```

应用层推荐 `bsp_status_led_set_duty_permille()`。

## 常见坑

- 未 init 就改占空比 → `STM_ERR_NOT_INITIALIZED`
- `duty_permille > 1000` → `STM_ERR_INVALID_ARG`

---

# English

# Breathing LED Driver (`breathing_led`)

## Purpose

Drives the onboard **status LED breathing effect**: PWM on PA0 (TIM2_CH1); the application periodically updates duty cycle for smooth brightness ramps. Duty is in **permille** (0–1000).

## Configuration

```c
typedef struct {
  uint32_t carrier_hz;
  uint16_t duty_permille;
} breathing_led_config_t;
```

## API

| Function | Description |
|------|------|
| `breathing_led_init_with_config(config)` | Init GPIO + TIM2 PWM |
| `breathing_led_set_duty_permille(duty)` | Update CCR1 only (typical in breath loop) |
| `breathing_led_stop()` | Stop output |

See `include/drivers/breathing_led.h` for full API. Prefer `bsp_status_led_set_duty_permille()` in application code.

For PWM register theory, see [topics/pwm.md](../topics/pwm.md).
