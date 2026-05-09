# STM32F103 时钟系统全整理（结合本工程实现）

本文把本项目中与时钟相关的知识一次讲清：  
包括时钟树思路、代码流程、RCC/FLASH/SysTick/TIM 关键寄存器，以及常见坑和排障方法。

---

## 1. 先建立时钟心智模型

在 STM32F103 里，时钟不是一个值，而是一棵树：

1. 时钟源（HSI/HSE）
2. 经过 PLL（可选倍频）
3. 形成 SYSCLK（系统时钟）
4. 再分频得到：
   - HCLK（AHB）
   - PCLK1（APB1）
   - PCLK2（APB2）
5. 外设从各自总线取时钟（USART、TIM、I2C、SysTick 等）

你现在项目里的接口化目标就是：  
**上层只管“我要 8MHz 还是 72MHz”，底层负责 RCC 细节。**

---

## 2. 本工程时钟接口设计

文件：

- `include/bsp/clock.h`
- `src/bsp/clock.c`

核心 API：

- `bsp_clock_apply_profile(BSP_CLOCK_PROFILE_HSI_8MHZ)`
- `bsp_clock_apply_profile(BSP_CLOCK_PROFILE_HSE_PLL_72MHZ)`
- `bsp_clock_get_sysclk_hz()`
- `bsp_clock_get_hclk_hz()`
- `bsp_clock_get_pclk1_hz()`
- `bsp_clock_get_pclk2_hz()`

思路：

- profile 负责封装完整 RCC 切换流程
- getter 负责给驱动层提供“当前真实频率”
- 驱动（USART/TIM/SysTick/I2C）不再硬编码 8MHz

---

## 3. 代码流程（本工程实际）

`main` 中先设时钟，再做板级初始化：

1. `bsp_clock_apply_profile(...)`
2. `bsp_board_init()`（打开外设时钟门控）
3. `app_init()`

为什么先配时钟：

- 外设参数（如 BRR、PSC/ARR）依赖 `PCLK`  
- 先定时钟，后算分频，逻辑最稳

---

## 4. RCC 关键寄存器与位（时钟核心）

你工程里定义在 `include/bsp/stm32f103_regs.h`。

## 4.1 `RCC_CR`（Clock control）

常用位：

- `HSION/HSIRDY`：内部高速时钟使能/就绪（HSI 8MHz）
- `HSEON/HSERDY`：外部高速时钟使能/就绪（常见外部 8MHz 晶振）
- `PLLON/PLLRDY`：PLL 使能/就绪

用法：

- 先开某时钟源，再轮询 RDY
- 没等 RDY 就切换，系统可能跑飞

## 4.2 `RCC_CFGR`（Clock configuration）

常用位：

- `SW[1:0]`：系统时钟选择（HSI/HSE/PLL）
- `SWS[1:0]`：系统时钟实际状态（确认是否切换成功）
- `HPRE`：AHB 分频
- `PPRE1`：APB1 分频
- `PPRE2`：APB2 分频
- `PLLSRC`：PLL 输入源（HSI/2 或 HSE）
- `PLLXTPRE`：HSE 预分频
- `PLLMUL`：PLL 倍频因子（x2~x16，F1 常用 x9）

典型 72MHz 路线：

- HSE = 8MHz
- PLLSRC = HSE
- PLLXTPRE = /1
- PLLMUL = x9
- SYSCLK = 72MHz

## 4.3 `RCC_APB2ENR` / `RCC_APB1ENR`（外设时钟门控）

说明：

- 就算系统时钟已经配置好，外设不打开对应 EN 位也不会工作

本工程示例：

- APB2：AFIO / GPIOA / GPIOB / USART1
- APB1：I2C1 / TIM2

---

## 5. FLASH 与高频关系（72MHz 必看）

寄存器：`FLASH_ACR`

常用位：

- `LATENCY`：Flash 等待周期
- `PRFTBE`：预取缓冲使能

为什么要配：

- 主频升到 72MHz 后，Flash 访问速度跟不上，必须加 wait states
- 常见配置：`LATENCY=2` + 打开预取

不配会怎样：

- 随机异常、HardFault、跑飞、偶发死机

---

## 6. SysTick 与时钟关系

寄存器：

- `SYST_CSR`
- `SYST_RVR`
- `SYST_CVR`

你工程里 1ms 公式：

`reload = HCLK / 1000 - 1`

注意：

- 改主频后，必须用新 `HCLK` 重算 `reload`
- 这就是为什么你现在改成了通过 `bsp_clock_get_hclk_hz()` 间接算

---

## 7. TIM 与时钟关系（TIM2）

关键点：

- TIM2 在 APB1 上
- 当 APB1 分频不为 1 时，定时器时钟是 `2 * PCLK1`（F1 规则）

所以 TIM 实际输入时钟不是永远等于 PCLK1。

本工程已做：

- `tim2_init_1hz_interrupt()` 内判断 APB1 是否分频
- 自动选 `tim_clk = pclk1` 或 `2*pclk1`

---

## 8. USART 与时钟关系

USART1 在 APB2，总线时钟是 `PCLK2`。

BRR 计算必须用 `PCLK2`，不是盲目用 `SYSCLK`。  
否则波特率会偏差，串口乱码。

本工程已改成使用 `BSP_PCLK2_HZ`。

---

## 9. I2C 与时钟关系

I2C1 在 APB1，关键参数依赖 `PCLK1`：

- `CR2.FREQ`（MHz）
- `CCR`
- `TRISE`

改频后这三者必须重算，否则时序不准甚至无 ACK。

---

## 10. 8MHz 与 72MHz 对比（工程角度）

8MHz（HSI）：

- 配置简单、最稳定、学习友好
- 性能较低

72MHz（HSE+PLL）：

- 性能高（约 9 倍）
- 必须额外处理 FLASH 延迟、分频、更多边界条件

建议：

- 学习阶段先 8MHz
- 进入性能阶段切 72MHz

---

## 11. 72MHz 切换标准步骤（寄存器顺序）

建议顺序：

1. 开 HSE，等 HSERDY
2. 配 FLASH `LATENCY/PRFTBE`
3. 配 CFGR：分频、PLLSRC、PLLMUL
4. 开 PLL，等 PLLRDY
5. `SW=PLL` 切系统时钟
6. 读 `SWS` 确认已经切成功
7. 更新软件中的频率缓存（sysclk/hclk/pclk）

---

## 12. 常见错误与症状

1. **没等 RDY 就切换**
   - 症状：偶发卡死、启动不稳定

2. **忘设 FLASH 延迟**
   - 症状：72MHz 下随机 HardFault

3. **驱动仍用旧频率**
   - 症状：串口波特率错、TIM 周期不准、I2C 无 ACK

4. **APB1 定时器时钟计算错**
   - 症状：TIM 频率差一倍

5. **只改宏不改实际 RCC**
   - 症状：代码“以为”72MHz，硬件实际还在 8MHz

---

## 13. 本工程你该怎么用

在 `main` 开始位置：

- 8MHz：
  - `bsp_clock_apply_profile(BSP_CLOCK_PROFILE_HSI_8MHZ)`
- 72MHz：
  - `bsp_clock_apply_profile(BSP_CLOCK_PROFILE_HSE_PLL_72MHZ)`

然后其余驱动会通过 getter 拿频率，自动按当前时钟计算参数。

---

## 14. 你后续可以继续扩展

可扩展方向：

- 新增 36MHz / 48MHz profile
- 增加 `bsp_clock_config_t` 自定义配置接口
- 增加时钟失败回退（HSE失败自动退回HSI）
- 输出当前时钟信息到串口用于自检

---

## 15. 一句话总结

时钟系统工程化的核心不是“会配某个寄存器”，而是：  
**把 RCC 细节封装在 `bsp_clock`，让业务层只声明频率目标，并确保所有外设都用统一的频率来源进行参数计算。**

