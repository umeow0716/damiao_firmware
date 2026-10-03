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

void calibration_store_decode_zero(MotorController *controller,
                                   const uint32_t words[ZERO_POSITION_RECORD_WORD_COUNT])
{
    const uint32_t output_bits = words[0];
    const uint32_t motor_bits = words[1];
    const float output_position_offset =
        bits_float((output_bits & UINT32_C(0x7fffffff)) > UINT32_C(0x7f800000) ? 0U : output_bits);
    const float motor_output_position_offset =
        bits_float((motor_bits & UINT32_C(0x7fffffff)) > UINT32_C(0x7f800000) ? 0U : motor_bits);
    controller->output_position_offset = output_position_offset;
    controller->motor_output_position_offset = motor_output_position_offset;
}

void calibration_store_encode_zero(const MotorController *controller,
                                   uint32_t words[ZERO_POSITION_RECORD_WORD_COUNT])
{
    /* FE stores both live offsets verbatim.  Non-finite values are normalized
     * only by the next boot's decode path, not before the Flash write. */
    words[0] = float_bits(controller->output_position_offset);
    words[1] = float_bits(controller->motor_output_position_offset);
}
