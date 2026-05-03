/*
 * SSD1306 OLED 驱动（128×64，I2C）
 * ---------------------------------------------------------------------------
 * 在干什么：
 *   用 PB8/PB9 两个 GPIO「软件模拟」I2C，给 SSD1306 发命令和显存数据；屏幕内容
 *   先写在单片机里的数组 g_fb（帧缓冲），再一次性 I2C 写到屏的 GDDRAM。
 *
 * 显存与 g_fb 的对应（水平寻址模式 0x20 0x00）：
 *   - 屏宽 128 像素；纵向每 8 个像素为一「页(page)」，共 8 页 → 64 像素高。
 *   - g_fb 按「先列后页」排：下标 0～127 是第 0 页的一行 128 列，128～255 是第 1 页……
 *   - 每个字节里 bit 控制该列上纵向 8 个像素（具体上下与 SEG/COM 配置有关）。
 *
 * 文本光标（putc 用）：
 *   g_col_px：当前字符从第几列像素开始画（0～127）。
 *   g_row_page：当前字符画在第几「页」上（0～7，一页高度 8 像素，刚好容纳 5×7 字模）。
 *
 * 对外流程简述：
 *   ssd1306_oled_init()     → 配 PB8/9 GPIO、发初始化命令、清屏并 refresh（须先 bsp_board_init）。
 *   ssd1306_oled_putc()     → 写 g_fb，并尽量只 I2C 更新 6 列×当前页（滚屏后则全屏 refresh）。
 *   ssd1306_oled_refresh()  → 全屏同步 g_fb → GDDRAM（清屏等可显式调用）。
 */
#include "drivers/ssd1306_oled.h"

#include "bsp/stm32f103_regs.h"

#include <stddef.h>
#include <string.h>

/* SSD1306 的 7 位 I2C 地址（手册/丝印常见 0x3C；另一接法为 0x3D） */
#define OLED_I2C_ADDR7      (0x3CU)
/* 8 位写地址 = 7 位地址左移 |0（写）；即 0x3C → 总线上发 0x78 */
#define OLED_I2C_ADDR8_W   ((uint8_t)(OLED_I2C_ADDR7 << 1))

#define SSD1306_WIDTH       (128U)  /* 横向像素数 */
#define SSD1306_PAGE_COUNT  (8U)    /* 纵向 64/8 = 8 页 */
#define FB_SIZE             (SSD1306_WIDTH * SSD1306_PAGE_COUNT) /* 128*8=1024 字节 */

#define PB_SCL_PIN          (8U)    /* 位带 I2C：SCL 接 PB8 */
#define PB_SDA_PIN          (9U)    /* SDA 接 PB9；开漏输出，上拉在 OLED 小板 */

#define SCL_H()             (GPIOB_BSRR = (1U << PB_SCL_PIN))
#define SCL_L()             (GPIOB_BSRR = (1U << (PB_SCL_PIN + 16U)))
#define SDA_H()             (GPIOB_BSRR = (1U << PB_SDA_PIN))
#define SDA_L()             (GPIOB_BSRR = (1U << (PB_SDA_PIN + 16U)))

extern const uint8_t oled_font5x7[]; /* 256 字符 × 5 字节/字，见 oled_font5x7.c */

/* 帧缓冲：与 SSD1306 GDDRAM 同布局，refresh 时整包写出 */
static uint8_t g_fb[FB_SIZE];
/* 下一个字符左上角所在列(0～127)、所在页(0～7) */
static unsigned g_col_px;
static unsigned g_row_page;

/* 位带 I2C 位时间：空循环拖慢 GPIO，避免超过从机/上拉允许的速率 */
static void i2c_delay(void)
{
  for (volatile unsigned n = 0U; n < 50U; ++n) {
    __asm volatile("" ::: "memory");
  }
}

/* I2C START：SCL 高时 SDA 由高变低 */
static void i2c_start(void)
{
  SDA_H();
  SCL_H();
  i2c_delay();
  SDA_L();
  i2c_delay();
  SCL_L();
  i2c_delay();
}

/* I2C STOP：SCL 高时 SDA 由低变高 */
static void i2c_stop(void)
{
  SDA_L();
  i2c_delay();
  SCL_H();
  i2c_delay();
  SDA_H();
  i2c_delay();
}

/*
 * 主机写 1 字节：8 个数据位(MSB 先发) + 第 9 位 ACK。
 * I2C 规范：第 9 个 SCL 为高期间由从机驱动 SDA；SDA=0 为 ACK，=1 为 NACK，
 * 主机应在 SCL 高电平有效区间内采样 SDA。收到 NACK 时上层应发 STOP 并中止。
 * 返回值：1=收到 ACK，0=NACK 或总线异常（SDA 仍为高）。
 */
static uint8_t i2c_write_byte(uint8_t b)
{
  /* 8 个数据位：SCL 低时建立 SDA，SCL 上升沿从机采样 SDA。 */
  for (unsigned i = 0U; i < 8U; ++i) {
    if ((b & 0x80U) != 0U) {
      SDA_H();
    } else {
      SDA_L();
    }
    i2c_delay();
    SCL_H();
    i2c_delay();
    SCL_L();
    i2c_delay();
    b = (uint8_t)(b << 1);
  }
  /* 第 9 位：主机释放 SDA，从机在 SCL 高期间拉低表示 ACK */
  SDA_H();
  i2c_delay();
  SCL_H();
  i2c_delay();
  {
    const uint32_t sda_high = GPIOB_IDR & (1U << PB_SDA_PIN);
    const uint8_t ack = (sda_high == 0U) ? 1U : 0U;
    SCL_L();
    i2c_delay();
    return ack;
  }
}

/*
 * 通过 I2C 向 SSD1306 发一串「显示控制器命令」(非显存数据)。
 * 帧格式：START → 7 位器件地址+写 → 控制字节 0x00(表示后续均为命令) → 各命令字节 → STOP。
 * 任一字节无 ACK 时发 STOP 并返回 0（符合主机收到 NACK 后结束总线）。
 */
static uint8_t oled_send_commands(const uint8_t *cmds, size_t n)
{
  if (n == 0U) {
    return 1U;
  }
  i2c_start();
  if (i2c_write_byte(OLED_I2C_ADDR8_W) == 0U) {
    i2c_stop();
    return 0U;
  }
  if (i2c_write_byte(0x00U) == 0U) {
    i2c_stop();
    return 0U;
  }
  for (size_t i = 0U; i < n; ++i) {
    if (i2c_write_byte(cmds[i]) == 0U) {
      i2c_stop();
      return 0U;
    }
  }
  i2c_stop();
  return 1U;
}

/* 新开 I2C 写事务：地址 + 0x40 后连续写 len 字节到 GDDRAM */
static uint8_t oled_push_gddram(const uint8_t *data, size_t len)
{
  i2c_start();
  if (i2c_write_byte(OLED_I2C_ADDR8_W) == 0U) {
    i2c_stop();
    return 0U;
  }
  if (i2c_write_byte(0x40U) == 0U) {
    i2c_stop();
    return 0U;
  }
  for (size_t i = 0U; i < len; ++i) {
    if (i2c_write_byte(data[i]) == 0U) {
      i2c_stop();
      return 0U;
    }
  }
  i2c_stop();
  return 1U;
}

/*
 * 只更新 g_fb 中一页里的一段连续列（水平寻址下与 GDDRAM 顺序一致）。
 * col0：起始列；page：页号 0～7；ncol：列数（通常 6，含字间距列）。
 */
static void ssd1306_oled_refresh_region(unsigned col0, unsigned page, unsigned ncol)
{
  if (ncol == 0U || page >= SSD1306_PAGE_COUNT) {
    return;
  }
  if (col0 >= SSD1306_WIDTH) {
    return;
  }
  if (col0 + ncol > SSD1306_WIDTH) {
    ncol = SSD1306_WIDTH - col0;
  }
  {
    const uint8_t col_end = (uint8_t)(col0 + ncol - 1U);
    const uint8_t pg = (uint8_t)page;
    const uint8_t setwin[] = {
        0x21U, (uint8_t)col0, col_end, 0x22U, pg, pg,
    };
    if (oled_send_commands(setwin, sizeof setwin) == 0U) {
      return;
    }
  }
  (void)oled_push_gddram(&g_fb[page * SSD1306_WIDTH + col0], ncol);
}

/*
 * 软滚动一行「字符页」：显存整体上移 8 个像素（一页），底部新的一页填 0。
 * 同时若文本光标在下方，把 g_row_page 减 1，使逻辑行号与视觉上移后的内容对齐。
 */
static void scroll_up_one_page(void)
{
  (void)memmove(&g_fb[0], &g_fb[SSD1306_WIDTH], FB_SIZE - SSD1306_WIDTH);
  (void)memset(&g_fb[FB_SIZE - SSD1306_WIDTH], 0, SSD1306_WIDTH);
  if (g_row_page > 0U) {
    g_row_page--;
  }
}

/* 若当前字符行 g_row_page 已超出屏内 8 页，则反复向上滚，直到可见。返回 1 表示发生过滚屏。 */
static uint8_t ensure_row_visible(void)
{
  uint8_t scrolled = 0U;
  while (g_row_page >= SSD1306_PAGE_COUNT) {
    scroll_up_one_page();
    scrolled = 1U;
  }
  return scrolled;
}

void ssd1306_oled_init(void)
{
  /* RCC：IOPB 由 bsp_board_init() 统一打开 */

  /* PB8/PB9：CNF=01 通用开漏，MODE=10 输出 2MHz；配合模块上拉作 I2C */
  GPIOB_CRH = (GPIOB_CRH & 0xFFFFFF00UL) | 0x00000066UL;
  /* 开漏空闲：输出寄存器置 1 释放总线，由上拉拉高 */
  GPIOB_ODR |= (1U << PB_SCL_PIN) | (1U << PB_SDA_PIN);

  g_col_px = 0U;
  g_row_page = 0U;
  (void)memset(g_fb, 0, sizeof g_fb);

  /*
   * SSD1306 上电初始化命令序列（成对出现的为「命令码 + 参数」）：
   * 0xAE       关显示
   * 0xD5 0x80  时钟分频/振荡频率
   * 0xA8 0x3F  多路复用率 64 行（0x3F=63 → 64 COM）
   * 0xD3 0x00  显示垂直偏移 0
   * 0x40       起始行 0
   * 0x8D 0x14  电荷泵使能（模块内部升压，必开否则很多屏不亮）
   * 0x20 0x00  显存寻址：水平地址递增（写满一行列自动进下一页）
   * 0xA1       段重映射（列方向，配合屏焊接方向）
   * 0xC8       COM 扫描方向
   * 0xDA 0x12  COM 硬件引脚配置（常见 12864 用 0x12）
   * 0x81 0xCF  对比度（亮度）
   * 0xD9 0xF1  预充电周期
   * 0xDB 0x40  VCOM 取消选择电平
   * 0xA4       正常显示（RAM 内容即显示内容）
   * 0xA6       非反色（白像素=亮）
   * 0xAF       开显示
   */
  static const uint8_t init_cmds[] = {
      0xAEU, 0xD5U, 0x80U, 0xA8U, 0x3FU, 0xD3U, 0x00U, 0x40U, 0x8DU, 0x14U,
      0x20U, 0x00U, 0xA1U, 0xC8U, 0xDAU, 0x12U, 0x81U, 0xCFU, 0xD9U, 0xF1U,
      0xDBU, 0x40U, 0xA4U, 0xA6U, 0xAFU,
  };
  (void)oled_send_commands(init_cmds, sizeof init_cmds);
  ssd1306_oled_refresh();
}

void ssd1306_oled_clear(void)
{
  (void)memset(g_fb, 0, sizeof g_fb);
  g_col_px = 0U;
  g_row_page = 0U;
  ssd1306_oled_refresh();
}

void ssd1306_oled_cursor_home(void)
{
  g_col_px = 0U;
  g_row_page = 0U;
}

/*
 * 在「当前文本行、当前列」画一个字符到 g_fb，并刷新 OLED。
 * - '\\r'：忽略。
 * - '\\n'：列回到 0，字符行进到下一页；若发生滚屏则全屏 refresh，否则不改屏（g_fb 未变）。
 * - 其它：写字模后仅把该字符占用的 6 列推到 GDDRAM（滚屏后须全屏 refresh）。
 */
void ssd1306_oled_putc(uint8_t c)
{
  if (c == '\r') {
    return;
  }
  if (c == '\n') {
    g_col_px = 0U;
    g_row_page++;
    if (ensure_row_visible() != 0U) {
      ssd1306_oled_refresh();
    }
    return;
  }

  uint8_t need_full = 0U;

  /* 当前行剩余宽度放不下 6 像素宽字符(5+间距)则换行 */
  if (g_col_px + 6U > SSD1306_WIDTH) {
    g_col_px = 0U;
    g_row_page++;
    need_full |= ensure_row_visible();
  }

  {
    const unsigned col0 = g_col_px;
    const unsigned row_pg = g_row_page;
    const unsigned ch = (unsigned)c;
    const uint8_t *glyph = &oled_font5x7[ch * 5U];
    const unsigned base = row_pg * SSD1306_WIDTH + col0;

    for (unsigned i = 0U; i < 5U; ++i) {
      if ((col0 + i) < SSD1306_WIDTH) {
        g_fb[base + i] = glyph[i];
      }
    }
    if ((col0 + 5U) < SSD1306_WIDTH) {
      g_fb[base + 5U] = 0U;
    }
    g_col_px += 6U;

    if (need_full != 0U) {
      ssd1306_oled_refresh();
    } else {
      ssd1306_oled_refresh_region(col0, row_pg, 6U);
    }
  }
}

/*
 * 把整块 g_fb 同步到 OLED 的 GDDRAM（真正点亮/刷新像素）。
 * 1) 发命令 0x21/列起止、0x22/页起止，设写入窗口为全屏 128×8 页。
 * 2) 再发一次 I2C 写：控制字节 0x40 表示后续全是「显存数据」。
 * 3) 按字节顺序写满 FB_SIZE，与水平寻址下 GDDRAM 线性顺序一致。
 */
void ssd1306_oled_refresh(void)
{
  /* 列 0～127，页 0～7（整屏） */
  static const uint8_t setwin[] = {0x21U, 0x00U, 0x7FU, 0x22U, 0x00U, 0x07U};
  if (oled_send_commands(setwin, sizeof setwin) == 0U) {
    return;
  }
  (void)oled_push_gddram(g_fb, FB_SIZE);
}
