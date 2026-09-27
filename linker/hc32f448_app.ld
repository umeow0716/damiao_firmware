MEMORY
{
    /* 0x30000 and above contain the recovered configuration and calibration
     * records.  Keep source application code in the original 64 KiB window. */
    FLASH (rx)  : ORIGIN = 0x00020000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x1FFF8000, LENGTH = 64K
    RAMB  (rwx) : ORIGIN = 0x200F0000, LENGTH = 4K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .vectors : {
        . = ALIGN(4);
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > FLASH

    .text : {
        . = ALIGN(4);
        *(.text .text*)
        *(.glue_7 .glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } > FLASH

    .rodata : {
        . = ALIGN(4);
        *(.rodata .rodata*)
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

    __etext_ramb = __etext + ALIGN(SIZEOF(.data), 4);
    .ramb_data : AT(__etext_ramb) {
        __data_start_ramb__ = .;
        *(.ramb_data .ramb_data*)
        __data_end_ramb__ = .;
    } > RAMB

    .bss (NOLOAD) : {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss .bss*) *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        *(.noinit .noinit*)
    } > RAM
    .ramb_bss (NOLOAD) : {
        __bss_start_ramb__ = .;
        *(.ramb_bss .ramb_bss*)
        __bss_end_ramb__ = .;
    } > RAMB

    __end__ = ALIGN(__bss_end__, 8);
    PROVIDE(end = __end__);
    PROVIDE(_end = __end__);
    __HeapBase = __end__;
    __HeapLimit = __end__;
    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - 0x1000;

    .ARM.attributes 0 : { *(.ARM.attributes) }
    PROVIDE(_stack = __StackTop);
    ASSERT(__end__ <= __StackLimit, "app RAM overlaps the 4 KiB stack")
    ASSERT(__etext_ramb + SIZEOF(.ramb_data) <= ORIGIN(FLASH) + LENGTH(FLASH), "app FLASH overflow")
}
