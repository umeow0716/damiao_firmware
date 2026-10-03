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

#if !defined(DAMIAO_DM4310)
    /* Legacy target startup clears SRAM parity/status latches. DM4310
     * factory SystemInit/scatter performs no write to 0x40050810. */
    ldr r0, =0x40050810
    ldr r1, =0x1f8
    str r1, [r0]
#endif

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
#if defined(DAMIAO_DM4310)
    /* Copy the sparse source-built regions whose runtime addresses are fixed
     * by the factory vector table and RAM-helper ABI. */
    ldr r1, =__dm4310_helper_fault_monitor_load__
    ldr r2, =__dm4310_helper_fault_monitor_start__
    ldr r3, =__dm4310_helper_fault_monitor_end__
66:
    cmp r2, r3
    bcs 67f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 66b
67:
    ldr r1, =__dm4310_irq002_load__
    ldr r2, =__dm4310_irq002_start__
    ldr r3, =__dm4310_irq002_end__
20:
    cmp r2, r3
    bcs 21f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 20b
21:
    ldr r1, =__dm4310_boot_record_writer_load__
    ldr r2, =__dm4310_boot_record_writer_start__
    ldr r3, =__dm4310_boot_record_writer_end__
68:
    cmp r2, r3
    bcs 69f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 68b
69:
    ldr r1, =__dm4310_irq000_load__
    ldr r2, =__dm4310_irq000_start__
    ldr r3, =__dm4310_irq000_end__
22:
    cmp r2, r3
    bcs 23f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 22b
23:
    ldr r1, =__dm4310_irq001_load__
    ldr r2, =__dm4310_irq001_start__
    ldr r3, =__dm4310_irq001_end__
24:
    cmp r2, r3
    bcs 25f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 24b
25:
    ldr r1, =__dm4310_irq003_load__
    ldr r2, =__dm4310_irq003_start__
    ldr r3, =__dm4310_irq003_end__
26:
    cmp r2, r3
    bcs 27f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 26b
27:
    ldr r1, =__dm4310_flash_erase_only_load__
    ldr r2, =__dm4310_flash_erase_only_start__
    ldr r3, =__dm4310_flash_erase_only_end__
.Lcopy_flash_erase_only:
    cmp r2, r3
    bcs .Lcopy_flash_erase_only_done
    ldr r0, [r1], #4
    str r0, [r2], #4
    b .Lcopy_flash_erase_only
.Lcopy_flash_erase_only_done:
    ldr r1, =__dm4310_helper_clear_runtime_loop_states_load__
    ldr r2, =__dm4310_helper_clear_runtime_loop_states_start__
    ldr r3, =__dm4310_helper_clear_runtime_loop_states_end__
.Lcopy_clear_runtime_loop_states:
    cmp r2, r3
    bcs .Lcopy_clear_runtime_loop_states_done
    ldr r0, [r1], #4
    str r0, [r2], #4
    b .Lcopy_clear_runtime_loop_states
.Lcopy_clear_runtime_loop_states_done:
    ldr r1, =__dm4310_flash_writer_load__
    ldr r2, =__dm4310_flash_writer_start__
    ldr r3, =__dm4310_flash_writer_end__
28:
    cmp r2, r3
    bcs 29f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 28b
29:
    /* Restore callable entry points from the factory RAM-helper ABI. */
    ldr r1, =__dm4310_helper_output_sensor_load__
    ldr r2, =__dm4310_helper_output_sensor_start__
    ldr r3, =__dm4310_helper_output_sensor_end__
.Lcopy_output_sensor_helper:
    cmp r2, r3
    bcs .Lcopy_output_sensor_helper_done
    ldr r0, [r1], #4
    str r0, [r2], #4
    b .Lcopy_output_sensor_helper
.Lcopy_output_sensor_helper_done:
    ldr r1, =__dm4310_helper_mcan_send_classic_load__
    ldr r2, =__dm4310_helper_mcan_send_classic_start__
    ldr r3, =__dm4310_helper_mcan_send_classic_end__
.Lcopy_mcan_send_classic:
    cmp r2, r3
    bcs .Lcopy_mcan_send_classic_done
    ldr r0, [r1], #4
    str r0, [r2], #4
    b .Lcopy_mcan_send_classic
.Lcopy_mcan_send_classic_done:
    ldr r1, =__dm4310_helper_mcan_send_fd_load__
    ldr r2, =__dm4310_helper_mcan_send_fd_start__
    ldr r3, =__dm4310_helper_mcan_send_fd_end__
.Lcopy_mcan_send_fd:
    cmp r2, r3
    bcs .Lcopy_mcan_send_fd_done
    ldr r0, [r1], #4
    str r0, [r2], #4
    b .Lcopy_mcan_send_fd
.Lcopy_mcan_send_fd_done:
    ldr r1, =__dm4310_helper_mcan_send_variable_fd_load__
    ldr r2, =__dm4310_helper_mcan_send_variable_fd_start__
    ldr r3, =__dm4310_helper_mcan_send_variable_fd_end__
.Lcopy_mcan_send_variable_fd:
    cmp r2, r3
    bcs .Lcopy_mcan_send_variable_fd_done
    ldr r0, [r1], #4
    /* The runtime entry is only halfword aligned: STM cannot be used. */
    str r0, [r2], #4
    b .Lcopy_mcan_send_variable_fd
.Lcopy_mcan_send_variable_fd_done:
    ldr r1, =__dm4310_helper_svpwm_load__
    ldr r2, =__dm4310_helper_svpwm_start__
    ldr r3, =__dm4310_helper_svpwm_end__
.Lcopy_svpwm:
    cmp r2, r3
    bcs .Lcopy_svpwm_done
    ldr r0, [r1], #4
    str r0, [r2], #4
    b .Lcopy_svpwm
.Lcopy_svpwm_done:
    ldr r1, =__dm4310_helper_reset_control_state_load__
    ldr r2, =__dm4310_helper_reset_control_state_start__
    ldr r3, =__dm4310_helper_reset_control_state_end__
.Lcopy_reset_control_state:
    cmp r2, r3
    bcs .Lcopy_reset_control_state_done
    ldr r0, [r1], #4
    str r0, [r2], #4
    b .Lcopy_reset_control_state
.Lcopy_reset_control_state_done:
    ldr r1, =__dm4310_motion_literal_load__
    ldr r2, =__dm4310_motion_literal_start__
    ldr r0, [r1]
    str r0, [r2]
    ldr r1, =__dm4310_helper_motion_observer_load__
    ldr r2, =__dm4310_helper_motion_observer_start__
    ldr r3, =__dm4310_helper_motion_observer_end__
58:
    cmp r2, r3
    bcs 59f
    ldrb r0, [r1], #1
    strb r0, [r2], #1
    b 58b
59:
    ldr r1, =__dm4310_helper_current_controller_load__
    ldr r2, =__dm4310_helper_current_controller_start__
    ldr r3, =__dm4310_helper_current_controller_end__
60:
    cmp r2, r3
    bcs 61f
    ldrb r0, [r1], #1
    strb r0, [r2], #1
    b 60b
61:
    ldr r1, =__dm4310_filter_literal_load__
    ldr r2, =__dm4310_filter_literal_start__
    ldr r0, [r1]
    str r0, [r2]
    ldr r1, =__dm4310_helper_identification_filter_load__
    ldr r2, =__dm4310_helper_identification_filter_start__
    ldr r3, =__dm4310_helper_identification_filter_end__
30:
    cmp r2, r3
    bcs 31f
    ldrb r0, [r1], #1
    strb r0, [r2], #1
    b 30b
31:
    ldr r1, =__dm4310_helper_derive_runtime_controller_states_load__
    ldr r2, =__dm4310_helper_derive_runtime_controller_states_start__
    ldr r3, =__dm4310_helper_derive_runtime_controller_states_end__
.Lcopy_derive_runtime_controller_states:
    cmp r2, r3
    bcs .Lcopy_derive_runtime_controller_states_done
    ldr r0, [r1], #4
    str r0, [r2], #4
    b .Lcopy_derive_runtime_controller_states
.Lcopy_derive_runtime_controller_states_done:
    ldr r1, =__dm4310_rls_literals_load__
    ldr r2, =__dm4310_rls_literals_start__
    ldr r0, [r1], #4
    str r0, [r2], #4
    ldr r0, [r1], #4
    str r0, [r2], #4
    ldr r0, [r1], #4
    str r0, [r2], #4
    ldr r1, =__dm4310_helper_rls2_load__
    ldr r2, =__dm4310_helper_rls2_start__
    ldr r3, =__dm4310_helper_rls2_end__
32:
    cmp r2, r3
    bcs 33f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 32b
33:
    ldr r1, =__dm4310_helper_flux_observer_load__
    ldr r2, =__dm4310_helper_flux_observer_start__
    ldr r3, =__dm4310_helper_flux_observer_end__
34:
    cmp r2, r3
    bcs 35f
    ldrb r0, [r1], #1
    strb r0, [r2], #1
    b 34b
35:
    ldr r1, =__dm4310_helper_sincos_load__
    ldr r2, =__dm4310_helper_sincos_start__
    ldr r3, =__dm4310_helper_sincos_end__
36:
    cmp r2, r3
    bcs 37f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 36b
37:
    ldr r1, =__dm4310_helper_clamp_load__
    ldr r2, =__dm4310_helper_clamp_start__
    ldr r3, =__dm4310_helper_clamp_end__
38:
    cmp r2, r3
    bcs 39f
    ldrh r0, [r1], #2
    strh r0, [r2], #2
    b 38b
39:
    ldr r1, =__dm4310_helper_wrap_load__
    ldr r2, =__dm4310_helper_wrap_start__
    ldr r3, =__dm4310_helper_wrap_end__
40:
    cmp r2, r3
    bcs 41f
    ldrh r0, [r1], #2
    strh r0, [r2], #2
    b 40b
41:
    ldr r1, =__dm4310_helper_limit_vector_load__
    ldr r2, =__dm4310_helper_limit_vector_start__
    ldr r3, =__dm4310_helper_limit_vector_end__
56:
    cmp r2, r3
    bcs 57f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 56b
57:
    ldr r1, =__dm4310_helper_float_to_uint_load__
    ldr r2, =__dm4310_helper_float_to_uint_start__
    ldr r3, =__dm4310_helper_float_to_uint_end__
62:
    cmp r2, r3
    bcs 63f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 62b
63:
    ldr r1, =__dm4310_helper_uint_to_float_load__
    ldr r2, =__dm4310_helper_uint_to_float_start__
    ldr r3, =__dm4310_helper_uint_to_float_end__
64:
    cmp r2, r3
    bcs 65f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 64b
65:
    ldr r1, =__dm4310_helper_derive_control_parameters_load__
    ldr r2, =__dm4310_helper_derive_control_parameters_start__
    ldr r3, =__dm4310_helper_derive_control_parameters_end__
54:
    cmp r2, r3
    bcs 55f
    ldrh r0, [r1], #2
    strh r0, [r2], #2
    b 54b
55:
    ldr r1, =__dm4310_helper_atan2_load__
    ldr r2, =__dm4310_helper_atan2_start__
    ldr r3, =__dm4310_helper_atan2_end__
46:
    cmp r2, r3
    bcs 47f
    ldrh r0, [r1], #2
    strh r0, [r2], #2
    b 46b
47:
    ldr r1, =__dm4310_helper_delay_ms_load__
    ldr r2, =__dm4310_helper_delay_ms_start__
    ldr r3, =__dm4310_helper_delay_ms_end__
48:
    cmp r2, r3
    bcs 49f
    ldrh r0, [r1], #2
    strh r0, [r2], #2
    b 48b
49:
    ldr r1, =__dm4310_helper_select_configuration_bank_b_load__
    ldr r2, =__dm4310_helper_select_configuration_bank_b_start__
    ldr r3, =__dm4310_helper_select_configuration_bank_b_end__
52:
    cmp r2, r3
    bcs 53f
    ldrh r0, [r1], #2
    strh r0, [r2], #2
    b 52b
53:
    ldr r1, =__dm4310_helper_sqrt_load__
    ldr r2, =__dm4310_helper_sqrt_start__
    ldr r3, =__dm4310_helper_sqrt_end__
50:
    cmp r2, r3
    bcs 51f
    ldrh r0, [r1], #2
    strh r0, [r2], #2
    b 50b
51:
#endif
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
#if defined(DAMIAO_DM4310)
    /* This factory object lies inside the global BSS-clear span, so restore
     * its flash image only after both zeroing loops have completed. */
    ldr r1, =dm4310_position_sensor_expected_image
    ldr r2, =__dm4310_position_sensor_expected_start__
    ldr r3, =__dm4310_position_sensor_expected_end__
70:
    cmp r2, r3
    bcs 71f
    ldrb r0, [r1], #1
    strb r0, [r2], #1
    b 70b
71:
    ldr r1, =__dm4310_aes_sbox_load__
    ldr r2, =__dm4310_aes_sbox_start__
    ldr r3, =__dm4310_aes_sbox_end__
72:
    cmp r2, r3
    bcs 73f
    ldr r0, [r1], #4
    str r0, [r2], #4
    b 72b
73:
    /* V5017 aliases expected[10]; V6417 gives the DMA word its own slot. */
    movs r0, #0
    ldr r1, =position_dma_word
    strh r0, [r1]
    bl commissioning_initialize_scatter_defaults
    bl app_config_initialize_scatter_defaults
    /* DM4310 skips constructor arrays below. Reconstruct the sine table
     * explicitly after BSS clearing, as part of scatter initialization. */
    bl motor_math_initialize_sine_table
    /* The UART framing pair is factory initialized data, not peripheral
     * setup state. Restore it once after BSS clearing, before main/IRQs. */
    bl board_uart_initialize_state
    bl dm4310_initialize_shared_literals
#endif
#if defined(DAMIAO_DM4310)
    /* The factory image has no C/C++ constructor arrays.  Keil enters its
     * runtime setup here, immediately before the application main entry. */
    b dm4310_runtime_main_entry
#else
    /* The firmware owns Reset_Handler, so do not pull in newlib's crt0 just
     * to walk the constructor arrays.  Keep this generic path for targets
     * which may later add static initializers with -nostartfiles. */
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
#endif
#if !defined(DAMIAO_DM4310)
    bl main
    b Default_Handler
#endif
    .size Reset_Handler, . - Reset_Handler

#if defined(DAMIAO_DM4310)
    .section .dm4310_runtime_main_entry, "ax", %progbits
    .align 2
    .global dm4310_runtime_main_entry
    .type dm4310_runtime_main_entry, %function
    .thumb_func
dm4310_runtime_main_entry:
    bl dm4310_runtime_context_initialize
    mov r1, r2
    bl dm4310_runtime_initialize
    bl main
    bl dm4310_runtime_exit
    .size dm4310_runtime_main_entry, . - dm4310_runtime_main_entry

    /* Runtime entry 0x21088 uses context+0x60 as a temporary stack before
     * installing the fixed application stack.  This is startup plumbing,
     * not a normal AAPCS-callable function (r5-r8/r11 are changed). */
    .section .text.dm4310_runtime_context_initialize, "ax", %progbits
    .align 2
    .global dm4310_runtime_context_initialize
    .type dm4310_runtime_context_initialize, %function
    .thumb_func
dm4310_runtime_context_initialize:
    mov r5, lr
    bl .Ldm4310_runtime_context_address
    mov lr, r5
    movs r5, r0
    mov r1, sp
    mov r3, r10
    bic.w r0, r0, #7
    mov sp, r0
    add sp, #0x60
    push {r5, lr}
    bl .Ldm4310_runtime_memory_bounds
    pop {r5, lr}
    mov.w r6, #0
    mov.w r7, #0
    mov.w r8, #0
    mov.w r11, #0
    bic.w r1, r1, #7
    mov r12, r5
    stmia.w r12!, {r6, r7, r8, r11}
    stmia.w r12!, {r6, r7, r8, r11}
    stmia.w r12!, {r6, r7, r8, r11}
    stmia.w r12!, {r6, r7, r8, r11}
    mov sp, r1
    bx lr
    .size dm4310_runtime_context_initialize, . - dm4310_runtime_context_initialize
.Ldm4310_runtime_context_address:
    ldr r0, =dm4310_c_runtime_state
    bx lr
.Ldm4310_runtime_memory_bounds:
    ldr r0, =__HeapBase
    ldr r1, =__StackTop
    ldr r2, =__HeapLimit
    ldr r3, =__StackLimit
    bx lr
    .ltorg
#endif

    .section .text.Default_Handler, "ax", %progbits
    .align 2
    .global Default_Handler
    .type Default_Handler, %function
    .thumb_func
Default_Handler:
9:
    b 9b
    .size Default_Handler, . - Default_Handler

    .section .note.GNU-stack, "", %progbits
