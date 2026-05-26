#ifndef COMMON_STM_STATUS_H
#define COMMON_STM_STATUS_H

/*
 * 通用驱动状态码
 *
 * 语义约定（用于调用方区分错误类型）：
 *   OK              : 操作成功
 *   INVALID_ARG     : 入参不合法（值超出范围、空指针等）
 *   BUSY            : 资源被其他事务占用，可重试
 *   TIMEOUT         : 等待硬件状态变化超时
 *   NACK            : 通信协议层应答失败（如 I2C NACK）
 *   OVERFLOW        : 缓冲区/计数器溢出
 *   IO              : 底层 I/O 操作失败（例如总线写入失败）
 *   NOT_INITIALIZED : 调用了 set/get 类 API，但模块尚未 init（或已 stop）
 *
 * 数值连续从 0 开始，便于打印日志或当作数组下标做 lookup。
 */
typedef enum {
  STM_OK = 0,
  STM_ERR_INVALID_ARG = 1,
  STM_ERR_BUSY = 2,
  STM_ERR_TIMEOUT = 3,
  STM_ERR_NACK = 4,
  STM_ERR_OVERFLOW = 5,
  STM_ERR_IO = 6,
  STM_ERR_NOT_INITIALIZED = 7,
} stm_status_t;

/* status：状态码；返回可读英文字符串，未知码返回 "UNKNOWN"。 */
const char *stm_status_name(stm_status_t status);

#endif
