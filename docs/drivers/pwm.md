# TIM2 状态灯 PWM 驱动（`pwm`）

## 作用

在 **PA0（TIM2_CH1）** 输出边沿对齐 PWM，用于板载状态 LED 亮度调节（呼吸灯等）。占空比用 **千分比** 0~1000 表示。

## 硬件与依赖

| 项目 | 说明 |
|------|------|
| 引脚 | PA0 = TIM2_CH1，复用推挽 50MHz |
| 定时器 | TIM2（APB1） |
| 时钟 | `bsp_clock_get_apb1_timer_hz()`（APB1 预分频≠1 时自动 ×2） |
| 前置 | `bsp_clock_apply_profile()` → `bsp_board_init()`（开 GPIOA/TIM2 时钟） |

更深入的 PWM 原理见 [topics/pwm.md](../topics/pwm.md)。

## 配置结构体

```c
typedef struct {
  uint32_t pwm_hz;           /* PWM 载波频率，如 1000 */
  uint16_t duty_permille;    /* 0=全低，1000=100% */
} tim2_ch1_pwm_config_t;
```

## API 参考

| 函数 | 说明 |
|------|------|
| `tim2_ch1_pwm_init_with_config(config)` | 主初始化：GPIO + 反推 PSC/ARR + 启动 PWM |
| `tim2_ch1_pwm_init_hz(hz, duty)` | 便捷包装 |
| `tim2_ch1_pwm_set_duty_permille(duty)` | **仅改 CCR1**，频率不变（呼吸灯常用） |
| `tim2_ch1_pwm_set_config(config)` | 改频率 + 占空比 |
| `tim2_ch1_pwm_set_hz(hz, duty)` | 便捷包装 |
| `tim2_ch1_pwm_stop()` | 停 CNT、关 CC1E |

返回值均为 `stm_status_t`：`STM_ERR_INVALID_ARG` / `STM_ERR_NOT_INITIALIZED` 等。

## 实现说明

### 时基求解

由 `common/tim_timebase.c` 的 `stm_tim_resolve_timebase()` 统一实现：

```
total = TIM_CLK / pwm_hz = (PSC+1)(ARR+1)
```

策略：优先枚举小 PSC 求精确因子（占空比分辨率高）；无法整除时用近似解防 ARR 溢出。

### 寄存器配置顺序

停 CNT → CCMR1 PWM1 + 影子 → CCER 使能 → 写 PSC/ARR/CCR1 → ARPE → EGR.UG 同步 → 清 UIF → 启 CNT。

**为什么开影子寄存器（OC1PE/ARPE）**：运行时改 CCR1 不会在周期中间产生毛刺。

### 为什么独占 TIM2

本板 TIM2 只接状态灯；不启更新中断，避免与编码器（TIM3）、电机（TIM4）职责混淆。

## 使用示例

```c
const tim2_ch1_pwm_config_t cfg = { .pwm_hz = 1000U, .duty_permille = 500U };
tim2_ch1_pwm_init_with_config(&cfg);

/* 呼吸灯循环里只改占空比 */
tim2_ch1_pwm_set_duty_permille(new_duty);
```

应用层推荐经 `bsp_status_led_set_duty_permille()` 访问。

## 常见坑

- 未开 TIM2 时钟 → PA0 无波形。
- `duty_permille > 1000` → 返回 `STM_ERR_INVALID_ARG`。
- 未 init 就 `set_duty` → `STM_ERR_NOT_INITIALIZED`。
