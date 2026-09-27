# DM8009 V4 7318 firmware target profile.
#
# This initially shares the recovered HC32F448 board implementation with the
# DM4310 target. GPIO, motor constants, factory defaults, and any V7318-only
# behavior can be redirected from this profile as each item is verified.

set(DAMIAO_TARGET_COMPILE_DEFINITIONS
    DAMIAO_DM8009=1
)
