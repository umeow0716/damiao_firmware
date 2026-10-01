#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "sensor_calibration.h"

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float bits_float(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

int main(void)
{
    uint16_t samples[POSITION_CALIBRATION_SAMPLE_COUNT] = {0};
    float maximum_step;

    samples[0] = 35824U;
    samples[1] = 23940U;
    for (size_t index = 2U; index < POSITION_CALIBRATION_SAMPLE_COUNT;
         ++index) {
        samples[index] = samples[1];
    }
    assert(sensor_calibration_validate_position(
               samples, POSITION_CALIBRATION_SAMPLE_COUNT,
               &maximum_step) == MOTOR_FAULT_OUTPUT_CALIBRATION);
    /* Locks the factory's convert/multiply/subtract/wrap order. */
    assert(float_bits(maximum_step) == UINT32_C(0x413f2576));

    samples[1] = UINT16_MAX;
    assert(sensor_calibration_validate_position(
               samples, POSITION_CALIBRATION_SAMPLE_COUNT,
               &maximum_step) == MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING);

    assert(!sensor_calibration_current_mean_valid(
        bits_float(UINT32_C(0x433a2e8b))));
    assert(sensor_calibration_current_mean_valid(
        bits_float(UINT32_C(0x433a2e8c))));
    assert(sensor_calibration_current_mean_valid(
        bits_float(UINT32_C(0x45745d17))));
    assert(!sensor_calibration_current_mean_valid(
        bits_float(UINT32_C(0x45745d18))));
    assert(!sensor_calibration_current_mean_valid(
        bits_float(UINT32_C(0x7fc00001))));
    return 0;
}
