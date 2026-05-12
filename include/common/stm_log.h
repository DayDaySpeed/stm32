#ifndef COMMON_STM_LOG_H
#define COMMON_STM_LOG_H

#include "common/stm_status.h"

typedef stm_status_t (*stm_log_write_fn)(const char *text);

/* 注册一条轻量日志输出路径（例如串口控制台）。传 NULL 可关闭日志。 */
void stm_log_set_writer(stm_log_write_fn writer);

/* 原样输出字符串；若未注册 writer，则返回 NOT_INITIALIZED。 */
stm_status_t stm_log_write(const char *text);

/* 输出 "module: status\\r\\n" 这种简短诊断信息。 */
stm_status_t stm_log_write_status(const char *module, stm_status_t status);

#if STM_DEBUG_LOG
#define STM_LOG_TEXT(text) ((void)stm_log_write(text))
#define STM_LOG_STATUS(module, status) ((void)stm_log_write_status((module), (status)))
#else
#define STM_LOG_TEXT(text) ((void)0)
#define STM_LOG_STATUS(module, status) ((void)0)
#endif

#endif
