# DM4340 V3 5117 firmware target profile.
#
# The two factory images have byte-identical executable code and fixed-SRAM
# helpers.  DAMIAO_DM4310 therefore remains defined as a compatibility marker
# for the recovered DM43xx code/layout path while DAMIAO_DM4340 selects the
# model-specific identity and eight initialized configuration values.

set(DAMIAO_TARGET_COMPILE_DEFINITIONS
    DAMIAO_DM4310=1
    DAMIAO_DM4340=1
)

set(DAMIAO_TARGET_LINK_OPTIONS
    -Wl,--wrap=__aeabi_dadd
    -Wl,--wrap=__aeabi_dsub
    -Wl,--wrap=__aeabi_dmul
    -Wl,--wrap=__aeabi_ddiv
    -Wl,--wrap=__aeabi_f2d
    -Wl,--wrap=__aeabi_ui2d
    -Wl,--wrap=__aeabi_d2uiz)

# DM4310 V5017 and DM4340 V5117 use the same fixed SRAM ABI and vector table.
set(DAMIAO_TARGET_LINKER_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/linker/hc32f448_dm4310_app.ld")
