# DM4340 V3 5117 firmware target profile.
#
# The variants share executable logic and fixed-SRAM helpers. The model define
# selects only the DM4340 identity and profile values; every target uses the
# common V3 implementation.

set(DAMIAO_TARGET_COMPILE_DEFINITIONS
    DAMIAO_MODEL_DM4340=1
)

include("${CMAKE_CURRENT_LIST_DIR}/dm_v3_common.cmake")
