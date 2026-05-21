# board_config — 板级可调参数

文件：`include/bsp/board_config.h`

**改板子行为时优先改此文件**，无需动 `app.c` 或驱动源码。所有宏在编译期生效，改后重新 `cmake --build build` 并烧录即可。

---

## 编码器

```c
#define BOARD_WHEEL_ENCODER_DIRECTION TIM3_ENCODER_DIR_NORMAL
```

| 值 | 含义 |
|----|------|
| `TIM3_ENCODER_DIR_NORMAL` | A 相超前 B 相时 CNT 增加（默认） |
| `TIM3_ENCODER_DIR_INVERTED` | 软件翻转 A 相极性，计数方向相反 |

**何时改**：拧旋钮时 OLED 上 `ENC` 减少、或电机加减速与预期相反。

**替代方案**：对调编码器 A/B 接线（PA6↔PA7），效果类似 INVERTED。

被引用：`board_devices.c` → `g_board_wheel_encoder_config`。

---

## 电机符号

```c
#define BOARD_MOTOR_REVERSE_SIGN (0U)
```

| 值 | 含义 |
|----|------|
| `0` | 驱动层速度符号与 app 一致 |
| `1` | `bsp_dc_motor_set/get_speed_signed` 自动取反 |

**何时改**：顺时针拧旋钮应减速却加速，或电机正反转与标签相反。

**注意**：只交换软件语义，不改 TB6612 的 AO1/AO2 物理接线。

---

## 蜂鸣器

```c
#define BOARD_BUZZER_ACTIVE_HIGH      (0U)
#define BOARD_IR_PROXIMITY_BEEP_ENABLE  (1U)
#define BOARD_BUZZER_BEEP_MS          (80U)
#define BOARD_IR_BEEP_COOLDOWN_MS     (800U)
```

| 宏 | 说明 |
|----|------|
| `BOARD_BUZZER_ACTIVE_HIGH` | `0`=低电平响（常见三脚模块）；`1`=高电平响。上电一直叫时对调 0/1 |
| `BOARD_IR_PROXIMITY_BEEP_ENABLE` | `1`=红外靠近时鸣叫；`0`=关闭（排查硬件用） |
| `BOARD_BUZZER_BEEP_MS` | 单次鸣叫时长（ms） |
| `BOARD_IR_BEEP_COOLDOWN_MS` | 两次鸣叫最小间隔（ms） |

`ACTIVE_HIGH` → `bsp_buzzer_init` → `buzzer_config_t`。  
其余 IR 相关宏在 **`app.c`** 的 `app_ir_proximity_buzzer_task` 中使用（属于应用策略，但阈值集中在此便于调参）。

---

## 反射红外靠近判定

本板 TCRT5000 类模块实测：**远离 ~4000，靠近 ~100**（raw 越低越近）。

```c
#define BOARD_IR_NEAR_RAW_LOW         (500U)
#define BOARD_IR_LEAVE_RAW_HIGH       (3500U)
#define BOARD_IR_NEAR_STREAK_COUNT    (3U)
#define BOARD_IR_LEAVE_STREAK_COUNT   (2U)
#define BOARD_IR_ARM_RAW_HIGH         (3000U)
#define BOARD_IR_ARM_STREAK_COUNT     (5U)
```

### 状态机逻辑（app 层）

```
上电 → 须连续 N 次 raw >= ARM_HIGH → 「武装」完成
武装后 → raw <= NEAR_LOW 连续 M 次 → 判「靠近」→ 蜂鸣
       → raw >= LEAVE_HIGH 连续 K 次 → 判「离开」
中间区间 → 去抖，不改变状态
```

| 宏 | 作用 |
|----|------|
| `BOARD_IR_ARM_RAW_HIGH` + `ARM_STREAK_COUNT` | 防止上电误响：先确认手已远离 |
| `BOARD_IR_NEAR_RAW_LOW` + `NEAR_STREAK_COUNT` | 靠近阈值 + 去抖 |
| `BOARD_IR_LEAVE_RAW_HIGH` + `LEAVE_STREAK_COUNT` | 离开阈值 + 去抖 |

**调参建议**：串口/OLED 看 `IR=` raw，远离/靠近各测几次，再设 `NEAR_LOW` / `LEAVE_HIGH` 留余量。

---

## 传感器指示 LED

```c
#define BOARD_SENSOR_LED_PWM_HZ       (1000UL)
#define BOARD_NTC_LED_FULL_TEMP_X10   (500)
```

| 宏 | 说明 |
|----|------|
| `BOARD_SENSOR_LED_PWM_HZ` | TIM1 双 LED PWM 频率 |
| `BOARD_NTC_LED_FULL_TEMP_X10` | NTC 灯满亮度对应温度；`500` = 50.0°C |

LDR 灯：raw 线性 0~4095 → 0~1000 permille（在 `bsp_sensor_led_update_from_sensors` 内实现）。

---

## 不在此文件的板级常量

以下写在 `board_devices.c` 或 `board_pins.h`，换板时也可能要改：

| 位置 | 内容 |
|------|------|
| `board_devices.c` | 串口 115200、状态灯 1kHz、电机 PWM 10kHz、NTC 10k/3.3V |
| `board_pins.h` | 全部引脚与 GPIO 模式宏 |
| `rcc_board.h` | 本板使能哪些外设时钟 |

若希望用户只改一个头文件，可把波特率/PWM 频率也迁入 `board_config.h`（当前为减少宏数量而放在 devices 内）。

---

## 修改检查清单

1. 改宏 → 重新编译烧录  
2. 编码器/电机：拧旋钮看 ENC 与 MOT 符号  
3. 蜂鸣器：上电是否误响；靠近 IR 是否按预期鸣叫  
4. 红外：在 OLED page5 或串口看 raw，调整阈值  

## 相关文档

- [board_devices.md](./board_devices.md) — 这些宏如何被消费  
- [drivers/ir_reflect.md](../drivers/ir_reflect.md) — 红外驱动 API
