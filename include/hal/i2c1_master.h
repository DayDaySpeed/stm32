#ifndef HAL_I2C1_MASTER_H
#define HAL_I2C1_MASTER_H

#include <stddef.h>
#include <stdint.h>

#include "common/stm_status.h"

/*
 * I2C1 主机初始化配置。
 *
 * pclk1_hz      : I2C1 挂在 APB1，传入当前 PCLK1 实际频率（建议用
 *                 bsp_clock_get_pclk1_hz() 获取）。CR2.FREQ、CCR、TRISE
 *                 都依赖这个值计算，传错会导致时钟拍子不对、设备不识别。
 *                 必须 ≥ 2MHz（标准模式硬性要求）。
 *
 * bus_hz        : I2C 总线目标时钟频率。常见取值：
 *                   - 100000  : 标准模式 100kHz（最稳，SSD1306/MPU6050 等都吃）
 *                   - 400000  : 快速模式 400kHz（需要外设支持 + 上拉电阻够强）
 *                 本驱动当前仅做了标准模式时序，400kHz 需要额外配 CCR.FS。
 *
 * timeout_iter  : 轮询超时上限（循环次数，不是毫秒）。
 *                   - 传 0     : 退到内部默认 3000（72MHz 下单次等待约 1ms）
 *                   - 传 ≥1   : 直接生效；EEPROM 等慢设备可酌情调大
 *                 超时/总线忙时会自动做 9 脉冲恢复，避免长时间空转。
 */
typedef struct {
  uint32_t pclk1_hz;
  uint32_t bus_hz;
  uint32_t timeout_iter;
} i2c1_master_config_t;

/*
 * 初始化 I2C1 主机：
 *   - 把 I2C1 重映射到 PB8(SCL)/PB9(SDA)，配复用开漏输出 50MHz；
 *   - 按 (pclk1_hz, bus_hz) 计算 FREQ/CCR/TRISE 并写入；
 *   - 使能 I2C1（CR1.PE=1）。
 *
 * 前置条件：调用方应已使能 RCC.AFIO/IOPB/I2C1 时钟（本工程在 bsp_board_init()）。
 *
 * 返回：
 *   STM_OK              : 成功
 *   STM_ERR_INVALID_ARG : cfg=NULL 或参数超界
 */
stm_status_t i2c1_master_init(const i2c1_master_config_t *cfg);

/*
 * 主机写一帧：
 *   START → 写 7 位地址(W) → 写 ctrl 字节 → 写 payload[0..n-1] → STOP
 *
 * 参数：
 *   addr7       : 7 位从机地址（低位会被自动左移加写位）
 *   ctrl        : 控制字节（SSD1306 用 0x00=命令/0x40=数据；其它器件按数据手册）
 *   payload     : 数据指针；payload_len=0 时可传 NULL
 *   payload_len : 数据字节数
 *
 * 返回：
 *   STM_OK              : 整帧发送完成
 *   STM_ERR_INVALID_ARG : payload=NULL 但 payload_len>0
 *   STM_ERR_BUSY        : 起始前总线一直 BUSY，无法发 START
 *   STM_ERR_NACK        : 从机在任意阶段返回 NACK（设备没接、地址错）
 *   STM_ERR_TIMEOUT     : 某阶段事件位等待超过 timeout_iter，调用方应重试或上报
 *
 * 所有错误路径都会自动发 STOP，保证总线不挂死。
 */
stm_status_t i2c1_master_write_frame(uint8_t addr7, uint8_t ctrl,
                                     const uint8_t *payload,
                                     size_t payload_len);

#endif
