.syntax unified
.cpu cortex-m3
.thumb

/* 导出符号：
 * _estack       在链接脚本中定义的栈顶地址
 * Reset_Handler 复位后 CPU 跳转的第一段代码
 */
.global _estack
.global Reset_Handler

/* 这些边界符号由链接脚本提供，供复位阶段初始化内存使用 */
.word _sidata   /* data 段在 Flash 中的起始地址（源） */
.word _sdata    /* data 段在 RAM 中的起始地址（目标） */
.word _edata    /* data 段在 RAM 中的结束地址 */
.word _sbss     /* bss 段在 RAM 中的起始地址 */
.word _ebss     /* bss 段在 RAM 中的结束地址 */

/* =========================
 * 中断向量表（放在 Flash 起始）
 * ========================= */
.section .isr_vector, "a", %progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors
g_pfnVectors:
  .word _estack             /* [0] 初始 MSP 栈顶值 */
  .word Reset_Handler       /* [1] 复位入口 */
  .word Default_Handler     /* [2] NMI */
  .word Default_Handler     /* [3] HardFault */
  .word Default_Handler     /* [4] MemManage */
  .word Default_Handler     /* [5] BusFault */
  .word Default_Handler     /* [6] UsageFault */
  .word 0                   /* [7] 保留 */
  .word 0                   /* [8] 保留 */
  .word 0                   /* [9] 保留 */
  .word 0                   /* [10] 保留 */
  .word Default_Handler     /* [11] SVC */
  .word Default_Handler     /* [12] DebugMon */
  .word 0                   /* [13] 保留 */
  .word Default_Handler     /* [14] PendSV */
  .word SysTick_Handler     /* [15] SysTick */
  .word Default_Handler     /* [16] IRQ0 */
  .word Default_Handler     /* [17] IRQ1 */
  .word Default_Handler     /* [18] IRQ2 */
  .word Default_Handler     /* [19] IRQ3 */
  .word Default_Handler     /* [20] IRQ4 */
  .word Default_Handler     /* [21] IRQ5 */
  .word Default_Handler     /* [22] IRQ6 */
  .word Default_Handler     /* [23] IRQ7 */
  .word Default_Handler     /* [24] IRQ8 */
  .word Default_Handler     /* [25] IRQ9 */
  .word Default_Handler     /* [26] IRQ10 */
  .word Default_Handler     /* [27] IRQ11 */
  .word Default_Handler     /* [28] IRQ12 */
  .word Default_Handler     /* [29] IRQ13 */
  .word Default_Handler     /* [30] IRQ14 */
  .word Default_Handler     /* [31] IRQ15 */
  .word Default_Handler     /* [32] IRQ16 */
  .word Default_Handler     /* [33] IRQ17 */
  .word Default_Handler     /* [34] IRQ18 */
  .word Default_Handler     /* [35] IRQ19 */
  .word Default_Handler     /* [36] IRQ20 */
  .word Default_Handler     /* [37] IRQ21 */
  .word Default_Handler     /* [38] IRQ22 */
  .word Default_Handler     /* [39] IRQ23 */
  .word Default_Handler     /* [40] IRQ24 */
  .word Default_Handler     /* [41] IRQ25 */
  .word Default_Handler     /* [42] IRQ26 */
  .word Default_Handler     /* [43] IRQ27 */
  .word Default_Handler     /* [44] IRQ28 */
  .word Default_Handler     /* [45] IRQ29 */
  .word Default_Handler     /* [46] IRQ30 */
  .word Default_Handler     /* [47] IRQ31 */
  .word Default_Handler     /* [48] IRQ32 */
  .word Default_Handler     /* [49] IRQ33 */
  .word Default_Handler     /* [50] IRQ34 */
  .word Default_Handler     /* [51] IRQ35 */
  .word Default_Handler     /* [52] IRQ36 */
  .word USART1_IRQHandler   /* [53] IRQ37 = USART1 */

/* =========================
 * 复位处理函数
 * 1) 拷贝 data 段到 RAM
 * 2) 清零 bss 段
 * 3) 调用 main
 * ========================= */
.section .text.Reset_Handler, "ax", %progbits
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
  ldr r0, =_sdata           /* r0 = data 目标起始（RAM） */
  ldr r1, =_edata           /* r1 = data 目标结束（RAM） */
  ldr r2, =_sidata          /* r2 = data 源起始（Flash） */
1:
  cmp r0, r1                /* 若 r0 < r1 继续拷贝 */
  bcc 2f
  b 3f                      /* 否则 data 拷贝完成 */
2:
  ldr r3, [r2], #4          /* 读取 Flash 一个字 */
  str r3, [r0], #4          /* 写入 RAM 一个字 */
  b 1b

3:
  ldr r0, =_sbss            /* r0 = bss 起始（RAM） */
  ldr r1, =_ebss            /* r1 = bss 结束（RAM） */
  movs r2, #0               /* r2 = 0，用于清零 */
4:
  cmp r0, r1                /* 若 r0 < r1 继续清零 */
  bcc 5f
  b 6f                      /* 否则 bss 清零完成 */
5:
  str r2, [r0], #4          /* 逐字写 0 */
  b 4b

6:
  bl main                   /* 进入 C 程序入口 */

7:
  b 7b                      /* main 若返回，进入死循环 */

.size Reset_Handler, .-Reset_Handler

/* 默认中断处理：所有未实现中断都会卡在这里 */
.section .text.Default_Handler, "ax", %progbits
.type Default_Handler, %function
Default_Handler:
  b .
.size Default_Handler, .-Default_Handler

/* weak + thumb_set 机制：
 * 若 C 文件中提供了同名强符号（如 SysTick_Handler），
 * 链接时会覆盖这里的默认别名；否则使用 Default_Handler。
 */
.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler
.weak HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler
.weak MemManage_Handler
.thumb_set MemManage_Handler, Default_Handler
.weak BusFault_Handler
.thumb_set BusFault_Handler, Default_Handler
.weak UsageFault_Handler
.thumb_set UsageFault_Handler, Default_Handler
.weak SVC_Handler
.thumb_set SVC_Handler, Default_Handler
.weak DebugMon_Handler
.thumb_set DebugMon_Handler, Default_Handler
.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler
.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler
.weak USART1_IRQHandler
.thumb_set USART1_IRQHandler, Default_Handler
