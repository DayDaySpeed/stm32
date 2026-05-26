#ifndef APP_APP_H
#define APP_APP_H

#include "common/stm_status.h"

/*
 * 应用层公开接口。
 * app_init：SysTick + 板载默认设备 + 串口欢迎语。
 * app_run_forever：周期任务 + 串口行输入回显到 OLED。
 */
stm_status_t app_init(void);
void app_run_forever(void);

#endif
