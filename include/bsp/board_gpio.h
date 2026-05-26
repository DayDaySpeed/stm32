#ifndef BSP_BOARD_GPIO_H
#define BSP_BOARD_GPIO_H

#include "bsp/stm32f103_regs.h"

#include <stdint.h>

/*
 * 板级 GPIO 辅助宏/内联函数。
 * reg/bsrr/odr 为 GPIOx_CRL/BSRR/ODR 等寄存器左值宏。
 */

#define BOARD_GPIO_CRL_FIELD_POS(pin)   (((uint32_t)(pin)) * 4U)
#define BOARD_GPIO_CRH_FIELD_POS(pin)   ((((uint32_t)(pin)) - 8U) * 4U)
#define BOARD_GPIO_FIELD_POS(pin)                                              \
  (((pin) < 8U) ? BOARD_GPIO_CRL_FIELD_POS(pin)                                \
                : BOARD_GPIO_CRH_FIELD_POS(pin))

#define BOARD_GPIO_CRL_FIELD_MASK(pin)  (0xFU << BOARD_GPIO_CRL_FIELD_POS(pin))
#define BOARD_GPIO_CRH_FIELD_MASK(pin)  (0xFU << BOARD_GPIO_CRH_FIELD_POS(pin))
#define BOARD_GPIO_FIELD_MASK(pin)                                             \
  (((pin) < 8U) ? BOARD_GPIO_CRL_FIELD_MASK(pin)                               \
                : BOARD_GPIO_CRH_FIELD_MASK(pin))

#define BOARD_GPIO_MODE_ANALOG(pin)     (0x0U << BOARD_GPIO_FIELD_POS(pin)) /* 模拟输入 */
#define BOARD_GPIO_MODE_OUT_PP(pin)     (0x3U << BOARD_GPIO_FIELD_POS(pin)) /* 推挽输出 50MHz */
#define BOARD_GPIO_MODE_IN_FLOAT(pin)   (0x4U << BOARD_GPIO_FIELD_POS(pin)) /* 浮空输入 */
#define BOARD_GPIO_MODE_IN_PULL(pin)    (0x8U << BOARD_GPIO_FIELD_POS(pin)) /* 输入上/下拉 */
#define BOARD_GPIO_MODE_AF_PP(pin)      (0xBU << BOARD_GPIO_FIELD_POS(pin)) /* 复用推挽 */
#define BOARD_GPIO_MODE_AF_OD(pin)      (0xFU << BOARD_GPIO_FIELD_POS(pin)) /* 复用开漏（I2C） */
#define BOARD_GPIO_MODE_GPIO_OD(pin)    (0x7U << BOARD_GPIO_FIELD_POS(pin)) /* GPIO 开漏（总线恢复） */

#define BOARD_GPIO_ODR_PULLUP(pin)      (1U << (pin))

#define board_gpio_apply_crl(reg, mask, mode)                                  \
  do {                                                                         \
    (reg) = ((reg) & ~(mask)) | (mode);                                        \
  } while (0)

#define board_gpio_apply_crh(reg, mask, mode)                                  \
  do {                                                                         \
    (reg) = ((reg) & ~(mask)) | (mode);                                        \
  } while (0)

#define board_gpio_write(bsrr, pin, level)                                     \
  do {                                                                         \
    if ((level) != 0U) {                                                       \
      (bsrr) = (1U << (pin));                                                  \
    } else {                                                                   \
      (bsrr) = (1U << ((uint32_t)(pin) + 16U));                                \
    }                                                                          \
  } while (0)

#define board_gpio_odr_pullup(odr, mask)                                        \
  do {                                                                         \
    (odr) |= (mask);                                                           \
  } while (0)

/* 使能 GPIOB 端口时钟（APB2ENR.IOPBEN）。 */
static inline void board_gpio_enable_port_b_clock(void) {
  RCC_APB2ENR |= RCC_IOPBEN_BIT;
}

/* 使能 GPIOC 端口时钟（编码器重映射 PC6/PC7 时需要）。 */
static inline void board_gpio_enable_port_c_clock(void) {
  RCC_APB2ENR |= RCC_IOPCEN_BIT;
}

/* mask：AFIO_MAPR 位域掩码；value：要写入的 remap 值（先清 mask 再 OR）。 */
static inline void board_gpio_afio_apply(uint32_t mask, uint32_t value) {
  AFIO_MAPR = (AFIO_MAPR & ~mask) | value;
}

#endif
