# STM32F103 PWM 全整理（结合本工程实现）

本文把本项目中与 PWM 相关的内容一次讲清：
基本概念、寄存器流程、本工程的接口设计、频率/占空比反推、应用层呼吸灯实例、常见坑与排障。

---

## 1. 先建立 PWM 心智模型

PWM（Pulse Width Modulation，脉宽调制）= 在固定的「**载波周期**」内，让输出在「**有效电平**」和「**无效电平**」之间切换，通过改变两者的比例来表达一个 0~100% 的「**强度**」量。

最常见的使用场景：

- LED 亮度调节（人眼对短脉冲整体感受为变暗）
- 直流电机调速
- 舵机角度控制（标准 50Hz / 1ms~2ms 高电平）
- 简易 DAC（PWM + 低通滤波生成「平均电压」）

三个核心参数：

| 参数 | 含义 |
|---|---|
| **PWM 频率** | 载波频率，决定每秒重复多少次 (例：1kHz 表示 1 秒 1000 次) |
| **占空比** | 有效电平时长 / 周期时长，0%~100% |
| **极性** | 「有效电平」是高还是低（共阳/共阴 LED、PNP/NPN 驱动会反） |

---

## 2. STM32F1 通用定时器是怎么造出 PWM 的

STM32F1 的 PWM 不是单独外设，而是**通用定时器**（TIM2/TIM3/TIM4 等）的「**输出比较模式**」的一种。

核心逻辑：

```
              ┌────┐
   TIM_CLK ──>│ PSC│──> 计数器时钟
              └────┘
                 ↓
              ┌────┐
              │ CNT│  从 0 计到 ARR，到顶后回 0（一个周期）
              └────┘
                 ↓
            ┌────────┐
            │  比较  │       ← CCR1（捕获/比较寄存器）
            └────────┘
                 ↓
                输出（PWM 模式 1：CNT < CCR1 输出高）
```

各寄存器作用：

| 寄存器 | 作用 |
|---|---|
| **PSC** (Prescaler) | 把定时器输入时钟分频，得到计数器实际节拍 |
| **ARR** (Auto-Reload) | 计数器最大值；CNT 数到 ARR 后回 0 |
| **CCR1** (Capture/Compare 1) | 比较阈值；决定每周期前 CCR1 个 tick 输出有效电平 |
| **CCMR1.OC1M** | 输出比较模式（PWM 模式 1 / 模式 2 等） |
| **CCER.CC1E** | 通道输出使能（不开就只是比较器空转） |
| **CR1.CEN** | 计数器总使能 |

公式：

$$
T_{周期} = \frac{(\mathrm{PSC}+1)(\mathrm{ARR}+1)}{T_{IM\_CLK}}
$$

$$
\text{占空比} = \frac{\mathrm{CCR1}}{\mathrm{ARR}+1}
$$

---

## 3. 本工程的引脚与时钟

文件：`include/bsp/board_pins.h`、`include/bsp/rcc_board.h`、`src/bsp/board_init.c`

- **引脚**：PA0 = TIM2_CH1（默认映射，无需 AFIO 重映射）
  - GPIO 配置：复用推挽输出 50MHz（CRL 字段值 `0xB`）
  - 宏：`BOARD_GPIO_PA0_AF_PP_50MHZ`
- **时钟门控**：
  - `RCC_APB1ENR.TIM2EN = 1`（在 `rcc_board.h` 的 `RCC_BOARD_APB1_ENABLE_MASK` 中）
  - `RCC_APB2ENR.IOPAEN = 1`（GPIOA 时钟）
- **TIM2 输入时钟**（STM32F1 特殊规则）：
  - APB1 预分频 = 1：`TIM_CLK = PCLK1`
  - APB1 预分频 ≠ 1：`TIM_CLK = 2 × PCLK1`（自动 ×2）
  - 本工程 72MHz 档位：HCLK=72MHz，PCLK1=36MHz，所以 TIM2 输入 = 72MHz

---

## 4. 本工程 PWM 接口设计

文件：

- `include/drivers/pwm.h`
- `src/drivers/pwm.c`

公共 API：

```c
/* 初始化：自动反推 (PSC, ARR)，配置 GPIO + 寄存器，启动输出。 */
stm_status_t tim2_ch1_pwm_init_hz(uint32_t pwm_frequency_hz,
                                  uint16_t duty_permille);

/* 仅改 CCR1：PSC/ARR 不变，PWM 频率不变（运行中无毛刺）。 */
stm_status_t tim2_ch1_pwm_set_duty_permille(uint16_t duty_permille);

/* 改频率并可选同时更新占空比。 */
stm_status_t tim2_ch1_pwm_set_hz(uint32_t pwm_frequency_hz,
                                 uint16_t duty_permille);

/* 停止计数并关闭 CH1 引脚输出。 */
void tim2_ch1_pwm_stop(void);
```

设计思路：

- 上层只声明「我要 1kHz、500‰ 占空」，底层自动算 PSC/ARR/CCR1
- 占空比用**千分比 0..1000** 而非百分比 0..100：分辨率高 10 倍，且整数除法不丢精度
- 调频和调占空比拆成两个 API：呼吸灯只调占空比，频率扫描可调 set_hz

---

## 5. 占空比为什么用千分比

如果用百分比 0..100，每个百分比对应 ARR 上的多少 tick？以 ARR=999 为例：

- `1% → CCR1 = 9`（9/1000 ≈ 0.9%，已经丢精度）
- 所有奇数百分比都会向下截断

用千分比 0..1000：

- `CCR1 = (duty × ticks + 500) / 1000`（带四舍五入）
- 1000 → CCR1 = ticks（> ARR），保证真正 100% 高电平
- 整数运算，无 FPU 也准确

源码：

```c
static uint32_t duty_permille_to_ccr1(uint16_t duty_permille,
                                      uint32_t ticks_per_period) {
  return ((uint32_t)duty_permille * ticks_per_period + 500U) / 1000U;
}
```

---

## 6. 频率反推 (PSC, ARR)

给定 `pwm_hz`，要找一对 `(PSC, ARR)` 满足：

$$
(\mathrm{PSC}+1)(\mathrm{ARR}+1) = \frac{T_{IM\_CLK}}{pwm\_hz}
$$

记右侧为 `total`。两个寄存器都是 16 位，所以 `PSC+1 ∈ [1, 65536]`，`ARR+1 ∈ [1, 65536]`。

本工程的两步策略（见 `tim2_ch1_pwm_resolve_timebase`）：

### 策略 1：精确解（优先）

枚举 `PSC+1` 从 1 到 65536，找能整除 `total` 的因子；如果对应的 `ARR+1` 也在 [1, 65536]，就用它。

- `PSC+1` 越小 → `ARR+1` 越大 → 占空比分辨率越高，所以从小往大扫
- 例：72MHz / 1kHz = 72000，可拆 (1, 72000) 不行（ARR 太大），(2, 36000) 不行，… (1100, 65454) 不行，… (1126, 64?) ……实际 72000 = 1×72000 = 8×9000 = 9×8000 = … = 1125×64，所以最优解是 PSC+1=1125、ARR+1=64？
- 但 64 太小，占空分辨率只有 64 档；策略 1 找的是「ARR 在范围内的第一个因子」，给出 PSC+1=2、ARR+1=36000，占空分辨率 36000 档

### 策略 2：近似解（回退）

如果 `total > 65536` 但找不到精确因子（罕见，比如 total 是大质数），则：

```c
psc = (total + 0xFFFF) / 0x10000;   /* 向上取整除 65536 */
arr = total / psc;
```

这会让实际频率比目标频率略低（0.x% 的偏差），但保证 ARR 不溢出。对 LED、电机这种应用完全可接受。

---

## 7. 寄存器配置时序（关键，避免毛刺）

`tim2_ch1_pwm_apply_hw()` 的写寄存器顺序很有讲究：

```
1. 停 CNT、屏蔽更新中断          ← 防止配到一半被旧周期打断
2. 配 PWM 模式（CCMR1）            CC1S=00（输出）、OC1M=110（PWM1）、OC1PE=1
3. 配输出极性和使能（CCER）        CC1P=0（高有效）、CC1E=1（输出连到 PA0）
4. 写 PSC / ARR / CCR1             ARR 同时打开 ARPE（影子）
5. EGR.UG=1（强制刷新影子）        否则首个周期会用旧 PSC/ARR
6. 清 SR.UIF（UG 副作用）          ~UIF 表示「只清 UIF，其它位不动」
7. CR1.CEN=1（启动 CNT）
```

**为什么需要影子寄存器（ARPE/OC1PE）**：

ARR 和 CCR1 在硬件里有「**预装载寄存器**」（影子）+「**当前生效值**」两层。开了 ARPE/OC1PE 后：

- 你 `TIM2_ARR = X` 写入的是预装载
- 当 CNT 数到顶（更新事件 UEV）时，硬件原子地把预装载值搬到生效寄存器
- 所以**跨周期改值不会出现「上半周期 1ms、下半周期 0.5ms」这种瞬间被切断的毛刺**

**为什么需要 EGR.UG**：

刚配完寄存器，CNT 还停在 0 没跑过任何更新事件，预装载值还没搬到生效。手动写 `EGR.UG=1` 触发一次「假的」UEV，立刻同步影子。但 UG 顺手把 SR.UIF 也置 1，下一行的 `TIM2_SR = ~TIM_SR_UIF_BIT` 就是为了清掉它。

---

## 8. 应用：呼吸灯（cooperative multitasking）

文件：`src/app/app.c` 的 `app_breath_led_poll()`

设计：

- 主循环每轮调用 `app_breath_led_poll()`，但内部**节流**到每 12ms 才真正干活
- 维护一个 0..399 的 `phase` 计数，每 12ms +1
- 三角波：`phase ∈ [0, 200] → tri = phase`，`phase ∈ [201, 399] → tri = 400 - phase`
- 线性映射：`duty = tri × 1000 / 200`，得到 [0, 1000] 千分比
- 整圈 400 × 12ms ≈ 4.8 秒

关键代码：

```c
static void app_breath_led_poll(void) {
  static uint32_t s_last_step_ms;
  static uint8_t  s_time_inited;
  static uint16_t s_phase;

  uint32_t now = systick_get_ms();
  if (s_time_inited == 0U) {
    s_time_inited = 1U;
    s_last_step_ms = now;
  }
  /* 12ms 没到就立刻 return，让出 CPU */
  if ((uint32_t)(now - s_last_step_ms) < APP_BREATH_STEP_MS) {
    return;
  }
  s_last_step_ms = now;

  s_phase = (uint16_t)(s_phase + 1U);
  if (s_phase >= APP_BREATH_PHASE_MAX) {
    s_phase = 0U;
  }
  uint16_t tri = (s_phase <= 200U) ? s_phase
                                   : (uint16_t)(APP_BREATH_PHASE_MAX - s_phase);
  uint16_t duty = (uint16_t)(((uint32_t)tri * 1000U) / 200U);
  (void)tim2_ch1_pwm_set_duty_permille(duty);
}
```

注意：

- `s_last_step_ms` 用 `uint32_t` 减法，自动处理 49.7 天后 systick 溢出回零的边界
- 所有 `static` 局部都加 `s_` 前缀（项目命名约定）
- 不在中断里改占空比，主循环非阻塞推进

---

## 9. 接线（直驱 LED 的两种方案）

### 方案 A：带限流电阻（推荐）

```
PA0 ──[220Ω~1kΩ]──┤▶├── GND
                  ↑    ↑
                  长脚 短脚
```

- 普通红/绿/黄 LED：220Ω~470Ω
- 高亮蓝/白 LED：1kΩ
- 电流计算：$I = (V_{CC} - V_F) / R$，例 (3.3-2.0)/220 ≈ 6mA，安全

### 方案 B：无电阻（应急，把占空比上限压低）

如果手头没电阻，把 `app.c` 里的占空映射改一下：

```c
/* 原来：duty 上限 1000 (100%) */
// uint16_t duty = (uint16_t)(((uint32_t)tri * 1000U) / 200U);

/* 改后：duty 上限 200 (20%)，等效平均电流降到 1/5 */
uint16_t duty = tri;  /* tri 已经是 0..200，直接当千分比用 */
```

PWM 频率 1kHz、20% 占空时，等效平均电流 ≈ 6mA，临时跑没问题。但**长期仍建议加电阻**：GPIO 输出阻抗 ~40Ω + LED 二极管特性的非线性电流，瞬时峰值仍超 GPIO 25mA 上限。

---

## 10. 常见参数搭配建议

| 应用 | 推荐 PWM 频率 | 备注 |
|---|---|---|
| LED 亮度调节 | 500Hz~2kHz | 100Hz 以下肉眼会看到闪烁 |
| 直流电机 | 5kHz~25kHz | 高于 20kHz 听不到啸叫 |
| 舵机 | 50Hz | 高电平 1ms~2ms（占空 5%~10%）|
| 蜂鸣器（无源） | 频率即音高，占空固定 50% | 改频率而非占空 |
| 简易 DAC | 越高越好（≥10kHz）| 后接 RC 低通 |

---

## 11. 常见错误与症状

1. **GPIO 没配复用推挽**
   - 症状：PA0 一直为 0 或随机电平
   - 检查：`GPIOA_CRL` 字段为 `0xB`（CNF=10、MODE=11）

2. **`TIM2EN` 没开**
   - 症状：写 TIM2 寄存器无效，读回全 0
   - 检查：`bsp_board_init()` 里 `RCC_APB1ENR |= RCC_TIM2EN_BIT`

3. **忘了 `CC1E`**
   - 症状：CNT 在跑、CCR1 在比较，但 PA0 不动
   - 检查：`TIM2_CCER & 1 == 1`

4. **没触发 UG，影子未同步**
   - 症状：第一个周期占空错，之后正常
   - 检查：配置完后写 `TIM2_EGR = TIM_EGR_UG_BIT`

5. **APB1 定时器时钟算错**
   - 症状：实际频率是预期的一半（或两倍）
   - 检查：72MHz 档位下 `tim2_input_clock_hz()` 应返回 72000000，不是 36000000

6. **ARR=0**
   - 症状：定时器卡死，UEV 不触发
   - 限制：`tim2_ch1_pwm_resolve_timebase` 已保证 `ARR+1 ≥ 1`

7. **直驱 LED 烧 GPIO**
   - 症状：跑几小时/几天后某个引脚不响应
   - 解决：加 220Ω~1kΩ 限流电阻

---

## 12. 一句话总结

PWM 在 STM32F1 上的本质 = **CNT 在 [0..ARR] 循环，CCR1 决定切换点**。
工程化的核心是：**PSC/ARR 一次配好不再动，运行中只改 CCR1 → 频率稳定、占空可平滑变化**。
