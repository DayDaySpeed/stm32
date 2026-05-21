# SysTick 驱动（`systick`）

## 作用

提供全系统 **1ms 毫秒节拍**：合作式任务调度、非阻塞超时、阻塞延时的时间基准。不依赖 HAL，直接配置 Cortex-M3 内核 SysTick 寄存器。

## 硬件与依赖

| 项目 | 说明 |
|------|------|
| 外设 | ARM Cortex-M3 SysTick（非 STM32 外设） |
| 时钟源 | HCLK（由 `bsp_clock_apply_profile()` 配置） |
| 重装载 | `BSP_SYSTICK_RELOAD_1MS = HCLK/1000 - 1` |
| 中断 | `SysTick_Handler` 中须调用 `systick_on_interrupt()` |

**前置条件**：先完成 `bsp_clock_apply_profile()`，再 `systick_init_1ms()`。

## API 参考

| 函数 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `systick_init_1ms()` | 无 | void | 配置 1ms 重装载并启动 SysTick |
| `systick_on_interrupt()` | 无 | void | 中断里递增毫秒计数（**必须**在 `SysTick_Handler` 调用） |
| `systick_get_ms()` | 无 | `uint32_t` | 自启动以来经过的毫秒数 |
| `systick_delay_ms(ms)` | 延时毫秒 | void | 忙等延时，基于 `g_ms_ticks` 差值 |

## 实现说明

### 怎么写的

1. 静态变量 `g_ms_ticks` 在中断中递增。
2. `systick_init_1ms()` 写 `SYST_RVR/CVR/CSR`：时钟源 HCLK、使能 TICKINT 与 ENABLE。
3. `systick_delay_ms` 用 `(g_ms_ticks - start) < ms` 轮询，无 WFI。

### 为什么这么写

- **不用 `stm_status_t`**：初始化参数来自宏，几乎不会失败；保持 API 极简，适合最底层时间服务。
- **忙等而非 WFI**：裸机合作式主循环里，延时期间其它任务本就不会跑；实现简单、行为可预测。
- **32 位无符号差值**：`(now - start) < ms` 在计数回绕时仍正确（约 49 天回绕一次）。

## 使用示例

```c
systick_init_1ms();

uint32_t t0 = systick_get_ms();
while ((systick_get_ms() - t0) < 100U) {
  /* 其它非阻塞任务 */
}
```

`startup_stm32f103c8tx.s` 或弱符号里：

```c
void SysTick_Handler(void) {
  systick_on_interrupt();
}
```

## 常见坑

- 忘记在中断里调 `systick_on_interrupt()` → `get_ms` 永远为 0。
- 在 SysTick 初始化前调用 `delay_ms` → 可能死等。
- 高优先级中断长时间占用 CPU → 毫秒计数会「变慢」（所有基于 ms 的周期任务都会漂移）。
