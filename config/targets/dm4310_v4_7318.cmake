# DM4310 V4 7318 firmware target profile.
#
# Keep this file intentionally small for the first multi-target split: it
# creates a concrete target identity without changing board code paths yet.
# Model-specific constants should move here as they are recovered.

set(DAMIAO_TARGET_COMPILE_DEFINITIONS
    DAMIAO_DM4310=1
)
