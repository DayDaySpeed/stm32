#ifndef BSP_BOARD_PIN_MUX_H
#define BSP_BOARD_PIN_MUX_H

/*
 * 板级 GPIO 复用选项 —— 换引脚或解决冲突时只改本文件。
 * 驱动与 app 通过 board_pins.h 的语义宏消费，无需改动源码。
 *
 * 用法：每个功能从下方 *_OPTIONS 中取消注释一项，或改 #define 右侧常量。
 */

/* ========== 状态呼吸灯 (TIM2 CH1 PWM) ========== */
#define BOARD_STATUS_LED_MUX_PA0_TIM2_CH1    (0U)
#define BOARD_STATUS_LED_MUX_PA15_TIM2_CH1   (1U) /* 需 AFIO 部分重映射；占用 JTDI */

#define BOARD_STATUS_LED_PIN_MUX             BOARD_STATUS_LED_MUX_PA0_TIM2_CH1

/* ========== ADC 模拟传感器 (ADC1) ========== */
#define BOARD_ADC_MUX_PA1_IN1                (0U)
#define BOARD_ADC_MUX_PA4_IN4                (1U)

#define BOARD_ADC_MUX_PA2_IN2                (0U)
#define BOARD_ADC_MUX_PA5_IN5                (1U)

#define BOARD_ADC_MUX_PA3_IN3                (0U)
#define BOARD_ADC_MUX_PA6_IN6                (1U)

#define BOARD_ADC_PHOTO_PIN_MUX              BOARD_ADC_MUX_PA1_IN1
#define BOARD_ADC_THERM_PIN_MUX              BOARD_ADC_MUX_PA2_IN2
#define BOARD_ADC_IR_PIN_MUX                 BOARD_ADC_MUX_PA3_IN3

/* ========== 有源蜂鸣器 (GPIO 推挽) ========== */
#define BOARD_BUZZER_MUX_PA4                 (0U)
#define BOARD_BUZZER_MUX_PA5                 (1U)
#define BOARD_BUZZER_MUX_PB0                 (2U)

#define BOARD_BUZZER_PIN_MUX                 BOARD_BUZZER_MUX_PA4

/* ========== 正交编码器 (TIM3 CH1/CH2) ========== */
#define BOARD_ENCODER_MUX_PA6_PA7            (0U)
#define BOARD_ENCODER_MUX_PB4_PB5            (1U) /* TIM3 部分重映射；与电机 AIN2(PB5) 冲突 */
#define BOARD_ENCODER_MUX_PC6_PC7            (2U) /* TIM3 完全重映射；需 GPIOC 时钟 */

#define BOARD_ENCODER_PIN_MUX                BOARD_ENCODER_MUX_PA6_PA7

/* ========== 传感器指示 LED (TIM1 PWM) ========== */
#define BOARD_LDR_LED_MUX_PA8_TIM1_CH1       (0U)
#define BOARD_NTC_LED_MUX_PA11_TIM1_CH4      (0U)

#define BOARD_LDR_LED_PIN_MUX                BOARD_LDR_LED_MUX_PA8_TIM1_CH1
#define BOARD_NTC_LED_PIN_MUX                BOARD_NTC_LED_MUX_PA11_TIM1_CH4

/* ========== 调试串口 (USART1) ========== */
#define BOARD_USART1_MUX_PA9_PA10            (0U)

#define BOARD_USART1_PIN_MUX                 BOARD_USART1_MUX_PA9_PA10

/* ========== 直流电机 TB6612 ========== */
#define BOARD_MOTOR_PWM_MUX_PB6_TIM4_CH1     (0U)

#define BOARD_MOTOR_AIN1_MUX_PB7             (0U)
#define BOARD_MOTOR_AIN1_MUX_PB8             (1U)

#define BOARD_MOTOR_AIN2_MUX_PB5             (0U)
#define BOARD_MOTOR_AIN2_MUX_PB9             (1U)

#define BOARD_MOTOR_PWM_PIN_MUX              BOARD_MOTOR_PWM_MUX_PB6_TIM4_CH1
#define BOARD_MOTOR_AIN1_PIN_MUX             BOARD_MOTOR_AIN1_MUX_PB7
#define BOARD_MOTOR_AIN2_PIN_MUX             BOARD_MOTOR_AIN2_MUX_PB5

/* ========== OLED I2C (I2C1) ========== */
#define BOARD_I2C1_MUX_DEFAULT_PB6_PB7       (0U) /* 与电机 PWM(PB6) 冲突 */
#define BOARD_I2C1_MUX_REMAP_PB8_PB9         (1U) /* 推荐：AFIO 重映射 */

#define BOARD_I2C1_PIN_MUX                   BOARD_I2C1_MUX_REMAP_PB8_PB9

#endif
