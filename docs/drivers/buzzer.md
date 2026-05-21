# 有源蜂鸣器驱动（`buzzer`）

## 作用

用 **GPIO 电平** 驱动有源蜂鸣器（内置振荡电路，给电就响）。**不是** PWM 音调发生器。

## 硬件

| 项目 | 说明 |
|------|------|
| 引脚 | PA4 推挽输出 |
| 极性 | `BOARD_BUZZER_ACTIVE_HIGH`（0=低电平响，常见三脚模块） |
| 前置 | `bsp_board_init()`（GPIOA 时钟） |

## 配置结构体

```c
typedef struct {
  uint8_t active_high;  /* 1=高电平响，0=低电平响 */
} buzzer_config_t;
```

## API 参考

| 函数 | 说明 |
|------|------|
| `buzzer_init_with_config(config)` | GPIO + 默认关闭 |
| `buzzer_init()` | active_high=0 |
| `buzzer_on()` / `buzzer_off()` | 响 / 停 |
| `buzzer_beep_blocking(ms)` | 响 ms 毫秒后关（内部 `systick_delay_ms`） |

## 实现说明

### 极性抽象

`s_active_high` 决定「逻辑 on」映射到引脚高还是低。应用和 BSP 只调 `on/off`，不关心模块是低触发还是高触发。

### 为什么 init 默认 active_high=0

本板三脚有源蜂鸣器常见为 **低电平触发**；上电 GPIO 推挽输出后先 `off()`，避免一直叫。

若上电仍叫 → 在 `board_config.h` 把 `BOARD_BUZZER_ACTIVE_HIGH` 改为 1 试一次。

### beep_blocking 的代价

阻塞主循环，期间呼吸灯/OLED 任务暂停。红外任务已加冷却时间，避免连续占用 CPU。

## 使用示例

```c
buzzer_init();
buzzer_beep_blocking(100U);
```

应用层：`bsp_buzzer_beep_blocking(BOARD_BUZZER_BEEP_MS)`。

## 常见坑

- 无源蜂鸣器需要 PWM 方波，本驱动不适用
- 未 init 就 on → `STM_ERR_NOT_INITIALIZED`
- 与 IR 检测同周期调用 → 注意主循环实时性
