#ifndef DRIVERS_SSD1306_OLED_H
#define DRIVERS_SSD1306_OLED_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "common/stm_status.h"

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

/* 核心对象接口：所有写屏操作均为阻塞式 I2C 事务。 */
stm_status_t ssd1306_init(ssd1306_t *dev);
stm_status_t ssd1306_clear(ssd1306_t *dev);
stm_status_t ssd1306_cursor_home(ssd1306_t *dev);
stm_status_t ssd1306_putc(ssd1306_t *dev, uint8_t c);
stm_status_t ssd1306_flush(ssd1306_t *dev);
stm_status_t ssd1306_write_text(ssd1306_t *dev, const char *text);
stm_status_t ssd1306_write_text_at(ssd1306_t *dev, uint16_t page,
                                   uint16_t col_px, const char *text);
stm_status_t ssd1306_vwrite_text_atf(ssd1306_t *dev, uint16_t page,
                                     uint16_t col_px, const char *fmt,
                                     va_list ap);
stm_status_t ssd1306_write_text_atf(ssd1306_t *dev, uint16_t page,
                                    uint16_t col_px, const char *fmt, ...);

/*
 * 默认板载 OLED 实例接口：返回 `stm_status_t`，便于 app/BSP 做统一错误传播。
 * 新代码优先使用这些接口；`ssd1306_oled_*` 保留为兼容包装。
 */
ssd1306_t *ssd1306_default(void);
stm_status_t ssd1306_default_init(void);
stm_status_t ssd1306_default_clear(void);
stm_status_t ssd1306_default_cursor_home(void);
stm_status_t ssd1306_default_putc(uint8_t c);
stm_status_t ssd1306_default_refresh(void);
stm_status_t ssd1306_default_write_text_at(uint16_t page, uint16_t col_px,
                                           const char *text);
stm_status_t ssd1306_default_vwrite_text_atf(uint16_t page, uint16_t col_px,
                                             const char *fmt, va_list ap);
stm_status_t ssd1306_default_write_text_atf(uint16_t page, uint16_t col_px,
                                            const char *fmt, ...);

/* 兼容旧接口（吞掉返回值）；新代码不再推荐直接使用。 */
void ssd1306_oled_init(void);
void ssd1306_oled_clear(void);
void ssd1306_oled_cursor_home(void);
void ssd1306_oled_putc(uint8_t c);
void ssd1306_oled_refresh(void);
void ssd1306_oled_write_text_at(uint8_t page, uint8_t col_px, const char *text);
void ssd1306_oled_write_text_atf(uint8_t page, uint8_t col_px, const char *fmt,
                                 ...);

#endif
