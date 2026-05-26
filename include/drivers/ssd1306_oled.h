#ifndef DRIVERS_SSD1306_OLED_H
#define DRIVERS_SSD1306_OLED_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "common/stm_status.h"

/*
 * SSD1306 128×64 OLED（I2C 0x3C）。
 * dev 对象 API 可注入 bus_write；default_* 为板载单例。
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

/* dev：设备对象；内部 init I2C 并发送 SSD1306 上电命令序列。 */
stm_status_t ssd1306_init(ssd1306_t *dev);
/* 清帧缓冲并 flush 到屏。 */
stm_status_t ssd1306_clear(ssd1306_t *dev);
/* 将 framebuffer 全屏写入 GDDRAM。 */
stm_status_t ssd1306_flush(ssd1306_t *dev);
/* page：0..page_count-1；col_px：列起点；text：5×7 字体字符串。 */
stm_status_t ssd1306_write_text_at(ssd1306_t *dev, uint16_t page,
                                   uint16_t col_px, const char *text);
/* fmt/ap：printf 风格；先格式化再 write_text_at。 */
stm_status_t ssd1306_vwrite_text_atf(ssd1306_t *dev, uint16_t page,
                                     uint16_t col_px, const char *fmt,
                                     va_list ap);

stm_status_t ssd1306_default_init(void);
stm_status_t ssd1306_default_clear(void);
stm_status_t ssd1306_default_vwrite_text_atf(uint16_t page, uint16_t col_px,
                                             const char *fmt, va_list ap);

#endif
