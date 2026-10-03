#include <stdint.h>

#define MOTOR_SINE_TABLE_SECTION __attribute__((section(".sine_table")))

/* Storage is target-owned because the SRAM helper ABI fixes this table's
 * address.  The lookup algorithm remains shared. */
float motor_sine_table[2049] MOTOR_SINE_TABLE_SECTION;
