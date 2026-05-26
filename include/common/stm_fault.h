#ifndef COMMON_STM_FAULT_H
#define COMMON_STM_FAULT_H

#include "common/stm_status.h"

/*
 * 不可恢复错误：记录后停机循环。
 */

/* module：模块名；status：错误码。 */
void stm_fault_record(const char *module, stm_status_t status);

__attribute__((noreturn)) void stm_fault_halt(const char *module,
                                              stm_status_t status);

#endif
