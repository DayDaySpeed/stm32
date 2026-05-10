#include "drivers/ssd1306_oled.h"

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "bsp/clock.h"
#include "common/stm_macros.h"
#include "common/stm_status.h"
#include "hal/i2c1_master.h"

#define SSD1306_WIDTH 128U
#define SSD1306_PAGE_COUNT 8U
#define SSD1306_FB_SIZE (SSD1306_WIDTH * SSD1306_PAGE_COUNT)
#define SSD1306_ADDR7 0x3CU

extern const uint8_t oled_font5x7[];

static uint8_t g_default_fb[SSD1306_FB_SIZE];

static stm_status_t ssd1306_bus_write_default(uint8_t addr7, uint8_t ctrl,
                                              const uint8_t *payload,
                                              size_t payload_len) {
  return i2c1_master_write_frame(addr7, ctrl, payload, payload_len);
}

static ssd1306_t g_default_dev = {
    .width = SSD1306_WIDTH,
    .page_count = SSD1306_PAGE_COUNT,
    .addr7 = SSD1306_ADDR7,
    .bus_write = ssd1306_bus_write_default,
    .framebuffer = g_default_fb,
    .col_px = 0U,
    .row_page = 0U,
    .initialized = 0U,
};

static stm_status_t ssd1306_send_commands(ssd1306_t *dev, const uint8_t *cmds,
                                          size_t n) {
  if ((dev == NULL) || (dev->bus_write == NULL)) {
    return STM_ERR_INVALID_ARG;
  }
  if (n == 0U) {
    return STM_OK;
  }
  return dev->bus_write(dev->addr7, 0x00U, cmds, n);
}

static stm_status_t ssd1306_push_gddram(ssd1306_t *dev, const uint8_t *data,
                                        size_t len) {
  if ((dev == NULL) || (dev->bus_write == NULL)) {
    return STM_ERR_INVALID_ARG;
  }
  return dev->bus_write(dev->addr7, 0x40U, data, len);
}

static stm_status_t ssd1306_flush_region(ssd1306_t *dev, uint16_t col0,
                                         uint16_t page, uint16_t ncol) {
  uint8_t setwin[6];
  if ((dev == NULL) || (dev->framebuffer == NULL)) {
    return STM_ERR_INVALID_ARG;
  }
  if ((ncol == 0U) || (page >= dev->page_count) || (col0 >= dev->width)) {
    return STM_OK;
  }
  if ((uint32_t)col0 + ncol > dev->width) {
    ncol = (uint16_t)(dev->width - col0);
  }

  setwin[0] = 0x21U;
  setwin[1] = (uint8_t)col0;
  setwin[2] = (uint8_t)(col0 + ncol - 1U);
  setwin[3] = 0x22U;
  setwin[4] = (uint8_t)page;
  setwin[5] = (uint8_t)page;

  if (ssd1306_send_commands(dev, setwin, sizeof(setwin)) != STM_OK) {
    return STM_ERR_IO;
  }

  return ssd1306_push_gddram(dev, &dev->framebuffer[(page * dev->width) + col0],
                             ncol);
}

static uint8_t ssd1306_scroll_up_one_page(ssd1306_t *dev) {
  uint32_t width = 0U;
  uint32_t size = 0U;
  if ((dev == NULL) || (dev->framebuffer == NULL)) {
    return 0U;
  }
  width = dev->width;
  size = dev->width * dev->page_count;
  (void)memmove(&dev->framebuffer[0], &dev->framebuffer[width], size - width);
  (void)memset(&dev->framebuffer[size - width], 0, width);
  if (dev->row_page > 0U) {
    dev->row_page--;
  }
  return 1U;
}

static uint8_t ssd1306_ensure_row_visible(ssd1306_t *dev) {
  uint8_t scrolled = 0U;
  while ((dev != NULL) && (dev->row_page >= dev->page_count)) {
    scrolled |= ssd1306_scroll_up_one_page(dev);
  }
  return scrolled;
}

static uint32_t ssd1306_append_char(char *dst, uint32_t idx, uint32_t cap,
                                    char c) {
  if (idx + 1U < cap) {
    dst[idx] = c;
  }
  return idx + 1U;
}

static uint32_t ssd1306_append_str(char *dst, uint32_t idx, uint32_t cap,
                                   const char *s) {
  if (s == NULL) {
    return idx;
  }
  while (*s != '\0') {
    idx = ssd1306_append_char(dst, idx, cap, *s);
    s++;
  }
  return idx;
}

static uint32_t ssd1306_append_u32(char *dst, uint32_t idx, uint32_t cap,
                                   uint32_t v, uint32_t base, char hex_a) {
  char tmp[11];
  uint32_t n = 0U;

  if ((base != 10U) && (base != 16U)) {
    return idx;
  }
  if (v == 0U) {
    return ssd1306_append_char(dst, idx, cap, '0');
  }

  while ((v != 0U) && (n < STM_ARRAY_SIZE(tmp))) {
    uint32_t d = v % base;
    if (d < 10U) {
      tmp[n++] = (char)('0' + d);
    } else {
      tmp[n++] = (char)(hex_a + (d - 10U));
    }
    v /= base;
  }
  while (n > 0U) {
    n--;
    idx = ssd1306_append_char(dst, idx, cap, tmp[n]);
  }
  return idx;
}

static uint32_t ssd1306_append_i32(char *dst, uint32_t idx, uint32_t cap,
                                   int32_t v) {
  uint32_t uv = 0U;
  if (v < 0) {
    idx = ssd1306_append_char(dst, idx, cap, '-');
    uv = (uint32_t)(-(v + 1)) + 1U;
  } else {
    uv = (uint32_t)v;
  }
  return ssd1306_append_u32(dst, idx, cap, uv, 10U, 'a');
}

static uint32_t ssd1306_vformat(char *dst, uint32_t cap, const char *fmt,
                                va_list ap) {
  uint32_t idx = 0U;
  char spec = '\0';

  if ((dst == NULL) || (cap == 0U) || (fmt == NULL)) {
    return 0U;
  }

  while (*fmt != '\0') {
    if (*fmt != '%') {
      idx = ssd1306_append_char(dst, idx, cap, *fmt++);
      continue;
    }
    fmt++;
    spec = *fmt;
    if (spec == '\0') {
      break;
    }
    switch (spec) {
    case '%':
      idx = ssd1306_append_char(dst, idx, cap, '%');
      break;
    case 'c':
      idx = ssd1306_append_char(dst, idx, cap, (char)va_arg(ap, int));
      break;
    case 's':
      idx = ssd1306_append_str(dst, idx, cap, va_arg(ap, const char *));
      break;
    case 'u':
      idx = ssd1306_append_u32(dst, idx, cap, va_arg(ap, uint32_t), 10U, 'a');
      break;
    case 'd':
      idx = ssd1306_append_i32(dst, idx, cap, va_arg(ap, int32_t));
      break;
    case 'x':
      idx = ssd1306_append_u32(dst, idx, cap, va_arg(ap, uint32_t), 16U, 'a');
      break;
    case 'X':
      idx = ssd1306_append_u32(dst, idx, cap, va_arg(ap, uint32_t), 16U, 'A');
      break;
    default:
      idx = ssd1306_append_char(dst, idx, cap, '%');
      idx = ssd1306_append_char(dst, idx, cap, spec);
      break;
    }
    fmt++;
  }

  if (cap > 0U) {
    uint32_t end = (idx < (cap - 1U)) ? idx : (cap - 1U);
    dst[end] = '\0';
  }
  return idx;
}

ssd1306_t *ssd1306_default(void) { return &g_default_dev; }

stm_status_t ssd1306_init(ssd1306_t *dev) {
  static const uint8_t init_cmds[] = {
      0xAEU, 0xD5U, 0x80U, 0xA8U, 0x3FU, 0xD3U, 0x00U, 0x40U, 0x8DU,
      0x14U, 0x20U, 0x00U, 0xA1U, 0xC8U, 0xDAU, 0x12U, 0x81U, 0xCFU,
      0xD9U, 0xF1U, 0xDBU, 0x40U, 0xA4U, 0xA6U, 0xAFU,
  };
  i2c1_master_config_t i2c_cfg;

  if ((dev == NULL) || (dev->bus_write == NULL) ||
      (dev->framebuffer == NULL) || (dev->width != SSD1306_WIDTH) ||
      (dev->page_count != SSD1306_PAGE_COUNT)) {
    return STM_ERR_INVALID_ARG;
  }

  i2c_cfg.pclk_hz = bsp_clock_get_pclk1_hz();
  i2c_cfg.bus_hz = 100000UL;
  i2c_cfg.timeout_iter = 100000UL;

  if (i2c1_master_init(&i2c_cfg) != STM_OK) {
    return STM_ERR_IO;
  }

  dev->col_px = 0U;
  dev->row_page = 0U;
  (void)memset(dev->framebuffer, 0, SSD1306_FB_SIZE);

  if (ssd1306_send_commands(dev, init_cmds, sizeof(init_cmds)) != STM_OK) {
    return STM_ERR_IO;
  }

  dev->initialized = 1U;
  return ssd1306_flush(dev);
}

stm_status_t ssd1306_clear(ssd1306_t *dev) {
  if ((dev == NULL) || (dev->framebuffer == NULL)) {
    return STM_ERR_INVALID_ARG;
  }
  (void)memset(dev->framebuffer, 0, SSD1306_FB_SIZE);
  dev->col_px = 0U;
  dev->row_page = 0U;
  return ssd1306_flush(dev);
}

stm_status_t ssd1306_cursor_home(ssd1306_t *dev) {
  if (dev == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  dev->col_px = 0U;
  dev->row_page = 0U;
  return STM_OK;
}

stm_status_t ssd1306_putc(ssd1306_t *dev, uint8_t c) {
  uint8_t need_full = 0U;
  uint16_t col0 = 0U;
  uint16_t row_page = 0U;
  uint16_t base = 0U;

  if ((dev == NULL) || (dev->framebuffer == NULL) ||
      (dev->initialized == 0U)) {
    return STM_ERR_INVALID_ARG;
  }

  if (c == '\r') {
    return STM_OK;
  }

  if (c == '\n') {
    dev->col_px = 0U;
    dev->row_page++;
    if (ssd1306_ensure_row_visible(dev) != 0U) {
      return ssd1306_flush(dev);
    }
    return STM_OK;
  }

  if ((uint32_t)dev->col_px + 6U > dev->width) {
    dev->col_px = 0U;
    dev->row_page++;
    need_full = ssd1306_ensure_row_visible(dev);
  }

  col0 = dev->col_px;
  row_page = dev->row_page;
  base = (uint16_t)(row_page * dev->width + col0);

  for (uint16_t i = 0U; i < 5U; ++i) {
    dev->framebuffer[base + i] = oled_font5x7[((uint16_t)c * 5U) + i];
  }
  dev->framebuffer[base + 5U] = 0U;
  dev->col_px = (uint16_t)(dev->col_px + 6U);

  if (need_full != 0U) {
    return ssd1306_flush(dev);
  }
  return ssd1306_flush_region(dev, col0, row_page, 6U);
}

stm_status_t ssd1306_flush(ssd1306_t *dev) {
  static const uint8_t setwin[] = {0x21U, 0x00U, 0x7FU, 0x22U, 0x00U, 0x07U};
  stm_status_t st = STM_OK;

  if ((dev == NULL) || (dev->framebuffer == NULL) ||
      (dev->initialized == 0U)) {
    return STM_ERR_INVALID_ARG;
  }

  st = ssd1306_send_commands(dev, setwin, sizeof(setwin));
  if (st != STM_OK) {
    return st;
  }
  return ssd1306_push_gddram(dev, dev->framebuffer, SSD1306_FB_SIZE);
}

stm_status_t ssd1306_write_text(ssd1306_t *dev, const char *text) {
  stm_status_t st = STM_OK;
  if ((dev == NULL) || (text == NULL)) {
    return STM_ERR_INVALID_ARG;
  }
  while (*text != '\0') {
    st = ssd1306_putc(dev, (uint8_t)*text);
    if (st != STM_OK) {
      return st;
    }
    text++;
  }
  return STM_OK;
}

stm_status_t ssd1306_write_text_at(ssd1306_t *dev, uint16_t page,
                                   uint16_t col_px, const char *text) {
  uint8_t scrolled = 0U;
  stm_status_t st = STM_OK;

  if ((dev == NULL) || (text == NULL)) {
    return STM_ERR_INVALID_ARG;
  }
  if (col_px >= dev->width) {
    return STM_ERR_INVALID_ARG;
  }
  /* 超出页面时每次调用只上滚一页，并定位到最后一页写入。 */
  if (page >= dev->page_count) {
    (void)ssd1306_scroll_up_one_page(dev);
    page = (uint16_t)(dev->page_count - 1U);
    scrolled = 1U;
  }

  /* 滚屏改动了整块 framebuffer，先整屏同步，避免只更新最后一行。 */
  if (scrolled != 0U) {
    st = ssd1306_flush(dev);
    if (st != STM_OK) {
      return st;
    }
  }
  dev->row_page = page;
  dev->col_px = col_px;
  return ssd1306_write_text(dev, text);
}

stm_status_t ssd1306_write_text_atf(ssd1306_t *dev, uint16_t page,
                                    uint16_t col_px, const char *fmt, ...) {
  char text_buf[64];
  va_list ap;
  uint32_t n = 0U;

  if ((dev == NULL) || (fmt == NULL)) {
    return STM_ERR_INVALID_ARG;
  }

  va_start(ap, fmt);
  n = ssd1306_vformat(text_buf, sizeof(text_buf), fmt, ap);
  va_end(ap);

  if (n == 0U) {
    return STM_ERR_IO;
  }

  return ssd1306_write_text_at(dev, page, col_px, text_buf);
}

void ssd1306_oled_init(void) { (void)ssd1306_init(&g_default_dev); }

void ssd1306_oled_clear(void) { (void)ssd1306_clear(&g_default_dev); }

void ssd1306_oled_cursor_home(void) {
  (void)ssd1306_cursor_home(&g_default_dev);
}

void ssd1306_oled_putc(uint8_t c) { (void)ssd1306_putc(&g_default_dev, c); }

void ssd1306_oled_refresh(void) { (void)ssd1306_flush(&g_default_dev); }

void ssd1306_oled_write_text_at(uint8_t page, uint8_t col_px,
                                const char *text) {
  (void)ssd1306_write_text_at(&g_default_dev, (uint16_t)page, (uint16_t)col_px,
                              text);
}

void ssd1306_oled_write_text_atf(uint8_t page, uint8_t col_px, const char *fmt,
                                 ...) {
  char text_buf[64];
  va_list ap;
  uint32_t n = 0U;

  if (fmt == NULL) {
    return;
  }

  va_start(ap, fmt);
  n = ssd1306_vformat(text_buf, sizeof(text_buf), fmt, ap);
  va_end(ap);
  if (n == 0U) {
    return;
  }

  (void)ssd1306_write_text_at(&g_default_dev, (uint16_t)page, (uint16_t)col_px,
                              text_buf);
}
