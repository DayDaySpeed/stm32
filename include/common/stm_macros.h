#ifndef COMMON_STM_MACROS_H
#define COMMON_STM_MACROS_H

#include <stdint.h>

#define STM_ARRAY_SIZE(a) ((uint32_t)(sizeof(a) / sizeof((a)[0])))
#define STM_MIN(a, b) ((a) < (b) ? (a) : (b))

#ifndef STM_DEBUG_LOG
#define STM_DEBUG_LOG 0
#endif

#endif
