.syntax unified
.cpu cortex-m3
.thumb

.global _estack
.global Reset_Handler

.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss

.section .isr_vector, "a", %progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors
g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word 0
  .word 0
  .word 0
  .word 0
  .word Default_Handler
  .word Default_Handler
  .word 0
  .word Default_Handler
  .word SysTick_Handler

.section .text.Reset_Handler, "ax", %progbits
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
1:
  cmp r0, r1
  bcc 2f
  b 3f
2:
  ldr r3, [r2], #4
  str r3, [r0], #4
  b 1b

3:
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
4:
  cmp r0, r1
  bcc 5f
  b 6f
5:
  str r2, [r0], #4
  b 4b

6:
  bl main

7:
  b 7b

.size Reset_Handler, .-Reset_Handler

.section .text.Default_Handler, "ax", %progbits
.type Default_Handler, %function
Default_Handler:
  b .
.size Default_Handler, .-Default_Handler

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
