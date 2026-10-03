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

include("${CMAKE_CURRENT_LIST_DIR}/dm_v3_common.cmake")
