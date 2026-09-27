#include "calibration_store.h"

#include <math.h>
#include <string.h>

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

void calibration_store_decode_zero(
    MotorController *controller,
    const uint32_t words[ZERO_POSITION_RECORD_WORD_COUNT])
{
    float output_position_offset = bits_float(words[0]);
    float motor_output_position_offset = bits_float(words[1]);
    /* The original abs(bits)>0x7f800000 predicate replaces NaNs only;
     * infinities survive exactly as stored. */
    if (isnan(output_position_offset)) {
        output_position_offset = 0.0f;
    }
    if (isnan(motor_output_position_offset)) {
        motor_output_position_offset = 0.0f;
    }
    controller->output_position_offset = output_position_offset;
    controller->motor_output_position_offset = motor_output_position_offset;
}

void calibration_store_encode_zero(
    const MotorController *controller,
    uint32_t words[ZERO_POSITION_RECORD_WORD_COUNT])
{
    /* FE stores both live offsets verbatim.  Non-finite values are normalized
     * only by the next boot's decode path, not before the Flash write. */
    words[0] = float_bits(controller->output_position_offset);
    words[1] = float_bits(controller->motor_output_position_offset);
}
