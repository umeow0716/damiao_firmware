#include "can_protocol.h"

#include "memory_layout.h"

#include <stddef.h>
#include <string.h>

#include "motor_math.h"

#include "board_mcan.h"

typedef struct
{
    uint16_t packed[8];
    float converted[4];
} CanProtocolScratch;
static volatile CanProtocolScratch can_protocol_workspace
    __attribute__((section(".can_protocol_scratch")));
#define can_protocol_scratch (can_protocol_workspace.packed)
SRAM_ABI_ASSERT_SIZE(CanProtocolScratch, 0x20U);
SRAM_ABI_ASSERT_OFFSET(CanProtocolScratch, converted, 0x10U);
#define protocol_float_to_uint float_to_uint_helper
#define protocol_uint_to_float uint_to_float_helper

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static float read_f32_le(const uint8_t *data)
{
    const uint32_t bits = (uint32_t)data[0] | ((uint32_t)data[1] << 8U) |
                          ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint8_t encode_temperature(float temperature)
{
    /* Convert with VCVT.S32.F32 and store the low byte. */
    float converted;
    uint32_t bits;
    __asm volatile("vcvt.s32.f32 %0, %2\nvmov %1, %0"
                   : "=&t"(converted), "=r"(bits)
                   : "t"(temperature));
    return (uint8_t)bits;
}

static float apply_direction(float value, float direction)
{
    if (direction < 0.0f)
    {
        __asm volatile("vneg.f32 %0, %0" : "+t"(value));
    }
    return value;
}

static float apply_feedback_direction(float value, bool inverted)
{
    /* The feedback path branches on the raw direction word.  Keeping that
     * decision in an integer register avoids source-only VCMPE instructions
     * that would otherwise leave FPSCR.N set after the packet is sent. */
    if (inverted)
    {
        __asm volatile("vneg.f32 %0, %0" : "+t"(value));
    }
    return value;
}

static bool is_special_command(const CanFrame *frame, uint8_t command)
{
    for (uint8_t i = 0U; i < 7U; ++i)
    {
        if (frame->data[i] != 0xFFU)
        {
            return false;
        }
    }
    return frame->data[7] == command;
}

static CanCommandKind decode_command(const CanFrame *frame, const MotorConfig *config,
                                     MotorCommand *command, const McanIrqContext *references)
{
    if ((frame == NULL) || (config == NULL) || (command == NULL))
    {
        return CAN_COMMAND_NONE;
    }

    /* Filter 0 uses 0x0ff as the mask.  The receive path likewise
     * compares the received standard ID's low byte with CAN_ID, then uses
     * ID[10:8] to select the command family. */
    const uint32_t node_id = references != NULL
                                 ? references->node_id
                                 : *(const volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                       UINT32_C(0x1fffa5e8), UINT32_C(0x1fffa578));
    if ((frame->id & 0xFFU) != node_id)
    {
        return CAN_COMMAND_NONE;
    }
    /* Firmware refreshes communication age immediately after address match,
     * even for rejected command families, unless a fault is latched. */
    volatile uint32_t *const status = references != NULL
                                          ? references->status
                                          : (volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                                UINT32_C(0x1ffff1f0), UINT32_C(0x1ffff17c));
    if (status[1] == 0U)
    {
        status[0] = 0U;
    }
    const uint32_t family = (frame->id >> 8U) & 0x07U;

    {
        /* These four command bytes are independent protocol commands. */
        if (is_special_command(frame, 0xFCU))
        {
            return CAN_COMMAND_ENABLE;
        }
        if (is_special_command(frame, 0xFDU))
        {
            return CAN_COMMAND_DISABLE;
        }
        if (is_special_command(frame, 0xFEU))
        {
            return CAN_COMMAND_SET_ZERO;
        }
        if (is_special_command(frame, 0xFBU))
        {
            return CAN_COMMAND_CLEAR_FAULT;
        }
    }

    const volatile uint32_t *const motor_words =
        references != NULL ? (const volatile uint32_t *)references->motor
                           : (const volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                 UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
    if (motor_words[0x38U / 4U] != 2U)
    {
        return CAN_COMMAND_FEEDBACK_ONLY;
    }
    MotorCommand decoded = {0};
    volatile uint32_t *const sample_words =
        references != NULL ? (volatile uint32_t *)references->sample
                           : (volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                 UINT32_C(0x1ffff104), UINT32_C(0x1ffff090));
    volatile float *const sample = (volatile float *)(uintptr_t)sample_words;
    const volatile uint8_t *const config_bytes =
        references != NULL ? (const volatile uint8_t *)references->config
                           : (const volatile uint8_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                 UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558));
    const volatile float *const config_floats = (const volatile float *)(uintptr_t)config_bytes;
    volatile CanProtocolScratch *const workspace =
        references != NULL ? (volatile CanProtocolScratch *)(uintptr_t)references->received_id
                           : &can_protocol_workspace;
    volatile uint16_t *const packed = workspace->packed;
    volatile float *const converted = workspace->converted;
    const volatile uint32_t *const mode_owner =
        references != NULL ? (const volatile uint32_t *)(uintptr_t)*(volatile const uint32_t *)
                                 MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b94), UINT32_C(0x1fff8554))
                           : sample_words;
    const uint32_t mode = mode_owner[0x3CU / 4U];
    /* Load mode first. Only non-MIT ID families then read gear ratio and
     * direction; MIT reads its direction after the three conversions. */
    float gear_ratio = 0.0f;
    float direction = 1.0f;
    if (family != 0U)
    {
        const volatile float *const command_config =
            references != NULL
                ? (const volatile float *)(uintptr_t)*(volatile const uint32_t *)
                      MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b84), UINT32_C(0x1fff8544))
                : config_floats;
        gear_ratio = command_config[0x50U / 4U];
        const volatile uint32_t *const command_motor =
            references != NULL
                ? (const volatile uint32_t *)(uintptr_t)*(volatile const uint32_t *)
                      MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b50), UINT32_C(0x1fff8510))
                : motor_words;
        direction = command_motor[0x34U / 4U] == UINT32_C(0x40000000) ? -1.0f : 1.0f;
    }
    if ((family == 0U) && (mode == MOTOR_MODE_MIT))
    {
        const uint32_t p = ((uint32_t)frame->data[0] << 8U) | frame->data[1];
        const uint32_t v = ((uint32_t)frame->data[2] << 4U) | (frame->data[3] >> 4U);
        const uint32_t kp = ((uint32_t)(frame->data[3] & 0x0FU) << 8U) | frame->data[4];
        const uint32_t kd = ((uint32_t)frame->data[5] << 4U) | (frame->data[6] >> 4U);
        const uint32_t torque = ((uint32_t)(frame->data[6] & 0x0FU) << 8U) | frame->data[7];

        packed[1] = (uint16_t)p;
        packed[2] = (uint16_t)v;
        packed[4] = (uint16_t)kp;
        packed[5] = (uint16_t)kd;
        packed[3] = (uint16_t)torque;
        const float position_max = config_floats[0x54U / 4U];
        converted[0] = protocol_uint_to_float(p, -position_max, position_max, 16U);
        const float velocity_max = config_floats[0x58U / 4U];
        converted[1] = protocol_uint_to_float(packed[2], -velocity_max, velocity_max, 12U);
        const float torque_max = config_floats[0x5CU / 4U];
        const float converted_torque =
            protocol_uint_to_float(packed[3], -torque_max, torque_max, 12U);
        converted[2] = converted_torque;
        const float live_direction = motor_words[0x34U / 4U] == UINT32_C(0x40000000) ? -1.0f : 1.0f;
        if (live_direction < 0.0f)
        {
            /* Keep torque in a register, negate it before the position and
             * velocity stores, then publish it last. */
            __asm__ volatile("vldr s1, [%0]\n\t"
                             "vneg.f32 s0, %1\n\t"
                             "vneg.f32 s1, s1\n\t"
                             "vstr s1, [%0]\n\t"
                             "vldr s1, [%0, #4]\n\t"
                             "vneg.f32 s1, s1\n\t"
                             "vstr s1, [%0, #4]\n\t"
                             "vstr s0, [%0, #8]"
                             :
                             : "r"(converted), "t"(converted_torque)
                             : "s0", "s1", "memory");
        }
        decoded.position = converted[0];
        sample[0] = decoded.position;
        decoded.velocity = converted[1];
        sample[1] = decoded.velocity;
        decoded.torque = converted[2];
        sample[2] = decoded.torque;
        decoded.kp = protocol_uint_to_float(packed[4], 0.0f, 500.0f, 12U);
        sample[3] = decoded.kp;
        decoded.kd = protocol_uint_to_float(packed[5], 0.0f, 5.0f, 12U);
        sample[4] = decoded.kd;
        decoded.mode = MOTOR_MODE_MIT;
    }
    else if ((family == 1U) && (mode == MOTOR_MODE_POSITION_SPEED))
    {
        decoded.position = apply_direction(read_f32_le(&frame->data[0]), direction);
        /* This path publishes position before velocity arithmetic. */
        sample[0] = decoded.position;
        decoded.velocity = read_f32_le(&frame->data[4]);
        __asm volatile("vabs.f32 %0, %0" : "+t"(decoded.velocity));
        __asm volatile("vmul.f32 %0, %0, %1" : "+t"(decoded.velocity) : "t"(gear_ratio) : "memory");
        sample[1] = decoded.velocity;
        decoded.mode = MOTOR_MODE_POSITION_SPEED;
    }
    else if ((family == 2U) && (mode == MOTOR_MODE_SPEED))
    {
        float velocity = read_f32_le(&frame->data[0]);
        __asm volatile("vmul.f32 %0, %0, %1" : "+t"(velocity) : "t"(gear_ratio) : "memory");
        volatile float *const velocity_scratch = &converted[1];
        *velocity_scratch = velocity;
        if (direction < 0.0f)
        {
            velocity = apply_direction(velocity, direction);
            *velocity_scratch = velocity;
        }
        decoded.velocity = velocity;
        sample[1] = decoded.velocity;
        decoded.mode = MOTOR_MODE_SPEED;
    }
    else if ((family == 3U) && (mode == MOTOR_MODE_HYBRID))
    {
        decoded.position = apply_direction(read_f32_le(&frame->data[0]), direction);
        sample[0] = decoded.position;
        const uint16_t packed_velocity = read_u16_le(&frame->data[4]);
        packed[2] = packed_velocity;
        if (packed_velocity > 10000U)
        {
            packed[2] = 10000U;
        }
        const uint32_t velocity_limit = packed[2];
        float velocity;
        __asm volatile("vmov %0, %1\n\t"
                       "vcvt.f32.u32 %0, %0\n\t"
                       "vmul.f32 %0, %0, %2\n\t"
                       "vmul.f32 %0, %0, %3"
                       : "=&t"(velocity)
                       : "r"(velocity_limit), "t"(0.01f), "t"(gear_ratio)
                       : "memory");
        decoded.velocity = velocity;
        sample[1] = decoded.velocity;
        const uint16_t packed_limit = read_u16_le(&frame->data[6]);
        packed[3] = packed_limit;
        if (packed_limit > 10000U)
        {
            packed[3] = 10000U;
        }
        const uint32_t torque_limit = packed[3];
        __asm volatile("vmov %0, %1\n\t"
                       "vcvt.f32.u32 %0, %0\n\t"
                       "vmul.f32 %0, %0, %2"
                       : "=&t"(decoded.torque)
                       : "r"(torque_limit), "t"(0.0001f)
                       : "memory");
        sample[0x30U / 4U] = decoded.torque;
        decoded.mode = MOTOR_MODE_HYBRID;
    }
    else
    {
        /* A low-byte address match still refreshes communication state and
         * produces feedback in the shipped IRQ, even if CTRL_MODE rejects
         * that ID family. */
        return CAN_COMMAND_FEEDBACK_ONLY;
    }

    *command = decoded;
    return CAN_COMMAND_SETPOINT;
}

CanCommandKind can_protocol_decode_command(const CanFrame *frame, const MotorConfig *config,
                                           MotorCommand *command)
{
    return decode_command(frame, config, command, NULL);
}

CanCommandKind can_protocol_decode_command_irq(const CanFrame *frame, MotorCommand *command,
                                               const McanIrqContext *references)
{
    return decode_command(frame, (const MotorConfig *)(uintptr_t)references->config, command,
                          references);
}

static void publish_feedback_byte(CanFrame *frame, uint32_t index, uint8_t value,
                                  volatile uint8_t *payload)
{
    payload[index] = value;
    /* Both DM callers send the fixed payload, never frame->data. */
    (void)frame;
}

static void encode_feedback(const MotorFeedback *feedback, const MotorConfig *config,
                            CanFrame *frame, McanIrqContext *references, bool parameter_owner)
{
    frame->length = 8U;
    const volatile uint8_t *const config_bytes =
        references != NULL ? (const volatile uint8_t *)references->config
                           : (const volatile uint8_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                 UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558));
    const volatile float *const config_floats = (const volatile float *)(uintptr_t)config_bytes;
    const volatile uint32_t *const motor_words =
        references != NULL ? (const volatile uint32_t *)references->motor
                           : (const volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                 UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
    const volatile float *const motor = (const volatile float *)(uintptr_t)motor_words;
    const volatile uint32_t *const sample_words =
        references != NULL ? (const volatile uint32_t *)references->sample
                           : (const volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                 UINT32_C(0x1ffff104), UINT32_C(0x1ffff090));
    const volatile float *const sample = (const volatile float *)(uintptr_t)sample_words;
    volatile CanProtocolScratch *const scratch =
        references != NULL
            ? (volatile CanProtocolScratch *)(uintptr_t)(parameter_owner
                                                             ? references->parameter_scratch
                                                             : (volatile void *)
                                                                   references->received_id)
            : &can_protocol_workspace;
    const float position_max = config_floats[0x54U / 4U];
    const bool direction_inverted = motor_words[0x34U / 4U] != UINT32_C(0x3f800000);
    const float position = motor[0x18U / 4U];
    const uint32_t p = protocol_float_to_uint(
        apply_feedback_direction(position, direction_inverted), -position_max, position_max, 16U);
    scratch->packed[1] = (uint16_t)p;
    const float velocity_max = config_floats[0x58U / 4U];
    const float velocity = motor[0x1CU / 4U];
    const uint32_t v = protocol_float_to_uint(
        apply_feedback_direction(velocity, direction_inverted), -velocity_max, velocity_max, 12U);
    scratch->packed[2] = (uint16_t)v;
    const float torque_max = config_floats[0x5CU / 4U];
    const float output_torque = motor[0x30U / 4U];
    const uint32_t torque = protocol_float_to_uint(
        apply_feedback_direction(output_torque, direction_inverted), -torque_max, torque_max, 12U);
    scratch->packed[3] = (uint16_t)torque;
    /* The reference does not mask the node ID to four bits here.  IDs below
     * 16 leave the upper nibble exclusively for faults as documented; larger
     * IDs retain their low byte and therefore alias those fault bits exactly
     * as the shipped firmware does. */
    const uint8_t response_header =
        (uint8_t)(config_bytes[0x20U] | (((const volatile uint8_t *)sample_words)[0x80U] << 4U));
    volatile uint8_t *payload;
    if (references != NULL)
    {
        if (!parameter_owner)
        {
            references->response_dispatch =
                (volatile void *)(uintptr_t)*(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(
                    UINT32_C(0x1fff8f90), UINT32_C(0x1fff8950));
        }
        payload = (volatile uint8_t *)references->response_dispatch + 8U;
    }
    else
    {
        payload = (volatile uint8_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff2a4),
                                                                       UINT32_C(0x1ffff230));
    }
    publish_feedback_byte(frame, 0U, response_header, payload);
    const uint32_t packed_p = scratch->packed[1];
    publish_feedback_byte(frame, 1U, (uint8_t)(packed_p >> 8U), payload);
    publish_feedback_byte(frame, 2U, (uint8_t)packed_p, payload);
    const uint32_t packed_v = scratch->packed[2];
    publish_feedback_byte(frame, 3U, (uint8_t)(packed_v >> 4U), payload);
    const uint32_t packed_torque = scratch->packed[3];
    publish_feedback_byte(frame, 4U, (uint8_t)((packed_v << 4U) + (packed_torque >> 8U)), payload);
    publish_feedback_byte(frame, 5U, (uint8_t)packed_torque, payload);
    (void)feedback;
    (void)config;
    publish_feedback_byte(frame, 6U, encode_temperature(sample[0x84U / 4U]), payload);
    publish_feedback_byte(frame, 7U, encode_temperature(motor[0x40U / 4U]), payload);
    frame->id = *(const volatile uint16_t *)(uintptr_t)(config_bytes + 0x1CU);
}

void can_protocol_encode_feedback(const MotorFeedback *feedback, const MotorConfig *config,
                                  CanFrame *frame)
{
    encode_feedback(feedback, config, frame, NULL, false);
}

void can_protocol_encode_feedback_irq(McanIrqContext *references, CanFrame *frame)
{
    encode_feedback(NULL, NULL, frame, references, false);
}

void can_protocol_encode_parameter_feedback_irq(McanIrqContext *references, CanFrame *frame)
{
    encode_feedback(NULL, NULL, frame, references, true);
}
