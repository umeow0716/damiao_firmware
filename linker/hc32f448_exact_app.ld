MEMORY
{
    FLASH (rx) : ORIGIN = 0x00020000, LENGTH = 128K
}

dm4310_reset_entry = 0x00020389;
ENTRY(dm4310_reset_entry)

SECTIONS
{
    .app_image 0x00020000 :
    {
        KEEP(*(.app_image))
    } > FLASH

    /DISCARD/ :
    {
        *(.comment)
        *(.ARM.attributes)
        *(.note*)
    }

    ASSERT(ADDR(.app_image) == 0x00020000, "exact app base changed")
    ASSERT(SIZEOF(.app_image) == 51284, "exact app evidence size changed")
}
