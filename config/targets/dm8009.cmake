# DM8009 V3 6417 firmware target profile.
#
# V6417 uses the same recovered high-level implementation as the V3 DM43xx
# firmware.  Keep a dedicated model identity while the compatibility define
# selects that audited source path; the target-specific linker layout remains
# separate because the fixed SRAM ABI is different.

set(DAMIAO_TARGET_COMPILE_DEFINITIONS
    DAMIAO_DM4310=1
    DAMIAO_DM8009_V3=1
    DAMIAO_LAYOUT_DM800X=1
)

include("${CMAKE_CURRENT_LIST_DIR}/dm_v3_common.cmake")
list(APPEND DAMIAO_TARGET_LINK_OPTIONS
    -Wl,--defsym=DAMIAO_DM8009_LAYOUT=1)
