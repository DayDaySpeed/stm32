#ifndef COMMON_STM_FAULT_H
#define COMMON_STM_FAULT_H

#include "common/stm_status.h"

/* 记录最后一次故障，供调试器或上层查询。 */
void stm_fault_record(const char *module, stm_status_t status);

/* 获取最后一次记录的故障信息。 */
const char *stm_fault_last_module(void);
stm_status_t stm_fault_last_status(void);

/* 记录故障并停机。 */
__attribute__((noreturn)) void stm_fault_halt(const char *module,
                                              stm_status_t status);

#endif
