# DM8009 V4 7318 firmware target profile.
#
# Keep DM8009-specific constants, calibration layout, GPIO side effects, and
# V7318-only behavior isolated from the DM4310 golden candidate. Shared board
# support is reused only where the APP disassembly matches.

set(DAMIAO_TARGET_COMPILE_DEFINITIONS
    DAMIAO_DM8009=1
)
