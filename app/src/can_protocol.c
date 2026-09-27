#include "can_protocol.h"

#include <stddef.h>
#include <string.h>

#include "motor_math.h"

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static uint16_t clamp_pvt_u16(uint16_t value)
{
    /* The recovered IRQ stores each packed PVT limit in a halfword and
     * clamps it to 10000 before converting it to float. */
    return value > 10000U ? 10000U : value;
}

static float read_f32_le(const uint8_t *data)
{
    const uint32_t bits = (uint32_t)data[0] |
                          ((uint32_t)data[1] << 8U) |
                          ((uint32_t)data[2] << 16U) |
                          ((uint32_t)data[3] << 24U);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint8_t encode_temperature(float temperature)
{
    /* The original uses VCVT.S32.F32 and stores the low byte. */
    return (uint8_t)(int32_t)temperature;
}

static float external_direction_sign(const MotorConfig *config)
{
    /* detect_motor_direction_and_pole_pairs stores 1 for the positive wire
     * convention and 2 for the mechanically reversed convention.  The
     * original MCAN IRQ converts at the protocol boundary so the internal
     * controller and sensor coordinates remain coherent. */
    return config->direction == 2.0f ? -1.0f : 1.0f;
}

static bool is_special_command(const CanFrame *frame, uint8_t command)
{
    for (uint8_t i = 0U; i < 7U; ++i) {
        if (frame->data[i] != 0xFFU) {
            return false;
        }
    }
    return frame->data[7] == command;
}

CanCommandKind can_protocol_decode_command(const CanFrame *frame,
                                           const MotorConfig *config,
                                           MotorCommand *command)
{
    if ((frame == NULL) || (config == NULL) || (command == NULL)) {
        return CAN_COMMAND_NONE;
    }

    /* Filter 0 uses 0x0ff as the mask.  The original handler likewise
     * compares the received standard ID's low byte with CAN_ID, then uses
     * ID[10:8] to select the command family. */
    if ((frame->id & 0xFFU) != config->can_id) {
        return CAN_COMMAND_NONE;
    }
    const uint32_t family = (frame->id >> 8U) & 0x07U;

    {
        /* These four command bytes are independently documented and present
         * in the recovered MCAN receive handler at 0x1fff88b8. */
        if (is_special_command(frame, 0xFCU)) {
            return CAN_COMMAND_ENABLE;
        }
        if (is_special_command(frame, 0xFDU)) {
            return CAN_COMMAND_DISABLE;
        }
        if (is_special_command(frame, 0xFEU)) {
            return CAN_COMMAND_SET_ZERO;
        }
        if (is_special_command(frame, 0xFBU)) {
            return CAN_COMMAND_CLEAR_FAULT;
        }
    }

    MotorCommand decoded = {0};
    const float direction = external_direction_sign(config);
    if ((family == 0U) && (config->control_mode == MOTOR_MODE_MIT)) {
        const uint32_t p = ((uint32_t)frame->data[0] << 8U) | frame->data[1];
        const uint32_t v = ((uint32_t)frame->data[2] << 4U) |
                           (frame->data[3] >> 4U);
        const uint32_t kp = ((uint32_t)(frame->data[3] & 0x0FU) << 8U) |
                            frame->data[4];
        const uint32_t kd = ((uint32_t)frame->data[5] << 4U) |
                            (frame->data[6] >> 4U);
        const uint32_t torque = ((uint32_t)(frame->data[6] & 0x0FU) << 8U) |
                                frame->data[7];

        decoded.position = motor_uint_to_float(p, config->position_min,
                                               config->position_max, 16U);
        decoded.velocity = motor_uint_to_float(v, config->velocity_min,
                                               config->velocity_max, 12U);
        decoded.kp = motor_uint_to_float(kp, config->kp_min, config->kp_max, 12U);
        decoded.kd = motor_uint_to_float(kd, config->kd_min, config->kd_max, 12U);
        decoded.torque = motor_uint_to_float(torque, config->torque_min,
                                             config->torque_max, 12U);
        decoded.position *= direction;
        decoded.velocity *= direction;
        decoded.torque *= direction;
        decoded.mode = MOTOR_MODE_MIT;
    } else if ((family == 1U) &&
               (config->control_mode == MOTOR_MODE_POSITION_SPEED)) {
        decoded.position = read_f32_le(&frame->data[0]) * direction;
        decoded.velocity = read_f32_le(&frame->data[4]);
        if (decoded.velocity < 0.0f) {
            decoded.velocity = -decoded.velocity;
        }
        decoded.velocity *= config->gear_ratio;
        decoded.mode = MOTOR_MODE_POSITION_SPEED;
    } else if ((family == 2U) &&
               (config->control_mode == MOTOR_MODE_SPEED)) {
        decoded.velocity = read_f32_le(&frame->data[0]) *
                           config->gear_ratio * direction;
        decoded.mode = MOTOR_MODE_SPEED;
    } else if ((family == 3U) &&
               (config->control_mode == MOTOR_MODE_HYBRID)) {
        decoded.position = read_f32_le(&frame->data[0]) * direction;
        decoded.velocity = (float)clamp_pvt_u16(
                               read_u16_le(&frame->data[4])) * 0.01f *
                           config->gear_ratio;
        decoded.torque = (float)clamp_pvt_u16(
                             read_u16_le(&frame->data[6])) * 0.0001f;
        decoded.mode = MOTOR_MODE_HYBRID;
    } else {
        /* A low-byte address match still refreshes communication state and
         * produces feedback in the shipped IRQ, even if CTRL_MODE rejects
         * that ID family. */
        return CAN_COMMAND_FEEDBACK_ONLY;
    }

    *command = decoded;
    return CAN_COMMAND_SETPOINT;
}

void can_protocol_encode_feedback(const MotorFeedback *feedback,
                                  const MotorConfig *config,
                                  CanFrame *frame)
{
    memset(frame, 0, sizeof(*frame));
    frame->id = config->master_id;
    frame->length = 8U;
    const float direction = external_direction_sign(config);
    const uint32_t p = motor_float_to_uint(feedback->position * direction,
                                           config->position_min,
                                           config->position_max, 16U);
    const uint32_t v = motor_float_to_uint(feedback->velocity * direction,
                                           config->velocity_min,
                                           config->velocity_max, 12U);
    const uint32_t torque = motor_float_to_uint(feedback->output_torque * direction,
                                                config->torque_min,
                                                config->torque_max, 12U);
    /* The reference does not mask the node ID to four bits here.  IDs below
     * 16 leave the upper nibble exclusively for faults as documented; larger
     * IDs retain their low byte and therefore alias those fault bits exactly
     * as the shipped firmware does. */
    frame->data[0] = (uint8_t)((uint8_t)config->can_id |
                               ((uint8_t)feedback->fault << 4U));
    frame->data[1] = (uint8_t)(p >> 8U);
    frame->data[2] = (uint8_t)p;
    frame->data[3] = (uint8_t)(v >> 4U);
    frame->data[4] = (uint8_t)((v << 4U) | (torque >> 8U));
    frame->data[5] = (uint8_t)torque;
    frame->data[6] = encode_temperature(feedback->mos_temperature);
    frame->data[7] = encode_temperature(feedback->motor_temperature);
}
