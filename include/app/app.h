#ifndef APP_APP_H
#define APP_APP_H

#include "common/stm_status.h"

/*
 * 应用层公开接口。
 */

/* 初始化 SysTick、板载默认设备并打印串口欢迎语。须先完成 bsp_clock + bsp_board_init。 */
stm_status_t app_init(void);

/* 主循环：周期任务 + 串口行输入回显到 OLED；不返回。 */
void app_run_forever(void);

#endif
