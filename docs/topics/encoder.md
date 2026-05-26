# STM32F103 正交编码器全整理（结合本工程实现）

本文把本项目中与正交编码器（quadrature encoder）相关的内容一次讲清：
工作原理、STM32 硬件解码、寄存器流程、本工程的接口设计、数字滤波器、
**实测踩过的坑与解决方法**、平衡车移植要点。

---

## 1. 先建立正交编码器心智模型

正交编码器（quadrature encoder）= 在轴的圆周上做两路彼此相差 **90°** 的脉冲信号，
通过两路信号的「**相位先后关系**」判断方向，通过「**脉冲数量**」判断位移。

最常见的使用场景：

- 旋转旋钮（人机交互，EC11 这种带卡点的）
- 直流电机轴端测速（霍尔编码器、光电编码器）
- 平衡车 / 麦轮小车 的轮速反馈
- CNC 主轴位置反馈

三个核心概念：

| 概念 | 含义 |
|---|---|
| **A 相 / B 相** | 两路相差 90° 的方波信号 |
| **方向判定** | A 超前 B 90° = 正转；B 超前 A 90° = 反转 |
| **分辨率** | 每圈线数 N（CPR/PPR）；4x 解码后实际计数 = 4N |

```
正转（CW）：A 先翻，B 后翻
  A: ─┐    ┌─┐    ┌─┐
      └────┘ └────┘ └────
  B: ───┐    ┌─┐    ┌─┐
        └────┘ └────┘ └─

反转（CCW）：B 先翻，A 后翻
  A: ───┐    ┌─┐    ┌─┐
        └────┘ └────┘ └─
  B: ─┐    ┌─┐    ┌─┐
      └────┘ └────┘ └────
```

**4x 解码** = A、B 两根信号每根的「上升沿 + 下降沿」都数一次。
一个完整 quadrature 周期有 4 个边沿，对应 4 个 count，所以电机标 1000 PPR
（pulses per revolution）的编码器，4x 解码后一圈实际能数到 4000 count。
这是「最高分辨率」的解码方式，工业上几乎都用这种。

某些电机编码器还有 **Z 相**（Index），转一圈出一个脉冲，用来做绝对位置零点。
本工程未用 Z 相（平衡车控制只用 A/B 相对位移）。

---

## 2. STM32F1 通用定时器是怎么解码正交信号的

STM32F1 的 TIM2/TIM3/TIM4 / TIM5 都内置「**编码器接口模式**」，本质是把
**从模式控制器（Slave Mode Controller）** 配成「TI1+TI2 双相计数」，让 CNT
寄存器自动跟随旋转量。

核心数据通路：

```
PA6 (A 相) ──┐
              ▼
          ┌──────┐
          │TI1   │── Schmitt ── IC1F 数字滤波 ── 边沿检测 ──┐
          │      │                                          │
          └──────┘                                          ▼
                                                       ┌─────────┐
                                                       │ Quadrature
                                                       │ 解码状态机
                                                       │ (SMCR.SMS=011)
                                                       └─────────┘
          ┌──────┐                                          ▲
          │TI2   │── Schmitt ── IC2F 数字滤波 ── 边沿检测 ──┘
          │      │                                          ▼
          └──────┘                                       ┌──────┐
              ▲                                          │ CNT  │ ← TIM3_CNT
PA7 (B 相) ──┘                                          │ ±1   │   (16-bit)
                                                       └──────┘
                                                          ▲
                                                          │
                                                       CR1.DIR ← 方向自动更新
```

各寄存器的作用：

| 寄存器 | 作用 |
|---|---|
| **CCMR1.CC1S / CC2S** | 把 CH1/CH2 配成**输入**模式，分别连接到 TI1/TI2 |
| **CCMR1.IC1F / IC2F** | TI1/TI2 的数字滤波器档位（0~0xF） |
| **CCER.CC1E / CC2E** | 启用 TI1/TI2 输入通道 |
| **CCER.CC1P / CC2P** | 输入极性（翻一个等价交换 A/B、反转方向） |
| **SMCR.SMS[2:0]** | 从模式：001=ENC1（只数 TI2，2x），010=ENC2（只数 TI1，2x），**011=ENC3（双相 4x）** |
| **ARR** | 计数器最大值（编码器模式一般设 0xFFFF 让它自然回绕） |
| **PSC** | 预分频（**编码器模式必须为 0**，否则脉冲会被丢） |
| **CNT** | 当前计数值（硬件自动 ±1） |
| **CR1.DIR** | 方向标志位（硬件自动更新：0=正转，1=反转） |

**核心特性**：配好后**完全靠硬件运行**，CPU 只读 CNT 就能拿到当前位移，零中断、零开销。

---

## 3. 本工程的引脚与时钟

文件：`include/bsp/board_pins.h`、`include/bsp/rcc_board.h`、`src/bsp/board_init.c`

- **引脚**：PA6 = TIM3_CH1（A 相）、PA7 = TIM3_CH2（B 相）（默认映射，无需 AFIO 重映射）
  - GPIO 配置：输入带上拉/下拉（CNF=10、MODE=00 → CRL 字段值 `0x8`）
  - ODR 对应 bit = 1 → 上拉（外部直接接机械触点也能产生干净的高低电平）
  - 宏：`BOARD_GPIO_PA6_INPUT_PULL`、`BOARD_GPIO_PA7_INPUT_PULL`、`BOARD_GPIO_PA6_PA7_ODR_PULLUP`

- **时钟**：TIM3 挂在 APB1（PCLK1 = 36MHz）。APB1 prescaler ≠ 1 时定时器输入 ×2，所以
  TIM3 实际计数时钟 = **72MHz**。这只影响**滤波采样周期**，对编码器解码本身无关
  （编码器只看输入边沿先后顺序，跟定时器主时钟无关）。

- **使能位**：`rcc_board.h` 中 `RCC_BOARD_APB1_ENABLE_MASK` 已包含 `RCC_TIM3EN_BIT`，
  `bsp_board_init()` 一次性使能 TIM2/TIM3/I2C1。

---

## 4. 驱动 API 设计

文件：`include/drivers/encoder.h`、`src/drivers/encoder.c`

```c
typedef enum {
  TIM3_ENCODER_DIR_NORMAL   = 0,  /* A 超前 B = CNT++ */
  TIM3_ENCODER_DIR_INVERTED = 1,  /* 硬件翻转 A 相极性 = 方向反过来 */
} tim3_encoder_dir_t;

stm_status_t tim3_encoder_init(tim3_encoder_dir_t direction);
int16_t      tim3_encoder_get_count(void);
void         tim3_encoder_reset_count(void);
uint8_t      tim3_encoder_get_direction(void);
```

**设计要点**：

1. **`init` 入参控制方向**，不用编译宏。运行时也能换方向（虽然一般只在 init 用）。
   实测装到底盘上发现方向反了，**不需要交换 A/B 线，改个入参就行**。

2. **`get_count` 返回 `int16_t`**。TIM3_CNT 是 16 位寄存器，0xFFFF 直接强转为 -1。
   配合 `(int16_t)(now - prev)` 的有符号减法，可以**自然处理 16 位回绕**：

   ```c
   /* 旋转一直正向，CNT 从 65535 (-1) 回绕到 0：
    *   now=0, prev=-1
    *   delta = (int16_t)(0 - (-1)) = 1   ✓（不是 -65535）
    *
    * 旋转一直反向，CNT 从 0 回绕到 65535 (-1)：
    *   now=-1, prev=0
    *   delta = (int16_t)(-1 - 0) = -1    ✓
    */
   ```

   只要采样周期里 |delta| < 32768，回绕处理就永远正确。

3. **`get_direction` 仅供调试**。停转时方向位保持上一次值，所以判断「当前是正/反转/停」
   只能靠 delta 的符号，不能靠这个函数。

4. **`reset_count`** 用于「校零」场景（开机时、按钮触发等），平时不用。

---

## 5. 寄存器配置流程（init 一次性完成）

```
┌─────────────────────────────────────────────────────────────┐
│  上电                                                        │
│   │                                                          │
│   ▼                                                          │
│  bsp_clock_apply_profile()  ← 配 HSE→PLL→72MHz 主时钟        │
│   │                                                          │
│   ▼                                                          │
│  bsp_board_init()           ← 开 GPIOA / TIM3 / AFIO 时钟    │
│   │                                                          │
│   ▼                                                          │
│  tim3_encoder_init(direction)                                │
│   │                                                          │
│   ├── 1. GPIOA_CRL: PA6/PA7 = 输入带上拉                     │
│   ├── 2. GPIOA_ODR: PA6/PA7 = 1（拉高，选「上拉」）          │
│   ├── 3. CR1.CEN = 0   （停 CNT，配置过程不跑飞）            │
│   ├── 4. PSC = 0       （编码器模式必须）                    │
│   │   ARR = 0xFFFF     （16 位满量程自然回绕）               │
│   ├── 5. CCMR1:                                              │
│   │     CC1S = 01 (TI1)                                      │
│   │     CC2S = 01 (TI2)                                      │
│   │     IC1F = 0xF  (最大数字滤波)  ★ 关键，见 §6           │
│   │     IC2F = 0xF                                           │
│   │     IC1PSC = 00 (不分频，4x 必须)                        │
│   │     IC2PSC = 00                                          │
│   ├── 6. CCER:                                               │
│   │     CC1P, CC2P 清零（默认极性）                          │
│   │     如果 direction = INVERTED 则置 CC1P                  │
│   │     CC1E = CC2E = 1（启用通道）                          │
│   ├── 7. SMCR.SMS = 011（ENC3，TI1+TI2 双相 4x）             │
│   ├── 8. CNT = 0, SR = 0（清状态、清零）                     │
│   └── 9. CR1.CEN = 1（启动计数器，从此完全硬件运行）         │
│                                                              │
│  返回 STM_OK                                                 │
│                                                              │
│  之后 CPU 只读 CNT 即可，主循环里以一定周期采样算 delta      │
└─────────────────────────────────────────────────────────────┘
```

---

## 6. 数字滤波器（IC1F / IC2F）—— 本工程踩坑最深的地方

### 6.1 滤波器是什么

每个 input capture 通道在 Schmitt 触发器之后、边沿检测器之前，都有一个
**硬件数字滤波器**。它本质是一个「**N-out-of-N 采样多数投票电路**」：

```
伪代码（持续运行，无 CPU 参与）：
  every (1 / fSAMPLING):
      raw = read_pin_after_schmitt()
      if raw != stable_level:
          counter++
          if counter >= N:        ← 连续 N 次采样都翻转才认
              stable_level = raw
              counter = 0
              emit_edge()
      else:
          counter = 0
```

**0b1111 档位**（本工程用的）：

| 参数 | 值 |
|---|---|
| fSAMPLING | fDTS / 32 = 72MHz / 32 = **2.25 MHz** |
| N | 8 |
| 滤波窗口 | 8 / 2.25M ≈ **3.5 μs** |

**意思**：持续时间 < 3.5μs 的任何电压跳变 → 直接被吞掉，下游什么都不知道。
持续时间 ≥ 3.5μs 的电平变化 → 透传（延迟 3.5μs）。

### 6.2 为什么这个滤波器对编码器特别关键

环境里有大量**亚微秒级噪声源**：

| 噪声源 | 时间尺度 |
|---|---|
| PWM 走线辐射（PA0 跑 PWM 时） | ~ns |
| I2C 总线翻转串扰 | ~百 ns |
| USART 边沿耦合 | ~ns |
| 电源纹波 | ~μs |
| 板内地反弹 | ~ns |
| 静电/触摸耦合 | ~ns~μs |

**全部都在 3.5μs 以下**。而**编码器机械动作**（无论 EC11 旋钮还是电机轴上的霍尔编码器）
产生的脉冲都是 **ms 级**的，远超过滤波窗口，不会被滤掉。

所以滤波器在这里就是个**高通噪声切除器**：

- 高频（< 3.5μs）电气干扰 → **全砍**
- 低频（> 3.5μs）真实机械事件 → **全留**

### 6.3 16 个档位对照表（方便以后调整）

档位定义：CCMR1.IC1F[3:0] / IC2F[3:0]

| IC1F 值 | fSAMPLING | N | 窗口（@fDTS=72MHz） | 适用场景 |
|---|---|---|---|---|
| 0000 | — | — | **0（不滤波）** | 干净光电编码器、内部触发 |
| 0001 | fCK_INT | 2 | ~28 ns | 极轻度去抖 |
| 0010 | fCK_INT | 4 | ~56 ns | 抗短毛刺 |
| 0011 | fCK_INT | 8 | ~110 ns | 抗高频 EMI |
| 0100 | fDTS/2 | 6 | ~167 ns |  |
| 0101 | fDTS/2 | 8 | ~222 ns |  |
| 0110 | fDTS/4 | 6 | ~333 ns |  |
| 0111 | fDTS/4 | 8 | ~444 ns | 抗较慢 EMI |
| 1000 | fDTS/8 | 6 | ~667 ns |  |
| 1001 | fDTS/8 | 8 | ~889 ns |  |
| 1010 | fDTS/16 | 5 | ~1.1 μs |  |
| 1011 | fDTS/16 | 6 | ~1.3 μs | 中度去抖 |
| 1100 | fDTS/16 | 8 | ~1.8 μs |  |
| 1101 | fDTS/32 | 5 | ~2.2 μs |  |
| 1110 | fDTS/32 | 6 | ~2.7 μs |  |
| **1111** | **fDTS/32** | **8** | **~3.5 μs** | **最强（本工程默认）** |

**公式**：

$$
T_{窗口} = \frac{N \times 分频系数}{f_{DTS}}
$$

如果以后需要更长窗口，可以查 `CR1.CKD` 位把 fDTS 再分频（默认 CKD=00 让 fDTS = fCK_INT）。

---

## 7. 接线

### 7.1 标准接法

| 编码器脚 | STM32 引脚 |
|---|---|
| A 相 | PA6 |
| B 相 | PA7 |
| C / COM / GND | **GND**（必须接！见 §9） |
| VCC（仅模块版有） | 3.3V |

机械 EC11 编码器**自身不需要供电**，靠 STM32 内部上拉就能产生数字电平。
KY-040 这种模块版上有外部上拉电阻，需要接 + 到 3.3V。

### 7.2 验证步骤

烧好固件 → OLED 应该显示：

```
TIM3 ENC (PA6/7)
CNT = 0
dlt = 0
dir = 0
```

- 不动：CNT 稳定，dlt = 0
- 顺时针转 1 卡（detent）：CNT 变化 **±4**，dlt = ±4，dir = +/-
- 逆时针转 1 卡：CNT 变化方向**相反**

如果转方向跟期望相反，把 `app.c` 里 `tim3_encoder_init(TIM3_ENCODER_DIR_NORMAL)`
改成 `TIM3_ENCODER_DIR_INVERTED` 重编。

---

## 8. 应用层使用方式（cooperative 模式，无中断）

```c
/* 在主循环或 SysTick handler 里，以固定周期采样： */
static int16_t s_enc_prev;

void control_loop_tick(void) {
  int16_t now   = tim3_encoder_get_count();
  int16_t delta = (int16_t)(now - s_enc_prev);   // 自动处理 16 位回绕
  s_enc_prev    = now;

  /* delta = 这个采样周期内累计的脉冲数（带符号）
   * 速度（脉冲/秒）= delta / dt
   * 真实角速度（rad/s）= 速度 × (2π / 4N)，N = 编码器 PPR
   */
}
```

**采样周期建议**：

| 用途 | 推荐周期 | 理由 |
|---|---|---|
| 平衡车闭环控制 | 5~10 ms | PID 控制速率 |
| 旋钮 UI | 10~20 ms | 响应跟得上手感 |
| 速度统计 / 显示 | 100~500 ms | 数据稳定 |

**保证 |delta| < 32768** 即可不溢出。最快电机假设 10000 rpm × 4 × 1000 PPR
= 6.7e5 脉冲/秒，10 ms 周期 ≈ 6700 < 32768 ✓。

---

## 9. 实战踩坑复盘：方向恒为负 + 每格计数不一致

### 9.1 症状

加入编码器驱动后，OLED 显示的现象：

| 维度 | 实测 |
|---|---|
| 静止时 CNT | 稳定不动（✓ 这是正常的） |
| 转动每卡变化 | **不稳定**：-2、-7、-15、-3 各种值乱来 |
| 顺时针/逆时针 | **都是负数累加**，不管哪个方向 |

### 9.2 假设演进

**假设 1（错的）：C 脚悬空，A/B 没有真正的 0V 参考**

最初判断 EC11 的 C 公共端必须接 GND，否则触点闭合无法把 A/B 拉到地，
信号靠寄生耦合 → 不对称 → 方向偏置。

→ 建议接 GND。但用户报告**没接 GND，只改了代码就好了**。

**假设 2（对的）：芯片内部多个外设的高频 EMI 淹没了真信号**

板上同时跑：

- PA0：TIM2 PWM @ 1kHz（边沿陡，辐射强）
- PB8/PB9：I2C1 在跟 OLED 通信（持续翻转）
- PA9：USART1 TX 持续输出调试信息

这些信号在 PCB 走线、飞线、空间里**通过电容耦合**注入 PA6/PA7：

```
PA0 PWM 边沿翻转 → 通过 PCB 寄生电容耦合到 PA6 / PA7
                ├── PA6 距离 PA0 近 → 拾取多
                └── PA7 距离 PA0 远 → 拾取少
                ↓
            A、B 两路在 ns 级时间内出现**有先后顺序**的毛刺
                ↓
            TIM3 编码器解码器看到「A 先翻，B 后翻」的模式 → 计为「正转」边沿
                ↓
            但 PWM 翻转每秒发生几千次，远多于手动转旋钮产生的脉冲
                ↓
            CNT 被噪声推着按固定方向漂
```

**电气不对称导致方向偏置**（耦合系数 A ≠ B），这就是为什么**永远偏一个方向**而不是
左右各半。**幅度可变**（PWM 占空比变、I2C 时序变），所以**每格的额外干扰量不一样**。

### 9.3 解决方法

**唯一代码改动**：开启 TI1/TI2 数字滤波器到最大档位。

```c
TIM3_CCMR1 |= TIM_CCMR1_CC1S_TI1 | TIM_CCMR1_CC2S_TI2 |
              TIM_CCMR1_IC1F_MAX | TIM_CCMR1_IC2F_MAX;  // ← 这两个
```

**效果**：

| 维度 | 修复前 | 修复后 |
|---|---|---|
| 静止稳定性 | ✓ | ✓ |
| 每格变化 | 乱（4 + 各种噪声） | **恒为 4** |
| 方向 | 永远负 | 正/负正确对应物理方向 |

3.5μs 滤波窗口足够把所有 ns 级 EMI 滤干净，但远小于机械动作的 ms 时间常数，
所以**对真信号完全透明**。

### 9.4 复盘要点

1. **第一直觉别太相信**。"接 GND" 是教科书答案，但不是这次的真实病因。
   滤波器本来就该开，**没开是缺省配置的疏忽**。

2. **判断病因有诊断流程**：

   | 症状 | 大概率元凶 | 修复方向 |
   |---|---|---|
   | 静止漂、转动也漂 | 信号悬空 / 接地不良 | 检查接线、上拉、参考电平 |
   | **静止稳 + 转动方向偏置** | **EMI / 高频耦合** | **开数字滤波** ← 本次 |
   | 每格变化不一样 + 偏置 | 噪声幅度可变 → 信号叠加噪声 | 滤波 |
   | 每格变化稳定 + 方向对 | 信号干净，工作正常 | 无需处理 |

3. **C 脚接 GND 仍然推荐**。理论上摆幅更大（接 0V vs 接浮空耦合电位），
   抗干扰更强。本工程能跑只是因为耦合电位**勉强够过 Schmitt 阈值**，温度湿度一变
   未必稳。**严肃应用一定接 GND**。

---

## 10. 平衡车移植要点

平衡车通常每个轮子带一个 **6 线霍尔编码器**：

```
M+, M-      → 电机驱动板（不接 STM32）
VCC, GND    → 3.3V / 5V 给霍尔传感器供电
A, B        → 接 STM32 输入（一个轮子用 TIM3、另一个用 TIM4）
```

**本工程驱动直接复用，需要做的事**：

1. **加 TIM4 驱动**：复制 `encoder.c`，改名 `encoder_tim4.c`，改寄存器（PB6=A, PB7=B），
   同 §3 操作。

2. **PSC 仍设 0、ARR 仍设 0xFFFF**：电机最快 ~600 rpm × 4 × 13 PPR ≈ 31k 脉冲/秒，
   16 位计数器 5ms 采样最多累计 156 < 32768 ✓。

3. **数字滤波器档位**：电机驱动板（H 桥）开关频率几十 kHz，辐射比 PWM LED 强得多，
   **必须开 IC1F/IC2F = 0xF**。本工程的滤波档位是按平衡车场景定的，到时候不用改。

4. **测速公式**：

   $$
   v_{wheel} (\text{rad/s}) = \frac{\Delta_{count}}{4 \times N_{PPR}} \times \frac{2\pi}{\Delta t}
   $$

   $$
   v_{wheel} (\text{rpm}) = \frac{\Delta_{count}}{4 \times N_{PPR}} \times \frac{60}{\Delta t}
   $$

5. **左右轮独立**：左轮用 TIM3、右轮用 TIM4，两个 CNT 独立累加，互不干扰。

---

## 11. 常见错误与症状

1. **`TIM3EN` 没开**
   - 症状：写 TIM3 寄存器无效，读回全 0
   - 检查：`rcc_board.h` 的 `RCC_BOARD_APB1_ENABLE_MASK` 包含 `RCC_TIM3EN_BIT`

2. **GPIO 没配输入上拉**
   - 症状：PA6/PA7 一直是 0 或随机电平，CNT 乱跳
   - 检查：`GPIOA_CRL` 对应字段 = `0x8`、`GPIOA_ODR` 对应 bit = 1

3. **PSC ≠ 0**
   - 症状：4x 解码丢边沿，每格只数 2 或 1
   - 检查：`TIM3_PSC == 0`（编码器模式禁止预分频，**这跟 PWM 不同**）

4. **SMS 没设 011**
   - 症状：CNT 不动
   - 检查：`TIM3_SMCR & 7 == 3`

5. **CCxE 没开**
   - 症状：输入接到 TI1/TI2 但解码器看不到信号
   - 检查：`TIM3_CCER & (CC1E | CC2E) == 全 1`

6. **方向反了**
   - 症状：物理顺时针转，CNT 反而减少
   - 解决：`init` 入参换成 `TIM3_ENCODER_DIR_INVERTED`（**别改硬件接线**）

7. **每格计数不一致 + 方向偏置（§9 的坑）**
   - 症状：每格变化忽多忽少，总朝某方向漂
   - 解决：**IC1F/IC2F 开最大档位**

8. **delta 偶尔出现很大的值**（接近 32768）
   - 症状：电机速度统计偶发跳变
   - 检查：采样周期太长了，|delta| 超过 32768 → 16 位回绕处理失效
   - 解决：缩短采样周期

9. **C 脚没接 GND**
   - 症状：可能能跑（滤波兜底），可能温变后失效
   - 解决：接 GND

10. **飞线太长 / 没接屏蔽**
    - 症状：电机一启动，编码器 CNT 就开始漂
    - 解决：缩短走线、双绞 A/B 线、加 PCB 地平面

---

## 12. 一句话总结

正交编码器在 STM32F1 上的本质 = **TIM 编码器模式让硬件自动数 A/B 边沿、判方向**，CPU 只读 CNT。
工程化的核心是：**PSC=0、SMS=ENC3、IC1F/IC2F 开最大档位** —— 三件事配对，4x 解码 + EMI 免疫一步到位。

---

# English

# STM32F103 Quadrature Encoder Overview (This Project)

Covers principles, STM32 hardware decode, registers, API, digital filter, **field debugging**, and balance-bot porting notes.

---

## 1. Quadrature Encoder Mental Model

Two signals **90° apart** on a shaft: **phase order** → direction, **edge count** → displacement.

Uses: knobs (EC11), motor speed (Hall/optical), balance bots, CNC feedback.

| Concept | Meaning |
|---------|---------|
| A / B phase | Quadrature square waves |
| Direction | A leads B = CW; B leads A = CCW |
| Resolution | Lines N (PPR); **4× decode** → 4N counts/rev |

**4× decoding**: count rising+falling on both A and B → 4 edges per quadrature cycle. 1000 PPR motor → 4000 counts/rev. **Z (index)** optional for absolute zero—not used here.

---

## 2. STM32F1 Hardware Decode

TIM2/3/4/5 **encoder interface**: slave mode **TI1+TI2** drives **CNT**.

```
PA6 (A) → TI1 ─┐
               ├→ quadrature FSM (SMCR.SMS=011) → CNT ±1, CR1.DIR
PA7 (B) → TI2 ─┘
```

| Register | Role |
|----------|------|
| CCMR1.CC1S/CC2S | CH1/CH2 as TI1/TI2 inputs |
| CCMR1.IC1F/IC2F | Digital filter 0..0xF |
| CCER.CC1E/CC2E | Enable inputs |
| CCER.CC1P/CC2P | Polarity (swap/invert direction) |
| SMCR.SMS | 001=ENC1, 010=ENC2, **011=ENC3 (4×)** |
| ARR | Often 0xFFFF wrap |
| PSC | **Must be 0** in encoder mode |
| CNT | Position counter |
| CR1.DIR | Hardware direction flag |

After setup, **hardware runs**; CPU reads CNT only.

---

## 3. Pins and Clock

- **PA6** = TIM3_CH1 (A), **PA7** = TIM3_CH2 (B)
- GPIO: input with pull-up (`0x8` in CRL, ODR bits = 1)
- TIM3 on APB1; when prescaler ≠ 1, timer clk = **72 MHz** (affects filter sampling only, not decode logic)
- `RCC_TIM3EN` in `bsp_board_init()`

---

## 4. Driver API

`encoder.h` / `encoder.c`:

```c
stm_status_t tim3_encoder_init(tim3_encoder_dir_t direction);
int16_t      tim3_encoder_get_count(void);
void         tim3_encoder_reset_count(void);
uint8_t      tim3_encoder_get_direction(void);
```

1. `direction` at init—flip in software without rewiring
2. `int16_t` count + `(int16_t)(now - prev)` handles **16-bit wrap**
3. `get_direction` for debug only when moving
4. `reset_count` for zeroing

---

## 5. Init Register Flow

1. GPIO PA6/PA7 input pull-up
2. `CEN=0`, `PSC=0`, `ARR=0xFFFF`
3. CCMR1: TI1/TI2, **IC1F/IC2F = max**
4. CCER: polarity; `CC1P` if inverted; CC1E/CC2E=1
5. `SMCR.SMS = 011` (ENC3)
6. `CNT=0`, clear SR
7. `CEN=1`

---

## 6. Digital Filter (IC1F / IC2F) — Main Pitfall

### 6.1 What it does

N-of-N sampling after Schmitt: edge only accepted after **N** consecutive filtered samples differ.

**0b1111** (project default):

| Parameter | Value |
|-----------|-------|
| fSAMPLING | fDTS/32 = 72 MHz/32 = **2.25 MHz** |
| N | 8 |
| Window | ~**3.5 µs** |

Pulses shorter than 3.5 µs rejected; mechanical edges (ms) pass.

### 6.2 Why critical here

Board runs PWM (PA0), I2C (PB8/9), USART (PA9)—capacitive coupling injects **ns–µs** glitches on PA6/7. Asymmetric coupling → **direction bias**; variable noise → **inconsistent counts per detent**.

### 6.3 Filter table (IC1F @ fDTS=72 MHz)

| IC1F | fSAMPLING | N | Window | Use |
|------|-----------|---|--------|-----|
| 0000 | — | — | none | Clean optical only |
| … | … | … | … | … |
| **1111** | fDTS/32 | 8 | **~3.5 µs** | **Strongest (default)** |

$$T_{window} = \frac{N \times divisor}{f_{DTS}}$$

### 6.4 Fix in project

Enable `TIM_CCMR1_IC1F_MAX | IC2F_MAX` → stable **±4 per detent**, correct sign.

---

## 7. Wiring

| Encoder pin | STM32 |
|-------------|-------|
| A | PA6 |
| B | PA7 |
| C/COM/GND | **GND** (recommended) |
| VCC (module) | 3.3 V |

EC11 mechanical encoder often needs no VCC; KY-040 module needs 3.3 V.

OLED test: `CNT`, `dlt`, `dir`—one detent ≈ **±4** in 4× mode. Wrong direction → `TIM3_ENCODER_DIR_INVERTED` in `app.c`.

---

## 8. Application (Cooperative, No IRQ)

```c
int16_t delta = (int16_t)(now - s_enc_prev);
/* speed = delta / dt; rad/s = speed * (2π / (4*N_PPR)) */
```

| Use | Period |
|-----|--------|
| Balance control | 5–10 ms |
| Knob UI | 10–20 ms |
| Display | 100–500 ms |

Need `|delta| < 32768` per period.

---

## 9. Field Debug: Wrong Sign + Variable Steps

### Symptoms

- Idle stable ✓
- Steps: -2, -7, -15, … inconsistent
- Both rotation directions count negative

### Root cause (actual)

EMI from PWM/I2C/UART—not missing GND alone. ns glitches look like valid quadrature edges; PWM edges dominate manual rotation.

### Fix

`IC1F/IC2F = MAX` only—stable ±4, correct direction.

### Lessons

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| Drift when idle + moving | Floating reference | Wiring, pull-up, GND |
| Idle OK + direction bias | EMI | **Max digital filter** |
| Variable step + bias | Variable noise | Filter |
| Stable step + correct dir | OK | — |

Still **connect C to GND** for production.

---

## 10. Balance Bot Porting

Per wheel: Hall encoder A/B → TIM3 + TIM4 copy.

- PSC=0, ARR=0xFFFF
- Keep **IC1F/IC2F = 0xF** (H-bridge EMI)
- Speed:

$$v\ (\mathrm{rad/s}) = \frac{\Delta}{4 N_{PPR}} \cdot \frac{2\pi}{\Delta t}$$

Left TIM3, right TIM4 independent.

---

## 11. Common Errors

1. `TIM3EN` off — registers zero
2. No input pull-up — random CNT
3. **PSC ≠ 0** — lost edges (not like PWM)
4. SMS ≠ 011 — CNT frozen
5. CCxE off — no TI path
6. Direction inverted — `TIM3_ENCODER_DIR_INVERTED`
7. §9 symptom — enable max filter
8. Huge sporadic delta — sample period too long
9. C not GND — may work until environment changes
10. Long unshielded wires — motor EMI drift

---

## 12. One-Line Summary

Encoder on F1 = **TIM encoder mode counts A/B in hardware**. Engineering trio: **`PSC=0`, `SMS=ENC3`, max `IC1F/IC2F`** for 4× decode and EMI immunity.
