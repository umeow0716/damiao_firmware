# Shared source/runtime settings for the recovered V3 single-motor APPs.
# Model profiles own only their identity and fixed-SRAM layout selector.

set(DAMIAO_TARGET_LINK_OPTIONS
    -Wl,--wrap=__aeabi_dadd
    -Wl,--wrap=__aeabi_dsub
    -Wl,--wrap=__aeabi_dmul
    -Wl,--wrap=__aeabi_ddiv
    -Wl,--wrap=__aeabi_f2d
    -Wl,--wrap=__aeabi_ui2d
    -Wl,--wrap=__aeabi_d2uiz)

set(DAMIAO_TARGET_LINKER_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/linker/hc32f448_dm4310_app.ld")
