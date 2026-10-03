MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00020000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x1FFF8000, LENGTH = 64K
    RAMB  (rwx) : ORIGIN = 0x200F0000, LENGTH = 4K
}

ENTRY(Reset_Handler)

/* V3 firmwares share one recovered source graph but not one SRAM ABI.  Keep
 * every address in this table so profile differences remain reviewable and
 * source modules do not invent local offsets. */
__dm8009_layout = DEFINED(DAMIAO_DM8009_LAYOUT); /* DM8006/DM8009 family */
__dm43_48v_layout = DEFINED(DAMIAO_DM43_48V_LAYOUT);

__layout_helper_fault_monitor = __dm8009_layout ? 0x1FFF9314 : 0x1FFF8000;
__layout_irq002 = __dm8009_layout ? 0x1FFF944A : 0x1FFF8136;
__layout_boot_record_writer = __dm8009_layout ? 0x1FFF8000 : 0x1FFF8640;
__layout_irq000 = __dm8009_layout ? 0x1FFF8104 : 0x1FFF8744;
__layout_irq001 = __dm8009_layout ? 0x1FFF8130 : 0x1FFF8770;
__layout_irq003 = __dm8009_layout ? 0x1FFF8278 : 0x1FFF88B8;
__layout_helper_output_sensor = __dm8009_layout ? 0x1FFF9240 : (__dm43_48v_layout ? 0x1FFF9880 : 0x1FFF987C);
__layout_flash_writer = __dm8009_layout ? 0x1FFF9988 : (__dm43_48v_layout ? 0x1FFF9954 : 0x1FFF9950);
__layout_flash_erase_only = __dm8009_layout ? 0x1FFF9A70 : (__dm43_48v_layout ? 0x1FFF9A3C : 0x1FFF9A38);
__layout_helper_mcan_send_classic = __dm8009_layout ? 0x1FFF9B04 : (__dm43_48v_layout ? 0x1FFF9AD0 : 0x1FFF9ACC);
__layout_helper_mcan_send_variable_fd = __dm8009_layout ? 0x1FFF9B46 : (__dm43_48v_layout ? 0x1FFF9B12 : 0x1FFF9B0E);
__layout_helper_mcan_send_fd = __dm8009_layout ? 0x1FFF9BD8 : (__dm43_48v_layout ? 0x1FFF9BA4 : 0x1FFF9BA0);
__layout_helper_svpwm = __dm8009_layout ? 0x1FFF9C78 : (__dm43_48v_layout ? 0x1FFF9C44 : 0x1FFF9C40);
__layout_helper_clear_runtime_loop_states = __dm8009_layout ? 0x1FFF9D94 : (__dm43_48v_layout ? 0x1FFF9D60 : 0x1FFF9D5C);
__layout_helper_reset_control_state = __dm8009_layout ? 0x1FFF9E00 : (__dm43_48v_layout ? 0x1FFF9DCC : 0x1FFF9DC8);
__layout_helper_identification_filter = __dm8009_layout ? 0x1FFF9E7C : (__dm43_48v_layout ? 0x1FFF9E48 : 0x1FFF9E44);
__layout_helper_derive_runtime_controller_states = __dm8009_layout ? 0x1FFF9EDE : (__dm43_48v_layout ? 0x1FFF9EAA : 0x1FFF9EA6);
__layout_helper_motion_observer = __dm8009_layout ? 0x1FFFA016 : (__dm43_48v_layout ? 0x1FFF9FE2 : 0x1FFF9FDE);
__layout_helper_current_controller = __dm8009_layout ? 0x1FFFA0B2 : (__dm43_48v_layout ? 0x1FFFA07E : 0x1FFFA07A);
__layout_helper_rls2 = __dm8009_layout ? 0x1FFFA198 : (__dm43_48v_layout ? 0x1FFFA164 : 0x1FFFA160);
__layout_helper_flux_observer = __dm8009_layout ? 0x1FFFA26C : (__dm43_48v_layout ? 0x1FFFA238 : 0x1FFFA234);
__layout_helper_sincos = __dm8009_layout ? 0x1FFFA344 : (__dm43_48v_layout ? 0x1FFFA310 : 0x1FFFA30C);
__layout_helper_clamp = __dm8009_layout ? 0x1FFFA3C2 : (__dm43_48v_layout ? 0x1FFFA38E : 0x1FFFA38A);
__layout_helper_wrap = __dm8009_layout ? 0x1FFFA3E2 : (__dm43_48v_layout ? 0x1FFFA3AE : 0x1FFFA3AA);
__layout_helper_limit_vector = __dm8009_layout ? 0x1FFFA44C : (__dm43_48v_layout ? 0x1FFFA418 : 0x1FFFA414);
__layout_helper_float_to_uint = __dm8009_layout ? 0x1FFFA4A2 : (__dm43_48v_layout ? 0x1FFFA46E : 0x1FFFA46A);
__layout_helper_uint_to_float = __dm8009_layout ? 0x1FFFA4CA : (__dm43_48v_layout ? 0x1FFFA496 : 0x1FFFA492);
__layout_helper_derive_control_parameters = __dm8009_layout ? 0x1FFFA514 : (__dm43_48v_layout ? 0x1FFFA4E0 : 0x1FFFA4DC);
__layout_helper_delay_ms = __dm8009_layout ? 0x1FFFA51E : (__dm43_48v_layout ? 0x1FFFA4EA : 0x1FFFA4E6);
__layout_helper_select_configuration_bank_b = __dm8009_layout ? 0x1FFFA528 : (__dm43_48v_layout ? 0x1FFFA4F4 : 0x1FFFA4F0);
__layout_helper_atan2 = __dm8009_layout ? 0x1FFFA532 : (__dm43_48v_layout ? 0x1FFFA4FE : 0x1FFFA4FA);
__layout_helper_sqrt = __dm8009_layout ? 0x1FFFA53C : (__dm43_48v_layout ? 0x1FFFA508 : 0x1FFFA504);

__layout_literals_1fff8630 = __dm8009_layout ? 0x1FFF9978 : 0x1FFF8630;
__layout_literals_1fff8724 = __dm8009_layout ? 0x1FFF80E4 : 0x1FFF8724;
__layout_literals_1fff9830 = __dm8009_layout ? 0x1FFF91F0 : 0x1FFF9830;
__layout_literals_1fff9934 = __dm8009_layout ? 0x1FFF92F8 : (__dm43_48v_layout ? 0x1FFF9938 : 0x1FFF9934);
__layout_literals_1fff9ac0 = __dm8009_layout ? 0x1FFF9AF8 : (__dm43_48v_layout ? 0x1FFF9AC4 : 0x1FFF9AC0);
__layout_literals_1fff9c34 = __dm8009_layout ? 0x1FFF9C6C : (__dm43_48v_layout ? 0x1FFF9C38 : 0x1FFF9C34);
__layout_literals_1fff9d4c = __dm8009_layout ? 0x1FFF9D84 : (__dm43_48v_layout ? 0x1FFF9D50 : 0x1FFF9D4C);
__layout_literals_1fffa118 = __dm8009_layout ? 0x1FFFA150 : (__dm43_48v_layout ? 0x1FFFA11C : 0x1FFFA118);
__layout_literals_1fffa13c = __dm8009_layout ? 0x1FFFA174 : (__dm43_48v_layout ? 0x1FFFA140 : 0x1FFFA13C);
__layout_filter_literal = __dm8009_layout ? 0x1FFFA170 : (__dm43_48v_layout ? 0x1FFFA13C : 0x1FFFA138);
__layout_motion_literal = __dm8009_layout ? 0x1FFFA194 : (__dm43_48v_layout ? 0x1FFFA160 : 0x1FFFA15C);
__layout_rls_literals = __dm8009_layout ? 0x1FFFA338 : (__dm43_48v_layout ? 0x1FFFA304 : 0x1FFFA300);

__layout_initialized_state = __dm8009_layout ? 0x1FFFA548 : (__dm43_48v_layout ? 0x1FFFA514 : 0x1FFFA510);
__layout_runtime_drive_d = __dm8009_layout ? 0x1FFFA5F8 : (__dm43_48v_layout ? 0x1FFFA514 : 0x1FFFA510);
__layout_runtime_drive_q = __dm8009_layout ? 0x1FFFA620 : (__dm43_48v_layout ? 0x1FFFA53C : 0x1FFFA538);
__layout_runtime_speed_loop = __dm8009_layout ? 0x1FFFA650 : (__dm43_48v_layout ? 0x1FFFA56C : 0x1FFFA568);
__layout_runtime_position_loop = __dm8009_layout ? 0x1FFFA678 : (__dm43_48v_layout ? 0x1FFFA594 : 0x1FFFA590);
__layout_uart_length_state = __dm8009_layout ? 0x1FFFA548 : (__dm43_48v_layout ? 0x1FFFA5BC : 0x1FFFA5B8);
__layout_hardware_variant = __dm8009_layout ? 0x1FFFA54C : (__dm43_48v_layout ? 0x1FFFA5C0 : 0x1FFFA5BC);
__layout_zero_position_staging = __dm8009_layout ? 0x1FFFA550 : (__dm43_48v_layout ? 0x1FFFA5C4 : 0x1FFFA5C0);
__layout_config_staging = __dm8009_layout ? 0x1FFFA558 : (__dm43_48v_layout ? 0x1FFFA5CC : 0x1FFFA5C8);
__layout_position_sensor_expected = __dm8009_layout ? 0x1FFFA5EC : (__dm43_48v_layout ? 0x1FFFA660 : 0x1FFFA65C);
__layout_position_dma_word = __dm8009_layout ? 0x1FFFA6A0 : (__dm43_48v_layout ? 0x1FFFA66A : 0x1FFFA666);
__layout_sine_table = __dm8009_layout ? 0x1FFFA6AC : (__dm43_48v_layout ? 0x1FFFA678 : 0x1FFFA674);
__layout_aes_sbox = __dm8009_layout ? 0x1FFFC6B0 : (__dm43_48v_layout ? 0x1FFFC67C : 0x1FFFC678);

__layout_boot_record_staging = __dm8009_layout ? 0x1FFFC7B0 : (__dm43_48v_layout ? 0x1FFFC828 : 0x1FFFC824);
__layout_short_response_buffer = __dm8009_layout ? 0x1FFFC7C4 : (__dm43_48v_layout ? 0x1FFFC83C : 0x1FFFC838);
__layout_motor_correction = __dm8009_layout ? 0x1FFFC7D8 : (__dm43_48v_layout ? 0x1FFFC850 : 0x1FFFC84C);
__layout_can_protocol_scratch = __dm8009_layout ? 0x1FFFCBD8 : (__dm43_48v_layout ? 0x1FFFCC50 : 0x1FFFCC4C);
__layout_motor_record = __dm8009_layout ? 0x1FFFCBF8 : (__dm43_48v_layout ? 0x1FFFCC70 : 0x1FFFCC6C);
__layout_output_table = __dm8009_layout ? 0x1FFFD004 : (__dm43_48v_layout ? 0x1FFFD07C : 0x1FFFD078);
__layout_output_calibration = __dm8009_layout ? 0x1FFFF004 : (__dm43_48v_layout ? 0x1FFFF07C : 0x1FFFF078);
__layout_motor_runtime_state = __dm8009_layout ? 0x1FFFF014 : (__dm43_48v_layout ? 0x1FFFF08C : 0x1FFFF088);
__layout_sample_runtime_state = __dm8009_layout ? 0x1FFFF090 : (__dm43_48v_layout ? 0x1FFFF108 : 0x1FFFF104);
__layout_output_sensor_state = __dm8009_layout ? 0x1FFFF134 : (__dm43_48v_layout ? 0x1FFFF1AC : 0x1FFFF1A8);
__layout_runtime_status = __dm8009_layout ? 0x1FFFF17C : (__dm43_48v_layout ? 0x1FFFF1F4 : 0x1FFFF1F0);
__layout_runtime_parameter_cache = __dm8009_layout ? 0x1FFFF1C8 : (__dm43_48v_layout ? 0x1FFFF240 : 0x1FFFF23C);
__layout_mcan_dispatch = __dm8009_layout ? 0x1FFFF228 : (__dm43_48v_layout ? 0x1FFFF2A0 : 0x1FFFF29C);
__layout_calibration_frame_scratch = __dm8009_layout ? 0x1FFFF2CC : (__dm43_48v_layout ? 0x1FFFF344 : 0x1FFFF340);
__layout_motion_observer = __dm8009_layout ? 0x1FFFF30C : (__dm43_48v_layout ? 0x1FFFF384 : 0x1FFFF380);
__layout_adc_raw = __dm8009_layout ? 0x1FFFF348 : (__dm43_48v_layout ? 0x1FFFC77C : 0x1FFFC778);
__layout_current_d = __dm8009_layout ? 0x1FFFF35C : (__dm43_48v_layout ? 0x1FFFC790 : 0x1FFFC78C);
__layout_current_q = __dm8009_layout ? 0x1FFFF3A8 : (__dm43_48v_layout ? 0x1FFFC7DC : 0x1FFFC7D8);
__layout_position_sensor_observed = __dm8009_layout ? 0x1FFFF3F4 : (__dm43_48v_layout ? 0x1FFFF3C0 : 0x1FFFF3BC);
__layout_uart_rx_buffer = __dm8009_layout ? 0x1FFFF3FF : (__dm43_48v_layout ? 0x1FFFF3CB : 0x1FFFF3C7);
__layout_c_runtime_state = __dm8009_layout ? 0x1FFFF4C8 : (__dm43_48v_layout ? 0x1FFFF494 : 0x1FFFF490);
__layout_system_core_clock = __dm8009_layout ? 0x1FFFF528 : (__dm43_48v_layout ? 0x1FFFF4F4 : 0x1FFFF4F0);
__layout_hrc_value = __dm8009_layout ? 0x1FFFF52C : (__dm43_48v_layout ? 0x1FFFF4F8 : 0x1FFFF4F4);
__layout_factory_bss_end = __dm8009_layout ? 0x1FFFF530 : (__dm43_48v_layout ? 0x1FFFF500 : 0x1FFFF4F8);
__layout_heap_limit = __dm8009_layout ? 0x1FFFF930 : (__dm43_48v_layout ? 0x1FFFF900 : 0x1FFFF8F8);
__layout_stack_top = __dm8009_layout ? 0x20000530 : (__dm43_48v_layout ? 0x20000500 : 0x200004F8);

SECTIONS
{
    .vectors : {
        . = ALIGN(4);
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > FLASH

    .text : {
        . = ALIGN(4);
        /* Development/source builds use ordinary sequential Flash placement.
         * Fixed SRAM VMAs below remain ABI contracts; Flash function addresses
         * are deliberately free to move as source code evolves. */
        KEEP(*(.dm4310_runtime_main_entry))
        *(EXCLUDE_FILE (*dm4310_extended_core.S.obj) .text*)
        KEEP(*(.dm4310_mcan_init_classic))
        KEEP(*(.dm4310_mcan_transition_delay))
        KEEP(*(.dm4310_mcan_init_fd))
        KEEP(*(.dm4310_irq004))

        /* The recovered arithmetic implementation remains isolated from C,
         * but its Flash base is sequential.  Keep only the internal ordering
         * required by its hand-audited instruction/literal relationships. */
        KEEP(*(.text.dm4310_factory_runtime_round_to_int))
        KEEP(*(.text.dm4310_factory_extended_divide_core))
        KEEP(*(.text.dm4310_factory_extended_round_core))
        KEEP(*(.text.dm4310_factory_extended_divide_entry))
        KEEP(*(.text.dm4310_factory_extended_multiply_entry))
        KEEP(*(.text.dm4310_factory_extended_multiply_core))
        *(.glue_7 .glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } > FLASH

    .dm4310_factory_d2f : {
        KEEP(*(.text.dm4310_factory_d2f))
        . = 0x00000058;
        KEEP(*(.text.dm4310_factory_d2f_exception_result))
        . = 0x00000064;
    } > FLASH
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_factory_d2f) == 0x64,
           "DM4310 double-to-float runtime layout changed")

    /* Keep the recovered ARM runtime outside .text so GNU ld can put its
     * source-only long-call veneers in the free bank before 0x270d8. */
    .dm4310_factory_runtime : {
        KEEP(*(.text.dm4310_factory_dadd))
        . = 0x00000150;
        KEEP(*(.text.dm4310_factory_ddiv))
        . = 0x00000378;
        __dm4310_division_seeds_start__ = .;
        KEEP(*(.dm4310_division_seeds))
        __dm4310_division_seeds_end__ = .;
        KEEP(*(.rodata.dm4310_factory_ddiv_literals))
        . = 0x00000400;
        KEEP(*(.text.dm4310_factory_d2uiz))
        . = 0x00000458;
        KEEP(*(.text.dm4310_factory_d2uiz_saturate))
        . = 0x0000045A;
        KEEP(*(.text.dm4310_factory_ui2d))
        . = 0x00000480;
        KEEP(*(.text.dm4310_factory_dmul))
        . = 0x000005D4;
        KEEP(*(.text.dm4310_factory_softdouble_exception))
        . = 0x0000067C;
        KEEP(*(.text.dm4310_factory_reverse_subtract))
        . = 0x0000082C;
        KEEP(*(.text.dm4310_factory_dsub))
        . = 0x00000A00;
        KEEP(*(.text.dm4310_factory_f2d))
        . = 0x00000A48;
        KEEP(*(.text.dm4310_factory_f2d_exception_result))
        . = 0x00000A56;
        KEEP(*(.text.dm4310_factory_float_to_int))
        . = 0x00000A64;
        KEEP(*(.text.dm4310_factory_uint64_to_float_entry))
        . = 0x00000A7E;
        KEEP(*(.text.dm4310_factory_uint64_to_float_core))
        . = 0x00000AC4;
        KEEP(*(.text.dm4310_factory_softfloat_exception))
        . = 0x00000B5A;
        KEEP(*(.text.dm4310_factory_d2f_pack))
        . = 0x00000B64;
        KEEP(*(.text.dm4310_factory_round_even))
    } > FLASH

    ASSERT(__dm8009_layout || __dm4310_division_seeds_end__ - __dm4310_division_seeds_start__ == 0x80,
           "DM4310 reciprocal seed table must contain 128 bytes")
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_factory_runtime) == 0xBC8,
           "DM4310 factory arithmetic runtime layout changed")

    .dm4310_temperature_table : {
        KEEP(*(.rodata.temperature_celsius_table))
    } > FLASH
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_temperature_table) == 0x400,
           "DM4310 temperature table must retain all 256 factory words")

    .dm4310_c_locale_name : {
        KEEP(*(.dm4310_c_locale_name))
    } > FLASH

    .dm4310_c_locale : {
        KEEP(*(.dm4310_c_locale))
    } > FLASH

    .text_platform : {
        . = ALIGN(4);
        *(.text .text*)
        . = ALIGN(4);
    } > FLASH

    .rodata : {
        . = ALIGN(4);
        __dm4310_literal_copies_start__ = .;
        LONG(__dm4310_literals_1fff8630_load__);
        LONG(ADDR(.dm4310_literals_1fff8630));
        LONG(4);
        LONG(__dm4310_literals_1fff8724_load__);
        LONG(ADDR(.dm4310_literals_1fff8724));
        LONG(8);
        LONG(__dm4310_literals_1fff9830_load__);
        LONG(ADDR(.dm4310_literals_1fff9830));
        LONG(19);
        LONG(__dm4310_literals_1fff9934_load__);
        LONG(ADDR(.dm4310_literals_1fff9934));
        LONG(7);
        LONG(__dm4310_literals_1fff9ac0_load__);
        LONG(ADDR(.dm4310_literals_1fff9ac0));
        LONG(3);
        LONG(__dm4310_literals_1fff9c34_load__);
        LONG(ADDR(.dm4310_literals_1fff9c34));
        LONG(3);
        LONG(__dm4310_literals_1fff9d4c_load__);
        LONG(ADDR(.dm4310_literals_1fff9d4c));
        LONG(4);
        LONG(__dm4310_literals_1fffa118_load__);
        LONG(ADDR(.dm4310_literals_1fffa118));
        LONG(8);
        LONG(__dm4310_literals_1fffa13c_load__);
        LONG(ADDR(.dm4310_literals_1fffa13c));
        LONG(8);
        __dm4310_literal_copies_end__ = .;
        *(.rodata)
        *(EXCLUDE_FILE (*aes256_ctr.c.obj) .rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    __exidx_start = .;
    .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } > FLASH
    __exidx_end = .;

    .preinit_array (READONLY) : {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH
    .init_array (READONLY) : {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH
    .fini_array (READONLY) : {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    __etext = ALIGN(4);
    .data : AT(__etext) {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data .data*) *(.ramfunc .ramfunc*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM

    /* Source-built entry veneers occupy the factory RAM vector destinations.
     * Each remains ordinary compiled C and is copied by Reset_Handler; no
     * bytes from the factory image are linked into this firmware. */
    __dm4310_ram_lma = __etext + ALIGN(SIZEOF(.data), 4);
    .dm4310_helper_fault_monitor __layout_helper_fault_monitor : AT(__dm4310_ram_lma) {
        __dm4310_helper_fault_monitor_start__ = .;
        KEEP(*(.dm4310_helper_fault_monitor))
        __dm4310_helper_fault_monitor_end__ = .;
    } > RAM
    __dm4310_helper_fault_monitor_load__ =
        LOADADDR(.dm4310_helper_fault_monitor);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_fault_monitor), 4);

    .dm4310_irq002 __layout_irq002 : AT(__dm4310_ram_lma) {
        __dm4310_irq002_start__ = .;
        KEEP(*(.dm4310_irq002))
        /* Embedded factory data pool shared by fault monitor and ADC IRQ.
         * Startup copies it with the existing IRQ section, not as code. */
        . = __dm4310_irq002_start__ + 0x2C2;
        KEEP(*(.dm4310_literals_1fff83f8))
        . = ALIGN(4);
        __dm4310_irq002_end__ = .;
    } > RAM
    __dm4310_irq002_load__ = LOADADDR(.dm4310_irq002);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_irq002), 4);

    .dm4310_boot_record_writer __layout_boot_record_writer : AT(__dm4310_ram_lma) {
        __dm4310_boot_record_writer_start__ = .;
        KEEP(*(.dm4310_boot_record_writer))
        . = ALIGN(4);
        __dm4310_boot_record_writer_end__ = .;
    } > RAM
    __dm4310_boot_record_writer_load__ =
        LOADADDR(.dm4310_boot_record_writer);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_boot_record_writer), 4);

    .dm4310_irq000 __layout_irq000 : AT(__dm4310_ram_lma) {
        __dm4310_irq000_start__ = .;
        KEEP(*(.dm4310_irq000))
        . = ALIGN(4);
        __dm4310_irq000_end__ = .;
    } > RAM
    __dm4310_irq000_load__ = LOADADDR(.dm4310_irq000);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_irq000), 4);

    .dm4310_irq001 __layout_irq001 : AT(__dm4310_ram_lma) {
        __dm4310_irq001_start__ = .;
        KEEP(*(.dm4310_irq001))
        . = ALIGN(4);
        __dm4310_irq001_end__ = .;
    } > RAM
    __dm4310_irq001_load__ = LOADADDR(.dm4310_irq001);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_irq001), 4);

    .dm4310_irq003 __layout_irq003 : AT(__dm4310_ram_lma) {
        __dm4310_irq003_start__ = .;
        KEEP(*(.dm4310_irq003))
        . = __dm4310_irq003_start__ + 0x288;
        KEEP(*(.dm4310_literals_1fff8b40))
        . = __dm4310_irq003_start__ + 0x6D4;
        KEEP(*(.dm4310_literals_1fff8f8c))
        . = __dm4310_irq003_start__ + 0xB10;
        KEEP(*(.dm4310_literals_1fff93c8))
        . = ALIGN(4);
        __dm4310_irq003_end__ = .;
    } > RAM
    __dm4310_irq003_load__ = LOADADDR(.dm4310_irq003);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_irq003), 4);

    .dm4310_flash_writer __layout_flash_writer : AT(__dm4310_ram_lma) {
        __dm4310_flash_writer_start__ = .;
        KEEP(*(.dm4310_flash_writer))
        . = ALIGN(4);
        __dm4310_flash_writer_end__ = .;
    } > RAM
    __dm4310_flash_writer_load__ = LOADADDR(.dm4310_flash_writer);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_flash_writer), 4);

    .dm4310_flash_erase_only __layout_flash_erase_only : AT(__dm4310_ram_lma) {
        __dm4310_flash_erase_only_start__ = .;
        KEEP(*(.dm4310_flash_erase_only))
        __dm4310_flash_erase_only_end__ = .;
    } > RAM
    __dm4310_flash_erase_only_load__ = LOADADDR(.dm4310_flash_erase_only);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_flash_erase_only), 4);

    .dm4310_helper_output_sensor __layout_helper_output_sensor : AT(__dm4310_ram_lma) {
        __dm4310_helper_output_sensor_start__ = .;
        KEEP(*(.dm4310_helper_output_sensor))
        __dm4310_helper_output_sensor_end__ = .;
    } > RAM
    __dm4310_helper_output_sensor_load__ =
        LOADADDR(.dm4310_helper_output_sensor);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_output_sensor), 4);

    .dm4310_helper_mcan_send_classic __layout_helper_mcan_send_classic : AT(__dm4310_ram_lma) {
        __dm4310_helper_mcan_send_classic_start__ = .;
        KEEP(*(.dm4310_helper_mcan_send_classic))
        __dm4310_helper_mcan_send_classic_end__ = .;
    } > RAM
    __dm4310_helper_mcan_send_classic_load__ =
        LOADADDR(.dm4310_helper_mcan_send_classic);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_mcan_send_classic), 4);

    .dm4310_helper_mcan_send_variable_fd __layout_helper_mcan_send_variable_fd : AT(__dm4310_ram_lma) {
        __dm4310_helper_mcan_send_variable_fd_start__ = .;
        KEEP(*(.dm4310_helper_mcan_send_variable_fd))
        SHORT(0)
        __dm4310_helper_mcan_send_variable_fd_end__ = .;
    } > RAM
    __dm4310_helper_mcan_send_variable_fd_load__ =
        LOADADDR(.dm4310_helper_mcan_send_variable_fd);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_mcan_send_variable_fd), 4);

    .dm4310_helper_mcan_send_fd __layout_helper_mcan_send_fd : AT(__dm4310_ram_lma) {
        __dm4310_helper_mcan_send_fd_start__ = .;
        KEEP(*(.dm4310_helper_mcan_send_fd))
        __dm4310_helper_mcan_send_fd_end__ = .;
    } > RAM
    __dm4310_helper_mcan_send_fd_load__ =
        LOADADDR(.dm4310_helper_mcan_send_fd);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_mcan_send_fd), 4);

    .dm4310_helper_svpwm __layout_helper_svpwm : AT(__dm4310_ram_lma) {
        __dm4310_helper_svpwm_start__ = .;
        KEEP(*(.dm4310_helper_svpwm))
        __dm4310_helper_svpwm_end__ = .;
    } > RAM
    __dm4310_helper_svpwm_load__ = LOADADDR(.dm4310_helper_svpwm);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_svpwm), 4);

    .dm4310_helper_clear_runtime_loop_states __layout_helper_clear_runtime_loop_states : AT(__dm4310_ram_lma) {
        __dm4310_helper_clear_runtime_loop_states_start__ = .;
        KEEP(*(.dm4310_helper_clear_runtime_loop_states))
        __dm4310_helper_clear_runtime_loop_states_end__ = .;
    } > RAM
    __dm4310_helper_clear_runtime_loop_states_load__ =
        LOADADDR(.dm4310_helper_clear_runtime_loop_states);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_clear_runtime_loop_states), 4);

    .dm4310_helper_reset_control_state __layout_helper_reset_control_state : AT(__dm4310_ram_lma) {
        __dm4310_helper_reset_control_state_start__ = .;
        KEEP(*(.dm4310_helper_reset_control_state))
        __dm4310_helper_reset_control_state_end__ = .;
    } > RAM
    __dm4310_helper_reset_control_state_load__ =
        LOADADDR(.dm4310_helper_reset_control_state);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_reset_control_state), 4);

    .dm4310_helper_identification_filter __layout_helper_identification_filter : AT(__dm4310_ram_lma) {
        __dm4310_helper_identification_filter_start__ = .;
        KEEP(*(.dm4310_helper_identification_filter))
        __dm4310_helper_identification_filter_end__ = .;
    } > RAM
    __dm4310_helper_identification_filter_load__ =
        LOADADDR(.dm4310_helper_identification_filter);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_identification_filter), 4);

    .dm4310_helper_derive_runtime_controller_states __layout_helper_derive_runtime_controller_states : AT(__dm4310_ram_lma) {
        __dm4310_helper_derive_runtime_controller_states_start__ = .;
        KEEP(*(.dm4310_helper_derive_runtime_controller_states))
        SHORT(0)
        __dm4310_helper_derive_runtime_controller_states_end__ = .;
    } > RAM
    __dm4310_helper_derive_runtime_controller_states_load__ =
        LOADADDR(.dm4310_helper_derive_runtime_controller_states);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_derive_runtime_controller_states), 4);
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_helper_derive_runtime_controller_states) == 0x0C,
           "DM4310 controller derivation entry size changed")

    .dm4310_helper_motion_observer __layout_helper_motion_observer : AT(__dm4310_ram_lma) {
        __dm4310_helper_motion_observer_start__ = .;
        KEEP(*(.dm4310_helper_motion_observer))
        __dm4310_helper_motion_observer_end__ = .;
    } > RAM
    __dm4310_helper_motion_observer_load__ =
        LOADADDR(.dm4310_helper_motion_observer);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_motion_observer), 4);

    .dm4310_helper_current_controller __layout_helper_current_controller : AT(__dm4310_ram_lma) {
        __dm4310_helper_current_controller_start__ = .;
        KEEP(*(.dm4310_helper_current_controller))
        __dm4310_helper_current_controller_end__ = .;
    } > RAM
    __dm4310_helper_current_controller_load__ =
        LOADADDR(.dm4310_helper_current_controller);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_current_controller), 4);

    .dm4310_literals_1fff8630 __layout_literals_1fff8630 : AT(LOADADDR(.dm4310_helper_current_controller) + ALIGN(SIZEOF(.dm4310_helper_current_controller), 4)) {
        KEEP(*(.dm4310_literals_1fff8630))
    } > RAM
    __dm4310_literals_1fff8630_load__ = LOADADDR(.dm4310_literals_1fff8630);
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_literals_1fff8630) == 16, "Factory literal pool size changed")
    .dm4310_literals_1fff8724 __layout_literals_1fff8724 : AT(LOADADDR(.dm4310_literals_1fff8630) + ALIGN(SIZEOF(.dm4310_literals_1fff8630), 4)) {
        KEEP(*(.dm4310_literals_1fff8724))
    } > RAM
    __dm4310_literals_1fff8724_load__ = LOADADDR(.dm4310_literals_1fff8724);
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_literals_1fff8724) == 32, "Factory literal pool size changed")
    .dm4310_literals_1fff9830 __layout_literals_1fff9830 : AT(LOADADDR(.dm4310_literals_1fff8724) + ALIGN(SIZEOF(.dm4310_literals_1fff8724), 4)) {
        KEEP(*(.dm4310_literals_1fff9830))
    } > RAM
    __dm4310_literals_1fff9830_load__ = LOADADDR(.dm4310_literals_1fff9830);
    ASSERT(SIZEOF(.dm4310_literals_1fff9830) ==
           ((__dm8009_layout || __dm43_48v_layout) ? 80 : 76),
           "Factory literal pool size changed")
    .dm4310_literals_1fff9934 __layout_literals_1fff9934 : AT(LOADADDR(.dm4310_literals_1fff9830) + ALIGN(SIZEOF(.dm4310_literals_1fff9830), 4)) {
        KEEP(*(.dm4310_literals_1fff9934))
    } > RAM
    __dm4310_literals_1fff9934_load__ = LOADADDR(.dm4310_literals_1fff9934);
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_literals_1fff9934) == 28, "Factory literal pool size changed")
    .dm4310_literals_1fff9ac0 __layout_literals_1fff9ac0 : AT(LOADADDR(.dm4310_literals_1fff9934) + ALIGN(SIZEOF(.dm4310_literals_1fff9934), 4)) {
        KEEP(*(.dm4310_literals_1fff9ac0))
    } > RAM
    __dm4310_literals_1fff9ac0_load__ = LOADADDR(.dm4310_literals_1fff9ac0);
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_literals_1fff9ac0) == 12, "Factory literal pool size changed")
    .dm4310_literals_1fff9c34 __layout_literals_1fff9c34 : AT(LOADADDR(.dm4310_literals_1fff9ac0) + ALIGN(SIZEOF(.dm4310_literals_1fff9ac0), 4)) {
        KEEP(*(.dm4310_literals_1fff9c34))
    } > RAM
    __dm4310_literals_1fff9c34_load__ = LOADADDR(.dm4310_literals_1fff9c34);
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_literals_1fff9c34) == 12, "Factory literal pool size changed")
    .dm4310_literals_1fff9d4c __layout_literals_1fff9d4c : AT(LOADADDR(.dm4310_literals_1fff9c34) + ALIGN(SIZEOF(.dm4310_literals_1fff9c34), 4)) {
        KEEP(*(.dm4310_literals_1fff9d4c))
    } > RAM
    __dm4310_literals_1fff9d4c_load__ = LOADADDR(.dm4310_literals_1fff9d4c);
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_literals_1fff9d4c) == 16, "Factory literal pool size changed")
    .dm4310_literals_1fffa118 __layout_literals_1fffa118 : AT(LOADADDR(.dm4310_literals_1fff9d4c) + ALIGN(SIZEOF(.dm4310_literals_1fff9d4c), 4)) {
        KEEP(*(.dm4310_literals_1fffa118))
    } > RAM
    __dm4310_literals_1fffa118_load__ = LOADADDR(.dm4310_literals_1fffa118);
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_literals_1fffa118) == 32, "Factory literal pool size changed")
    .dm4310_literals_1fffa13c __layout_literals_1fffa13c : AT(LOADADDR(.dm4310_literals_1fffa118) + ALIGN(SIZEOF(.dm4310_literals_1fffa118), 4)) {
        KEEP(*(.dm4310_literals_1fffa13c))
    } > RAM
    __dm4310_literals_1fffa13c_load__ = LOADADDR(.dm4310_literals_1fffa118) + SIZEOF(.dm4310_literals_1fffa118);
    __dm4310_ram_lma = __dm4310_literals_1fffa13c_load__ + SIZEOF(.dm4310_literals_1fffa13c);
    ASSERT(__dm8009_layout || SIZEOF(.dm4310_literals_1fffa13c) == 32, "Factory literal pool size changed")
    .dm4310_filter_literal __layout_filter_literal : AT(__dm4310_ram_lma) {
        __dm4310_filter_literal_start__ = .;
        KEEP(*(.dm4310_filter_literal))
        __dm4310_filter_literal_end__ = .;
    } > RAM
    __dm4310_filter_literal_load__ = LOADADDR(.dm4310_filter_literal);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_filter_literal), 4);

    .dm4310_motion_literal __layout_motion_literal : AT(__dm4310_ram_lma) {
        __dm4310_motion_literal_start__ = .;
        KEEP(*(.dm4310_motion_literal))
        __dm4310_motion_literal_end__ = .;
    } > RAM
    __dm4310_motion_literal_load__ = LOADADDR(.dm4310_motion_literal);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_motion_literal), 4);

    .dm4310_helper_rls2 __layout_helper_rls2 : AT(__dm4310_ram_lma) {
        __dm4310_helper_rls2_start__ = .;
        KEEP(*(.dm4310_helper_rls2))
        __dm4310_helper_rls2_end__ = .;
    } > RAM
    __dm4310_helper_rls2_load__ = LOADADDR(.dm4310_helper_rls2);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_rls2), 4);

    .dm4310_rls_literals __layout_rls_literals : AT(__dm4310_ram_lma) {
        __dm4310_rls_literals_start__ = .;
        KEEP(*(.dm4310_rls_literals))
        __dm4310_rls_literals_end__ = .;
    } > RAM
    __dm4310_rls_literals_load__ = LOADADDR(.dm4310_rls_literals);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_rls_literals), 4);

    .dm4310_helper_flux_observer __layout_helper_flux_observer : AT(__dm4310_ram_lma) {
        __dm4310_helper_flux_observer_start__ = .;
        KEEP(*(.dm4310_helper_flux_observer))
        __dm4310_helper_flux_observer_end__ = .;
    } > RAM
    __dm4310_helper_flux_observer_load__ =
        LOADADDR(.dm4310_helper_flux_observer);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_flux_observer), 4);

    .dm4310_helper_sincos __layout_helper_sincos : AT(__dm4310_ram_lma) {
        __dm4310_helper_sincos_start__ = .;
        KEEP(*(.dm4310_helper_sincos))
        __dm4310_helper_sincos_end__ = .;
    } > RAM
    __dm4310_helper_sincos_load__ = LOADADDR(.dm4310_helper_sincos);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_sincos), 4);

    .dm4310_helper_clamp __layout_helper_clamp : AT(__dm4310_ram_lma) {
        __dm4310_helper_clamp_start__ = .;
        KEEP(*(.dm4310_helper_clamp))
        __dm4310_helper_clamp_end__ = .;
    } > RAM
    __dm4310_helper_clamp_load__ = LOADADDR(.dm4310_helper_clamp);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_clamp), 4);

    .dm4310_helper_wrap __layout_helper_wrap : AT(__dm4310_ram_lma) {
        __dm4310_helper_wrap_start__ = .;
        KEEP(*(.dm4310_helper_wrap))
        __dm4310_helper_wrap_end__ = .;
    } > RAM
    __dm4310_helper_wrap_load__ = LOADADDR(.dm4310_helper_wrap);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_wrap), 4);

    .dm4310_helper_limit_vector __layout_helper_limit_vector : AT(__dm4310_ram_lma) {
        __dm4310_helper_limit_vector_start__ = .;
        KEEP(*(.dm4310_helper_limit_vector))
        __dm4310_helper_limit_vector_end__ = .;
    } > RAM
    __dm4310_helper_limit_vector_load__ =
        LOADADDR(.dm4310_helper_limit_vector);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_limit_vector), 4);

    .dm4310_helper_float_to_uint __layout_helper_float_to_uint : AT(__dm4310_ram_lma) {
        __dm4310_helper_float_to_uint_start__ = .;
        KEEP(*(.dm4310_helper_float_to_uint))
        __dm4310_helper_float_to_uint_end__ = .;
    } > RAM
    __dm4310_helper_float_to_uint_load__ =
        LOADADDR(.dm4310_helper_float_to_uint);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_float_to_uint), 4);

    .dm4310_helper_uint_to_float __layout_helper_uint_to_float : AT(__dm4310_ram_lma) {
        __dm4310_helper_uint_to_float_start__ = .;
        KEEP(*(.dm4310_helper_uint_to_float))
        __dm4310_helper_uint_to_float_end__ = .;
    } > RAM
    __dm4310_helper_uint_to_float_load__ =
        LOADADDR(.dm4310_helper_uint_to_float);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_uint_to_float), 4);

    .dm4310_helper_derive_control_parameters __layout_helper_derive_control_parameters : AT(__dm4310_ram_lma) {
        __dm4310_helper_derive_control_parameters_start__ = .;
        KEEP(*(.dm4310_helper_derive_control_parameters))
        __dm4310_helper_derive_control_parameters_end__ = .;
    } > RAM
    __dm4310_helper_derive_control_parameters_load__ =
        LOADADDR(.dm4310_helper_derive_control_parameters);
    __dm4310_ram_lma = __dm4310_ram_lma +
        ALIGN(SIZEOF(.dm4310_helper_derive_control_parameters), 4);

    .dm4310_helper_delay_ms __layout_helper_delay_ms : AT(__dm4310_ram_lma) {
        __dm4310_helper_delay_ms_start__ = .;
        KEEP(*(.dm4310_helper_delay_ms))
        __dm4310_helper_delay_ms_end__ = .;
    } > RAM
    __dm4310_helper_delay_ms_load__ = LOADADDR(.dm4310_helper_delay_ms);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_delay_ms), 4);

    .dm4310_helper_select_configuration_bank_b __layout_helper_select_configuration_bank_b : AT(__dm4310_ram_lma) {
        __dm4310_helper_select_configuration_bank_b_start__ = .;
        KEEP(*(.dm4310_helper_select_configuration_bank_b))
        __dm4310_helper_select_configuration_bank_b_end__ = .;
    } > RAM
    __dm4310_helper_select_configuration_bank_b_load__ =
        LOADADDR(.dm4310_helper_select_configuration_bank_b);
    __dm4310_ram_lma = __dm4310_ram_lma +
        ALIGN(SIZEOF(.dm4310_helper_select_configuration_bank_b), 4);

    .dm4310_helper_atan2 __layout_helper_atan2 : AT(__dm4310_ram_lma) {
        __dm4310_helper_atan2_start__ = .;
        KEEP(*(.dm4310_helper_atan2))
        __dm4310_helper_atan2_end__ = .;
    } > RAM
    __dm4310_helper_atan2_load__ = LOADADDR(.dm4310_helper_atan2);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_atan2), 4);

    .dm4310_helper_sqrt __layout_helper_sqrt : AT(__dm4310_ram_lma) {
        __dm4310_helper_sqrt_start__ = .;
        KEEP(*(.dm4310_helper_sqrt))
        SHORT(0)
        __dm4310_helper_sqrt_end__ = .;
    } > RAM
    __dm4310_helper_sqrt_load__ = LOADADDR(.dm4310_helper_sqrt);
    __dm4310_ram_lma = __dm4310_ram_lma +
                       ALIGN(SIZEOF(.dm4310_helper_sqrt), 4);

    /* Reserve the flash load image here so RAMB data follows every sparse
     * initialized DM4310 SRAM object. */
    __dm4310_aes_sbox_load__ = __dm4310_ram_lma;
    __dm4310_ram_lma = __dm4310_ram_lma + 0x100;

    __etext_ramb = __dm4310_ram_lma;
    .ramb_data : AT(__etext_ramb) {
        __data_start_ramb__ = .;
        *(.ramb_data .ramb_data*)
        __data_end_ramb__ = .;
    } > RAMB

    /* Clear the fixed staging objects together with ordinary BSS.  The sine
     * table constructor then reconstructs the factory initialized image,
     * including relying on zero at its three axis crossings. */
    __bss_start__ = __layout_initialized_state;

    .dm4310_runtime_drive_d __layout_runtime_drive_d (NOLOAD) : {
        KEEP(*(.dm4310_runtime_drive_d))
    } > RAM
    .dm4310_runtime_drive_q __layout_runtime_drive_q (NOLOAD) : {
        KEEP(*(.dm4310_runtime_drive_q))
    } > RAM
    .dm4310_runtime_speed_loop __layout_runtime_speed_loop (NOLOAD) : {
        KEEP(*(.dm4310_runtime_speed_loop))
    } > RAM
    .dm4310_runtime_position_loop __layout_runtime_position_loop (NOLOAD) : {
        KEEP(*(.dm4310_runtime_position_loop))
    } > RAM

    /* Vector_20 compares received_length against expected_payload_length+1. */
    .dm4310_uart_length_state __layout_uart_length_state (NOLOAD) : {
        KEEP(*(.dm4310_uart_length_state))
    } > RAM

    .dm4310_hardware_variant __layout_hardware_variant (NOLOAD) : {
        KEEP(*(.dm4310_hardware_variant))
    } > RAM

    /* FE writes these two offsets directly from this fixed source. */
    .dm4310_zero_position_staging __layout_zero_position_staging (NOLOAD) : {
        KEEP(*(.dm4310_zero_position_staging))
    } > RAM

    /* The live 37-word motor configuration record used by UART ingress,
     * commissioning and flash commit sits immediately before the sine table. */
    .dm4310_config_staging __layout_config_staging (NOLOAD) : {
        KEEP(*(.dm4310_config_staging))
    } > RAM

    /* position_sensor_configure@0x22c10 compares its eleven readbacks with
     * this factory image.  Reset_Handler restores it after clearing BSS. */
    .dm4310_position_sensor_expected __layout_position_sensor_expected (NOLOAD) : {
        __dm4310_position_sensor_expected_start__ = .;
        KEEP(*(.dm4310_position_sensor_expected))
        __dm4310_position_sensor_expected_end__ =
            __dm4310_position_sensor_expected_start__ + 11;
    } > RAM

    /* V6417 places the four commissioning controller states after its
     * staging prefix.  Separate input names let one source graph retain both
     * factory orders without moving the linker location counter backwards. */
    .dm8009_runtime_drive_d __layout_runtime_drive_d (NOLOAD) : {
        KEEP(*(.dm8009_runtime_drive_d))
    } > RAM
    .dm8009_runtime_drive_q __layout_runtime_drive_q (NOLOAD) : {
        KEEP(*(.dm8009_runtime_drive_q))
    } > RAM
    .dm8009_runtime_speed_loop __layout_runtime_speed_loop (NOLOAD) : {
        KEEP(*(.dm8009_runtime_speed_loop))
    } > RAM
    .dm8009_runtime_position_loop __layout_runtime_position_loop (NOLOAD) : {
        KEEP(*(.dm8009_runtime_position_loop))
    } > RAM

    /* V5017 overlaps expected[10]; V6417 selects a distinct input section. */
    .dm4310_position_dma_word __layout_position_dma_word (NOLOAD) : {
        KEEP(*(.dm4310_position_dma_word))
    } > RAM

    /* V5017 RAM-code literal 0x1fffa4c8 points at this 2049-float table. */
    .dm4310_sine_table __layout_sine_table (NOLOAD) : {
        KEEP(*(.dm4310_sine_table))
    } > RAM

    /* AES key expansion and SubBytes share this factory SRAM S-box. */
    .dm4310_aes_sbox __layout_aes_sbox : AT(__dm4310_aes_sbox_load__) {
        __dm4310_aes_sbox_start__ = .;
        KEEP(*(.rodata.sbox))
        __dm4310_aes_sbox_end__ = .;
    } > RAM

    .dm4310_adc_raw __layout_adc_raw (NOLOAD) : {
        KEEP(*(.dm4310_adc_raw))
    } > RAM

    /* Exact 19-float D/Q observer-controller states consumed by
     * current_controller_step@0x2a6fa. */
    .dm4310_current_d __layout_current_d (NOLOAD) : {
        KEEP(*(.dm4310_current_d))
    } > RAM
    .dm4310_current_q __layout_current_q (NOLOAD) : {
        KEEP(*(.dm4310_current_q))
    } > RAM

    .dm4310_boot_record_staging __layout_boot_record_staging (NOLOAD) : {
        KEEP(*(.dm4310_boot_record_staging))
    } > RAM

    /* Vector_20 assembles every 2/9/13-byte command response here. */
    .dm4310_short_response_buffer __layout_short_response_buffer (NOLOAD) : {
        KEEP(*(.dm4310_short_response_buffer))
    } > RAM

    /* load_and_validate_calibration@0x22658 copies the 256 motor-encoder
     * correction floats from flash to this runtime lookup table. */
    .dm4310_motor_correction __layout_motor_correction (NOLOAD) : {
        KEEP(*(.dm4310_motor_correction))
    } > RAM

    .dm4310_can_protocol_scratch __layout_can_protocol_scratch (NOLOAD) : {
        KEEP(*(.dm4310_can_protocol_scratch))
    } > RAM

    /* Vector_20 writes Ud and UM chunks directly into these two regions. */
    .dm4310_motor_record __layout_motor_record (NOLOAD) : {
        KEEP(*(.dm4310_motor_record))
    } > RAM
    .dm4310_output_table __layout_output_table (NOLOAD) : {
        KEEP(*(.dm4310_output_table))
    } > RAM

    /* load_and_validate_calibration keeps the four analogue-output sensor
     * calibration floats at 0x1ffff078 before scattering them into state. */
    .dm4310_output_calibration __layout_output_calibration (NOLOAD) : {
        KEEP(*(.dm4310_output_calibration))
    } > RAM

    /* Shared motor/control and ADC/sample objects.  Their full boundaries
     * are proven by the adjacent factory literals; field ownership is being
     * recovered across startup, commissioning and the fast-control helpers. */
    .dm4310_motor_runtime_state __layout_motor_runtime_state (NOLOAD) : {
        KEEP(*(.dm4310_motor_runtime_state))
    } > RAM
    .dm4310_sample_runtime_state __layout_sample_runtime_state (NOLOAD) : {
        KEEP(*(.dm4310_sample_runtime_state))
    } > RAM
    dm4310_position_sensor_scratch = __layout_sample_runtime_state + 0x8C;

    /* validate_current_sensors passes this base to the relocated output
     * lookup helper.  The source model covers the complete 0x48-byte
     * factory state slot, including the two unresolved reserved words. */
    .dm4310_output_sensor_state __layout_output_sensor_state (NOLOAD) : {
        KEEP(*(.dm4310_output_sensor_state))
        . = 0x48;
    } > RAM

    /* Shared deferred-event, fault debounce and status object.  The next
     * recovered object starts exactly at 0x1ffff23c. */
    .dm4310_runtime_status __layout_runtime_status (NOLOAD) : {
        KEEP(*(.dm4310_runtime_status))
    } > RAM

    .dm4310_runtime_parameter_cache __layout_runtime_parameter_cache (NOLOAD) : {
        KEEP(*(.dm4310_runtime_parameter_cache))
    } > RAM

    /* Classic/FD callback pair followed by the persistent CAN protocol
     * workspace.  The adjacent calibration scratch starts at 0x1ffff340. */
    .dm4310_mcan_dispatch __layout_mcan_dispatch (NOLOAD) : {
        KEEP(*(.dm4310_mcan_dispatch))
    } > RAM

    /* Vector_20 copies rx+1 here before dispatching Ud/UM slot writes. */
    .dm4310_calibration_frame_scratch __layout_calibration_frame_scratch (NOLOAD) : {
        KEEP(*(.dm4310_calibration_frame_scratch))
    } > RAM

    /* Exact 15-float motion observer consumed by state_step@0x2a65e. */
    .dm4310_motion_observer __layout_motion_observer (NOLOAD) : {
        KEEP(*(.dm4310_motion_observer))
    } > RAM

    /* V6417 keeps ADC raw values and both current controllers beside the
     * motion observer instead of ahead of the protocol/calibration state. */
    .dm8009_adc_raw __layout_adc_raw (NOLOAD) : {
        KEEP(*(.dm8009_adc_raw))
    } > RAM
    .dm8009_current_d __layout_current_d (NOLOAD) : {
        KEEP(*(.dm8009_current_d))
    } > RAM
    .dm8009_current_q __layout_current_q (NOLOAD) : {
        KEEP(*(.dm8009_current_q))
    } > RAM

    /* position_sensor_configure@0x22c10 keeps the eleven register readbacks
     * immediately before the unaligned UART DMA buffer. */
    .dm4310_position_sensor_observed __layout_position_sensor_observed (NOLOAD) : {
        KEEP(*(.dm4310_position_sensor_observed))
    } > RAM

    /* 0x1ffff1f0 starts the factory deferred-event/status object.  Keep the
     * unresolved tail free of unrelated source BSS.  USART1 DMA writes its
     * 200-byte receive window at the unaligned literal 0x1ffff3c7. */
    .dm4310_uart_rx_buffer __layout_uart_rx_buffer (NOLOAD) : {
        KEEP(*(.dm4310_uart_rx_buffer))
    } > RAM

    .dm4310_c_runtime_state __layout_c_runtime_state (NOLOAD) : {
        KEEP(*(.dm4310_c_runtime_state))
    } > RAM

    /* SystemCoreClockUpdate@0x234bc addresses this adjacent pair through the
     * literal 0x1ffff4f0: system frequency first, HRC frequency second. */
    .dm4310_system_core_clock __layout_system_core_clock (NOLOAD) : {
        KEEP(*(.dm4310_system_core_clock))
    } > RAM
    .dm4310_hrc_value __layout_hrc_value (NOLOAD) : {
        KEEP(*(.dm4310_hrc_value))
    } > RAM

    /* Factory zero-initialized state ends where its 1 KiB heap begins. */
    __factory_bss_end__ = __layout_factory_bss_end;

    .ramb_bss (NOLOAD) : {
        __bss_start_ramb__ = .;
        *(.ramb_bss .ramb_bss*)
        __bss_end_ramb__ = .;
    } > RAMB

    __HeapBase = __layout_factory_bss_end;
    __HeapLimit = __layout_heap_limit;

    /* Source-level aggregate/work objects have no one-to-one factory
     * layout.  Keep them above the factory MSP instead of covering recovered
     * globals below it.  Both are explicitly initialized by their owners. */
    .dm4310_app_state 0x20001600 (NOLOAD) : {
        KEEP(*(.dm4310_app_state))
    } > RAM

    /* Source-only bookkeeping has no factory address contract.  Keep it in
     * the same above-MSP development area as the aggregate work objects so
     * it cannot consume the factory heap or recovered state. */
    .bss 0x20002000 (NOLOAD) : {
        __source_bss_start__ = .;
        . = ALIGN(4);
        *(.bss .bss*) *(COMMON)
        . = ALIGN(4);
        *(.noinit .noinit*)
        __source_bss_end__ = .;
    } > RAM
    __bss_end__ = __source_bss_end__;
    __end__ = ALIGN(__source_bss_end__, 8);
    PROVIDE(end = __end__);
    PROVIDE(_end = __end__);

    /* The factory vector table uses this exact initial MSP.  Keep one KiB
     * beyond static storage as a link-time floor; the recovered deepest
     * application frame is well below that allowance. */
    __StackTop = __layout_stack_top;
    __StackLimit = __layout_heap_limit;

    .ARM.attributes 0 : { *(.ARM.attributes) }
    PROVIDE(_stack = __StackTop);
    /* Sparse SRAM sections use explicit AT() LMAs, so the FLASH MEMORY
     * region alone cannot diagnose their load image crossing 0x30000. */
    ASSERT(LOADADDR(.ramb_data) + SIZEOF(.ramb_data) <=
           ORIGIN(FLASH) + LENGTH(FLASH),
           "DM43xx APP load image exceeds its 64 KiB partition")

    /* V6417 uses a different, fully recovered fixed-SRAM ordering.  Keep
     * direct constants here as an independent guard around the model layout
     * table above: changing a table entry must not silently move the ABI. */
    ASSERT(!__dm8009_layout || IRQ000_Handler == 0x1FFF8104,
           "DM8009 IRQ000 RAM entry moved")
    ASSERT(!__dm8009_layout || IRQ001_Handler == 0x1FFF8130,
           "DM8009 IRQ001 RAM entry moved")
    ASSERT(!__dm8009_layout || IRQ002_Handler == 0x1FFF944A,
           "DM8009 IRQ002 RAM entry moved")
    ASSERT(!__dm8009_layout || IRQ003_Handler == 0x1FFF8278,
           "DM8009 IRQ003 RAM entry moved")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_boot_record_writer) == 0x1FFF8000 &&
            ADDR(.dm4310_helper_fault_monitor) == 0x1FFF9314 &&
            ADDR(.dm4310_helper_output_sensor) == 0x1FFF9240 &&
            ADDR(.dm4310_flash_writer) == 0x1FFF9988 &&
            ADDR(.dm4310_flash_erase_only) == 0x1FFF9A70),
           "DM8009 recovered RAM primitive moved")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_helper_mcan_send_classic) == 0x1FFF9B04 &&
            ADDR(.dm4310_helper_mcan_send_variable_fd) == 0x1FFF9B46 &&
            ADDR(.dm4310_helper_mcan_send_fd) == 0x1FFF9BD8 &&
            ADDR(.dm4310_helper_svpwm) == 0x1FFF9C78 &&
            ADDR(.dm4310_helper_clear_runtime_loop_states) == 0x1FFF9D94 &&
            ADDR(.dm4310_helper_reset_control_state) == 0x1FFF9E00),
           "DM8009 recovered control helper moved")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_helper_identification_filter) == 0x1FFF9E7C &&
            ADDR(.dm4310_helper_derive_runtime_controller_states) == 0x1FFF9EDE &&
            ADDR(.dm4310_helper_motion_observer) == 0x1FFFA016 &&
            ADDR(.dm4310_helper_current_controller) == 0x1FFFA0B2 &&
            ADDR(.dm4310_helper_rls2) == 0x1FFFA198 &&
            ADDR(.dm4310_helper_flux_observer) == 0x1FFFA26C),
           "DM8009 recovered observer helper moved")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_helper_sincos) == 0x1FFFA344 &&
            ADDR(.dm4310_helper_clamp) == 0x1FFFA3C2 &&
            ADDR(.dm4310_helper_wrap) == 0x1FFFA3E2 &&
            ADDR(.dm4310_helper_limit_vector) == 0x1FFFA44C &&
            ADDR(.dm4310_helper_float_to_uint) == 0x1FFFA4A2 &&
            ADDR(.dm4310_helper_uint_to_float) == 0x1FFFA4CA),
           "DM8009 recovered arithmetic helper moved")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_helper_derive_control_parameters) == 0x1FFFA514 &&
            ADDR(.dm4310_helper_delay_ms) == 0x1FFFA51E &&
            ADDR(.dm4310_helper_select_configuration_bank_b) == 0x1FFFA528 &&
            ADDR(.dm4310_helper_atan2) == 0x1FFFA532 &&
            ADDR(.dm4310_helper_sqrt) == 0x1FFFA53C),
           "DM8009 recovered leaf helper moved")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_uart_length_state) == 0x1FFFA548 &&
            SIZEOF(.dm4310_uart_length_state) == 0x4 &&
            ADDR(.dm4310_hardware_variant) == 0x1FFFA54C &&
            SIZEOF(.dm4310_hardware_variant) == 0x1 &&
            ADDR(.dm4310_zero_position_staging) == 0x1FFFA550 &&
            SIZEOF(.dm4310_zero_position_staging) == 0x8 &&
            ADDR(.dm4310_config_staging) == 0x1FFFA558 &&
            SIZEOF(.dm4310_config_staging) == 0x94),
           "DM8009 initialized-state prefix layout mismatch")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_position_sensor_expected) == 0x1FFFA5EC &&
            SIZEOF(.dm4310_position_sensor_expected) == 0x0B &&
            ADDR(.dm8009_runtime_drive_d) == 0x1FFFA5F8 &&
            SIZEOF(.dm8009_runtime_drive_d) == 0x28 &&
            ADDR(.dm8009_runtime_drive_q) == 0x1FFFA620 &&
            SIZEOF(.dm8009_runtime_drive_q) == 0x28 &&
            ADDR(.dm8009_runtime_speed_loop) == 0x1FFFA650 &&
            SIZEOF(.dm8009_runtime_speed_loop) == 0x28 &&
            ADDR(.dm8009_runtime_position_loop) == 0x1FFFA678 &&
            SIZEOF(.dm8009_runtime_position_loop) == 0x28),
           "DM8009 controller-state layout mismatch")
    ASSERT(!__dm8009_layout ||
           (position_dma_word == 0x1FFFA6A0 &&
            ADDR(.dm4310_sine_table) == 0x1FFFA6AC &&
            SIZEOF(.dm4310_sine_table) == 0x2004 &&
            ADDR(.dm4310_aes_sbox) == 0x1FFFC6B0 &&
            SIZEOF(.dm4310_aes_sbox) == 0x100),
           "DM8009 lookup-table layout mismatch")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_boot_record_staging) == 0x1FFFC7B0 &&
            SIZEOF(.dm4310_boot_record_staging) == 0x14 &&
            ADDR(.dm4310_short_response_buffer) == 0x1FFFC7C4 &&
            SIZEOF(.dm4310_short_response_buffer) == 0x14 &&
            ADDR(.dm4310_motor_correction) == 0x1FFFC7D8 &&
            SIZEOF(.dm4310_motor_correction) == 0x400 &&
            ADDR(.dm4310_can_protocol_scratch) == 0x1FFFCBD8 &&
            SIZEOF(.dm4310_can_protocol_scratch) == 0x20),
           "DM8009 calibration-staging layout mismatch")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_motor_record) == 0x1FFFCBF8 &&
            SIZEOF(.dm4310_motor_record) == 0x40C &&
            ADDR(.dm4310_output_table) == 0x1FFFD004 &&
            SIZEOF(.dm4310_output_table) == 0x2000 &&
            ADDR(.dm4310_output_calibration) == 0x1FFFF004 &&
            SIZEOF(.dm4310_output_calibration) == 0x10),
           "DM8009 persistent-record layout mismatch")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_motor_runtime_state) == 0x1FFFF014 &&
            SIZEOF(.dm4310_motor_runtime_state) == 0x7C &&
            ADDR(.dm4310_sample_runtime_state) == 0x1FFFF090 &&
            SIZEOF(.dm4310_sample_runtime_state) == 0xA4 &&
            ADDR(.dm4310_output_sensor_state) == 0x1FFFF134 &&
            SIZEOF(.dm4310_output_sensor_state) == 0x48 &&
            ADDR(.dm4310_runtime_status) == 0x1FFFF17C &&
            SIZEOF(.dm4310_runtime_status) == 0x4C),
           "DM8009 runtime-state layout mismatch")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_runtime_parameter_cache) == 0x1FFFF1C8 &&
            SIZEOF(.dm4310_runtime_parameter_cache) == 0x60 &&
            ADDR(.dm4310_mcan_dispatch) == 0x1FFFF228 &&
            SIZEOF(.dm4310_mcan_dispatch) == 0xA4 &&
            ADDR(.dm4310_calibration_frame_scratch) == 0x1FFFF2CC &&
            SIZEOF(.dm4310_calibration_frame_scratch) == 0x40 &&
            ADDR(.dm4310_motion_observer) == 0x1FFFF30C &&
            SIZEOF(.dm4310_motion_observer) == 0x3C),
           "DM8009 protocol-state layout mismatch")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm8009_adc_raw) == 0x1FFFF348 &&
            SIZEOF(.dm8009_adc_raw) == 0x14 &&
            ADDR(.dm8009_current_d) == 0x1FFFF35C &&
            SIZEOF(.dm8009_current_d) == 0x4C &&
            ADDR(.dm8009_current_q) == 0x1FFFF3A8 &&
            SIZEOF(.dm8009_current_q) == 0x4C &&
            ADDR(.dm4310_position_sensor_observed) == 0x1FFFF3F4 &&
            SIZEOF(.dm4310_position_sensor_observed) == 0x0B &&
            ADDR(.dm4310_uart_rx_buffer) == 0x1FFFF3FF &&
            SIZEOF(.dm4310_uart_rx_buffer) == 0xC8),
           "DM8009 sampled-state tail layout mismatch")
    ASSERT(!__dm8009_layout ||
           (ADDR(.dm4310_c_runtime_state) == 0x1FFFF4C8 &&
            SIZEOF(.dm4310_c_runtime_state) == 0x60 &&
            ADDR(.dm4310_system_core_clock) == 0x1FFFF528 &&
            SIZEOF(.dm4310_system_core_clock) == 0x4 &&
            ADDR(.dm4310_hrc_value) == 0x1FFFF52C &&
            SIZEOF(.dm4310_hrc_value) == 0x4 &&
            __factory_bss_end__ == 0x1FFFF530 &&
            __HeapBase == 0x1FFFF530 &&
            __HeapLimit == 0x1FFFF930 &&
            __StackTop == 0x20000530),
           "DM8009 C runtime/heap/stack layout mismatch")
    ASSERT(!__dm8009_layout || ADDR(.bss) >= __StackTop,
           "DM8009 source BSS overlaps the factory stack")
    ASSERT(!__dm43_48v_layout ||
           (IRQ000_Handler == 0x1FFF8744 &&
            IRQ001_Handler == 0x1FFF8770 &&
            IRQ002_Handler == 0x1FFF8136 &&
            IRQ003_Handler == 0x1FFF88B8),
           "DM43 48 V IRQ layout mismatch")
    ASSERT(!__dm43_48v_layout ||
           (ADDR(.dm4310_helper_output_sensor) == __layout_helper_output_sensor &&
            ADDR(.dm4310_flash_writer) == __layout_flash_writer &&
            ADDR(.dm4310_flash_erase_only) == __layout_flash_erase_only &&
            ADDR(.dm4310_helper_mcan_send_classic) == __layout_helper_mcan_send_classic &&
            ADDR(.dm4310_helper_mcan_send_variable_fd) == __layout_helper_mcan_send_variable_fd &&
            ADDR(.dm4310_helper_mcan_send_fd) == __layout_helper_mcan_send_fd),
           "DM43 48 V fixed helper layout mismatch")
    ASSERT(!__dm43_48v_layout ||
           (ADDR(.dm4310_helper_svpwm) == __layout_helper_svpwm &&
            ADDR(.dm4310_helper_clear_runtime_loop_states) == __layout_helper_clear_runtime_loop_states &&
            ADDR(.dm4310_helper_reset_control_state) == __layout_helper_reset_control_state &&
            ADDR(.dm4310_helper_identification_filter) == __layout_helper_identification_filter &&
            ADDR(.dm4310_helper_motion_observer) == __layout_helper_motion_observer &&
            ADDR(.dm4310_helper_current_controller) == __layout_helper_current_controller),
           "DM43 48 V control helper layout mismatch")
    ASSERT(!__dm43_48v_layout ||
           (ADDR(.dm4310_runtime_drive_d) == __layout_runtime_drive_d &&
            ADDR(.dm4310_runtime_drive_q) == __layout_runtime_drive_q &&
            ADDR(.dm4310_runtime_speed_loop) == __layout_runtime_speed_loop &&
            ADDR(.dm4310_runtime_position_loop) == __layout_runtime_position_loop &&
            ADDR(.dm4310_config_staging) == __layout_config_staging &&
            ADDR(.dm4310_sine_table) == __layout_sine_table &&
            ADDR(.dm4310_aes_sbox) == __layout_aes_sbox),
           "DM43 48 V initialized-state layout mismatch")
    ASSERT(!__dm43_48v_layout ||
           (ADDR(.dm4310_adc_raw) == __layout_adc_raw &&
            ADDR(.dm4310_current_d) == __layout_current_d &&
            ADDR(.dm4310_current_q) == __layout_current_q &&
            ADDR(.dm4310_boot_record_staging) == __layout_boot_record_staging &&
            ADDR(.dm4310_output_table) == __layout_output_table &&
            ADDR(.dm4310_runtime_status) == __layout_runtime_status &&
            ADDR(.dm4310_c_runtime_state) == __layout_c_runtime_state),
           "DM43 48 V BSS object layout mismatch")
    ASSERT(!__dm43_48v_layout ||
           (__factory_bss_end__ == __layout_factory_bss_end &&
            __HeapBase == __layout_factory_bss_end &&
            __HeapLimit == __layout_heap_limit &&
            __StackTop == __layout_stack_top),
           "DM43 48 V runtime/heap/stack layout mismatch")
    ASSERT(!__dm43_48v_layout || ADDR(.bss) >= __StackTop,
           "DM43 48 V source BSS overlaps the factory stack")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || IRQ000_Handler == 0x1FFF8744,
           "DM4310 IRQ000 RAM entry moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || IRQ001_Handler == 0x1FFF8770,
           "DM4310 IRQ001 RAM entry moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || IRQ002_Handler == 0x1FFF8136,
           "DM4310 IRQ002 RAM entry moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || IRQ003_Handler == 0x1FFF88B8,
           "DM4310 IRQ003 RAM entry moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_fault_monitor) == 0x1FFF8000,
           "DM4310 fault-monitor helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_fault_monitor) <= 0x136,
           "DM4310 fault-monitor veneer crossed ADC IRQ")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_output_sensor) == 0xB8,
           "DM4310 output-sensor helper size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_flash_writer) == 0x1FFF9950,
           "DM4310 flash writer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_flash_erase_only) == 0x1FFF9A38,
           "DM4310 erase-only primitive moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_flash_erase_only) <= 0x94,
           "DM4310 erase-only primitive crossed classic send helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_clear_runtime_loop_states) == 0x1FFF9D5C,
           "DM4310 clear-loop-state helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_clear_runtime_loop_states) <= 0x6C,
           "DM4310 clear-loop-state helper crossed selective reset")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_boot_record_writer) == 0x1FFF8640,
           "DM4310 fixed boot-record writer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_boot_record_writer) <=
               (0x1FFF8744 - 0x1FFF8640),
           "DM4310 fixed boot-record writer exceeds its recovered slot")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || dm4310_mcan_send_classic_helper == 0x1FFF9ACC,
           "DM4310 classic MCAN callback moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_mcan_send_classic) == 0x0C,
           "DM4310 classic MCAN callback veneer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || dm4310_mcan_send_fd_helper == 0x1FFF9BA0,
           "DM4310 FD MCAN callback moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_mcan_send_fd) == 0x10,
           "DM4310 FD MCAN callback veneer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_mcan_send_variable_fd) == 0x1FFF9B0E,
           "DM4310 variable-length FD sender moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_mcan_send_variable_fd) == 0x0C,
           "DM4310 variable-length FD sender copy size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_svpwm) == 0x10,
           "DM4310 SVPWM veneer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_irq002) <= (0x1FFF8640 - 0x1FFF8136),
           "DM4310 IRQ002 source veneer exceeds its recovered slot")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_irq000) <= (0x1FFF8770 - 0x1FFF8744),
           "DM4310 IRQ000 source veneer exceeds its recovered slot")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_irq001) <= (0x1FFF88B8 - 0x1FFF8770),
           "DM4310 IRQ001 source veneer exceeds its recovered slot")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_flash_writer) <= (0x1FFF9A38 - 0x1FFF9950),
           "DM4310 flash writer exceeds its recovered slot")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_sine_table) == 0x1FFFA674,
           "DM4310 sine table moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_runtime_drive_d) == 0x1FFFA510,
           "DM4310 D runtime drive state moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_runtime_drive_d) == 0x28,
           "DM4310 D runtime drive state size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_runtime_drive_q) == 0x1FFFA538,
           "DM4310 Q runtime drive state moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_runtime_drive_q) == 0x28,
           "DM4310 Q runtime drive state size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_runtime_speed_loop) == 0x1FFFA568,
           "DM4310 speed-loop state moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_runtime_speed_loop) == 0x28,
           "DM4310 speed-loop state size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_runtime_position_loop) == 0x1FFFA590,
           "DM4310 position-loop state moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_runtime_position_loop) == 0x28,
           "DM4310 position-loop state size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_config_staging) == 0x1FFFA5C8,
           "DM4310 config staging record moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_config_staging) == 0x94,
           "DM4310 config staging record size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_uart_length_state) == 0x1FFFA5B8,
           "DM4310 UART length state moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_uart_length_state) == 0x4,
           "DM4310 UART length state size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_hardware_variant) == 0x1FFFA5BC,
           "DM4310 hardware variant moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_hardware_variant) == 0x1,
           "DM4310 hardware variant size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_identification_filter) == 0x1FFF9E44,
           "DM4310 identification-filter helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_identification_filter) <= 0x31C,
           "DM4310 identification-filter veneer crossed next helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_output_sensor) == 0x1FFF987C,
           "DM4310 output-sensor helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_output_sensor) <= 0xD4,
           "DM4310 output-sensor veneer crossed flash writer")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_svpwm) == 0x1FFF9C40,
           "DM4310 SVPWM helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_svpwm) <= 0x204,
           "DM4310 SVPWM veneer crossed identification helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_reset_control_state) == 0x1FFF9DC8,
           "DM4310 control-reset helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_reset_control_state) == 0x10,
           "DM4310 control-reset veneer copy size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_motion_observer) == 0x1FFF9FDE,
           "DM4310 motion-observer helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_motion_observer) <= 0x9C,
           "DM4310 motion-observer veneer crossed current helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_current_controller) == 0x1FFFA07A,
           "DM4310 current-controller helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_current_controller) <= 0x9E,
           "DM4310 current-controller veneer crossed literal pool")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_rls2) == 0x1FFFA160,
           "DM4310 RLS helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_rls2) <= 0xD4,
           "DM4310 RLS veneer crossed next helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_flux_observer) == 0x1FFFA234,
           "DM4310 flux-observer helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_flux_observer) <= 0xD8,
           "DM4310 flux-observer veneer crossed next helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_sincos) == 0x1FFFA30C,
           "DM4310 sincos helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_sincos) <= 0x7E,
           "DM4310 sincos veneer crossed next helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_clamp) == 0x1FFFA38A,
           "DM4310 clamp helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_clamp) <= 0x20,
           "DM4310 clamp veneer crossed next helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_wrap) == 0x1FFFA3AA,
           "DM4310 wrap helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_wrap) == 0x6A,
           "DM4310 wrap and angle-reduction bodies changed size")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_limit_vector) == 0x1FFFA414,
           "DM4310 vector-limit helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_limit_vector) <= 0x56,
           "DM4310 vector-limit veneer crossed conversion helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_float_to_uint) == 0x1FFFA46A,
           "DM4310 float-to-uint helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_float_to_uint) <= 0x28,
           "DM4310 float-to-uint veneer crossed inverse conversion")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_uint_to_float) == 0x1FFFA492,
           "DM4310 uint-to-float helper moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_uint_to_float) <= 0x4A,
           "DM4310 uint-to-float veneer crossed derive-control helper")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_derive_control_parameters) == 0x1FFFA4DC,
           "DM4310 derive-control veneer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_derive_control_parameters) == 0xA,
           "DM4310 derive-control veneer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_atan2) == 0x1FFFA4FA,
           "DM4310 atan2 veneer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_atan2) == 0xA,
           "DM4310 atan2 veneer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_delay_ms) == 0x1FFFA4E6,
           "DM4310 delay-ms veneer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_delay_ms) == 0xA,
           "DM4310 delay-ms veneer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_select_configuration_bank_b) == 0x1FFFA4F0,
           "DM4310 bank-B veneer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_select_configuration_bank_b) == 0xA,
           "DM4310 bank-B veneer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_helper_sqrt) == 0x1FFFA504,
           "DM4310 sqrt veneer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_helper_sqrt) == 0xC,
           "DM4310 sqrt veneer plus scatter tail size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_c_runtime_state) == 0x1FFFF490,
           "DM4310 C runtime state moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_c_runtime_state) == 0x60,
           "DM4310 C runtime state size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_system_core_clock) == 0x1FFFF4F0,
           "DM4310 SystemCoreClock moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_system_core_clock) == 0x4,
           "DM4310 SystemCoreClock size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_hrc_value) == 0x1FFFF4F4,
           "DM4310 HRC_VALUE moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_hrc_value) == 0x4,
           "DM4310 HRC_VALUE size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || __factory_bss_end__ == 0x1FFFF4F8,
           "DM4310 factory BSS/heap boundary moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || __HeapBase == 0x1FFFF4F8,
           "DM4310 heap base moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || __HeapLimit == 0x1FFFF8F8,
           "DM4310 heap limit moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_sine_table) == 0x2004,
           "DM4310 sine table size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_aes_sbox) == 0x1FFFC678,
           "DM4310 AES S-box moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_aes_sbox) == 0x100,
           "DM4310 AES S-box size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_position_sensor_expected) == 0x1FFFA65C,
           "DM4310 position-sensor expected register image moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_position_sensor_expected) == 0x0C,
           "DM4310 position-sensor shared storage size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || position_dma_word == 0x1FFFA666,
           "DM4310 position DMA destination moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_adc_raw) == 0x1FFFC778 &&
           SIZEOF(.dm4310_adc_raw) == 0x14,
           "DM4310 ADC raw sample layout mismatch")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_current_d) == 0x1FFFC78C,
           "DM4310 D current controller moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_current_d) == 0x4C,
           "DM4310 D current controller size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_current_q) == 0x1FFFC7D8,
           "DM4310 Q current controller moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_current_q) == 0x4C,
           "DM4310 Q current controller size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_boot_record_staging) == 0x1FFFC824,
           "DM4310 boot-record staging moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_boot_record_staging) == 0x14,
           "DM4310 boot-record staging size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_short_response_buffer) == 0x1FFFC838,
           "DM4310 short response buffer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_short_response_buffer) == 0x14,
           "DM4310 short response buffer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_motor_correction) == 0x1FFFC84C,
           "DM4310 motor correction table moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_motor_correction) == 0x400,
           "DM4310 motor correction table size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_motor_record) == 0x1FFFCC6C,
           "DM4310 motor record moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_motor_record) == 0x40C,
           "DM4310 motor record size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_output_table) == 0x1FFFD078,
           "DM4310 output table moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_output_table) == 0x2000,
           "DM4310 output table size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_output_calibration) == 0x1FFFF078,
           "DM4310 output calibration moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_output_calibration) == 0x10,
           "DM4310 output calibration size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_motor_runtime_state) == 0x1FFFF088,
           "DM4310 motor runtime state moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_motor_runtime_state) == 0x7C,
           "DM4310 motor runtime state size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_sample_runtime_state) == 0x1FFFF104,
           "DM4310 sample runtime state moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_sample_runtime_state) == 0xA4,
           "DM4310 sample runtime state size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_output_sensor_state) == 0x1FFFF1A8,
           "DM4310 output sensor state moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_output_sensor_state) == 0x48,
           "DM4310 output sensor state slot changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_runtime_status) == 0x1FFFF1F0,
           "DM4310 runtime status moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_runtime_status) == 0x4C,
           "DM4310 runtime status size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_runtime_parameter_cache) == 0x1FFFF23C,
           "DM4310 runtime parameter cache moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_runtime_parameter_cache) == 0x60,
           "DM4310 runtime parameter cache size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_mcan_dispatch) == 0x1FFFF29C,
           "DM4310 MCAN dispatch moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_mcan_dispatch) == 0xA4,
           "DM4310 MCAN runtime workspace size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_calibration_frame_scratch) == 0x1FFFF340,
           "DM4310 calibration frame scratch moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_calibration_frame_scratch) == 0x40,
           "DM4310 calibration frame scratch size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_motion_observer) == 0x1FFFF380,
           "DM4310 motion observer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_motion_observer) == 0x3C,
           "DM4310 motion observer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_position_sensor_observed) == 0x1FFFF3BC,
           "DM4310 position-sensor observed register image moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_position_sensor_observed) == 0x0B,
           "DM4310 position-sensor observed register image size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.dm4310_uart_rx_buffer) == 0x1FFFF3C7,
           "DM4310 UART RX buffer moved")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_uart_rx_buffer) == 0xC8,
           "DM4310 UART RX buffer size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || SIZEOF(.dm4310_app_state) == 0x320,
           "DM4310 source app-state size changed")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || ADDR(.bss) >= __StackTop,
           "DM4310 source BSS overlaps the factory stack")
    ASSERT((__dm8009_layout || __dm43_48v_layout) || __etext_ramb + SIZEOF(.ramb_data) <= ORIGIN(FLASH) + LENGTH(FLASH),
           "app FLASH overflow")
}
