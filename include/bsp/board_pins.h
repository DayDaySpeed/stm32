#ifndef BSP_BOARD_PINS_H
#define BSP_BOARD_PINS_H

#include "bsp/board_gpio.h"
#include "bsp/board_pin_mux.h"
#include "bsp/stm32f103_regs.h"

/*
 * 由 board_pin_mux.h 解析出的语义化引脚绑定。
 * 驱动只 include 本头文件，不直接写 PA0/PB6 等物理引脚名。
 */

/* ---------- 状态呼吸灯 (TIM2 CH1) ---------- */
#if (BOARD_STATUS_LED_PIN_MUX == BOARD_STATUS_LED_MUX_PA0_TIM2_CH1)
#define BOARD_AFIO_STATUS_LED_REMAP      (0U)
#define BOARD_GPIO_STATUS_LED_CR_REG     GPIOA_CRL
#define BOARD_GPIO_STATUS_LED_CR_MASK    BOARD_GPIO_CRL_FIELD_MASK(0U)
#define BOARD_GPIO_STATUS_LED_MODE_AF    BOARD_GPIO_MODE_AF_PP(0U)
#elif (BOARD_STATUS_LED_PIN_MUX == BOARD_STATUS_LED_MUX_PA15_TIM2_CH1)
#define BOARD_AFIO_STATUS_LED_REMAP      (1U << 8U) /* TIM2_REMAP[1:0]=01 */
#define BOARD_GPIO_STATUS_LED_CR_REG     GPIOA_CRH
#define BOARD_GPIO_STATUS_LED_CR_MASK    BOARD_GPIO_CRH_FIELD_MASK(15U)
#define BOARD_GPIO_STATUS_LED_MODE_AF    BOARD_GPIO_MODE_AF_PP(15U)
#else
#error "Unsupported BOARD_STATUS_LED_PIN_MUX"
#endif

/* ---------- ADC 模拟输入 ---------- */
#if (BOARD_ADC_PHOTO_PIN_MUX == BOARD_ADC_MUX_PA1_IN1)
#define BOARD_ADC_CH_PHOTO               (1U)
#define BOARD_GPIO_ADC_PHOTO_PIN         (1U)
#define BOARD_GPIO_ADC_PHOTO_MODE        BOARD_GPIO_MODE_ANALOG(1U)
#elif (BOARD_ADC_PHOTO_PIN_MUX == BOARD_ADC_MUX_PA4_IN4)
#define BOARD_ADC_CH_PHOTO               (4U)
#define BOARD_GPIO_ADC_PHOTO_PIN         (4U)
#define BOARD_GPIO_ADC_PHOTO_MODE        BOARD_GPIO_MODE_ANALOG(4U)
#else
#error "Unsupported BOARD_ADC_PHOTO_PIN_MUX"
#endif

#if (BOARD_ADC_THERM_PIN_MUX == BOARD_ADC_MUX_PA2_IN2)
#define BOARD_ADC_CH_THERM               (2U)
#define BOARD_GPIO_ADC_THERM_PIN         (2U)
#define BOARD_GPIO_ADC_THERM_MODE        BOARD_GPIO_MODE_ANALOG(2U)
#elif (BOARD_ADC_THERM_PIN_MUX == BOARD_ADC_MUX_PA5_IN5)
#define BOARD_ADC_CH_THERM               (5U)
#define BOARD_GPIO_ADC_THERM_PIN         (5U)
#define BOARD_GPIO_ADC_THERM_MODE        BOARD_GPIO_MODE_ANALOG(5U)
#else
#error "Unsupported BOARD_ADC_THERM_PIN_MUX"
#endif

#if (BOARD_ADC_IR_PIN_MUX == BOARD_ADC_MUX_PA3_IN3)
#define BOARD_ADC_CH_IR                  (3U)
#define BOARD_GPIO_ADC_IR_PIN            (3U)
#define BOARD_GPIO_ADC_IR_MODE           BOARD_GPIO_MODE_ANALOG(3U)
#elif (BOARD_ADC_IR_PIN_MUX == BOARD_ADC_MUX_PA6_IN6)
#define BOARD_ADC_CH_IR                  (6U)
#define BOARD_GPIO_ADC_IR_PIN            (6U)
#define BOARD_GPIO_ADC_IR_MODE           BOARD_GPIO_MODE_ANALOG(6U)
#else
#error "Unsupported BOARD_ADC_IR_PIN_MUX"
#endif

#define BOARD_GPIO_ADC_CR_REG            GPIOA_CRL
#define BOARD_GPIO_ADC_CR_MASK                                                 \
  (BOARD_GPIO_CRL_FIELD_MASK(BOARD_GPIO_ADC_PHOTO_PIN) |                        \
   BOARD_GPIO_CRL_FIELD_MASK(BOARD_GPIO_ADC_THERM_PIN) |                         \
   BOARD_GPIO_CRL_FIELD_MASK(BOARD_GPIO_ADC_IR_PIN))
#define BOARD_GPIO_ADC_ANALOG_MODE                                             \
  (BOARD_GPIO_ADC_PHOTO_MODE | BOARD_GPIO_ADC_THERM_MODE |                       \
   BOARD_GPIO_ADC_IR_MODE)

/* ---------- 蜂鸣器 ---------- */
#if (BOARD_BUZZER_PIN_MUX == BOARD_BUZZER_MUX_PA4)
#define BOARD_GPIO_BUZZER_CR_REG         GPIOA_CRL
#define BOARD_GPIO_BUZZER_CR_MASK        BOARD_GPIO_CRL_FIELD_MASK(4U)
#define BOARD_GPIO_BUZZER_MODE_OUT       BOARD_GPIO_MODE_OUT_PP(4U)
#define BOARD_GPIO_BUZZER_BSRR_REG       GPIOA_BSRR
#define BOARD_GPIO_BUZZER_PIN            (4U)
#elif (BOARD_BUZZER_PIN_MUX == BOARD_BUZZER_MUX_PA5)
#define BOARD_GPIO_BUZZER_CR_REG         GPIOA_CRL
#define BOARD_GPIO_BUZZER_CR_MASK        BOARD_GPIO_CRL_FIELD_MASK(5U)
#define BOARD_GPIO_BUZZER_MODE_OUT       BOARD_GPIO_MODE_OUT_PP(5U)
#define BOARD_GPIO_BUZZER_BSRR_REG       GPIOA_BSRR
#define BOARD_GPIO_BUZZER_PIN            (5U)
#elif (BOARD_BUZZER_PIN_MUX == BOARD_BUZZER_MUX_PB0)
#define BOARD_GPIO_BUZZER_CR_REG         GPIOB_CRL
#define BOARD_GPIO_BUZZER_CR_MASK        BOARD_GPIO_CRL_FIELD_MASK(0U)
#define BOARD_GPIO_BUZZER_MODE_OUT       BOARD_GPIO_MODE_OUT_PP(0U)
#define BOARD_GPIO_BUZZER_BSRR_REG       GPIOB_BSRR
#define BOARD_GPIO_BUZZER_PIN            (0U)
#else
#error "Unsupported BOARD_BUZZER_PIN_MUX"
#endif

/* ---------- 编码器 (TIM3 CH1/CH2) ---------- */
#if (BOARD_ENCODER_PIN_MUX == BOARD_ENCODER_MUX_PA6_PA7)
#define BOARD_AFIO_ENCODER_REMAP         (0U)
#define BOARD_GPIO_ENCODER_CR_REG        GPIOA_CRL
#define BOARD_GPIO_ENCODER_ODR_REG         GPIOA_ODR
#define BOARD_GPIO_ENCODER_CHA_PIN       (6U)
#define BOARD_GPIO_ENCODER_CHB_PIN       (7U)
#define BOARD_GPIO_ENCODER_CR_MASK                                               \
  (BOARD_GPIO_CRL_FIELD_MASK(6U) | BOARD_GPIO_CRL_FIELD_MASK(7U))
#define BOARD_GPIO_ENCODER_MODE_IN_PULL                                          \
  (BOARD_GPIO_MODE_IN_PULL(6U) | BOARD_GPIO_MODE_IN_PULL(7U))
#define BOARD_GPIO_ENCODER_ODR_PULLUP                                            \
  (BOARD_GPIO_ODR_PULLUP(6U) | BOARD_GPIO_ODR_PULLUP(7U))
#elif (BOARD_ENCODER_PIN_MUX == BOARD_ENCODER_MUX_PB4_PB5)
#define BOARD_AFIO_ENCODER_REMAP         (1U << 10U) /* TIM3_REMAP[1:0]=01 */
#define BOARD_GPIO_ENCODER_CR_REG        GPIOB_CRL
#define BOARD_GPIO_ENCODER_ODR_REG         GPIOB_ODR
#define BOARD_GPIO_ENCODER_CHA_PIN       (4U)
#define BOARD_GPIO_ENCODER_CHB_PIN       (5U)
#define BOARD_GPIO_ENCODER_CR_MASK                                               \
  (BOARD_GPIO_CRL_FIELD_MASK(4U) | BOARD_GPIO_CRL_FIELD_MASK(5U))
#define BOARD_GPIO_ENCODER_MODE_IN_PULL                                          \
  (BOARD_GPIO_MODE_IN_PULL(4U) | BOARD_GPIO_MODE_IN_PULL(5U))
#define BOARD_GPIO_ENCODER_ODR_PULLUP                                            \
  (BOARD_GPIO_ODR_PULLUP(4U) | BOARD_GPIO_ODR_PULLUP(5U))
#elif (BOARD_ENCODER_PIN_MUX == BOARD_ENCODER_MUX_PC6_PC7)
#define BOARD_AFIO_ENCODER_REMAP         (2U << 10U) /* TIM3_REMAP[1:0]=10 */
#define BOARD_GPIO_ENCODER_CR_REG        GPIOC_CRL
#define BOARD_GPIO_ENCODER_ODR_REG         GPIOC_ODR
#define BOARD_GPIO_ENCODER_CHA_PIN       (6U)
#define BOARD_GPIO_ENCODER_CHB_PIN       (7U)
#define BOARD_GPIO_ENCODER_CR_MASK                                               \
  (BOARD_GPIO_CRL_FIELD_MASK(6U) | BOARD_GPIO_CRL_FIELD_MASK(7U))
#define BOARD_GPIO_ENCODER_MODE_IN_PULL                                          \
  (BOARD_GPIO_MODE_IN_PULL(6U) | BOARD_GPIO_MODE_IN_PULL(7U))
#define BOARD_GPIO_ENCODER_ODR_PULLUP                                            \
  (BOARD_GPIO_ODR_PULLUP(6U) | BOARD_GPIO_ODR_PULLUP(7U))
#else
#error "Unsupported BOARD_ENCODER_PIN_MUX"
#endif

/* ---------- 传感器指示 LED (TIM1 CH1 / CH4) ---------- */
#if (BOARD_LDR_LED_PIN_MUX == BOARD_LDR_LED_MUX_PA8_TIM1_CH1)
#define BOARD_GPIO_LDR_LED_CR_REG        GPIOA_CRH
#define BOARD_GPIO_LDR_LED_CR_MASK       BOARD_GPIO_CRH_FIELD_MASK(8U)
#define BOARD_GPIO_LDR_LED_MODE_AF       BOARD_GPIO_MODE_AF_PP(8U)
#else
#error "Unsupported BOARD_LDR_LED_PIN_MUX"
#endif

#if (BOARD_NTC_LED_PIN_MUX == BOARD_NTC_LED_MUX_PA11_TIM1_CH4)
#define BOARD_GPIO_NTC_LED_CR_REG        GPIOA_CRH
#define BOARD_GPIO_NTC_LED_CR_MASK       BOARD_GPIO_CRH_FIELD_MASK(11U)
#define BOARD_GPIO_NTC_LED_MODE_AF       BOARD_GPIO_MODE_AF_PP(11U)
#else
#error "Unsupported BOARD_NTC_LED_PIN_MUX"
#endif

#define BOARD_GPIO_SENSOR_LED_CR_REG     GPIOA_CRH
#define BOARD_GPIO_SENSOR_LED_CR_MASK                                            \
  (BOARD_GPIO_LDR_LED_CR_MASK | BOARD_GPIO_NTC_LED_CR_MASK)
#define BOARD_GPIO_SENSOR_LED_MODE_AF                                          \
  (BOARD_GPIO_LDR_LED_MODE_AF | BOARD_GPIO_NTC_LED_MODE_AF)

/* ---------- USART1 ---------- */
#if (BOARD_USART1_PIN_MUX == BOARD_USART1_MUX_PA9_PA10)
#define BOARD_GPIO_USART1_CR_REG         GPIOA_CRH
#define BOARD_GPIO_USART1_TX_CR_MASK     BOARD_GPIO_CRH_FIELD_MASK(9U)
#define BOARD_GPIO_USART1_RX_CR_MASK     BOARD_GPIO_CRH_FIELD_MASK(10U)
#define BOARD_GPIO_USART1_TX_MODE        BOARD_GPIO_MODE_AF_PP(9U)
#define BOARD_GPIO_USART1_RX_MODE        BOARD_GPIO_MODE_IN_FLOAT(10U)
#else
#error "Unsupported BOARD_USART1_PIN_MUX"
#endif

/* ---------- 直流电机 TB6612 ---------- */
#if (BOARD_MOTOR_PWM_PIN_MUX == BOARD_MOTOR_PWM_MUX_PB6_TIM4_CH1)
#define BOARD_GPIO_MOTOR_PWM_PIN         (6U)
#define BOARD_GPIO_MOTOR_PWM_CR_MASK     BOARD_GPIO_CRL_FIELD_MASK(6U)
#define BOARD_GPIO_MOTOR_PWM_MODE_AF     BOARD_GPIO_MODE_AF_PP(6U)
#define BOARD_GPIO_MOTOR_PWM_MODE_OUT    BOARD_GPIO_MODE_OUT_PP(6U)
#else
#error "Unsupported BOARD_MOTOR_PWM_PIN_MUX"
#endif

#if (BOARD_MOTOR_AIN1_PIN_MUX == BOARD_MOTOR_AIN1_MUX_PB7)
#define BOARD_GPIO_MOTOR_AIN1_PIN        (7U)
#elif (BOARD_MOTOR_AIN1_PIN_MUX == BOARD_MOTOR_AIN1_MUX_PB8)
#define BOARD_GPIO_MOTOR_AIN1_PIN        (8U)
#else
#error "Unsupported BOARD_MOTOR_AIN1_PIN_MUX"
#endif

#if (BOARD_MOTOR_AIN2_PIN_MUX == BOARD_MOTOR_AIN2_MUX_PB5)
#define BOARD_GPIO_MOTOR_AIN2_PIN        (5U)
#elif (BOARD_MOTOR_AIN2_PIN_MUX == BOARD_MOTOR_AIN2_MUX_PB9)
#define BOARD_GPIO_MOTOR_AIN2_PIN        (9U)
#else
#error "Unsupported BOARD_MOTOR_AIN2_PIN_MUX"
#endif

#define BOARD_GPIO_MOTOR_CR_REG          GPIOB_CRL
#define BOARD_GPIO_MOTOR_BSRR_REG        GPIOB_BSRR
#define BOARD_GPIO_MOTOR_AIN1_CR_MASK                                            \
  ((BOARD_GPIO_MOTOR_AIN1_PIN < 8U)                                            \
       ? BOARD_GPIO_CRL_FIELD_MASK(BOARD_GPIO_MOTOR_AIN1_PIN)                  \
       : BOARD_GPIO_CRH_FIELD_MASK(BOARD_GPIO_MOTOR_AIN1_PIN))
#define BOARD_GPIO_MOTOR_AIN2_CR_MASK                                            \
  ((BOARD_GPIO_MOTOR_AIN2_PIN < 8U)                                            \
       ? BOARD_GPIO_CRL_FIELD_MASK(BOARD_GPIO_MOTOR_AIN2_PIN)                  \
       : BOARD_GPIO_CRH_FIELD_MASK(BOARD_GPIO_MOTOR_AIN2_PIN))
#define BOARD_GPIO_MOTOR_AIN1_MODE_OUT                                         \
  BOARD_GPIO_MODE_OUT_PP(BOARD_GPIO_MOTOR_AIN1_PIN)
#define BOARD_GPIO_MOTOR_AIN2_MODE_OUT                                         \
  BOARD_GPIO_MODE_OUT_PP(BOARD_GPIO_MOTOR_AIN2_PIN)
#define BOARD_GPIO_MOTOR_GPIO_MASK                                             \
  (BOARD_GPIO_MOTOR_PWM_CR_MASK | BOARD_GPIO_MOTOR_AIN1_CR_MASK |              \
   BOARD_GPIO_MOTOR_AIN2_CR_MASK)
#define BOARD_GPIO_MOTOR_MODE_INIT                                             \
  (BOARD_GPIO_MOTOR_PWM_MODE_AF | BOARD_GPIO_MOTOR_AIN1_MODE_OUT |             \
   BOARD_GPIO_MOTOR_AIN2_MODE_OUT)
#define BOARD_GPIO_MOTOR_MODE_SAFE                                             \
  (BOARD_GPIO_MOTOR_PWM_MODE_OUT | BOARD_GPIO_MOTOR_AIN1_MODE_OUT |             \
   BOARD_GPIO_MOTOR_AIN2_MODE_OUT)

/* PB8/PB9 方向脚需 CRH；统一用 apply 两次或扩展 helper。电机默认 PB5/PB7 均在 CRL。 */
#if ((BOARD_GPIO_MOTOR_AIN1_PIN >= 8U) || (BOARD_GPIO_MOTOR_AIN2_PIN >= 8U))
#error "Motor AIN on CRH pins needs board_gpio helper extension"
#endif

/* ---------- I2C1 (OLED) ---------- */
#if (BOARD_I2C1_PIN_MUX == BOARD_I2C1_MUX_REMAP_PB8_PB9)
#define BOARD_AFIO_I2C1_REMAP            AFIO_MAPR_I2C1_REMAP_BIT
#define BOARD_GPIO_I2C1_SCL_PIN          (8U)
#define BOARD_GPIO_I2C1_SDA_PIN          (9U)
#define BOARD_GPIO_I2C1_CR_REG           GPIOB_CRH
#define BOARD_GPIO_I2C1_BSRR_REG         GPIOB_BSRR
#define BOARD_GPIO_I2C1_CR_MASK          (0x000000FFUL)
#define BOARD_GPIO_I2C1_MODE_AF_OD       (0x000000FFUL)
#define BOARD_GPIO_I2C1_MODE_GPIO_OD     (0x00000077UL)
#elif (BOARD_I2C1_PIN_MUX == BOARD_I2C1_MUX_DEFAULT_PB6_PB7)
#define BOARD_AFIO_I2C1_REMAP            (0U)
#define BOARD_GPIO_I2C1_SCL_PIN          (6U)
#define BOARD_GPIO_I2C1_SDA_PIN          (7U)
#define BOARD_GPIO_I2C1_CR_REG           GPIOB_CRL
#define BOARD_GPIO_I2C1_BSRR_REG         GPIOB_BSRR
#define BOARD_GPIO_I2C1_CR_MASK          (0x00FF0000UL)
#define BOARD_GPIO_I2C1_MODE_AF_OD                                               \
  (BOARD_GPIO_MODE_AF_OD(6U) | BOARD_GPIO_MODE_AF_OD(7U))
#define BOARD_GPIO_I2C1_MODE_GPIO_OD                                             \
  (BOARD_GPIO_MODE_GPIO_OD(6U) | BOARD_GPIO_MODE_GPIO_OD(7U))
#else
#error "Unsupported BOARD_I2C1_PIN_MUX"
#endif

#endif
