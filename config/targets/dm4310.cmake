# DM4310 V3 5017 firmware target profile.
#
# Keep this file intentionally small for the first multi-target split: it
# creates a concrete target identity without changing board code paths yet.
# Model-specific constants should move here as they are recovered.

set(DAMIAO_TARGET_COMPILE_DEFINITIONS
    DAMIAO_DM4310=1
)

# libgcc packages these conversions together with double addition. Redirect callers
# without allowing duplicate definitions or changing other model runtimes.
set(DAMIAO_TARGET_LINK_OPTIONS
    -Wl,--wrap=__aeabi_dadd
    -Wl,--wrap=__aeabi_dsub
    -Wl,--wrap=__aeabi_dmul
    -Wl,--wrap=__aeabi_ddiv
    -Wl,--wrap=__aeabi_f2d
    -Wl,--wrap=__aeabi_ui2d
    -Wl,--wrap=__aeabi_d2uiz)

# Keep the recovered V5017 SRAM layout target-specific: DM8009 uses different
# relocated data and must not inherit addresses merely because it shares the
# MCU and source modules.
set(DAMIAO_TARGET_LINKER_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/linker/hc32f448_dm4310_app.ld")
