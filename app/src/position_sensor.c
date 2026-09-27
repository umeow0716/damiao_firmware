#include "position_sensor.h"

#include <string.h>

/* Exact float constants used by position_sensor_dma_irq at 0x1fff8770. */
#define COUNTS_PER_BIN 64U
#define COUNT_TO_RAD 0x1.921fb6p-12f
#define TWO_PI 0x1.921fb6p+2f
#define WRAP_THRESHOLD 5.5f
#define VELOCITY_BANDWIDTH_LIMIT_BITS UINT32_C(0x43fa0000)

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void position_sensor_init(PositionSensorState *state, bool inverted,
                          const float correction_table[256],
                          float gear_ratio, float output_offset,
                          float velocity_bandwidth,
                          float velocity_sample_frequency,
                          uint8_t velocity_decimation)
{
    memset(state, 0, sizeof(*state));
    state->correction_table = correction_table;
    state->inverted = inverted;
    state->gear_ratio = gear_ratio;
    state->output_scale = 1.0f / gear_ratio;
    state->output_offset = output_offset;
    state->velocity_sample_frequency = velocity_sample_frequency;
    state->velocity_decimation = velocity_decimation;
    /* load_motor_configuration@0x228c0 performs this as a signed comparison
     * of the IEEE-754 word against 500.0f.  This deliberately accepts zero
     * and negative finite values; the persistent-record decoder is where
     * NaN/default and upper-bound normalization happens. */
    if ((int32_t)float_bits(velocity_bandwidth) <
        (int32_t)VELOCITY_BANDWIDTH_LIMIT_BITS) {
        state->velocity_previous_weight = velocity_sample_frequency /
            (velocity_sample_frequency + velocity_bandwidth * TWO_PI);
        state->velocity_new_weight = 1.0f -
                                     state->velocity_previous_weight;
    } else {
        state->velocity_new_weight = 1.0f;
    }
}

static float wrap_angle(float angle)
{
    while (angle >= TWO_PI) {
        angle -= TWO_PI;
    }
    while (angle < 0.0f) {
        angle += TWO_PI;
    }
    return angle;
}

void position_sensor_update(PositionSensorState *state, uint16_t dma_word)
{
    uint16_t raw = (uint16_t)((dma_word >> 2U) & 0x3fffU);
    if (state->inverted) {
        raw = (uint16_t)(0x3fffU - raw);
    }
    const uint16_t index = raw >> 6U;
    const uint16_t next = (uint16_t)((index + 1U) & 0xffU);
    const float fraction = (float)(raw & (COUNTS_PER_BIN - 1U)) /
                           (float)COUNTS_PER_BIN;
    float correction = 0.0f;
    if (state->correction_table != NULL) {
        correction = state->correction_table[index] +
            ((state->correction_table[next] -
              state->correction_table[index]) * fraction);
    }
    const float wrapped = wrap_angle(((float)raw + correction) * COUNT_TO_RAD);

    float delta = wrapped - state->previous_angle;
    if (delta > WRAP_THRESHOLD) {
        delta -= TWO_PI;
        --state->revolutions;
    } else if (delta < -WRAP_THRESHOLD) {
        delta += TWO_PI;
        ++state->revolutions;
    }

    state->raw_position = raw;
    state->wrapped_angle = wrapped;
    state->previous_angle = wrapped;
    state->delta_angle = delta;
    state->accumulated_delta += delta;
    state->continuous_angle = wrapped + ((float)state->revolutions * TWO_PI);
    state->output_position =
        (state->continuous_angle * state->output_scale) - state->output_offset;
}

void position_sensor_control_tick(PositionSensorState *state)
{
    if ((state->velocity_decimation != 0U) &&
        (++state->velocity_divider >= state->velocity_decimation)) {
        const float raw_velocity = state->accumulated_delta *
                                   state->velocity_sample_frequency;
        state->rotor_velocity =
            state->rotor_velocity * state->velocity_previous_weight +
            raw_velocity * state->velocity_new_weight;
        state->output_velocity = state->rotor_velocity * state->output_scale;
        state->accumulated_delta = 0.0f;
        state->velocity_divider = 0U;
    }
}

void position_sensor_align_to_output(PositionSensorState *state,
                                     float output_position)
{
    const float turns = (state->gear_ratio *
                         (output_position + state->output_offset) -
                         state->wrapped_angle) / TWO_PI;
    state->revolutions = turns < 0.0f ? (int32_t)(turns - 0.5f) :
                                       (int32_t)(turns + 0.5f);
    state->continuous_angle = state->wrapped_angle +
                              (float)state->revolutions * TWO_PI;
    state->output_position =
        state->continuous_angle * state->output_scale - state->output_offset;
}

void position_sensor_set_inverted(PositionSensorState *state, bool inverted)
{
    if (state->inverted != inverted) {
        state->inverted = inverted;
        state->revolutions = 0;
        state->accumulated_delta = 0.0f;
    }
}

void position_sensor_set_output_offset(PositionSensorState *state,
                                       float output_offset)
{
    state->output_offset = output_offset;
}
