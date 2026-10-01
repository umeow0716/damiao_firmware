#include "can_protocol.h"

#include "factory_layout.h"

#include <stddef.h>
#include <string.h>

#include "motor_math.h"

#if defined(DAMIAO_DM4310)
#include "board_mcan.h"

typedef struct {
    uint16_t packed[8];
    float converted[4];
} Dm4310CanProtocolScratch;
static volatile Dm4310CanProtocolScratch can_protocol_workspace
    __attribute__((section(".dm4310_can_protocol_scratch")));
#define can_protocol_scratch (can_protocol_workspace.packed)
_Static_assert(sizeof(Dm4310CanProtocolScratch) == 0x20U,
               "DM4310 CAN scratch size changed");
_Static_assert(offsetof(Dm4310CanProtocolScratch, converted) == 0x10U,
               "DM4310 CAN float scratch offset changed");
#define protocol_float_to_uint dm4310_float_to_uint_helper
#define protocol_uint_to_float dm4310_uint_to_float_helper
#else
#define protocol_float_to_uint motor_float_to_uint
#define protocol_uint_to_float motor_uint_to_float
#endif

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

#if !defined(DAMIAO_DM4310)
static uint16_t clamp_pvt_u16(uint16_t value)
{
    /* The recovered IRQ stores each packed PVT limit in a halfword and
     * clamps it to 10000 before converting it to float. */
    return value > 10000U ? 10000U : value;
}
#endif

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
#if defined(DAMIAO_DM4310)
    float converted;
    uint32_t bits;
    __asm volatile ("vcvt.s32.f32 %0, %2\nvmov %1, %0"
                    : "=&t" (converted), "=r" (bits)
                    : "t" (temperature));
    return (uint8_t)bits;
#else
    return (uint8_t)(int32_t)temperature;
#endif
}

#if !defined(DAMIAO_DM4310)
static float external_direction_sign(const MotorConfig *config)
{
    /* detect_motor_direction_and_pole_pairs stores 1 for the positive wire
     * convention and 2 for the mechanically reversed convention.  The
     * original MCAN IRQ converts at the protocol boundary so the internal
     * controller and sensor coordinates remain coherent. */
    return config->direction == 2.0f ? -1.0f : 1.0f;
}
#endif

static float apply_direction(float value, float direction)
{
#if defined(DAMIAO_DM4310)
    if (direction < 0.0f) {
        __asm volatile ("vneg.f32 %0, %0" : "+t" (value));
    }
    return value;
#else
    return value * direction;
#endif
}

#if defined(DAMIAO_DM4310)
static float apply_feedback_direction(float value, bool inverted)
{
    /* The feedback path branches on the raw direction word.  Keeping that
     * decision in an integer register avoids source-only VCMPE instructions
     * that would otherwise leave FPSCR.N set after the packet is sent. */
    if (inverted) {
        __asm volatile ("vneg.f32 %0, %0" : "+t" (value));
    }
    return value;
}
#endif

static bool is_special_command(const CanFrame *frame, uint8_t command)
{
    for (uint8_t i = 0U; i < 7U; ++i) {
        if (frame->data[i] != 0xFFU) {
            return false;
        }
    }
    return frame->data[7] == command;
}

static CanCommandKind decode_command(const CanFrame *frame,
                                     const MotorConfig *config,
                                     MotorCommand *command
#if defined(DAMIAO_DM4310)
                                     , const Dm4310McanIrqReferences *references
#endif
                                     )
{
    if ((frame == NULL) || (config == NULL) || (command == NULL)) {
        return CAN_COMMAND_NONE;
    }

    /* Filter 0 uses 0x0ff as the mask.  The original handler likewise
     * compares the received standard ID's low byte with CAN_ID, then uses
     * ID[10:8] to select the command family. */
#if defined(DAMIAO_DM4310)
    const uint32_t node_id = references != NULL ? references->node_id :
        *(const volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa5e8), UINT32_C(0x1fffa578));
#else
    const uint32_t node_id = config->can_id;
#endif
    if ((frame->id & 0xFFU) != node_id) {
        return CAN_COMMAND_NONE;
    }
#if defined(DAMIAO_DM4310)
    /* Factory refreshes communication age immediately after address match,
     * even for rejected command families, unless a fault is latched. */
    volatile uint32_t *const status = references != NULL ? references->status :
        (volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff1f0), UINT32_C(0x1ffff17c));
    if (status[1] == 0U) {
        status[0] = 0U;
    }
#endif
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

#if defined(DAMIAO_DM4310)
    const volatile uint32_t *const motor_words = references != NULL ?
        (const volatile uint32_t *)references->motor :
        (const volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
    if (motor_words[0x38U / 4U] != 2U) {
        return CAN_COMMAND_FEEDBACK_ONLY;
    }
#endif
    MotorCommand decoded = {0};
#if defined(DAMIAO_DM4310)
    volatile uint32_t *const sample_words = references != NULL ?
        (volatile uint32_t *)references->sample :
        (volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff104), UINT32_C(0x1ffff090));
    volatile float *const sample =
        (volatile float *)(uintptr_t)sample_words;
    const volatile uint8_t *const config_bytes = references != NULL ?
        (const volatile uint8_t *)references->config :
        (const volatile uint8_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558));
    const volatile float *const config_floats =
        (const volatile float *)(uintptr_t)config_bytes;
    volatile Dm4310CanProtocolScratch *const workspace = references != NULL ?
        (volatile Dm4310CanProtocolScratch *)(uintptr_t)references->received_id :
        &can_protocol_workspace;
    volatile uint16_t *const packed = workspace->packed;
    volatile float *const converted = workspace->converted;
    const volatile uint32_t *const mode_owner = references != NULL ?
        (const volatile uint32_t *)(uintptr_t)
            *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff8b94), UINT32_C(0x1fff8554)) :
        sample_words;
    const uint32_t mode = mode_owner[0x3CU / 4U];
    /* Factory 0x1fff8a4e loads mode first. Only non-MIT ID families
     * then read gear ratio and direction (0x1fff8a58..0x1fff8a60).
     * MIT reads its direction later, after the three conversions. */
    float gear_ratio = 0.0f;
    float direction = 1.0f;
    if (family != 0U) {
        const volatile float *const command_config = references != NULL ?
            (const volatile float *)(uintptr_t)
                *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff8b84), UINT32_C(0x1fff8544)) :
            config_floats;
        gear_ratio = command_config[0x50U / 4U];
        const volatile uint32_t *const command_motor = references != NULL ?
            (const volatile uint32_t *)(uintptr_t)
                *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff8b50), UINT32_C(0x1fff8510)) :
            motor_words;
        direction = command_motor[0x34U / 4U] == UINT32_C(0x40000000) ?
            -1.0f : 1.0f;
    }
#else
    const float direction = external_direction_sign(config);
    const uint32_t mode = (uint32_t)config->control_mode;
    const float gear_ratio = config->gear_ratio;
#endif
    if ((family == 0U) && (mode == MOTOR_MODE_MIT)) {
        const uint32_t p = ((uint32_t)frame->data[0] << 8U) | frame->data[1];
        const uint32_t v = ((uint32_t)frame->data[2] << 4U) |
                           (frame->data[3] >> 4U);
        const uint32_t kp = ((uint32_t)(frame->data[3] & 0x0FU) << 8U) |
                            frame->data[4];
        const uint32_t kd = ((uint32_t)frame->data[5] << 4U) |
                            (frame->data[6] >> 4U);
        const uint32_t torque = ((uint32_t)(frame->data[6] & 0x0FU) << 8U) |
                                frame->data[7];

#if defined(DAMIAO_DM4310)
        packed[1] = (uint16_t)p;
        packed[2] = (uint16_t)v;
        packed[4] = (uint16_t)kp;
        packed[5] = (uint16_t)kd;
        packed[3] = (uint16_t)torque;
        const float position_max = config_floats[0x54U / 4U];
        converted[0] = protocol_uint_to_float(p,
            -position_max, position_max, 16U);
        const float velocity_max = config_floats[0x58U / 4U];
        converted[1] = protocol_uint_to_float(packed[2],
            -velocity_max, velocity_max, 12U);
        const float torque_max = config_floats[0x5CU / 4U];
        const float converted_torque = protocol_uint_to_float(
            packed[3], -torque_max, torque_max, 12U);
        converted[2] = converted_torque;
        const float live_direction =
            motor_words[0x34U / 4U] == UINT32_C(0x40000000) ? -1.0f : 1.0f;
        if (live_direction < 0.0f) {
            /* 0x1fff8afe..8b1a keeps torque in a register, negates it
             * before the position/velocity stores, then publishes it last. */
            __asm__ volatile (
                "vldr s1, [%0]\n\t"
                "vneg.f32 s0, %1\n\t"
                "vneg.f32 s1, s1\n\t"
                "vstr s1, [%0]\n\t"
                "vldr s1, [%0, #4]\n\t"
                "vneg.f32 s1, s1\n\t"
                "vstr s1, [%0, #4]\n\t"
                "vstr s0, [%0, #8]"
                :
                : "r" (converted), "t" (converted_torque)
                : "s0", "s1", "memory");
        }
        decoded.position = converted[0];
        sample[0] = decoded.position;
        decoded.velocity = converted[1];
        sample[1] = decoded.velocity;
        decoded.torque = converted[2];
        sample[2] = decoded.torque;
        decoded.kp = protocol_uint_to_float(packed[4],
                                            0.0f, 500.0f, 12U);
        sample[3] = decoded.kp;
        decoded.kd = protocol_uint_to_float(packed[5],
                                            0.0f, 5.0f, 12U);
        sample[4] = decoded.kd;
#else
        decoded.position = protocol_uint_to_float(p, config->position_min,
                                                  config->position_max, 16U);
        decoded.velocity = protocol_uint_to_float(v, config->velocity_min,
                                                  config->velocity_max, 12U);
        decoded.kp = protocol_uint_to_float(kp, config->kp_min,
                                            config->kp_max, 12U);
        decoded.kd = protocol_uint_to_float(kd, config->kd_min,
                                            config->kd_max, 12U);
        decoded.torque = protocol_uint_to_float(torque, config->torque_min,
                                                config->torque_max, 12U);
        decoded.position = apply_direction(decoded.position, direction);
        decoded.velocity = apply_direction(decoded.velocity, direction);
        decoded.torque = apply_direction(decoded.torque, direction);
#endif
        decoded.mode = MOTOR_MODE_MIT;
    } else if ((family == 1U) &&
               (mode == MOTOR_MODE_POSITION_SPEED)) {
        decoded.position = apply_direction(read_f32_le(&frame->data[0]), direction);
#if defined(DAMIAO_DM4310)
        /* Factory 0x1fff8be8 publishes position before velocity arithmetic. */
        sample[0] = decoded.position;
#endif
        decoded.velocity = read_f32_le(&frame->data[4]);
#if defined(DAMIAO_DM4310)
        __asm volatile ("vabs.f32 %0, %0"
                        : "+t" (decoded.velocity));
#else
        if (decoded.velocity < 0.0f) {
            decoded.velocity = -decoded.velocity;
        }
#endif
#if defined(DAMIAO_DM4310)
        __asm volatile ("vmul.f32 %0, %0, %1"
                        : "+t" (decoded.velocity) : "t" (gear_ratio)
                        : "memory");
        sample[1] = decoded.velocity;
#else
        decoded.velocity *= gear_ratio;
#endif
        decoded.mode = MOTOR_MODE_POSITION_SPEED;
    } else if ((family == 2U) &&
               (mode == MOTOR_MODE_SPEED)) {
#if defined(DAMIAO_DM4310)
        float velocity = read_f32_le(&frame->data[0]);
        __asm volatile ("vmul.f32 %0, %0, %1"
                        : "+t" (velocity) : "t" (gear_ratio)
                        : "memory");
        volatile float *const velocity_scratch =
            &converted[1];
        *velocity_scratch = velocity;
        if (direction < 0.0f) {
            velocity = apply_direction(velocity, direction);
            *velocity_scratch = velocity;
        }
        decoded.velocity = velocity;
        sample[1] = decoded.velocity;
#else
        decoded.velocity = apply_direction(
            read_f32_le(&frame->data[0]) * gear_ratio, direction);
#endif
        decoded.mode = MOTOR_MODE_SPEED;
    } else if ((family == 3U) &&
               (mode == MOTOR_MODE_HYBRID)) {
        decoded.position = apply_direction(read_f32_le(&frame->data[0]), direction);
#if defined(DAMIAO_DM4310)
        sample[0] = decoded.position;
        const uint16_t packed_velocity = read_u16_le(&frame->data[4]);
        packed[2] = packed_velocity;
        if (packed_velocity > 10000U) {
            packed[2] = 10000U;
        }
        const uint32_t velocity_limit = packed[2];
        float velocity;
        __asm volatile ("vmov %0, %1\n\t"
                        "vcvt.f32.u32 %0, %0\n\t"
                        "vmul.f32 %0, %0, %2\n\t"
                        "vmul.f32 %0, %0, %3"
                        : "=&t" (velocity)
                        : "r" (velocity_limit), "t" (0.01f), "t" (gear_ratio)
                        : "memory");
        decoded.velocity = velocity;
        sample[1] = decoded.velocity;
        const uint16_t packed_limit = read_u16_le(&frame->data[6]);
        packed[3] = packed_limit;
        if (packed_limit > 10000U) {
            packed[3] = 10000U;
        }
        const uint32_t torque_limit = packed[3];
        __asm volatile ("vmov %0, %1\n\t"
                        "vcvt.f32.u32 %0, %0\n\t"
                        "vmul.f32 %0, %0, %2"
                        : "=&t" (decoded.torque)
                        : "r" (torque_limit), "t" (0.0001f)
                        : "memory");
        sample[0x30U / 4U] = decoded.torque;
#else
        decoded.velocity = (float)clamp_pvt_u16(
                               read_u16_le(&frame->data[4])) * 0.01f *
                           gear_ratio;
        decoded.torque = (float)clamp_pvt_u16(
                             read_u16_le(&frame->data[6])) * 0.0001f;
#endif
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

CanCommandKind can_protocol_decode_command(const CanFrame *frame,
                                           const MotorConfig *config,
                                           MotorCommand *command)
{
    return decode_command(frame, config, command
#if defined(DAMIAO_DM4310)
                          , NULL
#endif
                          );
}

#if defined(DAMIAO_DM4310)
CanCommandKind dm4310_can_protocol_decode_command_irq(
    const CanFrame *frame, MotorCommand *command,
    const Dm4310McanIrqReferences *references)
{
    return decode_command(frame,
                          (const MotorConfig *)(uintptr_t)references->config,
                          command, references);
}
#endif

static void publish_feedback_byte(CanFrame *frame, uint32_t index, uint8_t value
#if defined(DAMIAO_DM4310)
                                  , volatile uint8_t *payload
#endif
                                  )
{
#if defined(DAMIAO_DM4310)
    payload[index] = value;
    /* Both DM callers send the fixed payload, never frame->data. */
    (void)frame;
#else
    frame->data[index] = value;
#endif
}

static void encode_feedback(const MotorFeedback *feedback,
                            const MotorConfig *config, CanFrame *frame
#if defined(DAMIAO_DM4310)
                            , Dm4310McanIrqReferences *references,
                            bool parameter_owner
#endif
                            )
{
#if !defined(DAMIAO_DM4310)
    memset(frame, 0, sizeof(*frame));
    frame->id = config->master_id;
#endif
    frame->length = 8U;
#if defined(DAMIAO_DM4310)
    const volatile uint8_t *const config_bytes = references != NULL ?
        (const volatile uint8_t *)references->config :
        (const volatile uint8_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558));
    const volatile float *const config_floats =
        (const volatile float *)(uintptr_t)config_bytes;
    const volatile uint32_t *const motor_words = references != NULL ?
        (const volatile uint32_t *)references->motor :
        (const volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
    const volatile float *const motor =
        (const volatile float *)(uintptr_t)motor_words;
    const volatile uint32_t *const sample_words = references != NULL ?
        (const volatile uint32_t *)references->sample :
        (const volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff104), UINT32_C(0x1ffff090));
    const volatile float *const sample =
        (const volatile float *)(uintptr_t)sample_words;
    volatile Dm4310CanProtocolScratch *const scratch = references != NULL ?
        (volatile Dm4310CanProtocolScratch *)(uintptr_t)
            (parameter_owner ? references->parameter_scratch :
                               (volatile void *)references->received_id) :
        &can_protocol_workspace;
    const float position_max = config_floats[0x54U / 4U];
    const bool direction_inverted = motor_words[0x34U / 4U] !=
        UINT32_C(0x3f800000);
    const float position = motor[0x18U / 4U];
    const uint32_t p = protocol_float_to_uint(
        apply_feedback_direction(position, direction_inverted),
        -position_max, position_max, 16U);
    scratch->packed[1] = (uint16_t)p;
    const float velocity_max = config_floats[0x58U / 4U];
    const float velocity = motor[0x1CU / 4U];
    const uint32_t v = protocol_float_to_uint(
        apply_feedback_direction(velocity, direction_inverted),
        -velocity_max, velocity_max, 12U);
    scratch->packed[2] = (uint16_t)v;
    const float torque_max = config_floats[0x5CU / 4U];
    const float output_torque = motor[0x30U / 4U];
    const uint32_t torque = protocol_float_to_uint(
        apply_feedback_direction(output_torque, direction_inverted),
        -torque_max, torque_max, 12U);
    scratch->packed[3] = (uint16_t)torque;
#else
    const float direction = external_direction_sign(config);
    const uint32_t p = protocol_float_to_uint(apply_direction(feedback->position, direction),
                                              config->position_min,
                                              config->position_max, 16U);
    const uint32_t v = protocol_float_to_uint(apply_direction(feedback->velocity, direction),
                                              config->velocity_min,
                                              config->velocity_max, 12U);
    const uint32_t torque = protocol_float_to_uint(
        apply_direction(feedback->output_torque, direction), config->torque_min,
        config->torque_max, 12U);
#endif
    /* The reference does not mask the node ID to four bits here.  IDs below
     * 16 leave the upper nibble exclusively for faults as documented; larger
     * IDs retain their low byte and therefore alias those fault bits exactly
     * as the shipped firmware does. */
#if defined(DAMIAO_DM4310)
    const uint8_t response_header = (uint8_t)(config_bytes[0x20U] |
        (((const volatile uint8_t *)sample_words)[0x80U] << 4U));
    volatile uint8_t *payload;
    if (references != NULL) {
        if (!parameter_owner) {
            references->response_dispatch = (volatile void *)(uintptr_t)
                *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff8f90), UINT32_C(0x1fff8950));
        }
        payload = (volatile uint8_t *)references->response_dispatch + 8U;
    } else {
        payload = (volatile uint8_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff2a4), UINT32_C(0x1ffff230));
    }
#endif
    publish_feedback_byte(frame, 0U,
#if defined(DAMIAO_DM4310)
                          response_header
#else
                          (uint8_t)((uint8_t)config->can_id |
                                    ((uint8_t)feedback->fault << 4U))
#endif
#if defined(DAMIAO_DM4310)
                          , payload
#endif
                          );
#if defined(DAMIAO_DM4310)
    const uint32_t packed_p = scratch->packed[1];
    publish_feedback_byte(frame, 1U, (uint8_t)(packed_p >> 8U), payload);
    publish_feedback_byte(frame, 2U, (uint8_t)packed_p, payload);
    const uint32_t packed_v = scratch->packed[2];
    publish_feedback_byte(frame, 3U, (uint8_t)(packed_v >> 4U), payload);
    const uint32_t packed_torque = scratch->packed[3];
#else
    frame->data[1] = (uint8_t)(p >> 8U);
    frame->data[2] = (uint8_t)p;
    frame->data[3] = (uint8_t)(v >> 4U);
#endif
#if defined(DAMIAO_DM4310)
    publish_feedback_byte(frame, 4U,
        (uint8_t)((packed_v << 4U) + (packed_torque >> 8U)), payload);
    publish_feedback_byte(frame, 5U, (uint8_t)packed_torque, payload);
#else
    frame->data[4] = (uint8_t)((v << 4U) | (torque >> 8U));
    frame->data[5] = (uint8_t)torque;
#endif
#if defined(DAMIAO_DM4310)
    (void)feedback;
    (void)config;
    publish_feedback_byte(frame, 6U, encode_temperature(
        sample[0x84U / 4U]), payload);
    publish_feedback_byte(frame, 7U, encode_temperature(
        motor[0x40U / 4U]), payload);
    frame->id = *(const volatile uint16_t *)(uintptr_t)
        (config_bytes + 0x1CU);
#else
    frame->data[6] = encode_temperature(feedback->mos_temperature);
    frame->data[7] = encode_temperature(feedback->motor_temperature);
#endif
}

void can_protocol_encode_feedback(const MotorFeedback *feedback,
                                  const MotorConfig *config,
                                  CanFrame *frame)
{
    encode_feedback(feedback, config, frame
#if defined(DAMIAO_DM4310)
                    , NULL, false
#endif
                    );
}

#if defined(DAMIAO_DM4310)
void dm4310_can_protocol_encode_feedback_irq(
    Dm4310McanIrqReferences *references, CanFrame *frame)
{
    encode_feedback(NULL, NULL, frame, references, false);
}

void dm4310_can_protocol_encode_parameter_feedback_irq(
    Dm4310McanIrqReferences *references, CanFrame *frame)
{
    encode_feedback(NULL, NULL, frame, references, true);
}
#endif
