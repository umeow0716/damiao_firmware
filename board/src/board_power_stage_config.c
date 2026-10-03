#include "board_power_stage.h"

void board_power_stage_build_self_test_sequence(
    BoardPowerStageTestStep steps[BOARD_POWER_STAGE_TEST_STEP_COUNT])
{
    if (steps == NULL)
    {
        return;
    }

    /* The startup self-test maps PA8/9/10 and PB13/14/15 to the three low/high
     * pairs, but PCR POUTE is
     * deliberately clear during this routine: these steps change latches,
     * not enabled GPIO drivers.  Each pair is checked against its ADC DR0. */
    steps[0] =
        (BoardPowerStageTestStep){BOARD_POWER_STAGE_PORT_A, (uint16_t)(1U << 8U), 0U, (1U << 0U)};
    steps[1] =
        (BoardPowerStageTestStep){BOARD_POWER_STAGE_PORT_A, (uint16_t)(1U << 9U), 1U, (1U << 2U)};
    steps[2] =
        (BoardPowerStageTestStep){BOARD_POWER_STAGE_PORT_A, (uint16_t)(1U << 10U), 2U, (1U << 4U)};
    steps[3] =
        (BoardPowerStageTestStep){BOARD_POWER_STAGE_PORT_B, (uint16_t)(1U << 13U), 0U, (1U << 1U)};
    steps[4] =
        (BoardPowerStageTestStep){BOARD_POWER_STAGE_PORT_B, (uint16_t)(1U << 14U), 1U, (1U << 3U)};
    steps[5] =
        (BoardPowerStageTestStep){BOARD_POWER_STAGE_PORT_B, (uint16_t)(1U << 15U), 2U, (1U << 5U)};
}

bool board_power_stage_adc_sample_valid(uint16_t sample)
{
    /* Use unsigned (sample - 1000) <= 2000 so both limits
     * are inclusive and values below 1000 fail by unsigned underflow. */
    return ((uint32_t)sample - BOARD_POWER_STAGE_ADC_MIN) <=
           (BOARD_POWER_STAGE_ADC_MAX - BOARD_POWER_STAGE_ADC_MIN);
}
