/*
 * SSD1306 OLED 驱动（128×64，I2C）
 * ---------------------------------------------------------------------------
 * 在干什么：
 *   用片上 I2C1 硬件主机（AFIO 重映射：PB8=SCL、PB9=SDA）访问 SSD1306；屏幕内容
 *   先写在单片机里的数组 g_fb（帧缓冲），再通过 I2C 写到屏的 GDDRAM。
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
 *   ssd1306_oled_init()     → 配 I2C1 重映射与 GPIO、发初始化命令、清屏并 refresh（须先 bsp_board_init）。
 *   ssd1306_oled_putc()     → 写 g_fb，并尽量只 I2C 更新 6 列×当前页（滚屏后则全屏 refresh）。
 *   ssd1306_oled_refresh()  → 全屏同步 g_fb → GDDRAM（清屏等可显式调用）。
 */
#include "drivers/ssd1306_oled.h"

#include "bsp/clock.h"
#include "bsp/stm32f103_regs.h"

#include <stddef.h>
#include <string.h>

/* SSD1306 的 7 位 I2C 地址（手册/丝印常见 0x3C；另一接法为 0x3D） */
#define OLED_I2C_ADDR7      (0x3CU)
#define SSD1306_WIDTH       (128U)  /* 横向像素数 */
#define SSD1306_PAGE_COUNT  (8U)    /* 纵向 64/8 = 8 页 */
#define FB_SIZE             (SSD1306_WIDTH * SSD1306_PAGE_COUNT) /* 128*8=1024 字节 */

/* I2C1：标准模式 100kHz；CR2.FREQ = PCLK1(MHz)。CCR = PCLK1 / (2*100kHz) */
#define I2C1_SM_CCR         ((uint16_t)(BSP_PCLK1_HZ / 200000UL))
#define I2C1_CR2_FREQ_MHZ  ((uint8_t)(BSP_PCLK1_HZ / 1000000UL))
/* Sm 模式最大上升时间 1000ns：TRISE ≈ ceil(1µs / T_PCLK1)+1，简化为 MHz+1 */
#define I2C1_TRISE_VAL      ((uint8_t)(I2C1_CR2_FREQ_MHZ + 1U))

#define I2C1_TIMEOUT_ITER   (100000UL)

extern const uint8_t oled_font5x7[]; /* 256 字符 × 5 字节/字，见 oled_font5x7.c */

/* 帧缓冲：与 SSD1306 GDDRAM 同布局，refresh 时整包写出 */
static uint8_t g_fb[FB_SIZE];
/* 下一个字符左上角所在列(0～127)、所在页(0～7) */
static unsigned g_col_px;
static unsigned g_row_page;

static void i2c1_gpio_remap_init(void)
{
  AFIO_MAPR |= AFIO_MAPR_I2C1_REMAP_BIT;
  /* PB8/PB9：复用开漏(CNF/MODE:1111)（AF_OD），与 I2C1 重映射脚一致 */
  GPIOB_CRH = (GPIOB_CRH & 0xFFFFFF00UL) | 0x000000FFUL;
}

/*
 * 按 RM 要求：改 CCR/TRISE/CR2 前应先关 PE，配好后再开 PE。
 * CR2.FREQ = PCLK1(MHz)；CCR 决定 SCL 分频；TRISE 与标准模式最大上升时间(≈1µs)对齐。
 */
static void i2c1_periph_init(void)
{
  /* 关闭外设，否则某些位可能锁死不可写 */
  I2C1_CR1 &= (uint32_t)~I2C_CR1_PE_BIT;
  /* CR2[5:0]：I2C 外设挂在 APB1 上的时钟频率，单位 MHz（与 BSP_PCLK1_HZ 一致） */
  I2C1_CR2 = (uint32_t)I2C1_CR2_FREQ_MHZ & 0x3FUL;
  /* TRISE[5:0]：SCL 边沿允许的最大上升时间（以 PCLK1 周期换算），标准模式常用 FREQ_MHz+1 */
  I2C1_TRISE = (uint32_t)I2C1_TRISE_VAL & 0x3FUL;
  /* CCR[11:0]：SCL 周期分频；标准 100kHz 时 CCR ≈ PCLK1/(2×100k)，且 bit15 F/S=0 表示标准模式 */
  I2C1_CCR = (uint32_t)I2C1_SM_CCR & 0xFFFUL;
  /* 最后再使能 I2C1，开始按上述参数工作 */
  I2C1_CR1 = I2C_CR1_PE_BIT;
}

/*
 * 等待 SR1 中某几位等于 expect；若出现 AF 则清标志并 STOP，返回 0。
 */
static uint8_t i2c1_wait_sr1(uint32_t mask, uint32_t expect)
{
  /* 轮询等待 SR1 达到目标状态；同时做超时保护，避免外设异常时死循环。 */
  for (volatile uint32_t n = 0U; n < I2C1_TIMEOUT_ITER; ++n) {
    const uint32_t sr1 = I2C1_SR1;
    /* AF=1 表示应答失败（常见于从机 NACK），立即清错并发 STOP 结束事务。 */
    if ((sr1 & I2C_SR1_AF_BIT) != 0U) {
      /* 某些状态位的清除依赖读 SR1/SR2 的顺序，先执行一次读序列。 */
      (void)I2C1_SR1;
      (void)I2C1_SR2;
      /* 清 AF 标志位，避免后续事务被遗留错误状态影响。 */
      I2C1_SR1 &= (uint32_t)~I2C_SR1_AF_BIT;
      /* 主机发 STOP，释放总线。 */
      I2C1_CR1 |= I2C_CR1_STOP_BIT;
      return 0U;
    }
    /* 当 (SR1 & mask) 等于期望值 expect，表示等到了目标事件。 */
    if ((sr1 & mask) == expect) {
      return 1U;
    }
  }
  /* 超时仍未等到目标状态：主动 STOP，防止总线一直处于占用状态。 */
  I2C1_CR1 |= I2C_CR1_STOP_BIT;
  return 0U;
}

/*  对于SSD1306
 * 单帧写：START → 7 位地址写 → 控制字节 ctrl（0x00 命令流 / 0x40 显存流）→ payload → STOP。
 */
/*
 * 发起一次 I2C1 主机写事务：
 *   START -> 7位地址+W -> 控制字节(ctrl) -> payload若干字节 -> STOP
 * 返回值：
 *   1 = 整个事务成功完成
 *   0 = 任一步骤 NACK/超时，内部会发 STOP 收尾
 */
static uint8_t i2c1_write_ctrl_then_bytes(uint8_t addr7, uint8_t ctrl, const uint8_t *payload, size_t payload_len)
{
  /* 若总线正忙（BUSY=1），先等一小段时间，避免与前一事务冲突。 */
  for (volatile uint32_t n = 0U; n < I2C1_TIMEOUT_ITER; ++n) {
    if ((I2C1_SR2 & I2C_SR2_BUSY_BIT) == 0U) {
      break;
    }
  }

  /* 1) 发送 START，等待 SB=1（EV5） */
  I2C1_CR1 |= I2C_CR1_START_BIT;
  if (i2c1_wait_sr1(I2C_SR1_SB_BIT, I2C_SR1_SB_BIT) == 0U) {
    return 0U;
  }

  /* 2) 写入从机地址（7位左移 + W=0） */
  I2C1_DR = (uint32_t)(addr7 << 1);

  /* 3) 等待地址阶段完成（ADDR=1，EV6） */
  if (i2c1_wait_sr1(I2C_SR1_ADDR_BIT, I2C_SR1_ADDR_BIT) == 0U) {
    return 0U;
  }

  /* 4) 按手册读 SR1 再读 SR2，清除 ADDR 状态，进入数据阶段 */
  (void)I2C1_SR1;
  (void)I2C1_SR2;

  /* 5) 等待 TXE=1（EV8），写控制字节（SSD1306: 0x00 命令流 / 0x40 数据流） */
  if (i2c1_wait_sr1(I2C_SR1_TXE_BIT, I2C_SR1_TXE_BIT) == 0U) {
    return 0U;
  }
  I2C1_DR = (uint32_t)ctrl;

  /* 6) 逐字节写负载：每次先等 TXE，再写 DR */
  for (size_t i = 0U; i < payload_len; ++i) {
    if (i2c1_wait_sr1(I2C_SR1_TXE_BIT, I2C_SR1_TXE_BIT) == 0U) {
      return 0U;
    }
    I2C1_DR = (uint32_t)payload[i];
  }

  /* 7) 等待 BTF=1：最后一个字节已从移位寄存器真正发出 */
  if (i2c1_wait_sr1(I2C_SR1_BTF_BIT, I2C_SR1_BTF_BIT) == 0U) {
    return 0U;
  }

  /* 8) 发 STOP 结束事务，短暂延时让 STOP 传播到总线 */
  I2C1_CR1 |= I2C_CR1_STOP_BIT;
  for (volatile uint32_t d = 0U; d < 2000U; ++d) {
    __asm volatile("" ::: "memory");
  }
  return 1U;
}

/*
 * 通过 I2C 向 SSD1306 发一串「显示控制器命令」(非显存数据)。
 * 帧格式：START → 7 位器件地址+写 → 控制字节 0x00(表示后续均为命令) → 各命令字节 → STOP。
 */
static uint8_t oled_send_commands(const uint8_t *cmds, size_t n)
{
  if (n == 0U) {
    return 1U;
  }
  return i2c1_write_ctrl_then_bytes((uint8_t)OLED_I2C_ADDR7, 0x00U, cmds, n);
}

/* 新开 I2C 写事务：地址 + 0x40 后连续写 len 字节到 GDDRAM */
static uint8_t oled_push_gddram(const uint8_t *data, size_t len)
{
  return i2c1_write_ctrl_then_bytes((uint8_t)OLED_I2C_ADDR7, 0x40U, data, len);
}

/*
 * 只更新 g_fb 中一页里的一段连续列（水平寻址下与 GDDRAM 顺序一致）。
 * col0：起始列；page：页号 0～7；ncol：列数（通常 6，含字间距列）。底层为硬件 I2C1。
 */
static void ssd1306_oled_refresh_region(unsigned int col0, unsigned int page, unsigned int ncol)
{
  /* 参数检查 */
  if (ncol == 0U || page >= SSD1306_PAGE_COUNT) {
    return;
  }
  /* 起始列不能超出屏宽 */
  if (col0 >= SSD1306_WIDTH) {
    return;
  }
  /* 如果起始列加上列数超出屏宽，则截断 */
  if (col0 + ncol > SSD1306_WIDTH) {
    ncol = SSD1306_WIDTH - col0;
  }
  {
    const uint8_t col_end = (uint8_t)(col0 + ncol - 1U);
    const uint8_t pg = (uint8_t)page;
    /* setwin = Set Window：先设列范围，再设页范围，只刷新指定矩形区域 */
    const uint8_t setwin[] = {
        0x21U, (uint8_t)col0, col_end, /* 0x21: Set Column Address, 起始列/结束列 */
        0x22U, pg, pg,                 /* 0x22: Set Page Address, 起始页/结束页（这里同一页） */
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
  /* RCC：IOPB / AFIO / I2C1 由 bsp_board_init() 统一打开 */

  /* 1) 把 I2C1 重映射到 PB8/PB9，并配置为复用开漏输出 */
  i2c1_gpio_remap_init();
  /* 2) 初始化 I2C1 外设时序参数（CR2/CCR/TRISE）并使能 PE */
  i2c1_periph_init();

  /* 3) 初始化文本光标位置：从左上角（第0列、第0页）开始输出 */
  g_col_px = 0U;
  g_row_page = 0U;
  /* 4) 清空本地帧缓冲，避免上电后显示历史脏数据 */
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
  /* SSD1306 上电初始化命令序列（按手册推荐顺序） */
  static const uint8_t init_cmds[] = {
      0xAEU, 0xD5U, 0x80U, 0xA8U, 0x3FU, 0xD3U, 0x00U, 0x40U, 0x8DU, 0x14U,
      0x20U, 0x00U, 0xA1U, 0xC8U, 0xDAU, 0x12U, 0x81U, 0xCFU, 0xD9U, 0xF1U,
      0xDBU, 0x40U, 0xA4U, 0xA6U, 0xAFU,
  };
  /* 下发初始化命令，让控制器进入可显示状态 */
  (void)oled_send_commands(init_cmds, sizeof init_cmds);
  /* 把当前帧缓冲（此时为全 0）同步到面板，确保上电后是干净黑屏 */
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

    /* 若本次过程中发生过滚屏，g_fb 大范围搬移，必须整屏刷新保证显示一致 */
    if (need_full != 0U) {
      ssd1306_oled_refresh();
    } else {
      /* 正常单字符写入时，只刷新当前字符占用的 6 列（5 列字模 + 1 列字间距） */
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
  /* 1) 把 SSD1306 的写入窗口设为整屏：列 0~127、页 0~7 */
  static const uint8_t setwin[] = {0x21U, 0x00U, 0x7FU, 0x22U, 0x00U, 0x07U};
  /* 2) 先下发窗口命令；失败则直接返回，避免继续写数据 */
  if (oled_send_commands(setwin, sizeof setwin) == 0U) {
    return;
  }
  /* 3) 再把本地帧缓冲 g_fb 的 1024 字节整包推到 GDDRAM */
  (void)oled_push_gddram(g_fb, FB_SIZE);
}
