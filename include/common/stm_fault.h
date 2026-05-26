#ifndef COMMON_STM_FAULT_H
#define COMMON_STM_FAULT_H

#include "common/stm_status.h"

/*
 * 致命错误处理：记录模块名与状态码，可选串口打印后停机。
 * 用于 main/app_init 等无法恢复的路径。
 */

void stm_fault_record(const char *module, stm_status_t status);

__attribute__((noreturn)) void stm_fault_halt(const char *module,
                                              stm_status_t status);

#endif
