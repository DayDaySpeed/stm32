/*
 * 不可恢复错误处理：记录模块名与状态码后死循环，便于调试器定位。
 */
#include "common/stm_fault.h"

#include <stddef.h>

#include "common/stm_log.h"

static const char *g_fault_module = "none";
static stm_status_t g_fault_status = STM_OK;

void stm_fault_record(const char *module, stm_status_t status) {
  g_fault_module = (module != NULL) ? module : "unknown";
  g_fault_status = status;
}

__attribute__((noreturn)) void stm_fault_halt(const char *module,
                                              stm_status_t status) {
  stm_fault_record(module, status);
  (void)stm_log_write("FAULT ");
  (void)stm_log_write_status(g_fault_module, g_fault_status);
  while (1) {
  }
}
