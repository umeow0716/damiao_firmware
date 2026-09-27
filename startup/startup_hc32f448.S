/*
 * Source-owned HC32F448 startup.
 *
 * The vendor startup aliases every unimplemented exception and interrupt via
 * weak symbols.  That is convenient for examples, but it hides missing board
 * work in a reconstructed product.  This vector table contains no weak
 * symbols: the five recovered application IRQs are explicit and every other
 * vector has an equally explicit fail-stop destination.
 */

    .syntax unified
    .arch armv7e-m
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb

    .section .vectors, "a", %progbits
    .align 2
    .global __Vectors
    .global __Vectors_End
    .global __Vectors_Size
    .type __Vectors, %object
__Vectors:
    .long __StackTop
    .long Reset_Handler
    .long Default_Handler             /* NMI */
    .long Default_Handler             /* HardFault */
    .long Default_Handler             /* MemManage */
    .long Default_Handler             /* BusFault */
    .long Default_Handler             /* UsageFault */
    .long 0
    .long 0
    .long 0
    .long 0
    .long Default_Handler             /* SVC */
    .long Default_Handler             /* DebugMon */
    .long 0
    .long Default_Handler             /* PendSV */
    .long Default_Handler             /* SysTick */

    .long IRQ000_Handler              /* TMR4 position trigger */
    .long IRQ001_Handler              /* DMA2 position sample */
    .long IRQ002_Handler              /* synchronous ADC / FOC */
    .long IRQ003_Handler              /* MCAN1 receive */
    .long IRQ004_Handler              /* USART1 receive */
    /* Match the HC32F448 vector layout used by the official APP.  IRQ123 and
     * IRQ124 are reserved vector slots and therefore must remain zero. */
    .rept 118                         /* IRQ005..IRQ122 */
    .long Default_Handler
    .endr
    .long 0                           /* IRQ123: reserved */
    .long 0                           /* IRQ124: reserved */
    .rept 3                           /* IRQ125..IRQ127 */
    .long Default_Handler
    .endr

__Vectors_End:
    .equ __Vectors_Size, __Vectors_End - __Vectors
    .size __Vectors, . - __Vectors

    .section .text.Reset_Handler, "ax", %progbits
    .align 2
    .global Reset_Handler
    .type Reset_Handler, %function
    .thumb_func
Reset_Handler:
    /* Preserve the incoming PRIMASK exactly as the official Reset_Handler
     * does.  The installed loader hands off with interrupts masked; a cold
     * reset starts with every NVIC line disabled.  main re-enables globally
     * at the recovered point after the boot-record writes. */

    /* The official Reset_Handler calls SystemInit before entering Keil's
     * __main/scatter-loader.  This source-owned handler keeps that order and
     * performs the equivalent GCC data/BSS/constructor work below. */
    bl SystemInit

    /* Clear HC32 SRAM parity/status latches before touching RAM. */
    ldr r0, =0x40050810
    ldr r1, =0x1f8
    str r1, [r0]

    /* Copy the normal RAM data image. */
    ldr r1, =__etext
    ldr r2, =__data_start__
    ldr r3, =__data_end__
1:
    cmp r2, r3
    bcs 2f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 1b
2:
    /* Copy RAMB-resident data. */
    ldr r1, =__etext_ramb
    ldr r2, =__data_start_ramb__
    ldr r3, =__data_end_ramb__
3:
    cmp r2, r3
    bcs 4f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 3b
4:
    /* Clear both BSS regions. */
    movs r0, #0
    ldr r1, =__bss_start__
    ldr r2, =__bss_end__
5:
    cmp r1, r2
    bcs 6f
    str r0, [r1], #4
    b 5b
6:
    ldr r1, =__bss_start_ramb__
    ldr r2, =__bss_end_ramb__
7:
    cmp r1, r2
    bcs 8f
    str r0, [r1], #4
    b 7b
8:
    /* The firmware owns Reset_Handler, so do not pull in newlib's crt0 just
     * to walk the constructor arrays.  Keeping the walks here preserves
     * support for future C/C++ static initializers with -nostartfiles. */
    ldr r4, =__preinit_array_start
    ldr r5, =__preinit_array_end
10:
    cmp r4, r5
    bcs 11f
    ldr r0, [r4], #4
    blx r0
    b 10b
11:
    ldr r4, =__init_array_start
    ldr r5, =__init_array_end
12:
    cmp r4, r5
    bcs 13f
    ldr r0, [r4], #4
    blx r0
    b 12b
13:
    bl main
    b Default_Handler
    .size Reset_Handler, . - Reset_Handler

    .section .text.Default_Handler, "ax", %progbits
    .align 2
    .global Default_Handler
    .type Default_Handler, %function
    .thumb_func
Default_Handler:
    cpsid i
9:
    b 9b
    .size Default_Handler, . - Default_Handler

    .section .note.GNU-stack, "", %progbits
