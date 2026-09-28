#include "output_sensor.h"

#include <math.h>
#include <string.h>

#include "motor_math.h"
#include "app_profile.h"
/* Exact transforms used by output_sensor_lookup_and_unwrap@0x1fff987c.
 * This is the output-side analogue sin/cos encoder, distinct from the
 * SPI/DMA motor encoder. */
#define TWO_PI_F             0x1.921fb6p+2f
#if defined(DAMIAO_DM4310)
#define TABLE_INDEX_CENTER   2048.0f
#define ANGLE_TO_TABLE_INDEX 0x1.45f306p+9f
#define TABLE_COUNT_TO_RAD   0x1.921fb6p-10f
#elif defined(DAMIAO_DM8009)
#define TABLE_INDEX_CENTER   128.0f
#define ANGLE_TO_TABLE_INDEX 40.743663788f
#define TABLE_COUNT_TO_RAD   0x1.921fb6p-10f
#endif
#define WRAP_THRESHOLD       5.5f
#define FILTER_SAMPLE_RATE   20000.0f

static void apply_calibration(OutputSensorState *state,
                              const float calibration[4],
                              float initial_u, float initial_v)
{
    state->center_u = calibration[0];
    state->center_v = calibration[1];
    state->gain_v = calibration[2];
    motor_fast_sincos(calibration[3], &state->phase_sine,
                      &state->phase_cosine);
    state->filtered_u = initial_u;
    state->filtered_v = initial_v;
    state->wrapped_angle = 0.0f;
    state->previous_angle = 0.0f;
    state->continuous_angle = 0.0f;
    state->revolutions = 0;
}

static void decode_filtered_angle(OutputSensorState *state)
{
    const float uncorrected = output_sensor_uncorrected_angle(state);

#if defined(DAMIAO_DM4310)
    int32_t index = (int32_t)(TABLE_INDEX_CENTER +
                              uncorrected * ANGLE_TO_TABLE_INDEX);
    if (index < 0) {
        index += 4096;
    } else if (index >= 4096) {
        index -= 4096;
    }
    /* load_and_validate_calibration always copies the target's 8 KiB table,
     * even when it subsequently reports status 2 (erased entry) or 3
     * (excessive step).  Keeping the pointer check makes the reusable module
     * well-defined without changing the normal firmware path. */
    const float wrapped = state->correction_table != NULL ?
        ((float)state->correction_table[index] - TABLE_INDEX_CENTER) *
            TABLE_COUNT_TO_RAD : 0.0f;
#elif defined(DAMIAO_DM8009)
    float scaled = TABLE_INDEX_CENTER +
                   uncorrected * ANGLE_TO_TABLE_INDEX;
    while (scaled < 0.0f) {
        scaled += (float)CORRECTION_TABLE_COUNT;
    }
    while (scaled >= (float)CORRECTION_TABLE_COUNT) {
        scaled -= (float)CORRECTION_TABLE_COUNT;
    }
    const uint32_t index_u = (uint32_t)scaled;
    const uint32_t index_next = (index_u + 1U) & 0xFFU;
    const float fraction = scaled - (float)index_u;
    const float table_base = state->correction_table != NULL ?
        state->correction_table[index_u] : 0.0f;
    const float table_next = state->correction_table != NULL ?
        state->correction_table[index_next] : 0.0f;
    const float correction = table_base +
        fraction * (table_next - table_base);
    /* The DM8009 0x3a000 table stores the residual error of the raw
     * analogue output angle.  Factory V7318 subtracts that residual from
     * atan2(), rather than treating the entry as an absolute position or
     * adding it.  Adding it shifts the boot banner by roughly twice the
     * local correction, which is the observed -1.3978/-1.1833 drift away
     * from the factory -1.2582 position. */
    const float wrapped = state->correction_table != NULL ?
        uncorrected - correction : uncorrected;
#endif

    const float delta = wrapped - state->previous_angle;
    if (delta > WRAP_THRESHOLD) {
        --state->revolutions;
    } else if (delta < -WRAP_THRESHOLD) {
        ++state->revolutions;
    }

    state->wrapped_angle = wrapped;
    state->previous_angle = wrapped;
    state->continuous_angle = wrapped +
        (float)state->revolutions * TWO_PI_F - state->zero_offset;
}

void output_sensor_init(OutputSensorState *state,
                        const float calibration[4],
                        const CorrectionTableEntry correction_table[CORRECTION_TABLE_COUNT],
                        float zero_offset,
                        float initial_u, float initial_v)
{
    memset(state, 0, sizeof(*state));
    state->correction_table = correction_table;
    state->filtered_u = initial_u;
    state->filtered_v = initial_v;
    /* load_and_validate_calibration@0x22658 copies these four per-device
     * words verbatim.  Validation belongs to the commissioning producer,
     * not to the startup consumer. */
    apply_calibration(state, calibration, initial_u, initial_v);
    /* The 0x36000 loader has already applied the original NaN-only rule. */
    state->zero_offset = zero_offset;

    /* Captured state is a first-order 50 Hz filter at the 20 kHz ADC rate. */
    const float sensor_bandwidth = 50.0f;
    state->filter_previous_weight = FILTER_SAMPLE_RATE /
        (FILTER_SAMPLE_RATE + sensor_bandwidth * TWO_PI_F);
    state->filter_new_weight = 1.0f - state->filter_previous_weight;
    /* validate_current_sensors@0x24ec0 decodes the averaged U/V sample once
     * before the runtime ADC trigger is enabled. */
    decode_filtered_angle(state);
}

void output_sensor_update(OutputSensorState *state,
                          uint16_t raw_u, uint16_t raw_v)
{
    state->filtered_u = state->filtered_u * state->filter_previous_weight +
                        (float)raw_u * state->filter_new_weight;
    state->filtered_v = state->filtered_v * state->filter_previous_weight +
                        (float)raw_v * state->filter_new_weight;

    decode_filtered_angle(state);
}

void output_sensor_normalize_startup(OutputSensorState *state)
{
    /* validate_current_sensors normalizes the first zero-corrected output
     * angle and adjusts the matching turn count before aligning the SPI
     * encoder's multi-turn state. */
    if (state->continuous_angle > 3.1415927410125732f) {
        state->continuous_angle -= TWO_PI_F;
        --state->revolutions;
    }
    if (state->continuous_angle < -3.1415927410125732f) {
        state->continuous_angle += TWO_PI_F;
        ++state->revolutions;
    }
}

void output_sensor_set_zero_offset(OutputSensorState *state,
                                   float zero_offset)
{
    state->zero_offset = zero_offset;
}

bool output_sensor_set_calibration(OutputSensorState *state,
                                   const float calibration[4],
                                   float initial_u, float initial_v)
{
    if ((state == NULL) || (calibration == NULL)) {
        return false;
    }
    apply_calibration(state, calibration, initial_u, initial_v);
    return true;
}

float output_sensor_uncorrected_angle(const OutputSensorState *state)
{
    if (state == NULL) {
        return 0.0f;
    }
    const float u = state->filtered_u - state->center_u;
    const float v = (state->filtered_v - state->center_v) * state->gain_v;
    return atan2f(v - u * state->phase_cosine,
                  u * state->phase_sine);
}
