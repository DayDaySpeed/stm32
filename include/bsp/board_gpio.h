#ifndef BSP_BOARD_GPIO_H
#define BSP_BOARD_GPIO_H

#include "bsp/stm32f103_regs.h"

#include <stdint.h>

/*
 * 板级 GPIO 辅助 —— 驱动通过语义宏 + 本头文件宏/内联函数操作引脚，
 * 避免在各驱动中散落 GPIOA/PB6 等硬编码。
 *
 * 注意：GPIOx_CRL/BSRR 等是寄存器左值宏，配置操作用 macro 而非指针参数。
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

#define BOARD_GPIO_MODE_ANALOG(pin)     (0x0U << BOARD_GPIO_FIELD_POS(pin))
#define BOARD_GPIO_MODE_OUT_PP(pin)     (0x3U << BOARD_GPIO_FIELD_POS(pin))
#define BOARD_GPIO_MODE_IN_FLOAT(pin)   (0x4U << BOARD_GPIO_FIELD_POS(pin))
#define BOARD_GPIO_MODE_IN_PULL(pin)    (0x8U << BOARD_GPIO_FIELD_POS(pin))
#define BOARD_GPIO_MODE_AF_PP(pin)      (0xBU << BOARD_GPIO_FIELD_POS(pin))
#define BOARD_GPIO_MODE_AF_OD(pin)      (0xFU << BOARD_GPIO_FIELD_POS(pin))
#define BOARD_GPIO_MODE_GPIO_OD(pin)    (0x7U << BOARD_GPIO_FIELD_POS(pin))

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

static inline void board_gpio_enable_port_b_clock(void) {
  RCC_APB2ENR |= RCC_IOPBEN_BIT;
}

static inline void board_gpio_enable_port_c_clock(void) {
  RCC_APB2ENR |= RCC_IOPCEN_BIT;
}

static inline void board_gpio_afio_apply(uint32_t mask, uint32_t value) {
  AFIO_MAPR = (AFIO_MAPR & ~mask) | value;
}

#endif
