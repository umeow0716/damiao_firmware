#include <stdint.h>

#if defined(DAMIAO_DM4310)
#define MOTOR_SINE_TABLE_SECTION \
    __attribute__((section(".dm4310_sine_table")))
#else
#define MOTOR_SINE_TABLE_SECTION
#endif

/* Storage is target-owned because the V5017 RAM-code literal pool requires
 * this table at 0x1fffa674.  The lookup algorithm remains shared. */
float motor_sine_table[2049] MOTOR_SINE_TABLE_SECTION;
