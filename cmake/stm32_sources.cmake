# 固件源文件清单（增删模块时只改此处与对应 CMake 子目录说明）
set(STM32_ELF_SOURCES
  src/main.c
  src/app/app.c
  src/bsp/board_init.c
  src/bsp/clock.c
  src/common/ring_buffer.c
  src/hal/i2c1_master.c
  src/drivers/systick.c
  src/drivers/pwm.c
  src/drivers/usart1.c
  src/drivers/ssd1306_oled.c
  src/drivers/oled_font5x7.c
  startup/startup_stm32f103c8tx.s
)
