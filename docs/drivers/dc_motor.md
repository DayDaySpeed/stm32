# TB6612 直流电机驱动（`dc_motor`）

## 作用

通过 **TB6612FNG** 驱动有刷直流电机：**有符号速度** -1000~+1000，符号表示方向，绝对值表示 PWM 占空比千分比。

## 硬件接线

| 信号 | 引脚 | 说明 |
|------|------|------|
| PWMA | PB6 | TIM4_CH1 PWM |
| AIN1 | PB7 | GPIO，方向控制 |
| AIN2 | PB5 | GPIO，方向控制 |
| STBY | 3.3V | 使能（硬件常高） |

真值表：

| 状态 | AIN1 | AIN2 | PWMA |
|------|------|------|------|
| 正转 | 1 | 0 | PWM |
| 反转 | 0 | 1 | PWM |
| 停止 | 0 | 0 | 0 |

## API 参考

| 函数 | 说明 |
|------|------|
| `dc_motor_gpio_safe_early()` | **上电最早**拉低 PWM/方向，防误转（`main.c` 在 board_init 后立即调） |
| `dc_motor_init_with_config(config)` | TIM4 PWM + GPIO，默认 10kHz |
| `dc_motor_init()` | 速度 0 的默认 init |
| `dc_motor_set_speed_signed(speed)` | -1000..+1000 |
| `dc_motor_get_speed_signed(out)` | 读缓存速度（非实测转速） |
| `dc_motor_set_duty_permille(duty)` | 仅正转 0~1000 |
| `dc_motor_stop()` | 等价 speed=0 |

```c
typedef struct {
  uint32_t pwm_hz;
  int16_t speed_permille;
} dc_motor_config_t;
```

## 实现说明

### 为什么有 `gpio_safe_early`

TB6612 在 MCU 复位到 `main` 跑起来之前，GPIO 可能浮空或处于不确定态，电机可能误转。该函数只开 IOPB 时钟并立即拉低 PB5/PB6/PB7，**不依赖** `bsp_board_init()`。

### PWM 与方向分离

- 方向由 AIN1/AIN2 GPIO（BSRR 原子写）控制。
- 速度幅度由 TIM4 CCR1 控制；`speed=0` 时方向脚和 PWM 都关，避免制动模式误用。

### 时基与占空比

与 `breathing_led.c` 共用 `stm_tim_resolve_timebase()` 和 `bsp_clock_get_apb1_timer_hz()`，避免重复代码。

### 板级符号反转

若顺时针应反转，在 `board_config.h` 设 `BOARD_MOTOR_REVERSE_SIGN=1`，由 `bsp_dc_motor_set_speed_signed` 取反，驱动层保持物理真值表不变。

## 使用示例

```c
dc_motor_gpio_safe_early();  /* main 里最早 */
/* ... bsp_board_init ... */
dc_motor_init();
dc_motor_set_speed_signed(500);   /* 正转半速 */
dc_motor_set_speed_signed(-300);  /* 反转 */
dc_motor_stop();
```

## 常见坑

- 忘记 `gpio_safe_early` → 上电抖一下。
- AIN2 误接 GND 而软件也拉低 → 仍可能短路式制动，本驱动停止时双低。
- PWM 频率过低 → 电机啸叫；本板默认 10kHz。

---

# English

# TB6612 DC Motor Driver (`dc_motor`)

## Purpose

Drives a brushed DC motor via **TB6612FNG**: **signed speed** from -1000 to +1000; sign indicates direction, absolute value is PWM duty in permille.

## Hardware Wiring

| Signal | Pin | Description |
|------|------|------|
| PWMA | PB6 | TIM4_CH1 PWM |
| AIN1 | PB7 | GPIO, direction control |
| AIN2 | PB5 | GPIO, direction control |
| STBY | 3.3V | Enable (hardwired high) |

Truth table:

| State | AIN1 | AIN2 | PWMA |
|------|------|------|------|
| Forward | 1 | 0 | PWM |
| Reverse | 0 | 1 | PWM |
| Stop | 0 | 0 | 0 |

## API Reference

| Function | Description |
|------|------|
| `dc_motor_gpio_safe_early()` | **Earliest on power-up**: drive PWM/direction low to prevent unintended rotation (called in `main.c` immediately after board_init) |
| `dc_motor_init_with_config(config)` | TIM4 PWM + GPIO, default 10 kHz |
| `dc_motor_init()` | Default init at speed 0 |
| `dc_motor_set_speed_signed(speed)` | -1000..+1000 |
| `dc_motor_get_speed_signed(out)` | Read cached speed (not measured RPM) |
| `dc_motor_set_duty_permille(duty)` | Forward only, 0~1000 |
| `dc_motor_stop()` | Equivalent to speed=0 |

```c
typedef struct {
  uint32_t pwm_hz;
  int16_t speed_permille;
} dc_motor_config_t;
```

## Implementation Notes

### Why `gpio_safe_early` Exists

Before the MCU reset completes and `main` runs, GPIO may float or be indeterminate, causing unintended motor rotation. This function only enables IOPB clock and immediately drives PB5/PB6/PB7 low, **without** depending on `bsp_board_init()`.

### PWM and Direction Separation

- Direction controlled by AIN1/AIN2 GPIO (BSRR atomic writes).
- Speed magnitude controlled by TIM4 CCR1; at `speed=0`, direction pins and PWM are both off, avoiding misuse of brake mode.

### Timebase and Duty Cycle

Shares `stm_tim_resolve_timebase()` and `bsp_clock_get_apb1_timer_hz()` with `breathing_led.c` to avoid duplicated code.

### Board-Level Sign Inversion

If clockwise should be reversed, set `BOARD_MOTOR_REVERSE_SIGN=1` in `board_config.h`; `bsp_dc_motor_set_speed_signed` inverts the sign while the driver keeps the physical truth table unchanged.

## Usage Example

```c
dc_motor_gpio_safe_early();  /* earliest in main */
/* ... bsp_board_init ... */
dc_motor_init();
dc_motor_set_speed_signed(500);   /* forward half speed */
dc_motor_set_speed_signed(-300);  /* reverse */
dc_motor_stop();
```

## Common Pitfalls

- Forgetting `gpio_safe_early` → motor twitches on power-up.
- AIN2 tied to GND while software also drives low → possible short-brake condition; this driver uses both-low for stop.
- PWM frequency too low → audible whine; default on this board is 10 kHz.
