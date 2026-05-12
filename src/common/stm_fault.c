#include "common/stm_fault.h"

#include <stddef.h>

#include "common/stm_log.h"

static const char *g_fault_module = "none";
static stm_status_t g_fault_status = STM_OK;

void stm_fault_record(const char *module, stm_status_t status) {
  g_fault_module = (module != NULL) ? module : "unknown";
  g_fault_status = status;
}

const char *stm_fault_last_module(void) { return g_fault_module; }

stm_status_t stm_fault_last_status(void) { return g_fault_status; }

__attribute__((noreturn)) void stm_fault_halt(const char *module,
                                              stm_status_t status) {
  stm_fault_record(module, status);
  (void)stm_log_write("FAULT ");
  (void)stm_log_write_status(g_fault_module, g_fault_status);
  while (1) {
  }
}
