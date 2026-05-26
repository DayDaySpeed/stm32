/*
 * 状态码枚举 → 可读字符串，供日志与故障打印使用。
 */
#include "common/stm_status.h"

const char *stm_status_name(stm_status_t status) {
  switch (status) {
  case STM_OK:
    return "OK";
  case STM_ERR_INVALID_ARG:
    return "INVALID_ARG";
  case STM_ERR_BUSY:
    return "BUSY";
  case STM_ERR_TIMEOUT:
    return "TIMEOUT";
  case STM_ERR_NACK:
    return "NACK";
  case STM_ERR_OVERFLOW:
    return "OVERFLOW";
  case STM_ERR_IO:
    return "IO";
  case STM_ERR_NOT_INITIALIZED:
    return "NOT_INITIALIZED";
  default:
    return "UNKNOWN";
  }
}
