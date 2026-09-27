#ifndef DM4310_BOARD_POWER_STAGE_H
#define DM4310_BOARD_POWER_STAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    BOARD_POWER_STAGE_PORT_A = 0,
    BOARD_POWER_STAGE_PORT_B = 1,
} BoardPowerStagePort;

typedef struct {
    BoardPowerStagePort port;
    uint16_t pin_mask;
    uint8_t adc_index;
    uint8_t failure_mask;
} BoardPowerStageTestStep;

#define BOARD_POWER_STAGE_TEST_STEP_COUNT (6U)
#define BOARD_POWER_STAGE_ADC_MIN          (1000U)
#define BOARD_POWER_STAGE_ADC_MAX          (3000U)

void board_power_stage_build_self_test_sequence(
    BoardPowerStageTestStep steps[BOARD_POWER_STAGE_TEST_STEP_COUNT]);
bool board_power_stage_adc_sample_valid(uint16_t sample);

/* Recovered startup test.  It changes each pin's output-data latch while the
 * original PCR image keeps GPIO POUTE disabled, then checks ADC DR0.  Do not
 * add POERA/POERB writes here: that changes the target's electrical behavior.
 * failure_mask uses the original layout: bit 5..0 = WH, WL, VH, VL, UH, UL. */
bool board_power_stage_self_test(uint8_t *failure_mask);

#endif
