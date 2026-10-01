/* Isolated host test for recovered MCAN malformed-frame behavior.
 * It is intentionally absent from CMake and the normal make graph. */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "can_protocol.h"

/* Keep this host-only test independent of the target math/table objects. */
uint32_t motor_float_to_uint(float value, float minimum, float maximum,
                             uint8_t bits)
{
    const float span = maximum - minimum;
    const uint32_t maximum_integer = (1U << bits) - 1U;
    return (uint32_t)((value - minimum) * (float)maximum_integer / span);
}

float motor_uint_to_float(uint32_t value, float minimum, float maximum,
                          uint8_t bits)
{
    const uint32_t maximum_integer = (1U << bits) - 1U;
    return (float)value * (maximum - minimum) /
           (float)maximum_integer + minimum;
}

uint32_t dm4310_float_to_uint_helper(float value, float minimum,
                                     float maximum, uint8_t bits)
{
    return motor_float_to_uint(value, minimum, maximum, bits);
}

float dm4310_uint_to_float_helper(uint32_t value, float minimum,
                                  float maximum, uint8_t bits)
{
    return motor_uint_to_float(value, minimum, maximum, bits);
}

static MotorConfig test_config(void)
{
    MotorConfig config;
    memset(&config, 0, sizeof(config));
    config.can_id = 3U;
    config.master_id = 5U;
    config.control_mode = MOTOR_MODE_MIT;
    config.direction = 1.0f;
    config.position_min = -12.5f;
    config.position_max = 12.5f;
    config.velocity_min = -30.0f;
    config.velocity_max = 30.0f;
    config.torque_min = -10.0f;
    config.torque_max = 10.0f;
    config.kp_max = 500.0f;
    config.kd_max = 5.0f;
    return config;
}

int main(void)
{
    MotorConfig config = test_config();
    MotorCommand command;
    CanFrame frame;

    memset(&frame, 0xFF, sizeof(frame));
    frame.id = config.can_id;
    frame.length = 0U;
    frame.data[7] = 0xFCU;
    assert(can_protocol_decode_command(&frame, &config, &command) ==
           CAN_COMMAND_ENABLE);

    /* Factory IRQ reads two payload words regardless of DLC. Length zero is
     * therefore not a rejection condition once the zeroed RX object contains
     * an otherwise valid command image. */
    memset(&frame, 0, sizeof(frame));
    frame.id = config.can_id;
    frame.length = 0U;
    assert(can_protocol_decode_command(&frame, &config, &command) ==
           CAN_COMMAND_SETPOINT);

    frame.id = (7U << 8U) | config.can_id;
    assert(can_protocol_decode_command(&frame, &config, &command) ==
           CAN_COMMAND_FEEDBACK_ONLY);

    frame.id = config.can_id + 1U;
    assert(can_protocol_decode_command(&frame, &config, &command) ==
           CAN_COMMAND_NONE);

    /* A near-special payload with one non-FF prefix byte falls through to
     * normal family decoding; it is not treated as a malformed special. */
    memset(&frame, 0xFF, sizeof(frame));
    frame.id = config.can_id;
    frame.data[0] = 0xFEU;
    frame.data[7] = 0xFDU;
    assert(can_protocol_decode_command(&frame, &config, &command) ==
           CAN_COMMAND_SETPOINT);

    return 0;
}
