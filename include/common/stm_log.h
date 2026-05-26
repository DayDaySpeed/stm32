#ifndef COMMON_STM_LOG_H
#define COMMON_STM_LOG_H

#include "common/stm_status.h"

/*
 * 可注入的轻量日志；writer 通常为 usart1_write_string_blocking。
 */

typedef stm_status_t (*stm_log_write_fn)(const char *text);

/* writer：日志输出函数；NULL 关闭日志。 */
void stm_log_set_writer(stm_log_write_fn writer);
stm_status_t stm_log_write(const char *text);
/* 输出 "module: status\\r\\n"。 */
stm_status_t stm_log_write_status(const char *module, stm_status_t status);

#if STM_DEBUG_LOG
#define STM_LOG_TEXT(text) ((void)stm_log_write(text))
#define STM_LOG_STATUS(module, status) ((void)stm_log_write_status((module), (status)))
#else
#define STM_LOG_TEXT(text) ((void)0)
#define STM_LOG_STATUS(module, status) ((void)0)
#endif

#endif
