#ifndef COMMON_STM_LOG_H
#define COMMON_STM_LOG_H

#include "common/stm_status.h"

/*
 * 可注入的轻量日志：默认 writer 由 bsp_console_init 注册为串口 blocking 写。
 * STM_DEBUG_LOG=0 时 STM_LOG_* 宏编译为空操作。
 */

typedef stm_status_t (*stm_log_write_fn)(const char *text);

void stm_log_set_writer(stm_log_write_fn writer);
stm_status_t stm_log_write(const char *text);
stm_status_t stm_log_write_status(const char *module, stm_status_t status);

#if STM_DEBUG_LOG
#define STM_LOG_TEXT(text) ((void)stm_log_write(text))
#define STM_LOG_STATUS(module, status) ((void)stm_log_write_status((module), (status)))
#else
#define STM_LOG_TEXT(text) ((void)0)
#define STM_LOG_STATUS(module, status) ((void)0)
#endif

#endif
