#ifndef DRIVERS_SSD1306_OLED_H
#define DRIVERS_SSD1306_OLED_H

#include <stdint.h>

/* SSD1306 128×64，软件 I2C：PB8=SCL、PB9=SDA（开漏 + 模块上拉）。
 * 须先调用 bsp_board_init()。
 * 默认 7 位地址 0x3C（写地址字节 0x78）；丝印 0x7A 的模块为 0x3D，改 ssd1306_oled.c 中 OLED_I2C_ADDR7。 */

void ssd1306_oled_init(void);
void ssd1306_oled_clear(void);
void ssd1306_oled_cursor_home(void);
void ssd1306_oled_putc(uint8_t c); /* 内部按需局部/全屏刷新，一般不必再调 refresh */
void ssd1306_oled_refresh(void);

#endif
