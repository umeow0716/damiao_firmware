# Shared source and runtime settings for the V3 single-motor applications.
# Model profiles own only their identity and fixed-SRAM layout selector.

set(DAMIAO_TARGET_LINK_OPTIONS
    -Wl,--wrap=__aeabi_dadd
    -Wl,--wrap=__aeabi_dsub
    -Wl,--wrap=__aeabi_dmul
    -Wl,--wrap=__aeabi_ddiv
    -Wl,--wrap=__aeabi_f2d
    -Wl,--wrap=__aeabi_ui2d
    -Wl,--wrap=__aeabi_d2uiz
)

set(DAMIAO_TARGET_LINKER_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/linker/hc32f448_v3_app.ld"
)
