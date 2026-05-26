#ifndef COMMON_STM_ASSERT_H
#define COMMON_STM_ASSERT_H

#include "common/stm_status.h"

/*
 * 编译期断言宏：条件失败时调用 stm_fault_halt 停机。
 * STM_ASSERT_LEVEL=0 可完全关闭（Release 可选）。
 */

__attribute__((noreturn)) void stm_fault_halt(const char *module,
                                              stm_status_t status);

#ifndef STM_ASSERT_LEVEL
#define STM_ASSERT_LEVEL 1
#endif

#if STM_ASSERT_LEVEL > 0
#define STM_ASSERT(expr, module)                                                \
  do {                                                                          \
    if (!(expr)) {                                                              \
      stm_fault_halt((module), STM_ERR_INVALID_ARG);                            \
    }                                                                           \
  } while (0)

#define STM_ASSERT_OK(status, module)                                           \
  do {                                                                          \
    stm_status_t s_assert_status_ = (status);                                   \
    if (s_assert_status_ != STM_OK) {                                           \
      stm_fault_halt((module), s_assert_status_);                               \
    }                                                                           \
  } while (0)
#else
#define STM_ASSERT(expr, module) ((void)0)
#define STM_ASSERT_OK(status, module) ((void)(status))
#endif

#endif
