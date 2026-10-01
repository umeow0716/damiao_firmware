#include "parameter_protocol.h"

#include "factory_layout.h"

#include <stddef.h>
#include <string.h>

#include "app_config.h"
#include "motor_math.h"
#include "safety.h"
#include "platform.h"

#if defined(DAMIAO_DM4310)
#include "board_mcan.h"
#endif

#define PARAMETER_CAN_ID (0x7FFU)
#define PARAMETER_READ   (0x33U)
#define PARAMETER_WRITE  (0x55U)
#define PARAMETER_STORE  (0xAAU)
#define PARAMETER_LIVE   (0xCCU)

#define LEGACY_DISCOVERY_HEAD (0x55U)
#define LEGACY_DISCOVERY_TAIL (0xAAU)
#define LEGACY_ID_HEAD        (0xAAU)
#define LEGACY_ID_TAIL        (0x55U)
#define LEGACY_VERSION_HEAD   (0xEEU)
#define LEGACY_VERSION_TAIL   (0x11U)

#if !defined(DAMIAO_DM4310)
static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static void write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}
#endif

#if defined(DAMIAO_DM4310)
static void publish_response_byte(ParameterProtocolResult *result,
                                  uint8_t index, uint8_t value);
#endif

static void prepare_legacy_response(uint16_t id, const uint8_t *data,
                                    uint8_t length,
                                    ParameterProtocolResult *result)
{
    result->response.id = id;
    result->response.length = length;
#if defined(DAMIAO_DM4310)
    for (uint8_t index = 0U; index < length; ++index) {
        publish_response_byte(result, index, data[index]);
    }
    result->response_prebuilt = true;
#else
    memcpy(result->response.data, data, length);
#endif
    result->response_ready = true;
}

static bool process_legacy_command(const CanFrame *request,
                                   MotorConfig *config,
                                   const MotorController *controller,
#if defined(DAMIAO_DM4310)
                                   uint32_t retained_node,
                                   uint32_t parameter_header,
#endif
                                   ParameterProtocolResult *result)
{
#if defined(DAMIAO_DM4310)
    (void)controller;
    if (request->id != PARAMETER_CAN_ID) {
        return false;
    }
    if (((uint16_t)parameter_header == retained_node) ||
        (result->motor_owner[0x38U / 4U] != 0U)) {
#else
    if ((request->id != PARAMETER_CAN_ID) ||
        controller->armed) {
#endif
        return false;
    }

#if defined(DAMIAO_DM4310)
    const uint8_t head = (uint8_t)parameter_header;
    const uint8_t node_byte = (uint8_t)(parameter_header >> 8U);
    const uint8_t master_byte = (uint8_t)(parameter_header >> 16U);
    const uint8_t tail = (uint8_t)(parameter_header >> 24U);
#else
    const uint8_t *const data = request->data;
    const uint8_t head = data[4];
    const uint8_t node_byte = data[5];
    const uint8_t master_byte = data[6];
    const uint8_t tail = data[7];
#endif
    if ((head == LEGACY_DISCOVERY_HEAD) && (node_byte == 0U) &&
        (master_byte == 0U) && (tail == LEGACY_DISCOVERY_TAIL)) {
#if defined(DAMIAO_DM4310)
        publish_response_byte(result, 0U, LEGACY_DISCOVERY_HEAD);
        publish_response_byte(result, 1U, (uint8_t)(retained_node & 0x7FU));
        const uint8_t master = result->config_owner[0x1CU];
        publish_response_byte(result, 2U, master & 0x7FU);
        publish_response_byte(result, 3U, LEGACY_DISCOVERY_TAIL);
        result->response.id = PARAMETER_CAN_ID;
        result->response.length = 4U;
        result->response_prebuilt = true;
        result->response_ready = true;
#else
        const uint8_t response[4] = {
            LEGACY_DISCOVERY_HEAD,
            (uint8_t)(config->can_id & 0x7FU),
            (uint8_t)(config->master_id & 0x7FU),
            LEGACY_DISCOVERY_TAIL,
        };
        prepare_legacy_response(PARAMETER_CAN_ID, response,
                                sizeof(response), result);
#endif
        return true;
    }

    if ((head == LEGACY_ID_HEAD) && (tail == LEGACY_ID_TAIL)) {
#if defined(DAMIAO_DM4310)
        const uint32_t node = node_byte;
        const uint32_t master = master_byte;
        volatile uint32_t *const ids =
            (volatile uint32_t *)(uintptr_t)(result->config_owner + 0x1CU);
        /* Factory 0x1fff9716 publishes master/node in one STRD. */
        __asm__ volatile ("strd %1, %2, [%0]"
                          :
                          : "r" (ids), "r" (master), "r" (node)
                          : "memory");
        platform_update_mcan_node_filter((uint16_t)node);
        if (!result->irq_context) {
            config->can_id = (uint16_t)node;
            config->master_id = (uint16_t)master;
        }
        /* Factory 0x1fff9722..34 uses the retained ID bytes, not a
         * reread of the decoded configuration or a temporary payload. */
        publish_response_byte(result, 0U, LEGACY_ID_HEAD);
        publish_response_byte(result, 1U, node_byte & 0x7FU);
        publish_response_byte(result, 2U, master_byte & 0x7FU);
        publish_response_byte(result, 3U, LEGACY_ID_TAIL);
        result->response.id = PARAMETER_CAN_ID;
        result->response.length = 4U;
        result->response_prebuilt = true;
        result->response_ready = true;
#else
        config->can_id = node_byte;
        config->master_id = master_byte;
        const uint8_t response[4] = {
            LEGACY_ID_HEAD,
            (uint8_t)(config->can_id & 0x7FU),
            (uint8_t)(config->master_id & 0x7FU),
            LEGACY_ID_TAIL,
        };
        prepare_legacy_response(PARAMETER_CAN_ID, response,
                                sizeof(response), result);
#endif
        result->persist_requested = true;
#if !defined(DAMIAO_DM4310)
        result->filter_update_requested = true;
#endif
        return true;
    }

    if ((head == 0x55U) && (node_byte == 0x01U) &&
        (master_byte == 0x02U) && (tail == 0xAAU) &&
#if defined(DAMIAO_DM4310)
        (*(volatile const uint16_t *)(uintptr_t)(result->payload + 0x60U) ==
         retained_node)) {
#else
        (read_u16_le(&data[0]) ==
         config->can_id)) {
#endif
        static const uint8_t response[8] = {
            'A', 'u', 'p', 'g', 'r', 'a', 'd', 'e',
        };
        prepare_legacy_response(0x7FEU, response, sizeof(response), result);
        result->bootloader_requested = true;
        return true;
    }

    if ((head == LEGACY_VERSION_HEAD) && (tail == LEGACY_VERSION_TAIL)) {
#if defined(DAMIAO_DM4310)
        /* Factory 0x1fff97b8 posts the head before loading version. */
        publish_response_byte(result, 0U, LEGACY_VERSION_TAIL);
        const uint16_t version = *(volatile const uint16_t *)(uintptr_t)
            (result->config_owner + 0x38U);
        publish_response_byte(result, 1U, (uint8_t)version);
        publish_response_byte(result, 2U, (uint8_t)(version >> 8U));
        publish_response_byte(result, 3U, LEGACY_VERSION_HEAD);
        result->response.id = PARAMETER_CAN_ID;
        result->response.length = 4U;
        result->response_prebuilt = true;
        result->response_ready = true;
#else
        const uint16_t version = (uint16_t)config->software_version;
        const uint8_t response[4] = {
            LEGACY_VERSION_TAIL,
            (uint8_t)version,
            (uint8_t)(version >> 8U),
            LEGACY_VERSION_HEAD,
        };
        prepare_legacy_response(PARAMETER_CAN_ID, response,
                                sizeof(response), result);
#endif
        return true;
    }
    return false;
}

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

#if defined(DAMIAO_DM4310)
static uint32_t read_directed_parameter_word(
    volatile const uint32_t *value, uint32_t direction)
{
    if (direction == UINT32_C(0x3f800000)) {
        return *value;
    }
    uint32_t bits;
    __asm__ volatile (
        "vldr s0, [%1]\n\t"
        "vneg.f32 s0, s0\n\t"
        "vmov %0, s0"
        : "=r" (bits)
        : "r" (value)
        : "s0", "memory");
    return bits;
}
#endif

static float bits_float(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

#if !defined(DAMIAO_DM4310)
static bool read_register(uint8_t address, const MotorConfig *config,
                          const ParameterRuntime *runtime, uint32_t *value)
{
    switch (address) {
    case 0x00: *value = float_bits(config->bus_undervoltage); break;
    case 0x01: *value = float_bits(config->torque_constant); break;
    case 0x02: *value = float_bits(config->motor_temperature_limit); break;
    case 0x03: *value = float_bits(config->current_limit); break;
    case 0x04: *value = float_bits(config->acceleration_limit); break;
    case 0x05: *value = float_bits(config->deceleration_limit); break;
    case 0x06: *value = float_bits(config->speed_limit); break;
    case 0x07: *value = config->master_id; break;
    case 0x08: *value = config->can_id; break;
    case 0x09: *value = config->communication_timeout; break;
    case 0x0A: *value = (uint32_t)config->control_mode; break;
    case 0x0B: *value = float_bits(config->viscous_damping); break;
    case 0x0C: *value = float_bits(config->rotor_inertia); break;
    case 0x0D: *value = config->hardware_version; break;
    case 0x0E: *value = config->software_version; break;
    case 0x0F: *value = config->serial_number; break;
    case 0x10: *value = config->pole_pairs; break;
    case 0x11: *value = float_bits(config->phase_resistance); break;
    case 0x12: *value = float_bits(config->phase_inductance); break;
    case 0x13: *value = float_bits(config->flux_linkage); break;
    case 0x14: *value = float_bits(config->gear_ratio); break;
    case 0x15: *value = float_bits(config->position_max); break;
    case 0x16: *value = float_bits(config->velocity_max); break;
    case 0x17: *value = float_bits(config->torque_max); break;
    case 0x18: *value = float_bits(config->current_loop_bandwidth); break;
    case 0x19: *value = float_bits(config->speed_kp); break;
    case 0x1A: *value = float_bits(config->speed_ki); break;
    case 0x1B: *value = float_bits(config->position_kp); break;
    case 0x1C: *value = float_bits(config->position_ki); break;
    case 0x1D: *value = float_bits(config->bus_overvoltage); break;
    case 0x1E: *value = float_bits(config->gear_torque_efficiency); break;
    case 0x1F: *value = float_bits(config->speed_loop_damping); break;
    case 0x20: *value = float_bits(config->velocity_filter_bandwidth); break;
    case 0x21: *value = float_bits(config->current_loop_enhancement); break;
    case 0x22: *value = float_bits(config->velocity_loop_enhancement); break;
    case 0x23: *value = config->can_data_rate_selector; break;
    case 0x24: *value = config->firmware_subversion; break;
    /* Undocumented factory/debug reads present in the reference IRQ. */
    case 0x32: *value = float_bits(runtime->output_sensor_calibration[0]); break;
    case 0x33: *value = float_bits(runtime->output_sensor_calibration[1]); break;
    case 0x34: *value = float_bits(runtime->output_sensor_calibration[2]); break;
    case 0x35: *value = float_bits(runtime->output_sensor_calibration[3]); break;
    case 0x36: *value = float_bits(runtime->output_position_offset); break;
    case 0x37: *value = float_bits(config->direction); break;
    case 0x50:
        *value = float_bits(runtime->motor_position *
                            (config->direction == 1.0f ? 1.0f : -1.0f));
        break;
    case 0x51:
        *value = float_bits(runtime->output_position *
                            (config->direction == 1.0f ? 1.0f : -1.0f));
        break;
    default: return false;
    }
    return true;
}
#endif

static uint8_t live_temperature_byte(float value)
{
#if defined(DAMIAO_DM4310)
    int32_t converted;
    __asm__ volatile (
        "vcvt.s32.f32 %1, %1\n\t"
        "vmov %0, %1"
        : "=r" (converted), "+t" (value));
    return (uint8_t)converted;
#else
    return (uint8_t)(int32_t)value;
#endif
}

static void prepare_live_response(uint8_t selector,
                                  const MotorConfig *config,
                                  const MotorController *controller,
#if defined(DAMIAO_DM4310)
                                  uint32_t retained_node,
#endif
                                  ParameterProtocolResult *result)
{
#if defined(DAMIAO_DM4310)
    (void)config;
    (void)controller;
#endif
#if !defined(DAMIAO_DM4310)
    const float direction = config->direction == 1.0f ? 1.0f : -1.0f;
    result->response.id = config->master_id;
#endif
    result->response.length = 8U;
#if !defined(DAMIAO_DM4310)
    memset(result->response.data, 0, sizeof(result->response.data));
#endif

    if (selector == 0U) {
#if defined(DAMIAO_DM4310)
        dm4310_can_protocol_encode_parameter_feedback_irq(
            result->references, &result->response);
#else
        can_protocol_encode_feedback(&controller->feedback, config,
                                     &result->response);
#endif
#if defined(DAMIAO_DM4310)
        result->response_prebuilt = true;
#endif
        result->response_ready = true;
        return;
    }
    if ((selector >= 1U) && (selector <= 3U)) {
#if defined(DAMIAO_DM4310)
        publish_response_byte(result, 0U, (uint8_t)retained_node);
        const volatile uint8_t *const sample_bytes =
            (const volatile uint8_t *)result->sample_owner;
        const volatile float *const sample =
            (const volatile float *)result->sample_owner;
        const volatile uint32_t *const motor = result->motor_owner;
        publish_response_byte(result, 1U, sample_bytes[0x80U]);
        const float mos = sample[0x84U / 4U];
        publish_response_byte(result, 2U, live_temperature_byte(mos));
        const float motor_temperature =
            ((const volatile float *)motor)[0x40U / 4U];
        publish_response_byte(result, 3U,
                              live_temperature_byte(motor_temperature));
        const uint32_t direction_bits = motor[0x34U / 4U];
        const uint32_t offset = selector == 1U ? 0x18U :
                                selector == 2U ? 0x1CU : 0x30U;
        const uint32_t bits = read_directed_parameter_word(
            &motor[offset / 4U], direction_bits);
        publish_response_byte(result, 4U, (uint8_t)bits);
        publish_response_byte(result, 5U, (uint8_t)(bits >> 8U));
        publish_response_byte(result, 6U, (uint8_t)(bits >> 16U));
        publish_response_byte(result, 7U, (uint8_t)(bits >> 24U));
        result->response.id = *(volatile const uint16_t *)(uintptr_t)
            (result->config_owner + 0x1CU);
        result->response_prebuilt = true;
#else
        result->response.data[0] = (uint8_t)config->can_id;
        result->response.data[1] = (uint8_t)controller->feedback.fault;
        result->response.data[2] = live_temperature_byte(
            controller->feedback.mos_temperature);
        result->response.data[3] = live_temperature_byte(
            controller->feedback.motor_temperature);
        float value = controller->feedback.position;
        if (selector == 2U) {
            value = controller->feedback.velocity;
        } else if (selector == 3U) {
            value = controller->feedback.output_torque;
        }
        value *= direction;
        memcpy(&result->response.data[4], &value, sizeof(value));
#endif
        result->response_ready = true;
        return;
    }
    if (selector == 4U) {
#if defined(DAMIAO_DM4310)
        volatile uint32_t *const motor = result->motor_owner;
        volatile uint32_t *const sample = result->sample_owner;
        const volatile float *const config_floats =
            (const volatile float *)(uintptr_t)result->config_owner;
        const uint32_t direction_bits = motor[0x34U / 4U];
        uint32_t position_bits;
        float maximum;
        if (direction_bits == UINT32_C(0x3f800000)) {
            /* Factory 0x1fff901e loads the limit before the raw position. */
            maximum = config_floats[0x58U / 4U];
            position_bits = motor[0x18U / 4U];
        } else {
            /* The negative arm instead loads position through VLDR first. */
            float position = ((volatile float *)motor)[0x18U / 4U];
            maximum = config_floats[0x58U / 4U];
            __asm__ volatile ("vneg.f32 %0, %0" : "+t" (position));
            position_bits = float_bits(position);
        }
        float velocity = ((volatile float *)motor)[0x1CU / 4U];
        if (direction_bits != UINT32_C(0x3f800000)) {
            __asm__ volatile ("vneg.f32 %0, %0" : "+t" (velocity));
        }
        const uint16_t encoded_velocity = (uint16_t)dm4310_float_to_uint_helper(
            velocity, -maximum, maximum, 12U);
        volatile uint16_t *const velocity_scratch =
            (volatile uint16_t *)(uintptr_t)(result->scratch + 4U);
        *velocity_scratch = encoded_velocity;
        float current = ((volatile float *)sample)[0x64U / 4U];
        const float scale = 10000.0f;
        int32_t encoded_current;
        __asm__ volatile (
            "vmul.f32 %1, %1, %2\n\t"
            "vcvt.s32.f32 %1, %1\n\t"
            "vmov %0, %1"
            : "=r" (encoded_current), "+&t" (current)
            : "t" (scale)
            : "memory");
        volatile uint16_t *const current_scratch =
            (volatile uint16_t *)(uintptr_t)(result->scratch + 0x0CU);
        *current_scratch = (uint16_t)encoded_current;
        publish_response_byte(result, 0U, (uint8_t)position_bits);
        publish_response_byte(result, 1U, (uint8_t)(position_bits >> 8U));
        publish_response_byte(result, 2U, (uint8_t)(position_bits >> 16U));
        publish_response_byte(result, 3U, (uint8_t)(position_bits >> 24U));
        const uint16_t retained_velocity = *velocity_scratch;
        publish_response_byte(result, 4U, (uint8_t)retained_velocity);
        publish_response_byte(result, 5U, (uint8_t)(retained_velocity >> 8U));
        const uint16_t retained_current = *current_scratch;
        publish_response_byte(result, 6U, (uint8_t)retained_current);
        publish_response_byte(result, 7U, (uint8_t)(retained_current >> 8U));
        result->response.id = *(volatile const uint16_t *)(uintptr_t)
            (result->config_owner + 0x1CU);
        result->response_prebuilt = true;
#else
        const float position = controller->feedback.position * direction;
        memcpy(&result->response.data[0], &position, sizeof(position));
        const uint16_t velocity = (uint16_t)
            motor_float_to_uint(
            controller->feedback.velocity * direction,
            config->velocity_min, config->velocity_max, 12U);
        result->response.data[4] = (uint8_t)velocity;
        result->response.data[5] = (uint8_t)(velocity >> 8U);
        const int16_t current = (int16_t)(controller->feedback.current_q *
                                          direction * 10000.0f);
        result->response.data[6] = (uint8_t)current;
        result->response.data[7] = (uint8_t)((uint16_t)current >> 8U);
#endif
        result->response_ready = true;
    }
}

#if defined(DAMIAO_DM4310)
static uint32_t parameter_float_zero_flags(float value);

static void publish_torque_conversion(uint8_t address, uint32_t value,
                                      const ParameterProtocolResult *result)
{
    volatile const float *const record =
        (volatile const float *)(uintptr_t)result->config_owner;
    volatile float *const conversion =
        (volatile float *)(uintptr_t)&result->motor_owner[0x44U / 4U];
    const float scale = bits_float(APP_PROFILE_CURRENT_FULL_SCALE_BITS);
    const float torque = address == 0x01U ? bits_float(value) : record[1];
    /* Torque WRITE already normalized its value using VCMPE; don't compare
     * it again. Efficiency WRITE performs its own signaling NE check. */
    const bool explicit_torque = address == 0x01U ?
        (value & UINT32_C(0x7fffffff)) != 0U :
        (parameter_float_zero_flags(torque) & UINT32_C(0x40000000)) == 0U;
    if (explicit_torque) {
        __asm__ volatile (
            "vmul.f32 s0, %2, %3\n\t"
            "vstr s0, [%0]\n\t"
            "vmov.f32 s1, #1.0\n\t"
            "vdiv.f32 s2, s1, s0\n\t"
            "vstr s2, [%0, #4]"
            :
            : "r" (conversion), "r" (record), "t" (torque), "t" (scale)
            : "s0", "s1", "s2", "memory");
    } else {
        /* Factory fallback: uint pole pairs, 1.5, flux, literal scale,
         * gear ratio, efficiency. Keep every separate rounding step. */
        float factor;
        __asm__ volatile (
            "vldr s0, [%1, #64]\n\t"
            "vcvt.f32.u32 s0, s0\n\t"
            "vmov.f32 s1, #1.5\n\t"
            "vmul.f32 s0, s0, s1\n\t"
            "vldr s1, [%1, #76]\n\t"
            "vmul.f32 s0, s0, s1\n\t"
            "vmul.f32 s0, s0, %2\n\t"
            "vldr s1, [%1, #80]\n\t"
            "vmul.f32 s0, s0, s1\n\t"
            "vmov.f32 %0, s0"
            : "=t" (factor)
            : "r" (record), "t" (scale)
            : "s0", "s1", "memory");
        /* Efficiency WRITE retains its input; only torque WRITE reloads
         * staged efficiency after the factor's gear-ratio multiplication. */
        const float efficiency = address == 0x1EU ? bits_float(value) : record[30];
        __asm__ volatile (
            "vmul.f32 s0, %1, %2\n\t"
            "vstr s0, [%0]\n\t"
            "vmov.f32 s1, #1.0\n\t"
            "vdiv.f32 s2, s1, s0\n\t"
            "vstr s2, [%0, #4]"
            :
            : "r" (conversion), "t" (factor), "t" (efficiency)
            : "s0", "s1", "s2", "memory");
    }
}
#endif

#if defined(DAMIAO_DM4310)
static uint32_t parameter_float_zero_flags(float value)
{
    uint32_t flags;
    __asm volatile ("vcmpe.f32 %1, #0.0\n\t"
                    "vmrs %0, fpscr"
                    : "=r" (flags) : "t" (value) : "memory");
    return flags;
}

static bool parameter_float_positive(float value)
{
    const uint32_t flags = parameter_float_zero_flags(value);
    /* GT: Z clear and N == V, including unordered's N=0,V=1. */
    return (flags & UINT32_C(0x40000000)) == 0U &&
           (((flags >> 31U) ^ (flags >> 28U)) & 1U) == 0U;
}
static bool parameter_float_nonnegative(float value)
{
    const uint32_t flags = parameter_float_zero_flags(value);
    return (((flags >> 31U) ^ (flags >> 28U)) & 1U) == 0U;
}
static bool parameter_float_greater(float left, float right)
{
    uint32_t flags;
    __asm volatile ("vcmpe.f32 %1, %2\n\t"
                    "vmrs %0, fpscr"
                    : "=r" (flags) : "t" (left), "t" (right) : "memory");
    return (flags & UINT32_C(0x40000000)) == 0U &&
           (((flags >> 31U) ^ (flags >> 28U)) & 1U) == 0U;
}
#else
#define parameter_float_positive(value) ((value) > 0.0f)
#define parameter_float_nonnegative(value) ((value) >= 0.0f)
#endif

static bool write_register(uint8_t address, uint32_t raw, MotorConfig *config,
                           MotorController *controller
#if defined(DAMIAO_DM4310)
                           , uint32_t *normalized_value,
                           ParameterProtocolResult *result
#endif
                           )
{
    const float value = bits_float(raw);
#if defined(DAMIAO_DM4310)
    (void)controller;
#endif
    switch (address) {
    case 0x00:
        if ((int32_t)raw <= (int32_t)0x41200000U) return false;
#if !defined(DAMIAO_DM4310)
        config->bus_undervoltage = value;
#endif
        return true;
    case 0x01:
        /* The factory normalizes non-positive/unordered inputs to zero,
         * then immediately rebuilds torque conversion in the WRITE tail. */
#if defined(DAMIAO_DM4310)
        *normalized_value = parameter_float_positive(value) ? raw : 0U;
        (void)config;
#else
        config->torque_constant = value > 0.0f ? value : 0.0f;
#endif
        return true;
    case 0x02:
        if ((raw - 0x42A00000U) >= 0x00A80000U) return false;
#if !defined(DAMIAO_DM4310)
        config->motor_temperature_limit = value;
#endif
        return true;
    case 0x03:
        if (((int32_t)raw > (int32_t)0x3F7FFFFFU) ||
#if defined(DAMIAO_DM4310)
            !parameter_float_positive(value)) return false;
#else
            (value <= 0.0f)) return false;
#endif
#if !defined(DAMIAO_DM4310)
        config->current_limit = value;
#endif
        return true;
    case 0x04:
#if defined(DAMIAO_DM4310)
        if (!parameter_float_positive(value)) return false;
#else
        if (!(value > 0.0f)) return false;
#endif
#if !defined(DAMIAO_DM4310)
        config->acceleration_limit = value;
#endif
        return true;
    case 0x05:
#if defined(DAMIAO_DM4310)
        /* Factory 0x1fff9334 tests CC (C clear), not signed LT. */
        if ((parameter_float_zero_flags(value) & UINT32_C(0x20000000)) != 0U)
            return false;
#else
        if (!(value < 0.0f)) return false;
#endif
#if !defined(DAMIAO_DM4310)
        config->deceleration_limit = value;
#endif
        return true;
    case 0x06:
#if defined(DAMIAO_DM4310)
        if (!parameter_float_positive(value)) return false;
#else
        if (!(value > 0.0f)) return false;
#endif
#if !defined(DAMIAO_DM4310)
        config->speed_limit = value;
#endif
        return true;
    case 0x07:
        /* The reference stores the incoming word AND 0x7ff rather than
         * rejecting an oversized standard ID. */
#if !defined(DAMIAO_DM4310)
        config->master_id = (uint16_t)(raw & 0x7FFU);
#endif
        return true;
    case 0x08:
#if !defined(DAMIAO_DM4310)
        config->can_id = (uint16_t)(raw & 0x7FFU);
#endif
        return true;
    case 0x09:
#if !defined(DAMIAO_DM4310)
        config->communication_timeout = raw;
#endif
        return true;
    case 0x0A: {
        if (raw < MOTOR_MODE_MIT || raw > MOTOR_MODE_HYBRID) return false;
#if defined(DAMIAO_DM4310)
        volatile uint32_t *const record =
            (volatile uint32_t *)(uintptr_t)result->config_owner;
        if (record[0x0A] != raw) {
            const float zero = 0.0f;
            volatile float *const command =
                (volatile float *)(uintptr_t)result->sample_owner;
            __asm__ volatile (
                "vstr %1, [%0, #0]\n\t"
                "vstr %1, [%0, #4]\n\t"
                "vstr %1, [%0, #8]\n\t"
                "vstr %1, [%0, #12]\n\t"
                "vstr %1, [%0, #16]"
                :
                : "r" (command), "t" (zero)
                : "memory");
            record[0x0A] = raw;
            result->sample_owner[0x3CU / 4U] = raw;
        }
#else
        if (config->control_mode != (MotorControlMode)raw) {
            memset(&controller->command, 0, sizeof(controller->command));
            controller->command.mode = (MotorControlMode)raw;
            config->control_mode = (MotorControlMode)raw;
        }
#endif
        return true;
    }
    case 0x15:
        if (!parameter_float_positive(value)) return false;
#if !defined(DAMIAO_DM4310)
        config->position_min = -value;
        config->position_max = value;
#endif
        return true;
    case 0x16:
        if (!parameter_float_positive(value)) return false;
#if !defined(DAMIAO_DM4310)
        config->velocity_min = -value;
        config->velocity_max = value;
#endif
        return true;
    case 0x17:
        if (!parameter_float_positive(value)) return false;
#if !defined(DAMIAO_DM4310)
        config->torque_min = -value;
        config->torque_max = value;
#endif
        return true;
    case 0x18:
        if ((raw - 0x42C80000U) > 0x03544000U) return false;
#if !defined(DAMIAO_DM4310)
        config->current_loop_bandwidth = value;
#endif
        return true;
    case 0x19:
        if (!parameter_float_nonnegative(value)) return false;
#if !defined(DAMIAO_DM4310)
        config->speed_kp = value;
#endif
        return true;
    case 0x1A:
        if (!parameter_float_nonnegative(value)) return false;
#if !defined(DAMIAO_DM4310)
        config->speed_ki = value;
#endif
        return true;
    case 0x1B:
        if (!parameter_float_nonnegative(value)) return false;
#if !defined(DAMIAO_DM4310)
        config->position_kp = value;
#endif
        return true;
    case 0x1C:
        if (!parameter_float_nonnegative(value)) return false;
#if !defined(DAMIAO_DM4310)
        config->position_ki = value;
#endif
        return true;
    case 0x1D: {
#if defined(DAMIAO_DM4310)
        const float undervoltage =
            *(volatile const float *)(uintptr_t)result->config_owner;
        if (!parameter_float_greater(value, undervoltage) ||
            ((int32_t)raw >
             (int32_t)APP_PROFILE_STARTUP_BUS_OVERVOLTAGE_BITS)) return false;
#else
        if ((value <= config->bus_undervoltage) ||
            ((int32_t)raw >
             (int32_t)APP_PROFILE_STARTUP_BUS_OVERVOLTAGE_BITS)) return false;
#endif
#if !defined(DAMIAO_DM4310)
        config->bus_overvoltage = value;
#endif
        return true;
    }
    case 0x1E:
        if (!parameter_float_positive(value) ||
            ((int32_t)raw >= (int32_t)0x3F800001U)) return false;
#if !defined(DAMIAO_DM4310)
        config->gear_torque_efficiency = value;
#endif
        return true;
    case 0x1F:
        if ((raw - 0x3F800000U) >= 0x02700001U) return false;
#if !defined(DAMIAO_DM4310)
        config->speed_loop_damping = value;
#endif
        return true;
    case 0x20:
#if defined(DAMIAO_DM4310)
        if (!(parameter_float_positive(value) ||
              ((int32_t)raw >= (int32_t)0x43FA0000U))) {
            return false;
        }
#else
        if ((int32_t)raw <= 0) return false;
#endif
#if !defined(DAMIAO_DM4310)
        config->velocity_filter_bandwidth = value;
#endif
        return true;
    case 0x21:
        if ((raw - 0x42C80000U) > 0x03544000U) return false;
#if !defined(DAMIAO_DM4310)
        config->current_loop_enhancement = value;
#endif
        return true;
    case 0x22:
        if (!parameter_float_positive(value) || (raw > 0x461C4000U)) return false;
#if !defined(DAMIAO_DM4310)
        config->velocity_loop_enhancement = value;
#endif
        return true;
    case 0x23:
        if (raw > 11U) return false;
#if defined(DAMIAO_DM4310)
        /* Factory 0x1fff9632 stores the selector before publishing the
         * classic/FD initialize and send pair, before the WRITE reply. */
        ((volatile uint32_t *)(uintptr_t)result->config_owner)[0x23] = raw;
        platform_select_mcan_transport_format_irq((uint8_t)raw,
                                                  result->references);
#endif
#if !defined(DAMIAO_DM4310)
        config->can_data_rate_selector = (uint8_t)raw;
#endif
        return true;
    default:
        return false;
    }
}

#if defined(DAMIAO_DM4310)
static void publish_response_byte(ParameterProtocolResult *result,
                                  uint8_t index, uint8_t value)
{
    result->payload[index] = value;
    /* DM replies send the fixed payload directly; no stack mirror belongs
     * to the factory byte-store sequence. Metadata stays in result. */
    (void)result;
}
#endif

static void prepare_response(const MotorConfig *config, uint8_t operation,
                             uint8_t address, uint32_t value,
#if defined(DAMIAO_DM4310)
                             uint32_t retained_node,
#endif
                             ParameterProtocolResult *result)
{
#if defined(DAMIAO_DM4310)
    (void)config;
    /* READ/STORE retain r0 from addressed dispatch. Only WRITE reloads
     * the possibly changed node halfword (factory 0x1fff9650). */
    const uint16_t node_id = operation == PARAMETER_WRITE ?
        *(volatile const uint16_t *)(uintptr_t)(result->config_owner + 0x20U) :
        (uint16_t)retained_node;
    result->response.length = operation == PARAMETER_STORE ? 4U : 8U;
    publish_response_byte(result, 0U, (uint8_t)node_id);
    const uint8_t high_mask = operation == PARAMETER_WRITE ? 0x07U :
                             operation == PARAMETER_STORE ? 0x0FU : 0xFFU;
    publish_response_byte(result, 1U, (uint8_t)(node_id >> 8U) & high_mask);
    publish_response_byte(result, 2U, operation);
    if (operation == PARAMETER_WRITE) {
        /* Factory 0x1fff965e loads the fixed 0x1fffcc4c scratch literal;
         * this WRITE-tail reread is deliberately not the earlier 8f9c
         * indirection used by addressed dispatch. */
        address = *(volatile const uint8_t *)(uintptr_t)
            FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffcc6b), UINT32_C(0x1fffcbf7));
    }
    publish_response_byte(result, 3U, address);
    if (operation != PARAMETER_STORE) {
        publish_response_byte(result, 4U, (uint8_t)value);
        publish_response_byte(result, 5U, (uint8_t)(value >> 8U));
        publish_response_byte(result, 6U, (uint8_t)(value >> 16U));
        publish_response_byte(result, 7U, (uint8_t)(value >> 24U));
    }
    result->response.id = *(volatile const uint16_t *)(uintptr_t)
        (result->config_owner + 0x1CU);
    result->response_prebuilt = true;
#else
    result->response.id = config->master_id;
    result->response.length = 8U;
    result->response.data[0] = (uint8_t)config->can_id;
    result->response.data[1] = (uint8_t)(config->can_id >> 8U);
    result->response.data[2] = operation;
    result->response.data[3] = address;
    write_u32_le(&result->response.data[4], value);
#endif
}

static void parameter_protocol_process_impl(const CanFrame *request,
                                MotorConfig *config,
                                MotorController *controller,
                                const ParameterRuntime *runtime,
                                ParameterProtocolResult *result
#if defined(DAMIAO_DM4310)
                                , Dm4310McanIrqReferences *references
#endif
                                )
{
#if defined(DAMIAO_DM4310)
    /* Only dispatch metadata needs initialization. Each ready response
     * fills its transmitted bytes; do not clear unused frame storage. */
    (void)runtime;
    result->handled = false;
    result->response_ready = false;
    result->response_prebuilt = false;
    result->store_requested = false;
    result->persist_requested = false;
    result->filter_update_requested = false;
    result->transport_reconfigure_requested = false;
    result->bootloader_requested = false;
#else
    memset(result, 0, sizeof(*result));
#endif
    if ((request == NULL) || (config == NULL) || (controller == NULL)
#if !defined(DAMIAO_DM4310)
        || (runtime == NULL)
#endif
       ) {
        return;
    }
#if defined(DAMIAO_DM4310)
    uint32_t retained_node = 0U;
    uint32_t parameter_header = 0U;
    if (request->id == PARAMETER_CAN_ID) {
        volatile uint8_t *const response_owner =
            (volatile uint8_t *)(uintptr_t)
                *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff8f90), UINT32_C(0x1fff8950));
        volatile uint8_t *const parameter_scratch =
            (volatile uint8_t *)(uintptr_t)
                *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff8f9c), UINT32_C(0x1fff895c));
        const volatile uint8_t *const config_owner = references != NULL ?
            (const volatile uint8_t *)references->config :
            (const volatile uint8_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558));
        result->payload = response_owner + 8U;
        result->scratch = parameter_scratch;
        result->config_owner = config_owner;
        result->motor_owner = references != NULL ?
            (volatile uint32_t *)references->motor :
            (volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
        result->sample_owner = references != NULL ?
            (volatile uint32_t *)references->sample :
            (volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff104), UINT32_C(0x1ffff090));
        result->status_owner = references != NULL ? references->status :
            (volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff1f0), UINT32_C(0x1ffff17c));
        result->irq_context = references != NULL;
        result->references = references;
        if (references != NULL) {
            references->response_dispatch = response_owner;
            references->parameter_scratch = parameter_scratch;
        }
        /* Factory 0x1fff8d3e reloads payload word zero, not word one. */
        parameter_header = *(volatile const uint32_t *)(uintptr_t)
            (response_owner + 0x64U);
        /* Factory 0x1fff8d44..0x1fff8d50 publishes parser scratch before
         * addressed-node matching, including legacy/nonmatching requests. */
        *(volatile uint16_t *)(uintptr_t)(parameter_scratch + 0x1CU) =
            (uint16_t)parameter_header;
        parameter_scratch[0x1EU] = (uint8_t)(parameter_header >> 16U);
        parameter_scratch[0x1FU] = (uint8_t)(parameter_header >> 24U);
        /* Factory 0x1fff8d52 retains this full word through dispatch. */
        retained_node = *(volatile const uint32_t *)(uintptr_t)
            (config_owner + 0x20U);
    }
#endif
    if (
#if defined(DAMIAO_DM4310)
        ((uint16_t)parameter_header != retained_node) &&
#endif
        process_legacy_command(request, config, controller,
#if defined(DAMIAO_DM4310)
                               retained_node,
                               parameter_header,
#endif
                               result)) {
        result->handled = true;
        return;
    }
    if ((request->id != PARAMETER_CAN_ID) ||
#if defined(DAMIAO_DM4310)
        ((uint16_t)parameter_header !=
         retained_node)) {
#else
        (read_u16_le(&request->data[4]) !=
         config->can_id)) {
#endif
        return;
    }

#if defined(DAMIAO_DM4310)
    result->scratch = (volatile uint8_t *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff8f9c), UINT32_C(0x1fff895c));
    if (references != NULL) {
        references->parameter_scratch = result->scratch;
    }
    const uint8_t operation = (uint8_t)(parameter_header >> 16U);
    const uint8_t address = (uint8_t)(parameter_header >> 24U);
#else
    const uint8_t operation = request->data[6];
    const uint8_t address = request->data[7];
#endif
    result->handled = true;

    if (operation == PARAMETER_READ) {
        uint32_t value;
#if defined(DAMIAO_DM4310)
        if (address == 0x08U) {
            value = retained_node;
        } else if (address <= 0x24U) {
            /* Factory READ arms 0x1fff90aa..0x1fff9184 load raw words.
             * Do not narrow IDs/modes/selectors through the decoded view. */
            volatile const uint32_t *const record =
                (volatile const uint32_t *)(uintptr_t)result->config_owner;
            value = record[address];
        } else if ((address >= 0x32U) && (address <= 0x35U)) {
            volatile const uint32_t *const calibration =
                (volatile const uint32_t *)(uintptr_t)
                    *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff93d0), UINT32_C(0x1fff8d90));
            value = calibration[address - 0x32U];
        } else if (address == 0x36U) {
            const volatile uint32_t *const zero_staging =
                (volatile const uint32_t *)(uintptr_t)
                    *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff93d4), UINT32_C(0x1fff8d94));
            value = zero_staging[0];
        } else if (address == 0x37U) {
            const volatile uint32_t *const motor = references != NULL ?
                (const volatile uint32_t *)references->motor :
                (const volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
            value = motor[0x34U / 4U];
        } else if (address == 0x50U) {
            const volatile uint32_t *const motor = references != NULL ?
                (const volatile uint32_t *)references->motor :
                (const volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
            const uint32_t direction = motor[0x34U / 4U];
            value = read_directed_parameter_word(&motor[0x18U / 4U],
                                                 direction);
        } else if (address == 0x51U) {
            const volatile uint32_t *const motor = references != NULL ?
                (const volatile uint32_t *)references->motor :
                (const volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
            const uint32_t direction = motor[0x34U / 4U];
            const volatile uint32_t *const output =
                (const volatile uint32_t *)(uintptr_t)
                    *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff93d8), UINT32_C(0x1fff8d98));
            value = read_directed_parameter_word(&output[0x3CU / 4U],
                                                 direction);
        } else {
            value = 0U;
        }
#else
        if (!read_register(address, config, runtime, &value)) {
            /* The default arm of the factory READ jump table leaves r0
             * (the addressed node ID) as the returned value. */
            value = config->can_id;
        }
#endif
        prepare_response(config, operation, address, value,
#if defined(DAMIAO_DM4310)
                         retained_node,
#endif
                         result);
        result->response_ready = true;
        return;
    }

    if (operation == PARAMETER_LIVE) {
        prepare_live_response(address, config, controller,
#if defined(DAMIAO_DM4310)
                              retained_node,
#endif
                              result);
        return;
    }

    if (operation == PARAMETER_WRITE) {
#if !defined(DAMIAO_DM4310)
        const uint16_t old_node_id = config->can_id;
        const uint8_t old_rate = config->can_data_rate_selector;
#endif
#if defined(DAMIAO_DM4310)
        const uint32_t incoming_value =
            *(volatile const uint32_t *)(uintptr_t)(result->payload + 0x60U);
#else
        const uint32_t incoming_value = read_u32_le(request->data);
#endif
#if defined(DAMIAO_DM4310)
        uint32_t normalized_value = incoming_value;
#endif
        const bool accepted = write_register(address, incoming_value,
                                             config, controller
#if defined(DAMIAO_DM4310)
                                             , &normalized_value, result
#endif
                                             );
        if (accepted && (address == 0x20U)) {
#if defined(DAMIAO_DM4310)
            ((volatile uint32_t *)(uintptr_t)result->config_owner)[0x20] =
                incoming_value;
            /* Accepted input below the signed 500 threshold already took
             * factory GT; don't execute another VFP comparison. */
            dm4310_motor_control_configure_velocity_filter_irq(
                bits_float(incoming_value),
                (int32_t)incoming_value < (int32_t)UINT32_C(0x43fa0000),
                (volatile float *)(uintptr_t)&result->motor_owner[0x68U / 4U],
                references != NULL ? references->two_pi :
                    bits_float(UINT32_C(0x40c90fdb)));
#else
            motor_control_configure_velocity_filter(
                controller, config->velocity_filter_bandwidth);
#endif
        }
        uint32_t effective_value = 0U;
        if ((address <= 0x0AU) ||
            ((address >= 0x15U) && (address <= 0x23U))) {
            /* Valid but rejected writes return the retained register value.
             * Reserved entries in the write jump table fall through with
             * r9 == 0 and still transmit a response. */
#if defined(DAMIAO_DM4310)
            if (!accepted) {
                volatile const uint32_t *const record =
                    (volatile const uint32_t *)(uintptr_t)result->config_owner;
                effective_value = record[address];
            } else {
                effective_value = normalized_value;
            }
#else
            {
                (void)read_register(address, config, runtime, &effective_value);
            }
#endif
        }
#if defined(DAMIAO_DM4310)
        if (accepted && (address != 0x0AU) && (address != 0x20U) &&
            (address != 0x23U) && (address != 0x18U) && (address != 0x21U) &&
            ((address < 0x19U) || (address > 0x1CU))) {
            /* The factory parameter parser edits the fixed 37-word live
             * record directly.  Keep its corresponding word current so a
             * later STORE only has to post runtime-status +0x0c. */
            ((volatile uint32_t *)(uintptr_t)result->config_owner)[address] =
                ((address == 0x07U) || (address == 0x08U)) ?
                    effective_value & UINT32_C(0x7FF) : effective_value;
        }
        if (accepted && ((address == 0x01U) || (address == 0x1EU))) {
            publish_torque_conversion(address, effective_value, result);
        }
        if (accepted && (address >= 0x19U) && (address <= 0x1CU)) {
            /* Factory 0x1fff945e..0x1fff94d4 publishes only Kp or Ki.
             * No controller reset or global derivation belongs here. */
            const uintptr_t pool = address <= 0x1AU ?
                FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff9838), UINT32_C(0x1fff91f8)) : FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff983c), UINT32_C(0x1fff91fc));
            const uint32_t offset = ((uint32_t)address - 0x19U) & 1U;
            volatile float *const staged =
                (volatile float *)(uintptr_t)(result->config_owner +
                                             (uint32_t)address * sizeof(float));
            const float retained = bits_float(effective_value);
            uintptr_t coefficient;
            __asm__ volatile ("vstr %3, [%1]\n\t"
                              "ldr %0, [%2]\n\t"
                              "add %0, %0, %4\n\t"
                              "vstr %3, [%0]"
                              : "=&r" (coefficient)
                              : "r" (staged), "r" (pool), "t" (retained),
                                "r" (offset * sizeof(float))
                              : "memory");
        }
        if (accepted && ((address == 0x18U) || (address == 0x21U) ||
                         (address == 0x22U))) {
            if (address != 0x22U) {
                /* Bandwidth writes cache +0x38, enhancement +0x3c,
                 * before entering factory derivation veneer 0x1fffa4dc. */
                volatile uint32_t *const cache =
                    (volatile uint32_t *)(uintptr_t)
                        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fff9834), UINT32_C(0x1fff91f4));
                ((volatile uint32_t *)(uintptr_t)result->config_owner)[address] =
                    effective_value;
                cache[address == 0x18U ? 0x38U / 4U : 0x3CU / 4U] =
                    effective_value;
            }
            dm4310_derive_control_parameters_helper();
        }
#endif
#if defined(DAMIAO_DM4310)
        if (accepted && (address == 0x08U)) {
            /* Factory filter store precedes fixed reply-byte assembly. */
            platform_update_mcan_node_filter((uint16_t)(effective_value & 0x7FFU));
        }
#endif
        prepare_response(config, operation, address, effective_value,
#if defined(DAMIAO_DM4310)
                         retained_node,
#endif
                         result);
        result->response_ready = true;
#if defined(DAMIAO_DM4310)
        /* Every WRITE reaches the post-send scratch-address check. The
         * IRQ must decide there, not from this pre-send decoded address. */
        result->transport_reconfigure_requested = true;
#else
        result->filter_update_requested = accepted &&
            (config->can_id != old_node_id);
        result->transport_reconfigure_requested = accepted &&
            (config->can_data_rate_selector != old_rate);
#endif
        return;
    }

    if ((operation == PARAMETER_STORE) &&
#if defined(DAMIAO_DM4310)
        (result->motor_owner[0x38U / 4U] == 0U)) {
        /* Factory 0x1fff96aa posts the full-word flag before constructing
         * and transmitting its four-byte acknowledgement. */
        result->status_owner[0x0CU / 4U] = 1U;
#else
        !controller->armed) {
#endif
        prepare_response(config, operation, 0x01U, 0U,
#if defined(DAMIAO_DM4310)
                         retained_node,
#endif
                         result);
        result->response.length = 4U;
        result->store_requested = true;
#if defined(DAMIAO_DM4310)
        /* Main performs the blocking Flash operation later, without a
         * duplicate acknowledgement. The flag is already posted above. */
        result->response_ready = true;
#endif
    }
}

void parameter_protocol_process(const CanFrame *request,
                                MotorConfig *config,
                                MotorController *controller,
                                const ParameterRuntime *runtime,
                                ParameterProtocolResult *result)
{
    parameter_protocol_process_impl(request, config, controller, runtime, result
#if defined(DAMIAO_DM4310)
                                    , NULL
#endif
                                    );
}

#if defined(DAMIAO_DM4310)
void dm4310_parameter_protocol_process_irq(const CanFrame *request,
                                ParameterProtocolResult *result,
                                Dm4310McanIrqReferences *references)
{
    parameter_protocol_process_impl(
        request, (MotorConfig *)(uintptr_t)references->config,
        (MotorController *)(uintptr_t)references->motor, NULL, result,
        references);
}
#endif
