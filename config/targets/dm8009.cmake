# DM8009 V3 6417 firmware target profile.
#
# V6417 uses the same recovered high-level implementation as the V3 DM43xx
# firmware.  Keep a dedicated model identity while the compatibility define
# selects that audited source path; the target-specific linker layout remains
# separate because the fixed SRAM ABI is different.

set(DAMIAO_TARGET_COMPILE_DEFINITIONS
    DAMIAO_DM4310=1
    DAMIAO_DM8009_V3=1
)

set(DAMIAO_TARGET_LINK_OPTIONS
    -Wl,--defsym=DAMIAO_DM8009_LAYOUT=1
    -Wl,--wrap=__aeabi_dadd
    -Wl,--wrap=__aeabi_dsub
    -Wl,--wrap=__aeabi_dmul
    -Wl,--wrap=__aeabi_ddiv
    -Wl,--wrap=__aeabi_f2d
    -Wl,--wrap=__aeabi_ui2d
    -Wl,--wrap=__aeabi_d2uiz)

# Shared code keeps family-level section names; the linker selects the audited
# V6417 fixed-SRAM addresses and isolates the seven objects whose ordering
# differs from V5017/V5117.
set(DAMIAO_TARGET_LINKER_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/linker/hc32f448_dm4310_app.ld")
