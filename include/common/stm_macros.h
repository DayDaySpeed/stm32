#ifndef COMMON_STM_MACROS_H
#define COMMON_STM_MACROS_H

#include <stdint.h>

/*
 * 工程级通用宏：数组长度、调试日志开关（见 stm_log.h）。
 */

#define STM_ARRAY_SIZE(a) ((uint32_t)(sizeof(a) / sizeof((a)[0])))

#ifndef STM_DEBUG_LOG
#define STM_DEBUG_LOG 0 /* 1=启用 STM_LOG_* 宏输出 */
#endif

#endif
