MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00020000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x1FFF8000, LENGTH = 64K
    RAMB  (rwx) : ORIGIN = 0x200F0000, LENGTH = 4K
}

ENTRY(Reset_Handler)

/* V3 targets share one source graph but use several fixed SRAM layouts. Keep
  * every address in this table so profile differences remain reviewable and
  * source modules do not invent local offsets. */
__relocated_sram_layout = DEFINED(DAMIAO_LAYOUT_RELOCATED_SRAM);
__shifted_sram_layout = DEFINED(DAMIAO_LAYOUT_SHIFTED_SRAM);

__layout_helper_fault_monitor = __relocated_sram_layout ? 0x1FFF9314 : 0x1FFF8000;
__layout_irq002 = __relocated_sram_layout ? 0x1FFF944A : 0x1FFF8136;
__layout_boot_record_writer = __relocated_sram_layout ? 0x1FFF8000 : 0x1FFF8640;
__layout_irq000 = __relocated_sram_layout ? 0x1FFF8104 : 0x1FFF8744;
__layout_irq001 = __relocated_sram_layout ? 0x1FFF8130 : 0x1FFF8770;
__layout_irq003 = __relocated_sram_layout ? 0x1FFF8278 : 0x1FFF88B8;
__layout_helper_output_sensor =
__relocated_sram_layout ? 0x1FFF9240 : (__shifted_sram_layout ? 0x1FFF9880 : 0x1FFF987C);
__layout_flash_writer =
__relocated_sram_layout ? 0x1FFF9988 : (__shifted_sram_layout ? 0x1FFF9954 : 0x1FFF9950);
__layout_flash_erase_only =
__relocated_sram_layout ? 0x1FFF9A70 : (__shifted_sram_layout ? 0x1FFF9A3C : 0x1FFF9A38);
__layout_helper_mcan_send_classic =
__relocated_sram_layout ? 0x1FFF9B04 : (__shifted_sram_layout ? 0x1FFF9AD0 : 0x1FFF9ACC);
__layout_helper_mcan_send_variable_fd =
__relocated_sram_layout ? 0x1FFF9B46 : (__shifted_sram_layout ? 0x1FFF9B12 : 0x1FFF9B0E);
__layout_helper_mcan_send_fd =
__relocated_sram_layout ? 0x1FFF9BD8 : (__shifted_sram_layout ? 0x1FFF9BA4 : 0x1FFF9BA0);
__layout_helper_svpwm =
__relocated_sram_layout ? 0x1FFF9C78 : (__shifted_sram_layout ? 0x1FFF9C44 : 0x1FFF9C40);
__layout_helper_clear_runtime_loop_states =
__relocated_sram_layout ? 0x1FFF9D94 : (__shifted_sram_layout ? 0x1FFF9D60 : 0x1FFF9D5C);
__layout_helper_reset_control_state =
__relocated_sram_layout ? 0x1FFF9E00 : (__shifted_sram_layout ? 0x1FFF9DCC : 0x1FFF9DC8);
__layout_helper_identification_filter =
__relocated_sram_layout ? 0x1FFF9E7C : (__shifted_sram_layout ? 0x1FFF9E48 : 0x1FFF9E44);
__layout_helper_derive_runtime_controller_states =
__relocated_sram_layout ? 0x1FFF9EDE : (__shifted_sram_layout ? 0x1FFF9EAA : 0x1FFF9EA6);
__layout_helper_motion_observer =
__relocated_sram_layout ? 0x1FFFA016 : (__shifted_sram_layout ? 0x1FFF9FE2 : 0x1FFF9FDE);
__layout_helper_current_controller =
__relocated_sram_layout ? 0x1FFFA0B2 : (__shifted_sram_layout ? 0x1FFFA07E : 0x1FFFA07A);
__layout_helper_rls2 =
__relocated_sram_layout ? 0x1FFFA198 : (__shifted_sram_layout ? 0x1FFFA164 : 0x1FFFA160);
__layout_helper_flux_observer =
__relocated_sram_layout ? 0x1FFFA26C : (__shifted_sram_layout ? 0x1FFFA238 : 0x1FFFA234);
__layout_helper_sincos =
__relocated_sram_layout ? 0x1FFFA344 : (__shifted_sram_layout ? 0x1FFFA310 : 0x1FFFA30C);
__layout_helper_clamp =
__relocated_sram_layout ? 0x1FFFA3C2 : (__shifted_sram_layout ? 0x1FFFA38E : 0x1FFFA38A);
__layout_helper_wrap =
__relocated_sram_layout ? 0x1FFFA3E2 : (__shifted_sram_layout ? 0x1FFFA3AE : 0x1FFFA3AA);
__layout_helper_limit_vector =
__relocated_sram_layout ? 0x1FFFA44C : (__shifted_sram_layout ? 0x1FFFA418 : 0x1FFFA414);
__layout_helper_float_to_uint =
__relocated_sram_layout ? 0x1FFFA4A2 : (__shifted_sram_layout ? 0x1FFFA46E : 0x1FFFA46A);
__layout_helper_uint_to_float =
__relocated_sram_layout ? 0x1FFFA4CA : (__shifted_sram_layout ? 0x1FFFA496 : 0x1FFFA492);
__layout_helper_derive_control_parameters =
__relocated_sram_layout ? 0x1FFFA514 : (__shifted_sram_layout ? 0x1FFFA4E0 : 0x1FFFA4DC);
__layout_helper_delay_ms =
__relocated_sram_layout ? 0x1FFFA51E : (__shifted_sram_layout ? 0x1FFFA4EA : 0x1FFFA4E6);
__layout_helper_select_configuration_bank_b =
__relocated_sram_layout ? 0x1FFFA528 : (__shifted_sram_layout ? 0x1FFFA4F4 : 0x1FFFA4F0);
__layout_helper_atan2 =
__relocated_sram_layout ? 0x1FFFA532 : (__shifted_sram_layout ? 0x1FFFA4FE : 0x1FFFA4FA);
__layout_helper_sqrt =
__relocated_sram_layout ? 0x1FFFA53C : (__shifted_sram_layout ? 0x1FFFA508 : 0x1FFFA504);

__layout_current_control_constants = __relocated_sram_layout ? 0x1FFF9978 : 0x1FFF8630;
__layout_position_timer_constants = __relocated_sram_layout ? 0x1FFF80E4 : 0x1FFF8724;
__layout_output_sensor_constants = __relocated_sram_layout ? 0x1FFF91F0 : 0x1FFF9830;
__layout_output_scaling_constants =
__relocated_sram_layout ? 0x1FFF92F8 : (__shifted_sram_layout ? 0x1FFF9938 : 0x1FFF9934);
__layout_flash_operation_constants =
__relocated_sram_layout ? 0x1FFF9AF8 : (__shifted_sram_layout ? 0x1FFF9AC4 : 0x1FFF9AC0);
__layout_mcan_transmit_constants =
__relocated_sram_layout ? 0x1FFF9C6C : (__shifted_sram_layout ? 0x1FFF9C38 : 0x1FFF9C34);
__layout_pwm_constants =
__relocated_sram_layout ? 0x1FFF9D84 : (__shifted_sram_layout ? 0x1FFF9D50 : 0x1FFF9D4C);
__layout_control_loop_constants =
__relocated_sram_layout ? 0x1FFFA150 : (__shifted_sram_layout ? 0x1FFFA11C : 0x1FFFA118);
__layout_commissioning_constants =
__relocated_sram_layout ? 0x1FFFA174 : (__shifted_sram_layout ? 0x1FFFA140 : 0x1FFFA13C);
__layout_identification_filter_constants =
__relocated_sram_layout ? 0x1FFFA170 : (__shifted_sram_layout ? 0x1FFFA13C : 0x1FFFA138);
__layout_motion_observer_constants =
__relocated_sram_layout ? 0x1FFFA194 : (__shifted_sram_layout ? 0x1FFFA160 : 0x1FFFA15C);
__layout_rls_constants =
__relocated_sram_layout ? 0x1FFFA338 : (__shifted_sram_layout ? 0x1FFFA304 : 0x1FFFA300);

__layout_initialized_state =
__relocated_sram_layout ? 0x1FFFA548 : (__shifted_sram_layout ? 0x1FFFA514 : 0x1FFFA510);
__layout_runtime_drive_d =
__relocated_sram_layout ? 0x1FFFA5F8 : (__shifted_sram_layout ? 0x1FFFA514 : 0x1FFFA510);
__layout_runtime_drive_q =
__relocated_sram_layout ? 0x1FFFA620 : (__shifted_sram_layout ? 0x1FFFA53C : 0x1FFFA538);
__layout_runtime_speed_loop =
__relocated_sram_layout ? 0x1FFFA650 : (__shifted_sram_layout ? 0x1FFFA56C : 0x1FFFA568);
__layout_runtime_position_loop =
__relocated_sram_layout ? 0x1FFFA678 : (__shifted_sram_layout ? 0x1FFFA594 : 0x1FFFA590);
__layout_uart_length_state =
__relocated_sram_layout ? 0x1FFFA548 : (__shifted_sram_layout ? 0x1FFFA5BC : 0x1FFFA5B8);
__layout_hardware_variant =
__relocated_sram_layout ? 0x1FFFA54C : (__shifted_sram_layout ? 0x1FFFA5C0 : 0x1FFFA5BC);
__layout_zero_position_staging =
__relocated_sram_layout ? 0x1FFFA550 : (__shifted_sram_layout ? 0x1FFFA5C4 : 0x1FFFA5C0);
__layout_config_staging =
__relocated_sram_layout ? 0x1FFFA558 : (__shifted_sram_layout ? 0x1FFFA5CC : 0x1FFFA5C8);
__layout_position_sensor_expected =
__relocated_sram_layout ? 0x1FFFA5EC : (__shifted_sram_layout ? 0x1FFFA660 : 0x1FFFA65C);
__layout_position_dma_word =
__relocated_sram_layout ? 0x1FFFA6A0 : (__shifted_sram_layout ? 0x1FFFA66A : 0x1FFFA666);
__layout_sine_table =
__relocated_sram_layout ? 0x1FFFA6AC : (__shifted_sram_layout ? 0x1FFFA678 : 0x1FFFA674);
__layout_aes_sbox =
__relocated_sram_layout ? 0x1FFFC6B0 : (__shifted_sram_layout ? 0x1FFFC67C : 0x1FFFC678);

__layout_boot_record_staging =
__relocated_sram_layout ? 0x1FFFC7B0 : (__shifted_sram_layout ? 0x1FFFC828 : 0x1FFFC824);
__layout_short_response_buffer =
__relocated_sram_layout ? 0x1FFFC7C4 : (__shifted_sram_layout ? 0x1FFFC83C : 0x1FFFC838);
__layout_motor_correction =
__relocated_sram_layout ? 0x1FFFC7D8 : (__shifted_sram_layout ? 0x1FFFC850 : 0x1FFFC84C);
__layout_can_protocol_scratch =
__relocated_sram_layout ? 0x1FFFCBD8 : (__shifted_sram_layout ? 0x1FFFCC50 : 0x1FFFCC4C);
__layout_motor_record =
__relocated_sram_layout ? 0x1FFFCBF8 : (__shifted_sram_layout ? 0x1FFFCC70 : 0x1FFFCC6C);
__layout_output_table =
__relocated_sram_layout ? 0x1FFFD004 : (__shifted_sram_layout ? 0x1FFFD07C : 0x1FFFD078);
__layout_output_calibration =
__relocated_sram_layout ? 0x1FFFF004 : (__shifted_sram_layout ? 0x1FFFF07C : 0x1FFFF078);
__layout_motor_runtime_state =
__relocated_sram_layout ? 0x1FFFF014 : (__shifted_sram_layout ? 0x1FFFF08C : 0x1FFFF088);
__layout_sample_runtime_state =
__relocated_sram_layout ? 0x1FFFF090 : (__shifted_sram_layout ? 0x1FFFF108 : 0x1FFFF104);
__layout_output_sensor_state =
__relocated_sram_layout ? 0x1FFFF134 : (__shifted_sram_layout ? 0x1FFFF1AC : 0x1FFFF1A8);
__layout_runtime_status =
__relocated_sram_layout ? 0x1FFFF17C : (__shifted_sram_layout ? 0x1FFFF1F4 : 0x1FFFF1F0);
__layout_runtime_parameter_cache =
__relocated_sram_layout ? 0x1FFFF1C8 : (__shifted_sram_layout ? 0x1FFFF240 : 0x1FFFF23C);
__layout_mcan_dispatch =
__relocated_sram_layout ? 0x1FFFF228 : (__shifted_sram_layout ? 0x1FFFF2A0 : 0x1FFFF29C);
__layout_calibration_frame_scratch =
__relocated_sram_layout ? 0x1FFFF2CC : (__shifted_sram_layout ? 0x1FFFF344 : 0x1FFFF340);
__layout_motion_observer =
__relocated_sram_layout ? 0x1FFFF30C : (__shifted_sram_layout ? 0x1FFFF384 : 0x1FFFF380);
__layout_adc_raw =
__relocated_sram_layout ? 0x1FFFF348 : (__shifted_sram_layout ? 0x1FFFC77C : 0x1FFFC778);
__layout_current_d =
__relocated_sram_layout ? 0x1FFFF35C : (__shifted_sram_layout ? 0x1FFFC790 : 0x1FFFC78C);
__layout_current_q =
__relocated_sram_layout ? 0x1FFFF3A8 : (__shifted_sram_layout ? 0x1FFFC7DC : 0x1FFFC7D8);
__layout_position_sensor_observed =
__relocated_sram_layout ? 0x1FFFF3F4 : (__shifted_sram_layout ? 0x1FFFF3C0 : 0x1FFFF3BC);
__layout_uart_rx_buffer =
__relocated_sram_layout ? 0x1FFFF3FF : (__shifted_sram_layout ? 0x1FFFF3CB : 0x1FFFF3C7);
__layout_c_runtime_state =
__relocated_sram_layout ? 0x1FFFF4C8 : (__shifted_sram_layout ? 0x1FFFF494 : 0x1FFFF490);
__layout_system_core_clock =
__relocated_sram_layout ? 0x1FFFF528 : (__shifted_sram_layout ? 0x1FFFF4F4 : 0x1FFFF4F0);
__layout_hrc_value =
__relocated_sram_layout ? 0x1FFFF52C : (__shifted_sram_layout ? 0x1FFFF4F8 : 0x1FFFF4F4);
__layout_bss_end =
__relocated_sram_layout ? 0x1FFFF530 : (__shifted_sram_layout ? 0x1FFFF500 : 0x1FFFF4F8);
__layout_heap_limit =
__relocated_sram_layout ? 0x1FFFF930 : (__shifted_sram_layout ? 0x1FFFF900 : 0x1FFFF8F8);
__layout_stack_top =
__relocated_sram_layout ? 0x20000530 : (__shifted_sram_layout ? 0x20000500 : 0x200004F8);

SECTIONS
{
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))
        . = ALIGN(4);
    }
    > FLASH

    .text :
    {
        . = ALIGN(4);
        /* Development/source builds use ordinary sequential Flash placement.
         * Fixed SRAM VMAs below remain ABI contracts; Flash function addresses
         * are deliberately free to move as source code evolves. */
        KEEP(*(.runtime_main_entry))
        *(EXCLUDE_FILE (*extended_arithmetic_core.S.obj) .text*)
        KEEP(*(.mcan_init_classic))
        KEEP(*(.mcan_transition_delay)) KEEP(*(.mcan_init_fd))
        KEEP(*(.irq004))

        /* The fixed-layout arithmetic implementation remains isolated from C,
         * but its Flash base is sequential.  Keep only the internal ordering
         * required by its hand-audited instruction/literal relationships. */
        KEEP(*(.text.softfloat_runtime_round_to_int))
        KEEP(*(.text.softfloat_extended_divide_core))
        KEEP(*(.text.softfloat_extended_round_core))
        KEEP(*(.text.softfloat_extended_divide_entry))
        KEEP(*(.text.softfloat_extended_multiply_entry))
        KEEP(*(.text.softfloat_extended_multiply_core))
        *(.glue_7 .glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    }
    > FLASH

    .softfloat_d2f :
    {
        KEEP(*(.text.softfloat_d2f))
        . = 0x00000058;
        KEEP(*(.text.softfloat_d2f_exception_result))
        . = 0x00000064;
    }
    > FLASH
    ASSERT(__relocated_sram_layout || SIZEOF(.softfloat_d2f) == 0x64,
           "standard V3 double-to-float runtime layout changed")

    /* Keep the fixed-layout ARM runtime outside .text so GNU ld can put its
     * source-only long-call veneers in the free bank before 0x270d8. */
    .softfloat_runtime :
    {
        KEEP(*(.text.softfloat_dadd))
        . = 0x00000150;
        KEEP(*(.text.softfloat_ddiv))
        . = 0x00000378;
        __image_division_seeds_start__ = .;
        KEEP(*(.division_seeds))
        __image_division_seeds_end__ = .;
        KEEP(*(.rodata.softfloat_ddiv_literals))
        . = 0x00000400;
        KEEP(*(.text.softfloat_d2uiz))
        . = 0x00000458;
        KEEP(*(.text.softfloat_d2uiz_saturate))
        . = 0x0000045A;
        KEEP(*(.text.softfloat_ui2d))
        . = 0x00000480;
        KEEP(*(.text.softfloat_dmul))
        . = 0x000005D4;
        KEEP(*(.text.softfloat_softdouble_exception))
        . = 0x0000067C;
        KEEP(*(.text.softfloat_reverse_subtract))
        . = 0x0000082C;
        KEEP(*(.text.softfloat_dsub))
        . = 0x00000A00;
        KEEP(*(.text.softfloat_f2d))
        . = 0x00000A48;
        KEEP(*(.text.softfloat_f2d_exception_result))
        . = 0x00000A56;
        KEEP(*(.text.softfloat_float_to_int))
        . = 0x00000A64;
        KEEP(*(.text.softfloat_uint64_to_float_entry))
        . = 0x00000A7E;
        KEEP(*(.text.softfloat_uint64_to_float_core))
        . = 0x00000AC4;
        KEEP(*(.text.softfloat_softfloat_exception))
        . = 0x00000B5A;
        KEEP(*(.text.softfloat_d2f_pack))
        . = 0x00000B64;
        KEEP(*(.text.softfloat_round_even))
    }
    > FLASH

    ASSERT(__relocated_sram_layout ||
           __image_division_seeds_end__ - __image_division_seeds_start__ == 0x80,
           "standard V3 reciprocal seed table must contain 128 bytes")
    ASSERT(__relocated_sram_layout || SIZEOF(.softfloat_runtime) == 0xBC8,
           "standard V3 arithmetic runtime layout changed")

    .temperature_table :
    {
        KEEP(*(.rodata.temperature_celsius_table))
    } > FLASH
    ASSERT(__relocated_sram_layout || SIZEOF(.temperature_table) == 0x400,
           "standard V3 temperature table must retain all 256 words")

    .c_locale_name :
    {
        KEEP(*(.c_locale_name))
    } > FLASH

    .c_locale :
    {
        KEEP(*(.c_locale))
    } > FLASH

    .text_platform :
    {
        . = ALIGN(4);
        *(.text .text*)
        . = ALIGN(4);
    }
    > FLASH

    .rodata :
    {
        . = ALIGN(4);
        __image_literal_copies_start__ = .;
        LONG(__image_current_control_constants_load__);
        LONG(ADDR(.current_control_constants));
        LONG(4);
        LONG(__image_position_timer_constants_load__);
        LONG(ADDR(.position_timer_constants));
        LONG(8);
        LONG(__image_output_sensor_constants_load__);
        LONG(ADDR(.output_sensor_constants));
        LONG(19);
        LONG(__image_output_scaling_constants_load__);
        LONG(ADDR(.output_scaling_constants));
        LONG(7);
        LONG(__image_flash_operation_constants_load__);
        LONG(ADDR(.flash_operation_constants));
        LONG(3);
        LONG(__image_mcan_transmit_constants_load__);
        LONG(ADDR(.mcan_transmit_constants));
        LONG(3);
        LONG(__image_pwm_constants_load__);
        LONG(ADDR(.pwm_constants));
        LONG(4);
        LONG(__image_control_loop_constants_load__);
        LONG(ADDR(.control_loop_constants));
        LONG(8);
        LONG(__image_commissioning_constants_load__);
        LONG(ADDR(.commissioning_constants));
        LONG(8);
        __image_literal_copies_end__ = .;
        *(.rodata)
        *(EXCLUDE_FILE (*aes256_ctr.c.obj) .rodata*)
        . = ALIGN(4);
    }
    > FLASH

    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    }
    > FLASH
    __exidx_start = .;
    .ARM.exidx : {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    }
    > FLASH
    __exidx_end = .;

    .preinit_array(READONLY) :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    }
    > FLASH
    .init_array(READONLY) :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
    }
    > FLASH
    .fini_array(READONLY) :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
    }
    > FLASH

    __etext = ALIGN(4);
    .data : AT(__etext)
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data .data*)
        *(.ramfunc .ramfunc*)
        . = ALIGN(4);
        __data_end__ = .;
    }
    > RAM

    /* Entry veneers occupy the fixed RAM vector destinations. Reset_Handler
     * copies each source-built section into its runtime address. */
    __image_ram_lma = __etext + ALIGN(SIZEOF(.data), 4);
    .helper_fault_monitor __layout_helper_fault_monitor : AT(__image_ram_lma)
    {
        __image_helper_fault_monitor_start__ = .;
        KEEP(*(.helper_fault_monitor))
        __image_helper_fault_monitor_end__ = .;
    }
    > RAM
    __image_helper_fault_monitor_load__ = LOADADDR(.helper_fault_monitor);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_fault_monitor), 4);

    .irq002 __layout_irq002 : AT(__image_ram_lma)
    {
        __image_irq002_start__ = .;
        KEEP(*(.irq002))
        /* Embedded firmware data pool shared by fault monitor and ADC IRQ.
         * Startup copies it with the existing IRQ section, not as code. */
        . = __image_irq002_start__ + 0x2C2;
        KEEP(*(.fault_monitor_constants))
        . = ALIGN(4);
        __image_irq002_end__ = .;
    }
    > RAM
    __image_irq002_load__ = LOADADDR(.irq002);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.irq002), 4);

    .boot_record_writer __layout_boot_record_writer : AT(__image_ram_lma)
    {
        __image_boot_record_writer_start__ = .;
        KEEP(*(.boot_record_writer))
        . = ALIGN(4);
        __image_boot_record_writer_end__ = .;
    }
    > RAM
    __image_boot_record_writer_load__ = LOADADDR(.boot_record_writer);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.boot_record_writer), 4);

    .irq000 __layout_irq000 : AT(__image_ram_lma)
    {
        __image_irq000_start__ = .;
        KEEP(*(.irq000))
        . = ALIGN(4);
        __image_irq000_end__ = .;
    }
    > RAM
    __image_irq000_load__ = LOADADDR(.irq000);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.irq000), 4);

    .irq001 __layout_irq001 : AT(__image_ram_lma)
    {
        __image_irq001_start__ = .;
        KEEP(*(.irq001))
        . = ALIGN(4);
        __image_irq001_end__ = .;
    }
    > RAM
    __image_irq001_load__ = LOADADDR(.irq001);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.irq001), 4);

    .irq003 __layout_irq003 : AT(__image_ram_lma)
    {
        __image_irq003_start__ = .;
        KEEP(*(.irq003))
        . = __image_irq003_start__ + 0x288;
        KEEP(*(.mcan_receive_constants_primary))
        . = __image_irq003_start__ + 0x6D4;
        KEEP(*(.mcan_receive_constants_secondary))
        . = __image_irq003_start__ + 0xB10;
        KEEP(*(.mcan_receive_constants_tertiary))
        . = ALIGN(4);
        __image_irq003_end__ = .;
    }
    > RAM
    __image_irq003_load__ = LOADADDR(.irq003);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.irq003), 4);

    .flash_writer __layout_flash_writer : AT(__image_ram_lma)
    {
        __image_flash_writer_start__ = .;
        KEEP(*(.flash_writer))
        . = ALIGN(4);
        __image_flash_writer_end__ = .;
    }
    > RAM
    __image_flash_writer_load__ = LOADADDR(.flash_writer);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.flash_writer), 4);

    .flash_erase_only __layout_flash_erase_only : AT(__image_ram_lma)
    {
        __image_flash_erase_only_start__ = .;
        KEEP(*(.flash_erase_only))
        __image_flash_erase_only_end__ = .;
    }
    > RAM
    __image_flash_erase_only_load__ = LOADADDR(.flash_erase_only);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.flash_erase_only), 4);

    .helper_output_sensor __layout_helper_output_sensor : AT(__image_ram_lma)
    {
        __image_helper_output_sensor_start__ = .;
        KEEP(*(.helper_output_sensor))
        __image_helper_output_sensor_end__ = .;
    }
    > RAM
    __image_helper_output_sensor_load__ = LOADADDR(.helper_output_sensor);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_output_sensor), 4);

    .helper_mcan_send_classic __layout_helper_mcan_send_classic : AT(__image_ram_lma)
    {
        __image_helper_mcan_send_classic_start__ = .;
        KEEP(*(.helper_mcan_send_classic))
        __image_helper_mcan_send_classic_end__ = .;
    }
    > RAM
    __image_helper_mcan_send_classic_load__ = LOADADDR(.helper_mcan_send_classic);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_mcan_send_classic), 4);

    .helper_mcan_send_variable_fd __layout_helper_mcan_send_variable_fd : AT(__image_ram_lma)
    {
        __image_helper_mcan_send_variable_fd_start__ = .;
        KEEP(*(.helper_mcan_send_variable_fd))
        SHORT(0)
        __image_helper_mcan_send_variable_fd_end__ = .;
    }
    > RAM
    __image_helper_mcan_send_variable_fd_load__ = LOADADDR(.helper_mcan_send_variable_fd);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_mcan_send_variable_fd), 4);

    .helper_mcan_send_fd __layout_helper_mcan_send_fd : AT(__image_ram_lma)
    {
        __image_helper_mcan_send_fd_start__ = .;
        KEEP(*(.helper_mcan_send_fd))
        __image_helper_mcan_send_fd_end__ = .;
    }
    > RAM
    __image_helper_mcan_send_fd_load__ = LOADADDR(.helper_mcan_send_fd);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_mcan_send_fd), 4);

    .helper_svpwm __layout_helper_svpwm : AT(__image_ram_lma)
    {
        __image_helper_svpwm_start__ = .;
        KEEP(*(.helper_svpwm))
        __image_helper_svpwm_end__ = .;
    }
    > RAM
    __image_helper_svpwm_load__ = LOADADDR(.helper_svpwm);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_svpwm), 4);

    .helper_clear_runtime_loop_states __layout_helper_clear_runtime_loop_states
    : AT(__image_ram_lma)
    {
        __image_helper_clear_runtime_loop_states_start__ = .;
        KEEP(*(.helper_clear_runtime_loop_states))
        __image_helper_clear_runtime_loop_states_end__ = .;
    }
    > RAM
    __image_helper_clear_runtime_loop_states_load__ =
    LOADADDR(.helper_clear_runtime_loop_states);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_clear_runtime_loop_states), 4);

    .helper_reset_control_state __layout_helper_reset_control_state : AT(__image_ram_lma)
    {
        __image_helper_reset_control_state_start__ = .;
        KEEP(*(.helper_reset_control_state))
        __image_helper_reset_control_state_end__ = .;
    }
    > RAM
    __image_helper_reset_control_state_load__ = LOADADDR(.helper_reset_control_state);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_reset_control_state), 4);

    .helper_identification_filter __layout_helper_identification_filter : AT(__image_ram_lma)
    {
        __image_helper_identification_filter_start__ = .;
        KEEP(*(.helper_identification_filter))
        __image_helper_identification_filter_end__ = .;
    }
    > RAM
    __image_helper_identification_filter_load__ = LOADADDR(.helper_identification_filter);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_identification_filter), 4);

    .helper_derive_runtime_controller_states __layout_helper_derive_runtime_controller_states
    : AT(__image_ram_lma)
    {
        __image_helper_derive_runtime_controller_states_start__ = .;
        KEEP(*(.helper_derive_runtime_controller_states))
        SHORT(0)
        __image_helper_derive_runtime_controller_states_end__ = .;
    }
    > RAM
    __image_helper_derive_runtime_controller_states_load__ =
    LOADADDR(.helper_derive_runtime_controller_states);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_derive_runtime_controller_states), 4);
    ASSERT(__relocated_sram_layout || SIZEOF(.helper_derive_runtime_controller_states) == 0x0C,
    "standard V3 controller derivation entry size changed")

    .helper_motion_observer __layout_helper_motion_observer : AT(__image_ram_lma)
    {
        __image_helper_motion_observer_start__ = .;
        KEEP(*(.helper_motion_observer))
        __image_helper_motion_observer_end__ = .;
    }
    > RAM
    __image_helper_motion_observer_load__ = LOADADDR(.helper_motion_observer);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_motion_observer), 4);

    .helper_current_controller __layout_helper_current_controller : AT(__image_ram_lma)
    {
        __image_helper_current_controller_start__ = .;
        KEEP(*(.helper_current_controller))
        __image_helper_current_controller_end__ = .;
    }
    > RAM
    __image_helper_current_controller_load__ = LOADADDR(.helper_current_controller);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_current_controller), 4);

    .current_control_constants __layout_current_control_constants :
        AT(LOADADDR(.helper_current_controller) +
           ALIGN(SIZEOF(.helper_current_controller), 4))
    {
        KEEP(*(.current_control_constants))
    } > RAM
    __image_current_control_constants_load__ = LOADADDR(.current_control_constants);
    ASSERT(__relocated_sram_layout || SIZEOF(.current_control_constants) == 16,
           "current-control constant pool size changed")

    .position_timer_constants __layout_position_timer_constants :
        AT(LOADADDR(.current_control_constants) +
           ALIGN(SIZEOF(.current_control_constants), 4))
    {
        KEEP(*(.position_timer_constants))
    } > RAM
    __image_position_timer_constants_load__ = LOADADDR(.position_timer_constants);
    ASSERT(__relocated_sram_layout || SIZEOF(.position_timer_constants) == 32,
           "position-timer constant pool size changed")

    .output_sensor_constants __layout_output_sensor_constants :
        AT(LOADADDR(.position_timer_constants) +
           ALIGN(SIZEOF(.position_timer_constants), 4))
    {
        KEEP(*(.output_sensor_constants))
    } > RAM
    __image_output_sensor_constants_load__ = LOADADDR(.output_sensor_constants);
    ASSERT(SIZEOF(.output_sensor_constants) ==
               ((__relocated_sram_layout || __shifted_sram_layout) ? 80 : 76),
           "output-sensor constant pool size changed")

    .output_scaling_constants __layout_output_scaling_constants :
        AT(LOADADDR(.output_sensor_constants) +
           ALIGN(SIZEOF(.output_sensor_constants), 4))
    {
        KEEP(*(.output_scaling_constants))
    } > RAM
    __image_output_scaling_constants_load__ = LOADADDR(.output_scaling_constants);
    ASSERT(__relocated_sram_layout || SIZEOF(.output_scaling_constants) == 28,
           "output-scaling constant pool size changed")

    .flash_operation_constants __layout_flash_operation_constants :
        AT(LOADADDR(.output_scaling_constants) +
           ALIGN(SIZEOF(.output_scaling_constants), 4))
    {
        KEEP(*(.flash_operation_constants))
    } > RAM
    __image_flash_operation_constants_load__ = LOADADDR(.flash_operation_constants);
    ASSERT(__relocated_sram_layout || SIZEOF(.flash_operation_constants) == 12,
           "flash-operation constant pool size changed")

    .mcan_transmit_constants __layout_mcan_transmit_constants :
        AT(LOADADDR(.flash_operation_constants) +
           ALIGN(SIZEOF(.flash_operation_constants), 4))
    {
        KEEP(*(.mcan_transmit_constants))
    } > RAM
    __image_mcan_transmit_constants_load__ = LOADADDR(.mcan_transmit_constants);
    ASSERT(__relocated_sram_layout || SIZEOF(.mcan_transmit_constants) == 12,
           "MCAN transmit constant pool size changed")

    .pwm_constants __layout_pwm_constants :
        AT(LOADADDR(.mcan_transmit_constants) +
           ALIGN(SIZEOF(.mcan_transmit_constants), 4))
    {
        KEEP(*(.pwm_constants))
    } > RAM
    __image_pwm_constants_load__ = LOADADDR(.pwm_constants);
    ASSERT(__relocated_sram_layout || SIZEOF(.pwm_constants) == 16,
           "PWM constant pool size changed")

    .control_loop_constants __layout_control_loop_constants :
        AT(LOADADDR(.pwm_constants) + ALIGN(SIZEOF(.pwm_constants), 4))
    {
        KEEP(*(.control_loop_constants))
    } > RAM
    __image_control_loop_constants_load__ = LOADADDR(.control_loop_constants);
    ASSERT(__relocated_sram_layout || SIZEOF(.control_loop_constants) == 32,
           "control-loop constant pool size changed")

    .commissioning_constants __layout_commissioning_constants :
        AT(LOADADDR(.control_loop_constants) +
           ALIGN(SIZEOF(.control_loop_constants), 4))
    {
        KEEP(*(.commissioning_constants))
    } > RAM
    __image_commissioning_constants_load__ =
        LOADADDR(.control_loop_constants) + SIZEOF(.control_loop_constants);
    __image_ram_lma =
        __image_commissioning_constants_load__ + SIZEOF(.commissioning_constants);
    ASSERT(__relocated_sram_layout || SIZEOF(.commissioning_constants) == 32,
           "commissioning constant pool size changed")

    .identification_filter_constants __layout_identification_filter_constants
    : AT(__image_ram_lma)
    {
        __image_identification_filter_constants_start__ = .;
        KEEP(*(.identification_filter_constants))
        __image_identification_filter_constants_end__ = .;
    }
    > RAM
    __image_identification_filter_constants_load__ =
    LOADADDR(.identification_filter_constants);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.identification_filter_constants), 4);

    .motion_observer_constants __layout_motion_observer_constants : AT(__image_ram_lma)
    {
        __image_motion_observer_constants_start__ = .;
        KEEP(*(.motion_observer_constants))
        __image_motion_observer_constants_end__ = .;
    }
    > RAM
    __image_motion_observer_constants_load__ = LOADADDR(.motion_observer_constants);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.motion_observer_constants), 4);

    .helper_rls2 __layout_helper_rls2 : AT(__image_ram_lma)
    {
        __image_helper_rls2_start__ = .;
        KEEP(*(.helper_rls2))
        __image_helper_rls2_end__ = .;
    }
    > RAM
    __image_helper_rls2_load__ = LOADADDR(.helper_rls2);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_rls2), 4);

    .rls_constants __layout_rls_constants : AT(__image_ram_lma)
    {
        __image_rls_constants_start__ = .;
        KEEP(*(.rls_constants))
        __image_rls_constants_end__ = .;
    }
    > RAM
    __image_rls_constants_load__ = LOADADDR(.rls_constants);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.rls_constants), 4);

    .helper_flux_observer __layout_helper_flux_observer : AT(__image_ram_lma)
    {
        __image_helper_flux_observer_start__ = .;
        KEEP(*(.helper_flux_observer))
        __image_helper_flux_observer_end__ = .;
    }
    > RAM
    __image_helper_flux_observer_load__ = LOADADDR(.helper_flux_observer);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_flux_observer), 4);

    .helper_sincos __layout_helper_sincos : AT(__image_ram_lma)
    {
        __image_helper_sincos_start__ = .;
        KEEP(*(.helper_sincos))
        __image_helper_sincos_end__ = .;
    }
    > RAM
    __image_helper_sincos_load__ = LOADADDR(.helper_sincos);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_sincos), 4);

    .helper_clamp __layout_helper_clamp : AT(__image_ram_lma)
    {
        __image_helper_clamp_start__ = .;
        KEEP(*(.helper_clamp))
        __image_helper_clamp_end__ = .;
    }
    > RAM
    __image_helper_clamp_load__ = LOADADDR(.helper_clamp);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_clamp), 4);

    .helper_wrap __layout_helper_wrap : AT(__image_ram_lma)
    {
        __image_helper_wrap_start__ = .;
        KEEP(*(.helper_wrap))
        __image_helper_wrap_end__ = .;
    }
    > RAM
    __image_helper_wrap_load__ = LOADADDR(.helper_wrap);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_wrap), 4);

    .helper_limit_vector __layout_helper_limit_vector : AT(__image_ram_lma)
    {
        __image_helper_limit_vector_start__ = .;
        KEEP(*(.helper_limit_vector))
        __image_helper_limit_vector_end__ = .;
    }
    > RAM
    __image_helper_limit_vector_load__ = LOADADDR(.helper_limit_vector);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_limit_vector), 4);

    .helper_float_to_uint __layout_helper_float_to_uint : AT(__image_ram_lma)
    {
        __image_helper_float_to_uint_start__ = .;
        KEEP(*(.helper_float_to_uint))
        __image_helper_float_to_uint_end__ = .;
    }
    > RAM
    __image_helper_float_to_uint_load__ = LOADADDR(.helper_float_to_uint);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_float_to_uint), 4);

    .helper_uint_to_float __layout_helper_uint_to_float : AT(__image_ram_lma)
    {
        __image_helper_uint_to_float_start__ = .;
        KEEP(*(.helper_uint_to_float))
        __image_helper_uint_to_float_end__ = .;
    }
    > RAM
    __image_helper_uint_to_float_load__ = LOADADDR(.helper_uint_to_float);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_uint_to_float), 4);

    .helper_derive_control_parameters __layout_helper_derive_control_parameters
    : AT(__image_ram_lma)
    {
        __image_helper_derive_control_parameters_start__ = .;
        KEEP(*(.helper_derive_control_parameters))
        __image_helper_derive_control_parameters_end__ = .;
    }
    > RAM
    __image_helper_derive_control_parameters_load__ =
    LOADADDR(.helper_derive_control_parameters);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_derive_control_parameters), 4);

    .helper_delay_ms __layout_helper_delay_ms : AT(__image_ram_lma)
    {
        __image_helper_delay_ms_start__ = .;
        KEEP(*(.helper_delay_ms))
        __image_helper_delay_ms_end__ = .;
    }
    > RAM
    __image_helper_delay_ms_load__ = LOADADDR(.helper_delay_ms);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_delay_ms), 4);

    .helper_select_configuration_bank_b __layout_helper_select_configuration_bank_b
    : AT(__image_ram_lma)
    {
        __image_helper_select_configuration_bank_b_start__ = .;
        KEEP(*(.helper_select_configuration_bank_b))
        __image_helper_select_configuration_bank_b_end__ = .;
    }
    > RAM
    __image_helper_select_configuration_bank_b_load__ =
    LOADADDR(.helper_select_configuration_bank_b);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_select_configuration_bank_b), 4);

    .helper_atan2 __layout_helper_atan2 : AT(__image_ram_lma)
    {
        __image_helper_atan2_start__ = .;
        KEEP(*(.helper_atan2))
        __image_helper_atan2_end__ = .;
    }
    > RAM
    __image_helper_atan2_load__ = LOADADDR(.helper_atan2);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_atan2), 4);

    .helper_sqrt __layout_helper_sqrt : AT(__image_ram_lma)
    {
        __image_helper_sqrt_start__ = .;
        KEEP(*(.helper_sqrt))
        SHORT(0)
        __image_helper_sqrt_end__ = .;
    }
    > RAM
    __image_helper_sqrt_load__ = LOADADDR(.helper_sqrt);
    __image_ram_lma = __image_ram_lma + ALIGN(SIZEOF(.helper_sqrt), 4);

    /* Reserve the flash load image here so RAMB data follows every sparse
     * initialized SRAM object. */
    __image_aes_sbox_load__ = __image_ram_lma;
    __image_ram_lma = __image_ram_lma + 0x100;

    __etext_ramb = __image_ram_lma;
    .ramb_data : AT(__etext_ramb)
    {
        __data_start_ramb__ = .;
        *(.ramb_data .ramb_data*)
        __data_end_ramb__ = .;
    }
    > RAMB

    /* Clear the fixed staging objects together with ordinary BSS.  The sine
     * table constructor then builds the initialized image,
     * including relying on zero at its three axis crossings. */
    __bss_start__ = __layout_initialized_state;

    .runtime_drive_d __layout_runtime_drive_d (NOLOAD):
    {
        KEEP(*(.runtime_drive_d))
    }
    > RAM
    .runtime_drive_q __layout_runtime_drive_q (NOLOAD):
    {
        KEEP(*(.runtime_drive_q))
    }
    > RAM
    .runtime_speed_loop __layout_runtime_speed_loop (NOLOAD):
    {
        KEEP(*(.runtime_speed_loop))
    }
    > RAM
    .runtime_position_loop __layout_runtime_position_loop (NOLOAD):
    {
        KEEP(*(.runtime_position_loop))
    }
    > RAM

    /* The UART command IRQ compares received length with payload length + 1. */
    .uart_length_state __layout_uart_length_state (NOLOAD):
    {
        KEEP(*(.uart_length_state))
    }
    > RAM

    .hardware_variant __layout_hardware_variant (NOLOAD):
    {
        KEEP(*(.hardware_variant))
    }
    > RAM

    /* FE writes these two offsets directly from this fixed source. */
    .zero_position_staging __layout_zero_position_staging (NOLOAD):
    {
        KEEP(*(.zero_position_staging))
    }
    > RAM

    /* The live 37-word motor configuration record used by UART ingress,
     * commissioning and flash commit sits immediately before the sine table. */
    .config_staging __layout_config_staging (NOLOAD):
    {
        KEEP(*(.config_staging))
    }
    > RAM

    /* Position setup compares eleven readbacks with this expected register
     * image. Reset_Handler restores it after clearing BSS. */
    .position_sensor_expected __layout_position_sensor_expected (NOLOAD):
    {
        __image_position_sensor_expected_start__ = .;
        KEEP(*(.position_sensor_expected))
        __image_position_sensor_expected_end__ = __image_position_sensor_expected_start__ + 11;
    }
    > RAM

    /* The alternate layout places four commissioning controller states after
     * its staging prefix. Separate input names preserve both layout orders. */
    .alternate_runtime_drive_d __layout_runtime_drive_d (NOLOAD):
    {
        KEEP(*(.alternate_runtime_drive_d))
    }
    > RAM
    .alternate_runtime_drive_q __layout_runtime_drive_q (NOLOAD):
    {
        KEEP(*(.alternate_runtime_drive_q))
    }
    > RAM
    .alternate_runtime_speed_loop __layout_runtime_speed_loop (NOLOAD):
    {
        KEEP(*(.alternate_runtime_speed_loop))
    }
    > RAM
    .alternate_runtime_position_loop __layout_runtime_position_loop (NOLOAD):
    {
        KEEP(*(.alternate_runtime_position_loop))
    }
    > RAM

    /* Standard layouts overlap expected[10]; alternate layouts use a
     * dedicated input section. */
    .position_dma_word __layout_position_dma_word (NOLOAD):
    {
        KEEP(*(.position_dma_word))
    }
    > RAM

    /* SRAM helpers access this 2049-float table through the fixed memory ABI. */
    .sine_table __layout_sine_table (NOLOAD):
    {
        KEEP(*(.sine_table))
    }
    > RAM

    /* AES key expansion and SubBytes share this SRAM S-box. */
    .aes_sbox __layout_aes_sbox : AT(__image_aes_sbox_load__)
    {
        __image_aes_sbox_start__ = .;
        KEEP(*(.rodata.sbox))
        __image_aes_sbox_end__ = .;
    }
    > RAM

    .adc_raw __layout_adc_raw (NOLOAD):
    {
        KEEP(*(.adc_raw))
    }
    > RAM

    /* Exact 19-float D/Q observer-controller states consumed by
     * current_controller_step. */
    .current_d __layout_current_d (NOLOAD):
    {
        KEEP(*(.current_d))
    }
    > RAM
    .current_q __layout_current_q (NOLOAD):
    {
        KEEP(*(.current_q))
    }
    > RAM

    .boot_record_staging __layout_boot_record_staging (NOLOAD):
    {
        KEEP(*(.boot_record_staging))
    }
    > RAM

    /* The UART command IRQ assembles each short command response here. */
    .short_response_buffer __layout_short_response_buffer (NOLOAD):
    {
        KEEP(*(.short_response_buffer))
    }
    > RAM

    /* load_and_validate_calibration copies the 256 motor-encoder
     * correction floats from flash to this runtime lookup table. */
    .motor_correction __layout_motor_correction (NOLOAD):
    {
        KEEP(*(.motor_correction))
    }
    > RAM

    .can_protocol_scratch __layout_can_protocol_scratch (NOLOAD):
    {
        KEEP(*(.can_protocol_scratch))
    }
    > RAM

    /* The UART command IRQ writes Ud and UM chunks directly into these regions. */
    .motor_record __layout_motor_record (NOLOAD):
    {
        KEEP(*(.motor_record))
    }
    > RAM
    .output_table __layout_output_table (NOLOAD):
    {
        KEEP(*(.output_table))
    }
    > RAM

    /* load_and_validate_calibration keeps the four analogue-output sensor
     * calibration floats at 0x1ffff078 before scattering them into state. */
    .output_calibration __layout_output_calibration (NOLOAD):
    {
        KEEP(*(.output_calibration))
    }
    > RAM

    /* Shared motor/control and ADC/sample objects. Their boundaries are part
     * of the fixed SRAM ABI used by startup, commissioning, and fast control. */
    .motor_runtime_state __layout_motor_runtime_state (NOLOAD):
    {
        KEEP(*(.motor_runtime_state))
    }
    > RAM
    .sample_runtime_state __layout_sample_runtime_state (NOLOAD):
    {
        KEEP(*(.sample_runtime_state))
    }
    > RAM
    position_sensor_scratch = __layout_sample_runtime_state + 0x8C;

    /* Current-sensor validation passes this complete 0x48-byte state object
     * to the output-sensor lookup helper, including two reserved words. */
    .output_sensor_state __layout_output_sensor_state (NOLOAD) :
    {
        KEEP(*(.output_sensor_state))
        . = 0x48;
    }
    > RAM

    /* Shared deferred-event, fault debounce and status object.  The next
     * fixed-layout object starts exactly at 0x1ffff23c. */
    .runtime_status __layout_runtime_status (NOLOAD):
    {
        KEEP(*(.runtime_status))
    }
    > RAM

    .runtime_parameter_cache __layout_runtime_parameter_cache (NOLOAD):
    {
        KEEP(*(.runtime_parameter_cache))
    }
    > RAM

    /* Classic/FD callback pair followed by the persistent CAN protocol
     * workspace.  The adjacent calibration scratch starts at 0x1ffff340. */
    .mcan_dispatch __layout_mcan_dispatch (NOLOAD):
    {
        KEEP(*(.mcan_dispatch))
    }
    > RAM

    /* Stage rx+1 here before dispatching Ud and UM slot writes. */
    .calibration_frame_scratch __layout_calibration_frame_scratch (NOLOAD):
    {
        KEEP(*(.calibration_frame_scratch))
    }
    > RAM

    /* Exact 15-float motion observer consumed by state_step. */
    .motion_observer __layout_motion_observer (NOLOAD):
    {
        KEEP(*(.motion_observer))
    }
    > RAM

    /* Alternate layouts keep ADC raw values and both current controllers beside the
     * motion observer instead of ahead of the protocol/calibration state. */
    .alternate_adc_raw __layout_adc_raw (NOLOAD):
    {
        KEEP(*(.alternate_adc_raw))
    }
    > RAM
    .alternate_current_d __layout_current_d (NOLOAD):
    {
        KEEP(*(.alternate_current_d))
    }
    > RAM
    .alternate_current_q __layout_current_q (NOLOAD):
    {
        KEEP(*(.alternate_current_q))
    }
    > RAM

    /* position_sensor_configure keeps the eleven register readbacks
     * immediately before the unaligned UART DMA buffer. */
    .position_sensor_observed __layout_position_sensor_observed (NOLOAD):
    {
        KEEP(*(.position_sensor_observed))
    }
    > RAM

    /* Keep the gap after the deferred-event/status object free of unrelated
     * BSS.  The USART1 DMA receive window begins at an unaligned ABI address. */
    .uart_rx_buffer __layout_uart_rx_buffer (NOLOAD):
    {
        KEEP(*(.uart_rx_buffer))
    }
    > RAM

    .c_runtime_state __layout_c_runtime_state (NOLOAD):
    {
        KEEP(*(.c_runtime_state))
    }
    > RAM

    /* SystemCoreClockUpdate addresses this adjacent pair through the
     * literal 0x1ffff4f0: system frequency first, HRC frequency second. */
    .system_core_clock __layout_system_core_clock (NOLOAD):
    {
        KEEP(*(.system_core_clock))
    }
    > RAM
    .hrc_value __layout_hrc_value (NOLOAD):
    {
        KEEP(*(.hrc_value))
    }
    > RAM

    /* Fixed zero-initialized state ends where its 1 KiB heap begins. */
    __image_bss_end__ = __layout_bss_end;

    .ramb_bss (NOLOAD) :
    {
        __bss_start_ramb__ = .;
        *(.ramb_bss .ramb_bss*)
        __bss_end_ramb__ = .;
    }
    > RAMB

    __HeapBase = __layout_bss_end;
    __HeapLimit = __layout_heap_limit;

    /* Aggregate work objects do not participate in the fixed SRAM ABI. Keep
     * them above the application MSP so they cannot overlap fixed objects. */
    .app_state 0x20001600 (NOLOAD):
    {
        __app_state_start__ = .;
        KEEP(*(.app_state))
        __app_state_end__ = .;
    }
    > RAM

    /* General bookkeeping has no fixed address contract. Keep it in the same
     * above-MSP area so it cannot consume the runtime heap or fixed state. */
    .bss 0x20002000 (NOLOAD):
    {
        __source_bss_start__ = .;
        . = ALIGN(4);
        *(.bss .bss*)
        *(COMMON)
        . = ALIGN(4);
        *(.noinit .noinit*)
        __source_bss_end__ = .;
    }
    > RAM
    __bss_end__ = __source_bss_end__;
    __end__ = ALIGN(__source_bss_end__, 8);
    PROVIDE(end = __end__);
    PROVIDE(_end = __end__);

    /* The vector table uses this exact initial MSP. Keep one KiB beyond static
     * storage as a link-time floor for the deepest application frame. */
    __StackTop = __layout_stack_top;
    __StackLimit = __layout_heap_limit;

    .ARM.attributes 0 :
    {
        *(.ARM.attributes)
    }
    PROVIDE(_stack = __StackTop);
    /* Sparse SRAM sections use explicit AT() LMAs, so expose their true image
     * end and the remaining application-partition capacity. */
    __flash_image_end__ = LOADADDR(.ramb_data) + SIZEOF(.ramb_data);
    __flash_free_bytes__ = ABSOLUTE(ORIGIN(FLASH) + LENGTH(FLASH) - __flash_image_end__);
    __app_state_free_bytes__ = ABSOLUTE(0x20002000 - __app_state_end__);
    __source_bss_free_bytes__ = ABSOLUTE(ORIGIN(RAM) + LENGTH(RAM) - __source_bss_end__);
    ASSERT(__flash_image_end__ <= ORIGIN(FLASH) + LENGTH(FLASH),
           "V3 APP load image exceeds its 64 KiB partition")
    ASSERT(__app_state_end__ <= 0x20002000,
           "source-owned application state exceeds its reserved growth area")
    ASSERT(__source_bss_end__ <= ORIGIN(RAM) + LENGTH(RAM),
           "source-owned BSS exceeds available SRAM")

    /* Alternate layouts use a different fixed-SRAM ordering.  Keep
     * direct constants here as an independent guard around the model layout
     * table above: changing a table entry must not silently move the ABI. */
    ASSERT(!__relocated_sram_layout || IRQ000_Handler == 0x1FFF8104,
    "relocated SRAM IRQ000 RAM entry moved")
    ASSERT(!__relocated_sram_layout || IRQ001_Handler == 0x1FFF8130,
    "relocated SRAM IRQ001 RAM entry moved")
    ASSERT(!__relocated_sram_layout || IRQ002_Handler == 0x1FFF944A,
    "relocated SRAM IRQ002 RAM entry moved")
    ASSERT(!__relocated_sram_layout || IRQ003_Handler == 0x1FFF8278,
    "relocated SRAM IRQ003 RAM entry moved")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.boot_record_writer) == 0x1FFF8000 &&
    ADDR(.helper_fault_monitor) == 0x1FFF9314 &&
    ADDR(.helper_output_sensor) == 0x1FFF9240 && ADDR(.flash_writer) == 0x1FFF9988 &&
    ADDR(.flash_erase_only) == 0x1FFF9A70),
    "relocated SRAM fixed-layout RAM primitive moved")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.helper_mcan_send_classic) == 0x1FFF9B04 &&
    ADDR(.helper_mcan_send_variable_fd) == 0x1FFF9B46 &&
    ADDR(.helper_mcan_send_fd) == 0x1FFF9BD8 && ADDR(.helper_svpwm) == 0x1FFF9C78 &&
    ADDR(.helper_clear_runtime_loop_states) == 0x1FFF9D94 &&
    ADDR(.helper_reset_control_state) == 0x1FFF9E00),
    "relocated SRAM fixed-layout control helper moved")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.helper_identification_filter) == 0x1FFF9E7C &&
    ADDR(.helper_derive_runtime_controller_states) == 0x1FFF9EDE &&
    ADDR(.helper_motion_observer) == 0x1FFFA016 &&
    ADDR(.helper_current_controller) == 0x1FFFA0B2 &&
    ADDR(.helper_rls2) == 0x1FFFA198 && ADDR(.helper_flux_observer) == 0x1FFFA26C),
    "relocated SRAM fixed-layout observer helper moved")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.helper_sincos) == 0x1FFFA344 && ADDR(.helper_clamp) == 0x1FFFA3C2 &&
    ADDR(.helper_wrap) == 0x1FFFA3E2 && ADDR(.helper_limit_vector) == 0x1FFFA44C &&
    ADDR(.helper_float_to_uint) == 0x1FFFA4A2 &&
    ADDR(.helper_uint_to_float) == 0x1FFFA4CA),
    "relocated SRAM fixed-layout arithmetic helper moved")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.helper_derive_control_parameters) == 0x1FFFA514 &&
    ADDR(.helper_delay_ms) == 0x1FFFA51E &&
    ADDR(.helper_select_configuration_bank_b) == 0x1FFFA528 &&
    ADDR(.helper_atan2) == 0x1FFFA532 && ADDR(.helper_sqrt) == 0x1FFFA53C),
    "relocated SRAM fixed-layout leaf helper moved")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.uart_length_state) == 0x1FFFA548 && SIZEOF(.uart_length_state) == 0x4 &&
    ADDR(.hardware_variant) == 0x1FFFA54C && SIZEOF(.hardware_variant) == 0x1 &&
    ADDR(.zero_position_staging) == 0x1FFFA550 &&
    SIZEOF(.zero_position_staging) == 0x8 && ADDR(.config_staging) == 0x1FFFA558 &&
    SIZEOF(.config_staging) == 0x94),
    "relocated SRAM initialized-state prefix layout mismatch")
    ASSERT(!__relocated_sram_layout || (ADDR(.position_sensor_expected) == 0x1FFFA5EC &&
    SIZEOF(.position_sensor_expected) == 0x0B &&
    ADDR(.alternate_runtime_drive_d) == 0x1FFFA5F8 &&
    SIZEOF(.alternate_runtime_drive_d) == 0x28 &&
    ADDR(.alternate_runtime_drive_q) == 0x1FFFA620 &&
    SIZEOF(.alternate_runtime_drive_q) == 0x28 &&
    ADDR(.alternate_runtime_speed_loop) == 0x1FFFA650 &&
    SIZEOF(.alternate_runtime_speed_loop) == 0x28 &&
    ADDR(.alternate_runtime_position_loop) == 0x1FFFA678 &&
    SIZEOF(.alternate_runtime_position_loop) == 0x28),
    "relocated SRAM controller-state layout mismatch")
    ASSERT(!__relocated_sram_layout ||
    (position_dma_word == 0x1FFFA6A0 && ADDR(.sine_table) == 0x1FFFA6AC &&
    SIZEOF(.sine_table) == 0x2004 && ADDR(.aes_sbox) == 0x1FFFC6B0 &&
    SIZEOF(.aes_sbox) == 0x100),
    "relocated SRAM lookup-table layout mismatch")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.boot_record_staging) == 0x1FFFC7B0 && SIZEOF(.boot_record_staging) == 0x14 &&
    ADDR(.short_response_buffer) == 0x1FFFC7C4 &&
    SIZEOF(.short_response_buffer) == 0x14 && ADDR(.motor_correction) == 0x1FFFC7D8 &&
    SIZEOF(.motor_correction) == 0x400 && ADDR(.can_protocol_scratch) == 0x1FFFCBD8 &&
    SIZEOF(.can_protocol_scratch) == 0x20),
    "relocated SRAM calibration-staging layout mismatch")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.motor_record) == 0x1FFFCBF8 && SIZEOF(.motor_record) == 0x40C &&
    ADDR(.output_table) == 0x1FFFD004 && SIZEOF(.output_table) == 0x2000 &&
    ADDR(.output_calibration) == 0x1FFFF004 && SIZEOF(.output_calibration) == 0x10),
    "relocated SRAM persistent-record layout mismatch")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.motor_runtime_state) == 0x1FFFF014 && SIZEOF(.motor_runtime_state) == 0x7C &&
    ADDR(.sample_runtime_state) == 0x1FFFF090 &&
    SIZEOF(.sample_runtime_state) == 0xA4 && ADDR(.output_sensor_state) == 0x1FFFF134 &&
    SIZEOF(.output_sensor_state) == 0x48 && ADDR(.runtime_status) == 0x1FFFF17C &&
    SIZEOF(.runtime_status) == 0x4C),
    "relocated SRAM runtime-state layout mismatch")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.runtime_parameter_cache) == 0x1FFFF1C8 &&
    SIZEOF(.runtime_parameter_cache) == 0x60 && ADDR(.mcan_dispatch) == 0x1FFFF228 &&
    SIZEOF(.mcan_dispatch) == 0xA4 && ADDR(.calibration_frame_scratch) == 0x1FFFF2CC &&
    SIZEOF(.calibration_frame_scratch) == 0x40 &&
    ADDR(.motion_observer) == 0x1FFFF30C && SIZEOF(.motion_observer) == 0x3C),
    "relocated SRAM protocol-state layout mismatch")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.alternate_adc_raw) == 0x1FFFF348 && SIZEOF(.alternate_adc_raw) == 0x14 &&
    ADDR(.alternate_current_d) == 0x1FFFF35C && SIZEOF(.alternate_current_d) == 0x4C &&
    ADDR(.alternate_current_q) == 0x1FFFF3A8 && SIZEOF(.alternate_current_q) == 0x4C &&
    ADDR(.position_sensor_observed) == 0x1FFFF3F4 &&
    SIZEOF(.position_sensor_observed) == 0x0B && ADDR(.uart_rx_buffer) == 0x1FFFF3FF &&
    SIZEOF(.uart_rx_buffer) == 0xC8),
    "relocated SRAM sampled-state tail layout mismatch")
    ASSERT(!__relocated_sram_layout ||
    (ADDR(.c_runtime_state) == 0x1FFFF4C8 && SIZEOF(.c_runtime_state) == 0x60 &&
    ADDR(.system_core_clock) == 0x1FFFF528 && SIZEOF(.system_core_clock) == 0x4 &&
    ADDR(.hrc_value) == 0x1FFFF52C && SIZEOF(.hrc_value) == 0x4 &&
    __image_bss_end__ == 0x1FFFF530 && __HeapBase == 0x1FFFF530 &&
    __HeapLimit == 0x1FFFF930 && __StackTop == 0x20000530),
    "relocated SRAM C runtime/heap/stack layout mismatch")
    ASSERT(!__relocated_sram_layout || ADDR(.bss) >= __StackTop,
    "relocated SRAM source BSS overlaps the firmware stack")
    ASSERT(!__shifted_sram_layout ||
    (IRQ000_Handler == 0x1FFF8744 && IRQ001_Handler == 0x1FFF8770 &&
    IRQ002_Handler == 0x1FFF8136 && IRQ003_Handler == 0x1FFF88B8),
    "shifted SRAM IRQ layout mismatch")
    ASSERT(!__shifted_sram_layout ||
    (ADDR(.helper_output_sensor) == __layout_helper_output_sensor &&
    ADDR(.flash_writer) == __layout_flash_writer &&
    ADDR(.flash_erase_only) == __layout_flash_erase_only &&
    ADDR(.helper_mcan_send_classic) == __layout_helper_mcan_send_classic &&
    ADDR(.helper_mcan_send_variable_fd) == __layout_helper_mcan_send_variable_fd &&
    ADDR(.helper_mcan_send_fd) == __layout_helper_mcan_send_fd),
    "shifted SRAM fixed helper layout mismatch")
    ASSERT(
    !__shifted_sram_layout ||
    (ADDR(.helper_svpwm) == __layout_helper_svpwm &&
    ADDR(.helper_clear_runtime_loop_states) == __layout_helper_clear_runtime_loop_states &&
    ADDR(.helper_reset_control_state) == __layout_helper_reset_control_state &&
    ADDR(.helper_identification_filter) == __layout_helper_identification_filter &&
    ADDR(.helper_motion_observer) == __layout_helper_motion_observer &&
    ADDR(.helper_current_controller) == __layout_helper_current_controller),
    "shifted SRAM control helper layout mismatch")
    ASSERT(!__shifted_sram_layout ||
    (ADDR(.runtime_drive_d) == __layout_runtime_drive_d &&
    ADDR(.runtime_drive_q) == __layout_runtime_drive_q &&
    ADDR(.runtime_speed_loop) == __layout_runtime_speed_loop &&
    ADDR(.runtime_position_loop) == __layout_runtime_position_loop &&
    ADDR(.config_staging) == __layout_config_staging &&
    ADDR(.sine_table) == __layout_sine_table && ADDR(.aes_sbox) == __layout_aes_sbox),
    "shifted SRAM initialized-state layout mismatch")
    ASSERT(!__shifted_sram_layout ||
    (ADDR(.adc_raw) == __layout_adc_raw && ADDR(.current_d) == __layout_current_d &&
    ADDR(.current_q) == __layout_current_q &&
    ADDR(.boot_record_staging) == __layout_boot_record_staging &&
    ADDR(.output_table) == __layout_output_table &&
    ADDR(.runtime_status) == __layout_runtime_status &&
    ADDR(.c_runtime_state) == __layout_c_runtime_state),
    "shifted SRAM BSS object layout mismatch")
    ASSERT(!__shifted_sram_layout ||
    (__image_bss_end__ == __layout_bss_end && __HeapBase == __layout_bss_end &&
    __HeapLimit == __layout_heap_limit && __StackTop == __layout_stack_top),
    "shifted SRAM runtime/heap/stack layout mismatch")
    ASSERT(!__shifted_sram_layout || ADDR(.bss) >= __StackTop,
    "shifted SRAM source BSS overlaps the firmware stack")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || IRQ000_Handler == 0x1FFF8744,
    "standard V3 IRQ000 RAM entry moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || IRQ001_Handler == 0x1FFF8770,
    "standard V3 IRQ001 RAM entry moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || IRQ002_Handler == 0x1FFF8136,
    "standard V3 IRQ002 RAM entry moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || IRQ003_Handler == 0x1FFF88B8,
    "standard V3 IRQ003 RAM entry moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_fault_monitor) == 0x1FFF8000,
    "standard V3 fault-monitor helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_fault_monitor) <= 0x136,
    "standard V3 fault-monitor veneer crossed ADC IRQ")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_output_sensor) == 0xB8,
    "standard V3 output-sensor helper size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.flash_writer) == 0x1FFF9950,
    "standard V3 flash writer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.flash_erase_only) == 0x1FFF9A38,
    "standard V3 erase-only primitive moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.flash_erase_only) <= 0x94,
    "standard V3 erase-only primitive crossed classic send helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_clear_runtime_loop_states) == 0x1FFF9D5C,
    "standard V3 clear-loop-state helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_clear_runtime_loop_states) <= 0x6C,
    "standard V3 clear-loop-state helper crossed selective reset")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.boot_record_writer) == 0x1FFF8640,
    "standard V3 fixed boot-record writer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.boot_record_writer) <= (0x1FFF8744 - 0x1FFF8640),
    "standard V3 fixed boot-record writer exceeds its fixed-layout slot")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    mcan_send_classic_helper == 0x1FFF9ACC,
    "standard V3 classic MCAN callback moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_mcan_send_classic) == 0x0C,
    "standard V3 classic MCAN callback veneer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || mcan_send_fd_helper == 0x1FFF9BA0,
    "standard V3 FD MCAN callback moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_mcan_send_fd) == 0x10,
    "standard V3 FD MCAN callback veneer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_mcan_send_variable_fd) == 0x1FFF9B0E,
    "standard V3 variable-length FD sender moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_mcan_send_variable_fd) == 0x0C,
    "standard V3 variable-length FD sender copy size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.helper_svpwm) == 0x10,
    "standard V3 SVPWM veneer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.irq002) <= (0x1FFF8640 - 0x1FFF8136),
    "standard V3 IRQ002 source veneer exceeds its fixed-layout slot")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.irq000) <= (0x1FFF8770 - 0x1FFF8744),
    "standard V3 IRQ000 source veneer exceeds its fixed-layout slot")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.irq001) <= (0x1FFF88B8 - 0x1FFF8770),
    "standard V3 IRQ001 source veneer exceeds its fixed-layout slot")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.flash_writer) <= (0x1FFF9A38 - 0x1FFF9950),
    "standard V3 flash writer exceeds its fixed-layout slot")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.sine_table) == 0x1FFFA674,
    "standard V3 sine table moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.runtime_drive_d) == 0x1FFFA510,
    "standard V3 D runtime drive state moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.runtime_drive_d) == 0x28,
    "standard V3 D runtime drive state size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.runtime_drive_q) == 0x1FFFA538,
    "standard V3 Q runtime drive state moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.runtime_drive_q) == 0x28,
    "standard V3 Q runtime drive state size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.runtime_speed_loop) == 0x1FFFA568,
    "standard V3 speed-loop state moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.runtime_speed_loop) == 0x28,
    "standard V3 speed-loop state size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.runtime_position_loop) == 0x1FFFA590,
    "standard V3 position-loop state moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.runtime_position_loop) == 0x28,
    "standard V3 position-loop state size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.config_staging) == 0x1FFFA5C8,
    "standard V3 config staging record moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.config_staging) == 0x94,
    "standard V3 config staging record size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.uart_length_state) == 0x1FFFA5B8,
    "standard V3 UART length state moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.uart_length_state) == 0x4,
    "standard V3 UART length state size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.hardware_variant) == 0x1FFFA5BC,
    "standard V3 hardware variant moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.hardware_variant) == 0x1,
    "standard V3 hardware variant size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_identification_filter) == 0x1FFF9E44,
    "standard V3 identification-filter helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_identification_filter) <= 0x31C,
    "standard V3 identification-filter veneer crossed next helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_output_sensor) == 0x1FFF987C,
    "standard V3 output-sensor helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_output_sensor) <= 0xD4,
    "standard V3 output-sensor veneer crossed flash writer")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.helper_svpwm) == 0x1FFF9C40,
    "standard V3 SVPWM helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.helper_svpwm) <= 0x204,
    "standard V3 SVPWM veneer crossed identification helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_reset_control_state) == 0x1FFF9DC8,
    "standard V3 control-reset helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_reset_control_state) == 0x10,
    "standard V3 control-reset veneer copy size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_motion_observer) == 0x1FFF9FDE,
    "standard V3 motion-observer helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_motion_observer) <= 0x9C,
    "standard V3 motion-observer veneer crossed current helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_current_controller) == 0x1FFFA07A,
    "standard V3 current-controller helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_current_controller) <= 0x9E,
    "standard V3 current-controller veneer crossed literal pool")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.helper_rls2) == 0x1FFFA160,
    "standard V3 RLS helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.helper_rls2) <= 0xD4,
    "standard V3 RLS veneer crossed next helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_flux_observer) == 0x1FFFA234,
    "standard V3 flux-observer helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_flux_observer) <= 0xD8,
    "standard V3 flux-observer veneer crossed next helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.helper_sincos) == 0x1FFFA30C,
    "standard V3 sincos helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.helper_sincos) <= 0x7E,
    "standard V3 sincos veneer crossed next helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.helper_clamp) == 0x1FFFA38A,
    "standard V3 clamp helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.helper_clamp) <= 0x20,
    "standard V3 clamp veneer crossed next helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.helper_wrap) == 0x1FFFA3AA,
    "standard V3 wrap helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.helper_wrap) == 0x6A,
    "standard V3 wrap and angle-reduction bodies changed size")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_limit_vector) == 0x1FFFA414,
    "standard V3 vector-limit helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_limit_vector) <= 0x56,
    "standard V3 vector-limit veneer crossed conversion helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_float_to_uint) == 0x1FFFA46A,
    "standard V3 float-to-uint helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_float_to_uint) <= 0x28,
    "standard V3 float-to-uint veneer crossed inverse conversion")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_uint_to_float) == 0x1FFFA492,
    "standard V3 uint-to-float helper moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_uint_to_float) <= 0x4A,
    "standard V3 uint-to-float veneer crossed derive-control helper")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_derive_control_parameters) == 0x1FFFA4DC,
    "standard V3 derive-control veneer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_derive_control_parameters) == 0xA,
    "standard V3 derive-control veneer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.helper_atan2) == 0x1FFFA4FA,
    "standard V3 atan2 veneer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.helper_atan2) == 0xA,
    "standard V3 atan2 veneer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_delay_ms) == 0x1FFFA4E6,
    "standard V3 delay-ms veneer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.helper_delay_ms) == 0xA,
    "standard V3 delay-ms veneer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.helper_select_configuration_bank_b) == 0x1FFFA4F0,
    "standard V3 bank-B veneer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.helper_select_configuration_bank_b) == 0xA,
    "standard V3 bank-B veneer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.helper_sqrt) == 0x1FFFA504,
    "standard V3 sqrt veneer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.helper_sqrt) == 0xC,
    "standard V3 sqrt veneer plus scatter tail size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.c_runtime_state) == 0x1FFFF490,
    "standard V3 C runtime state moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.c_runtime_state) == 0x60,
    "standard V3 C runtime state size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.system_core_clock) == 0x1FFFF4F0,
    "standard V3 SystemCoreClock moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.system_core_clock) == 0x4,
    "standard V3 SystemCoreClock size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.hrc_value) == 0x1FFFF4F4,
    "standard V3 HRC_VALUE moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.hrc_value) == 0x4,
    "standard V3 HRC_VALUE size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || __image_bss_end__ == 0x1FFFF4F8,
    "standard V3 firmware BSS/heap boundary moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || __HeapBase == 0x1FFFF4F8,
    "standard V3 heap base moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || __HeapLimit == 0x1FFFF8F8,
    "standard V3 heap limit moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.sine_table) == 0x2004,
    "standard V3 sine table size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.aes_sbox) == 0x1FFFC678,
    "standard V3 AES S-box moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.aes_sbox) == 0x100,
    "standard V3 AES S-box size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.position_sensor_expected) == 0x1FFFA65C,
    "standard V3 position-sensor expected register image moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.position_sensor_expected) == 0x0C,
    "standard V3 position-sensor shared storage size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || position_dma_word == 0x1FFFA666,
    "standard V3 position DMA destination moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.adc_raw) == 0x1FFFC778 && SIZEOF(.adc_raw) == 0x14,
    "standard V3 ADC raw sample layout mismatch")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.current_d) == 0x1FFFC78C,
    "standard V3 D current controller moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.current_d) == 0x4C,
    "standard V3 D current controller size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.current_q) == 0x1FFFC7D8,
    "standard V3 Q current controller moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.current_q) == 0x4C,
    "standard V3 Q current controller size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.boot_record_staging) == 0x1FFFC824,
    "standard V3 boot-record staging moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.boot_record_staging) == 0x14,
    "standard V3 boot-record staging size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.short_response_buffer) == 0x1FFFC838,
    "standard V3 short response buffer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.short_response_buffer) == 0x14,
    "standard V3 short response buffer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.motor_correction) == 0x1FFFC84C,
    "standard V3 motor correction table moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.motor_correction) == 0x400,
    "standard V3 motor correction table size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.motor_record) == 0x1FFFCC6C,
    "standard V3 motor record moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.motor_record) == 0x40C,
    "standard V3 motor record size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.output_table) == 0x1FFFD078,
    "standard V3 output table moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.output_table) == 0x2000,
    "standard V3 output table size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.output_calibration) == 0x1FFFF078,
    "standard V3 output calibration moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.output_calibration) == 0x10,
    "standard V3 output calibration size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.motor_runtime_state) == 0x1FFFF088,
    "standard V3 motor runtime state moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.motor_runtime_state) == 0x7C,
    "standard V3 motor runtime state size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.sample_runtime_state) == 0x1FFFF104,
    "standard V3 sample runtime state moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.sample_runtime_state) == 0xA4,
    "standard V3 sample runtime state size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.output_sensor_state) == 0x1FFFF1A8,
    "standard V3 output sensor state moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.output_sensor_state) == 0x48,
    "standard V3 output sensor state slot changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.runtime_status) == 0x1FFFF1F0,
    "standard V3 runtime status moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.runtime_status) == 0x4C,
    "standard V3 runtime status size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.runtime_parameter_cache) == 0x1FFFF23C,
    "standard V3 runtime parameter cache moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.runtime_parameter_cache) == 0x60,
    "standard V3 runtime parameter cache size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.mcan_dispatch) == 0x1FFFF29C,
    "standard V3 MCAN dispatch moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.mcan_dispatch) == 0xA4,
    "standard V3 MCAN runtime workspace size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.calibration_frame_scratch) == 0x1FFFF340,
    "standard V3 calibration frame scratch moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.calibration_frame_scratch) == 0x40,
    "standard V3 calibration frame scratch size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.motion_observer) == 0x1FFFF380,
    "standard V3 motion observer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.motion_observer) == 0x3C,
    "standard V3 motion observer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.position_sensor_observed) == 0x1FFFF3BC,
    "standard V3 position-sensor observed register image moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    SIZEOF(.position_sensor_observed) == 0x0B,
    "standard V3 position-sensor observed register image size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    ADDR(.uart_rx_buffer) == 0x1FFFF3C7,
    "standard V3 UART RX buffer moved")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.uart_rx_buffer) == 0xC8,
    "standard V3 UART RX buffer size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || SIZEOF(.app_state) == 0x320,
    "standard V3 source app-state size changed")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) || ADDR(.bss) >= __StackTop,
    "standard V3 source BSS overlaps the firmware stack")
    ASSERT((__relocated_sram_layout || __shifted_sram_layout) ||
    __etext_ramb + SIZEOF(.ramb_data) <= ORIGIN(FLASH) + LENGTH(FLASH),
    "app FLASH overflow")
}
