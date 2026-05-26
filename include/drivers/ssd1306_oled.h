#ifndef DRIVERS_SSD1306_OLED_H
#define DRIVERS_SSD1306_OLED_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "common/stm_status.h"

/*
 * SSD1306 128×64 OLED（I2C 7 位地址 0x3C）。
 *
 * 分层：
 *   ssd1306_*        — 通用设备对象 API（可注入 bus_write）
 *   ssd1306_default_* — 板载默认实例，供 bsp_display_* 调用
 */

typedef stm_status_t (*ssd1306_bus_write_fn)(uint8_t addr7, uint8_t ctrl,
                                             const uint8_t *payload,
                                             size_t payload_len);

typedef struct {
  uint16_t width;
  uint16_t page_count;
  uint8_t addr7;
  ssd1306_bus_write_fn bus_write;
  uint8_t *framebuffer;
  uint16_t col_px;
  uint16_t row_page;
  uint8_t initialized;
} ssd1306_t;

stm_status_t ssd1306_init(ssd1306_t *dev);
stm_status_t ssd1306_clear(ssd1306_t *dev);
stm_status_t ssd1306_flush(ssd1306_t *dev);
stm_status_t ssd1306_write_text_at(ssd1306_t *dev, uint16_t page,
                                   uint16_t col_px, const char *text);
stm_status_t ssd1306_vwrite_text_atf(ssd1306_t *dev, uint16_t page,
                                     uint16_t col_px, const char *fmt,
                                     va_list ap);

stm_status_t ssd1306_default_init(void);
stm_status_t ssd1306_default_clear(void);
stm_status_t ssd1306_default_vwrite_text_atf(uint16_t page, uint16_t col_px,
                                             const char *fmt, va_list ap);

#endif
