# DM8009 V3 6417 firmware target profile.
#
# This target uses the shared V3 implementation. Its dedicated model identity
# selects profile values while the alternate layout selects its
# fixed-SRAM ABI.

set(DAMIAO_TARGET_COMPILE_DEFINITIONS
    DAMIAO_MODEL_DM8009=1
    DAMIAO_LAYOUT_RELOCATED_SRAM=1
)

include("${CMAKE_CURRENT_LIST_DIR}/dm_v3_common.cmake")
list(APPEND DAMIAO_TARGET_LINK_OPTIONS
    -Wl,--defsym=DAMIAO_LAYOUT_RELOCATED_SRAM=1
)
