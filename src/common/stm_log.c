/*
 * 轻量日志：通过注册的 writer（通常为串口）输出诊断文本。
 */
#include "common/stm_log.h"

#include <stddef.h>

static stm_log_write_fn g_stm_log_writer;

/* writer：日志输出回调；NULL 关闭日志。 */
void stm_log_set_writer(stm_log_write_fn writer) { g_stm_log_writer = writer; }

/* text：以 '\\0' 结尾字符串。 */
stm_status_t stm_log_write(const char *text) {
  if (text == NULL) {
    return STM_ERR_INVALID_ARG;
  }
  if (g_stm_log_writer == NULL) {
    return STM_ERR_NOT_INITIALIZED;
  }
  return g_stm_log_writer(text);
}

/* module/status：输出 "module: STATUS\\r\\n"。 */
stm_status_t stm_log_write_status(const char *module, stm_status_t status) {
  stm_status_t st = STM_OK;

  if (module == NULL) {
    return STM_ERR_INVALID_ARG;
  }

  st = stm_log_write(module);
  if (st != STM_OK) {
    return st;
  }
  st = stm_log_write(": ");
  if (st != STM_OK) {
    return st;
  }
  st = stm_log_write(stm_status_name(status));
  if (st != STM_OK) {
    return st;
  }
  return stm_log_write("\r\n");
}
