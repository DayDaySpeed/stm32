#ifndef BSP_STM32F103_REGS_H
#define BSP_STM32F103_REGS_H

#include <stdint.h>

/* RCC（复位与时钟控制，Reset and Clock Control）：负责时钟源选择、PLL 配置、总线分频与外设时钟门控 */
#define RCC_BASE            (0x40021000UL) /* RCC 外设基地址 */
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00UL)) /* 时钟控制寄存器：HSI/HSE/PLL 开关与就绪位 */
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x04UL)) /* 时钟配置寄存器：系统时钟源、分频、PLL 配置 */
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x1CUL)) /* APB1 外设时钟使能寄存器 */
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x18UL)) /* APB2 外设时钟使能寄存器 */
#define RCC_AHBENR          (*(volatile uint32_t *)(RCC_BASE + 0x14UL)) /* AHB 外设时钟使能寄存器 */

#define RCC_DMA1EN_BIT      (1U << 0)  /* DMA1 时钟使能（AHBENR） */

#define RCC_TIM2EN_BIT      (1U << 0)  /* TIM2 时钟使能（APB1ENR） */
#define RCC_TIM3EN_BIT      (1U << 1)  /* TIM3 时钟使能（APB1ENR） */
#define RCC_I2C1EN_BIT      (1U << 21) /* I2C1 时钟使能（APB1ENR） */

#define RCC_AFIOEN_BIT      (1U << 0)  /* AFIO 时钟使能（APB2ENR） */
#define RCC_IOPAEN_BIT      (1U << 2)  /* GPIOA 时钟使能（APB2ENR） */
#define RCC_IOPBEN_BIT      (1U << 3)  /* GPIOB 时钟使能（APB2ENR） */
#define RCC_IOPCEN_BIT      (1U << 4)  /* GPIOC 时钟使能（APB2ENR） */
#define RCC_USART1EN_BIT    (1U << 14) /* USART1 时钟使能（APB2ENR） */
#define RCC_ADC1EN_BIT      (1U << 9)  /* ADC1 时钟使能（APB2ENR） */

#define RCC_CR_HSION_BIT    (1U << 0)  /* HSI 时钟开关 */
#define RCC_CR_HSIRDY_BIT   (1U << 1)  /* HSI 就绪标志 */
#define RCC_CR_HSEON_BIT    (1U << 16) /* HSE 时钟开关 */
#define RCC_CR_HSERDY_BIT   (1U << 17) /* HSE 就绪标志 */
#define RCC_CR_PLLON_BIT    (1U << 24) /* PLL 开关 */
#define RCC_CR_PLLRDY_BIT   (1U << 25) /* PLL 就绪标志 */

#define RCC_CFGR_SW_MASK    (3U << 0)   /* 系统时钟源选择字段掩码（SW[1:0]） */
#define RCC_CFGR_SW_HSI     (0U << 0)   /* 系统时钟选择 HSI */
#define RCC_CFGR_SW_PLL     (2U << 0)   /* 系统时钟选择 PLL */
#define RCC_CFGR_SWS_MASK   (3U << 2)   /* 当前系统时钟源状态字段掩码（SWS[1:0]） */
#define RCC_CFGR_SWS_HSI    (0U << 2)   /* 当前系统时钟来自 HSI */
#define RCC_CFGR_SWS_PLL    (2U << 2)   /* 当前系统时钟来自 PLL */
#define RCC_CFGR_HPRE_MASK  (0xFU << 4) /* AHB 分频字段掩码（HPRE[3:0]） */
#define RCC_CFGR_HPRE_DIV1  (0U << 4)   /* AHB 分频 = /1（HCLK=SYSCLK） */
#define RCC_CFGR_PPRE1_MASK (0x7U << 8) /* APB1 分频字段掩码（PPRE1[2:0]） */
#define RCC_CFGR_PPRE1_DIV1 (0U << 8)   /* APB1 分频 = /1 */
#define RCC_CFGR_PPRE1_DIV2 (4U << 8)   /* APB1 分频 = /2 */
#define RCC_CFGR_PPRE2_MASK (0x7U << 11) /* APB2 分频字段掩码（PPRE2[2:0]） */
#define RCC_CFGR_PPRE2_DIV1 (0U << 11)  /* APB2 分频 = /1 */
#define RCC_CFGR_PLLSRC_MASK (1U << 16) /* PLL 输入源字段掩码（PLLSRC） */
#define RCC_CFGR_PLLSRC_HSE (1U << 16)  /* PLL 输入源选择 HSE */
#define RCC_CFGR_PLLXTPRE_MASK (1U << 17) /* HSE 预分频字段掩码（PLLXTPRE） */
#define RCC_CFGR_PLLXTPRE_HSE_DIV1 (0U << 17) /* HSE 送 PLL 前不分频 */
#define RCC_CFGR_PLLMUL_MASK (0xFU << 18) /* PLL 倍频字段掩码 */
#define RCC_CFGR_PLLMUL9    (7U << 18)  /* PLL 倍频 x9（8MHz->72MHz） */
#define RCC_CFGR_ADCPRE_MASK (3U << 14) /* ADC 预分频（PCLK2 分频后送 ADC） */
#define RCC_CFGR_ADCPRE_DIV2 (0U << 14) /* ADCCLK = PCLK2/2 */
#define RCC_CFGR_ADCPRE_DIV4 (1U << 14) /* ADCCLK = PCLK2/4 */
#define RCC_CFGR_ADCPRE_DIV6 (2U << 14) /* ADCCLK = PCLK2/6（72MHz 时常用，约 12MHz） */
#define RCC_CFGR_ADCPRE_DIV8 (3U << 14) /* ADCCLK = PCLK2/8 */

/* FLASH（闪存接口，Flash memory interface）：负责程序 Flash 访问控制，如等待周期与预取缓冲 */
#define FLASH_BASE             (0x40022000UL) /* Flash 控制器基地址 */
#define FLASH_ACR              (*(volatile uint32_t *)(FLASH_BASE + 0x00UL)) /* Flash 访问控制寄存器（等待周期/预取） */
#define FLASH_ACR_LATENCY_MASK (7U << 0)  /* Flash 等待周期字段掩码（LATENCY[2:0]） */
#define FLASH_ACR_LATENCY_2    (2U << 0)  /* Flash 等待周期 2（72MHz 常用） */
#define FLASH_ACR_PRFTBE_BIT   (1U << 4)  /* 预取缓冲使能 */

/* AFIO（复用功能 I/O，Alternate Function I/O）：负责外设引脚重映射与复用功能路由 */
#define AFIO_BASE           (0x40010000UL) /* AFIO 基地址 */
#define AFIO_MAPR           (*(volatile uint32_t *)(AFIO_BASE + 0x04UL)) /* AF 重映射寄存器 */
#define AFIO_MAPR_I2C1_REMAP_BIT (1U << 1) /* 1: I2C1_SCL=PB8, I2C1_SDA=PB9 */

/* GPIOA（通用输入输出端口 A，General Purpose Input/Output Port A）：配置 A 口引脚模式并进行输入输出控制 */
#define GPIOA_BASE          (0x40010800UL) /* GPIOA 基地址 */
#define GPIOA_CRL           (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL)) /* GPIOA 配置低寄存器（PA0~PA7） */
#define GPIOA_CRH           (*(volatile uint32_t *)(GPIOA_BASE + 0x04UL)) /* GPIOA 配置高寄存器（PA8~PA15） */
#define GPIOA_IDR           (*(volatile uint32_t *)(GPIOA_BASE + 0x08UL)) /* GPIOA 输入数据寄存器 */
#define GPIOA_ODR           (*(volatile uint32_t *)(GPIOA_BASE + 0x0CUL)) /* GPIOA 输出数据寄存器（输入模式下用于选上拉/下拉） */

/* GPIOB（通用输入输出端口 B，General Purpose Input/Output Port B）：配置 B 口引脚模式并进行输入输出控制 */
#define GPIOB_BASE          (0x40010C00UL) /* GPIOB 基地址 */
#define GPIOB_CRH           (*(volatile uint32_t *)(GPIOB_BASE + 0x04UL)) /* GPIOB 配置高寄存器（PB8~PB15） */
#define GPIOB_IDR           (*(volatile uint32_t *)(GPIOB_BASE + 0x08UL)) /* GPIOB 输入数据寄存器 */
#define GPIOB_ODR           (*(volatile uint32_t *)(GPIOB_BASE + 0x0CUL)) /* GPIOB 输出数据寄存器 */
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x10UL)) /* GPIOB 置位/复位寄存器（原子操作） */

/* GPIOC（通用输入输出端口 C，General Purpose Input/Output Port C）：配置 C 口引脚模式并进行输入输出控制 */
#define GPIOC_BASE          (0x40011000UL) /* GPIOC 基地址 */
#define GPIOC_CRH           (*(volatile uint32_t *)(GPIOC_BASE + 0x04UL)) /* GPIOC 配置高寄存器（PC8~PC15） */
#define GPIOC_ODR           (*(volatile uint32_t *)(GPIOC_BASE + 0x0CUL)) /* GPIOC 输出数据寄存器 */

/* USART1（通用同步异步收发器1，Universal Synchronous/Asynchronous Receiver/Transmitter 1）：负责串口发送、接收与中断控制 */
#define USART1_BASE         (0x40013800UL) /* USART1 基地址 */
#define USART1_SR           (*(volatile uint32_t *)(USART1_BASE + 0x00UL)) /* 状态寄存器 */
#define USART1_DR           (*(volatile uint32_t *)(USART1_BASE + 0x04UL)) /* 数据寄存器（读收发写发） */
#define USART1_BRR          (*(volatile uint32_t *)(USART1_BASE + 0x08UL)) /* 波特率寄存器 */
#define USART1_CR1          (*(volatile uint32_t *)(USART1_BASE + 0x0CUL)) /* 控制寄存器1 */

#define USART_SR_RXNE_BIT   (1U << 5)   /* 接收数据寄存器非空（有新数据可读） */
#define USART_SR_TXE_BIT    (1U << 7)   /* 发送数据寄存器空（可写入下一个字节） */
#define USART_CR1_RE_BIT    (1U << 2)   /* 接收使能 */
#define USART_CR1_TE_BIT    (1U << 3)   /* 发送使能 */
#define USART_CR1_RXNEIE_BIT (1U << 5)  /* RXNE 中断使能 */
#define USART_CR1_UE_BIT    (1U << 13)  /* USART 总使能 */
#define USART_CR1_OVER8_BIT (1U << 15)  /* 1=8倍过采样，0=16倍过采样 */

/* I2C1（内部集成电路总线1，Inter-Integrated Circuit 1）：负责 I2C 主从通信时序与数据传输 */
#define I2C1_BASE           (0x40005400UL) /* I2C1 基地址 */
#define I2C1_CR1            (*(volatile uint32_t *)(I2C1_BASE + 0x00UL)) /* 控制寄存器1（PE/START/STOP） */
#define I2C1_CR2            (*(volatile uint32_t *)(I2C1_BASE + 0x04UL)) /* 控制寄存器2（频率等） */
#define I2C1_DR             (*(volatile uint32_t *)(I2C1_BASE + 0x10UL)) /* 数据寄存器 */
#define I2C1_SR1            (*(volatile uint32_t *)(I2C1_BASE + 0x14UL)) /* 状态寄存器1（事件/错误） */
#define I2C1_SR2            (*(volatile uint32_t *)(I2C1_BASE + 0x18UL)) /* 状态寄存器2（BUSY 等） */
#define I2C1_CCR            (*(volatile uint32_t *)(I2C1_BASE + 0x1CUL)) /* 时钟控制寄存器 */
#define I2C1_TRISE          (*(volatile uint32_t *)(I2C1_BASE + 0x20UL)) /* 最大上升时间寄存器 */

#define I2C_CR1_PE_BIT      (1U << 0)   /* 外设使能（Peripheral Enable） */
#define I2C_CR1_START_BIT   (1U << 8)   /* 主机发 START 条件 */
#define I2C_CR1_STOP_BIT    (1U << 9)   /* 主机发 STOP 条件，结束当前传输 */

#define I2C_SR1_SB_BIT      (1U << 0)   /* START 已发送（EV5） */
#define I2C_SR1_ADDR_BIT    (1U << 1)   /* 地址阶段完成并收到应答（EV6） */
#define I2C_SR1_BTF_BIT     (1U << 2)   /* 字节传输完成（Data register + shift register 都空） */
#define I2C_SR1_TXE_BIT     (1U << 7)   /* DR 发送寄存器空，可写下一个字节（EV8） */
#define I2C_SR1_AF_BIT      (1U << 10)  /* 应答失败（Acknowledge Failure，常见于收到 NACK） */

#define I2C_SR2_BUSY_BIT    (1U << 1)   /* 总线忙：检测 START 到 STOP 之间的总线占用状态 */

/* TIM2（通用定时器2，General-purpose Timer 2）：负责计时、更新中断、输入捕获与输出比较等定时功能 */
#define TIM2_BASE           (0x40000000UL) /* TIM2 基地址 */
#define TIM2_CR1            (*(volatile uint32_t *)(TIM2_BASE + 0x00UL)) /* 控制寄存器1（CEN 等） */
#define TIM2_DIER           (*(volatile uint32_t *)(TIM2_BASE + 0x0CUL)) /* DMA/中断使能寄存器 */
#define TIM2_SR             (*(volatile uint32_t *)(TIM2_BASE + 0x10UL)) /* 状态寄存器（UIF 等） */
#define TIM2_EGR            (*(volatile uint32_t *)(TIM2_BASE + 0x14UL)) /* 事件生成寄存器（UG） */
#define TIM2_CCMR1          (*(volatile uint32_t *)(TIM2_BASE + 0x18UL)) /* 捕获/比较模式寄存器1（CH1/CH2） */
#define TIM2_CCER           (*(volatile uint32_t *)(TIM2_BASE + 0x20UL)) /* 捕获/比较使能寄存器 */
#define TIM2_CNT            (*(volatile uint32_t *)(TIM2_BASE + 0x24UL)) /* 计数器当前值 */
#define TIM2_PSC            (*(volatile uint32_t *)(TIM2_BASE + 0x28UL)) /* 预分频寄存器 */
#define TIM2_ARR            (*(volatile uint32_t *)(TIM2_BASE + 0x2CUL)) /* 自动重装寄存器 */
#define TIM2_CCR1           (*(volatile uint32_t *)(TIM2_BASE + 0x34UL)) /* 捕获/比较寄存器1（CH1 脉宽/PWM 占空比） */

#define TIM_CR1_CEN_BIT     (1U << 0) /* 计数器使能 */
#define TIM_CR1_ARPE_BIT    (1U << 7) /* 自动重装预装载使能（影子寄存器） */
#define TIM_DIER_UIE_BIT    (1U << 0) /* 更新中断使能 */
#define TIM_SR_UIF_BIT      (1U << 0) /* 更新中断标志 */
#define TIM_EGR_UG_BIT      (1U << 0) /* 更新事件生成（强制装载预分频） */
#define TIM_CCMR1_CC1S_MASK (3U << 0) /* CC1S：通道 1 方向选择（00=输出，其余=输入） */
#define TIM_CCMR1_CC1S_OUT  (0U << 0) /* CC1 = 输出模式 */
#define TIM_CCMR1_OC1PE_BIT (1U << 3) /* OC1 预装载使能 */
#define TIM_CCMR1_OC1M_MASK (7U << 4) /* OC1M：输出比较 1 模式 */
#define TIM_CCMR1_OC1M_PWM1 (6U << 4) /* PWM 模式 1（边沿对齐，CNT<CCR 时有效电平） */
#define TIM_CCER_CC1E_BIT   (1U << 0) /* 捕获/比较 1 输出使能 */
#define TIM_CCER_CC1P_BIT   (1U << 1) /* 捕获/比较 1 输出极性 */
#define TIM_CCER_CC2E_BIT   (1U << 4) /* 捕获/比较 2 使能 */
#define TIM_CCER_CC2P_BIT   (1U << 5) /* 捕获/比较 2 输出/输入极性 */

/* TIM3（通用定时器3）：本工程用于正交编码器模式（CH1/CH2 = TI1/TI2，PA6/PA7） */
#define TIM3_BASE           (0x40000400UL)
#define TIM3_CR1            (*(volatile uint32_t *)(TIM3_BASE + 0x00UL)) /* 控制寄存器1 */
#define TIM3_SMCR           (*(volatile uint32_t *)(TIM3_BASE + 0x08UL)) /* 从模式控制寄存器（SMS 编码器模式） */
#define TIM3_DIER           (*(volatile uint32_t *)(TIM3_BASE + 0x0CUL)) /* DMA/中断使能 */
#define TIM3_SR             (*(volatile uint32_t *)(TIM3_BASE + 0x10UL)) /* 状态寄存器 */
#define TIM3_EGR            (*(volatile uint32_t *)(TIM3_BASE + 0x14UL)) /* 事件生成 */
#define TIM3_CCMR1          (*(volatile uint32_t *)(TIM3_BASE + 0x18UL)) /* CCMR1（CH1/CH2 输入/输出配置） */
#define TIM3_CCER           (*(volatile uint32_t *)(TIM3_BASE + 0x20UL)) /* 捕获/比较使能（输入极性） */
#define TIM3_CNT            (*(volatile uint32_t *)(TIM3_BASE + 0x24UL)) /* 计数器（编码器模式下记录脉冲计数） */
#define TIM3_PSC            (*(volatile uint32_t *)(TIM3_BASE + 0x28UL)) /* 预分频（编码器模式必须=0） */
#define TIM3_ARR            (*(volatile uint32_t *)(TIM3_BASE + 0x2CUL)) /* 自动重装（编码器溢出/下溢边界） */

/* TIM 通用：CR1 方向位（编码器模式下只读，硬件按 A/B 相位关系自动写） */
#define TIM_CR1_DIR_BIT     (1U << 4) /* 0=向上计数，1=向下计数 */

/* TIM CCMR1 输入捕获字段（与 PWM 时的输出比较是同一个寄存器，但语义不同）。
 * 编码器模式下：CC1S/CC2S = 01 把 CH1/CH2 配成输入，分别连到 TI1/TI2。 */
#define TIM_CCMR1_CC1S_TI1  (1U << 0)   /* CC1 输入接到 TI1（编码器 A 相） */
#define TIM_CCMR1_CC2S_MASK (3U << 8)   /* CC2S 字段掩码 */
#define TIM_CCMR1_CC2S_TI2  (1U << 8)   /* CC2 输入接到 TI2（编码器 B 相） */
#define TIM_CCMR1_IC1F_MASK (0xFU << 4) /* TI1 输入数字滤波字段 */
#define TIM_CCMR1_IC2F_MASK (0xFU << 12)/* TI2 输入数字滤波字段 */
/* 滤波档位 0xF：fSAMPLING = fDTS/32, N=8。在 fDTS=72MHz 时窗口约 3.5μs；
 * 对干净的电机编码器毫无影响（脉冲周期 >> 3.5μs），但能把 EMI/接触毛刺挡掉。 */
#define TIM_CCMR1_IC1F_MAX  (0xFU << 4)
#define TIM_CCMR1_IC2F_MAX  (0xFU << 12)
#define TIM_CCMR1_IC1PSC_MASK (3U << 2) /* TI1 输入预分频（编码器要清零） */
#define TIM_CCMR1_IC2PSC_MASK (3U << 10)/* TI2 输入预分频 */

/* TIM SMCR.SMS[2:0]：从模式选择，编码器模式在此选三选一 */
#define TIM_SMCR_SMS_MASK   (7U << 0)
#define TIM_SMCR_SMS_ENC1   (1U << 0)   /* 编码器模式1：仅 TI2 边沿计数（2x） */
#define TIM_SMCR_SMS_ENC2   (2U << 0)   /* 编码器模式2：仅 TI1 边沿计数（2x） */
#define TIM_SMCR_SMS_ENC3   (3U << 0)   /* 编码器模式3：TI1+TI2 双边沿计数（4x，分辨率最高） */

/* ADC1（模数转换器1，Analog-to-Digital Converter 1）：规则组单次/连续转换，多通道采样时间可配 */
#define ADC1_BASE           (0x40012400UL) /* ADC1 外设基地址 */
#define ADC1_SR             (*(volatile uint32_t *)(ADC1_BASE + 0x00UL)) /* 状态寄存器（EOC 等） */
#define ADC1_CR1            (*(volatile uint32_t *)(ADC1_BASE + 0x04UL)) /* 控制寄存器1（扫描/间断等） */
#define ADC1_CR2            (*(volatile uint32_t *)(ADC1_BASE + 0x08UL)) /* 控制寄存器2（ADON/启动/校准） */
#define ADC1_SMPR2          (*(volatile uint32_t *)(ADC1_BASE + 0x10UL)) /* 采样时间寄存器2（IN0~IN9） */
#define ADC1_SQR1           (*(volatile uint32_t *)(ADC1_BASE + 0x2CUL)) /* 规则序列寄存器1（序列长度 L） */
#define ADC1_SQR3           (*(volatile uint32_t *)(ADC1_BASE + 0x34UL)) /* 规则序列寄存器3（SQ1..SQ6） */
#define ADC1_DR             (*(volatile uint32_t *)(ADC1_BASE + 0x4CUL)) /* 规则组数据寄存器（12 位右对齐） */

#define ADC_CR1_SCAN_BIT    (1U << 8)   /* 扫描模式：按 SQR 序列依次转换多路 */
#define ADC_CR1_L_SHIFT     (20U)       /* SQR1.L[3:0]：规则序列长度 - 1 */
#define ADC_CR1_L_MASK      (0xFU << ADC_CR1_L_SHIFT)
#define ADC_CR1_L_2_CONV    (1U << ADC_CR1_L_SHIFT) /* L=1 → 共 2 次转换 */

#define ADC_SQR3_SQ1_SHIFT  (0U)
#define ADC_SQR3_SQ2_SHIFT  (5U)
#define ADC_SQR3_SQ_MASK    (0x1FU)
#define ADC_SQR3_SQ1(ch)    (((uint32_t)(ch) & ADC_SQR3_SQ_MASK) << ADC_SQR3_SQ1_SHIFT)
#define ADC_SQR3_SQ2(ch)    (((uint32_t)(ch) & ADC_SQR3_SQ_MASK) << ADC_SQR3_SQ2_SHIFT)

#define ADC_SMPR2_SMP1_SHIFT (3U)
#define ADC_SMPR2_SMP2_SHIFT (6U)
#define ADC_SMPR2_SMP_MASK   (7U)
#define ADC_SMPR2_SMP1_MAX   (7U << ADC_SMPR2_SMP1_SHIFT)
#define ADC_SMPR2_SMP2_MAX   (7U << ADC_SMPR2_SMP2_SHIFT)

#define ADC_SR_EOC_BIT      (1U << 1)   /* 规则组转换结束（读 DR 或写 SR 可清） */
#define ADC_CR2_ADON_BIT    (1U << 0)   /* ADC 上电/使能 */
#define ADC_CR2_CONT_BIT    (1U << 1)   /* 连续转换模式 */
#define ADC_CR2_DMA_BIT     (1U << 8)   /* 规则组转换结果经 DMA 搬运 */
#define ADC_CR2_CAL_BIT     (1U << 2)   /* 启动校准（硬件完成后清零） */
#define ADC_CR2_RSTCAL_BIT  (1U << 3)   /* 复位校准寄存器（硬件完成后清零） */
#define ADC_CR2_EXTSEL_MASK (7U << 17)  /* 规则组外部触发源选择（EXTSEL[2:0]） */
#define ADC_CR2_EXTSEL_SWSTART (7U << 17) /* 规则组触发源选择为 SWSTART */
#define ADC_CR2_EXTTRIG_BIT (1U << 20)  /* 规则组外部触发允许；软件触发也需先放行 */
#define ADC_CR2_SWSTART_BIT (1U << 22)  /* 软件启动规则组转换 */

/* DMA1（直接存储器访问，Direct Memory Access）：ADC1 规则组固定走 Channel1 */
#define DMA1_BASE           (0x40020000UL)
#define DMA1_ISR            (*(volatile uint32_t *)(DMA1_BASE + 0x00UL))
#define DMA1_IFCR           (*(volatile uint32_t *)(DMA1_BASE + 0x04UL))
#define DMA1_CCR1           (*(volatile uint32_t *)(DMA1_BASE + 0x08UL))
#define DMA1_CNDTR1         (*(volatile uint32_t *)(DMA1_BASE + 0x0CUL))
#define DMA1_CPAR1          (*(volatile uint32_t *)(DMA1_BASE + 0x10UL))
#define DMA1_CMAR1          (*(volatile uint32_t *)(DMA1_BASE + 0x14UL))

#define DMA_ISR_TCIF1_BIT   (1U << 1)   /* 通道 1 传输完成 */
#define DMA_IFCR_CTCIF1_BIT (1U << 1)   /* 写 1 清通道 1 TC 标志 */
#define DMA_CCR_EN_BIT      (1U << 0)   /* 通道使能 */
#define DMA_CCR_MINC_BIT    (1U << 7)   /* 内存地址递增 */
#define DMA_CCR_PSIZE_16_BIT (1U << 8)  /* 外设宽度 16 位 */
#define DMA_CCR_MSIZE_16_BIT (1U << 10) /* 内存宽度 16 位 */

/* NVIC（嵌套向量中断控制器，Nested Vectored Interrupt Controller）：负责中断使能与中断分发 */
#define NVIC_BASE           (0xE000E100UL) /* NVIC 基地址 */
#define NVIC_ISER0          (*(volatile uint32_t *)(NVIC_BASE + 0x00UL)) /* 中断使能寄存器0（IRQ0~31） */
#define NVIC_ISER1          (*(volatile uint32_t *)(NVIC_BASE + 0x04UL)) /* 中断使能寄存器1（IRQ32~63） */
#define NVIC_TIM2_IRQ_BIT   (1U << 28)  /* TIM2 IRQn=28 -> ISER0 bit28 */
#define NVIC_USART1_IRQ_BIT (1U << 5)   /* USART1 IRQn=37 -> ISER1 bit5 */


/* SysTick（系统滴答定时器，System Tick Timer）：提供内核级周期中断与软件时基 */
#define SYSTICK_BASE        (0xE000E010UL) /* SysTick 基地址 */
#define SYST_CSR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x00UL)) /* 控制与状态寄存器 */
#define SYST_RVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x04UL)) /* 重装载值寄存器 */
#define SYST_CVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x08UL)) /* 当前计数值寄存器 */

#define SYSTICK_ENABLE_BIT  (1U << 0) /* 计数器使能 */
#define SYSTICK_TICKINT_BIT (1U << 1) /* 计数到 0 触发中断使能 */
#define SYSTICK_CLKSRC_BIT  (1U << 2) /* 时钟源选择：1=处理器时钟(HCLK) */

#endif
