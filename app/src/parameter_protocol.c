#include "parameter_protocol.h"

#include "memory_layout.h"

#include <stddef.h>
#include <string.h>

#include "app_config.h"
#include "feedback_measurements.h"
#include "motor_math.h"
#include "safety.h"
#include "platform.h"

#include "board_mcan.h"

#define PARAMETER_CAN_ID (0x7FFU)
#define PARAMETER_READ (0x33U)
#define PARAMETER_WRITE (0x55U)
#define PARAMETER_STORE (0xAAU)
#define PARAMETER_LIVE (0xCCU)

#define LEGACY_DISCOVERY_HEAD (0x55U)
#define LEGACY_DISCOVERY_TAIL (0xAAU)
#define LEGACY_ID_HEAD (0xAAU)
#define LEGACY_ID_TAIL (0x55U)
#define LEGACY_VERSION_HEAD (0xEEU)
#define LEGACY_VERSION_TAIL (0x11U)

static void publish_response_byte(ParameterProtocolResult *result, uint8_t index, uint8_t value);

static void prepare_legacy_response(uint16_t id, const uint8_t *data, uint8_t length,
                                    ParameterProtocolResult *result)
{
    result->response.id = id;
    result->response.length = length;
    for (uint8_t index = 0U; index < length; ++index)
    {
        publish_response_byte(result, index, data[index]);
    }
    result->response_prebuilt = true;
    result->response_ready = true;
}

static bool process_legacy_command(const CanFrame *request, MotorConfig *config,
                                   const MotorController *controller, uint32_t retained_node,
                                   uint32_t parameter_header, ParameterProtocolResult *result)
{
    (void)controller;
    if (request->id != PARAMETER_CAN_ID)
    {
        return false;
    }
    if (((uint16_t)parameter_header == retained_node) || (result->motor_owner[0x38U / 4U] != 0U))
    {
        return false;
    }

    const uint8_t head = (uint8_t)parameter_header;
    const uint8_t node_byte = (uint8_t)(parameter_header >> 8U);
    const uint8_t master_byte = (uint8_t)(parameter_header >> 16U);
    const uint8_t tail = (uint8_t)(parameter_header >> 24U);
    if ((head == LEGACY_DISCOVERY_HEAD) && (node_byte == 0U) && (master_byte == 0U) &&
        (tail == LEGACY_DISCOVERY_TAIL))
    {
        publish_response_byte(result, 0U, LEGACY_DISCOVERY_HEAD);
        publish_response_byte(result, 1U, (uint8_t)(retained_node & 0x7FU));
        const uint8_t master = result->config_owner[0x1CU];
        publish_response_byte(result, 2U, master & 0x7FU);
        publish_response_byte(result, 3U, LEGACY_DISCOVERY_TAIL);
        result->response.id = PARAMETER_CAN_ID;
        result->response.length = 4U;
        result->response_prebuilt = true;
        result->response_ready = true;
        return true;
    }

    if ((head == LEGACY_ID_HEAD) && (tail == LEGACY_ID_TAIL))
    {
        const uint32_t node = node_byte;
        const uint32_t master = master_byte;
        volatile uint32_t *const ids =
            (volatile uint32_t *)(uintptr_t)(result->config_owner + 0x1CU);
        /* This path publishes master/node in one STRD. */
        __asm__ volatile("strd %1, %2, [%0]" : : "r"(ids), "r"(master), "r"(node) : "memory");
        platform_update_mcan_node_filter((uint16_t)node);
        if (!result->irq_context)
        {
            config->can_id = (uint16_t)node;
            config->master_id = (uint16_t)master;
        }
        /* This path uses the retained ID bytes, not a
         * reread of the decoded configuration or a temporary payload. */
        publish_response_byte(result, 0U, LEGACY_ID_HEAD);
        publish_response_byte(result, 1U, node_byte & 0x7FU);
        publish_response_byte(result, 2U, master_byte & 0x7FU);
        publish_response_byte(result, 3U, LEGACY_ID_TAIL);
        result->response.id = PARAMETER_CAN_ID;
        result->response.length = 4U;
        result->response_prebuilt = true;
        result->response_ready = true;
        result->persist_requested = true;
        return true;
    }

    if ((head == 0x55U) && (node_byte == 0x01U) && (master_byte == 0x02U) && (tail == 0xAAU) &&
        (*(volatile const uint16_t *)(uintptr_t)(result->payload + 0x60U) == retained_node))
    {
        static const uint8_t response[8] = {
            'A', 'u', 'p', 'g', 'r', 'a', 'd', 'e',
        };
        prepare_legacy_response(0x7FEU, response, sizeof(response), result);
        result->bootloader_requested = true;
        return true;
    }

    if ((head == LEGACY_VERSION_HEAD) && (tail == LEGACY_VERSION_TAIL))
    {
        /* This path posts the head before loading version. */
        publish_response_byte(result, 0U, LEGACY_VERSION_TAIL);
        const uint16_t version =
            *(volatile const uint16_t *)(uintptr_t)(result->config_owner + 0x38U);
        publish_response_byte(result, 1U, (uint8_t)version);
        publish_response_byte(result, 2U, (uint8_t)(version >> 8U));
        publish_response_byte(result, 3U, LEGACY_VERSION_HEAD);
        result->response.id = PARAMETER_CAN_ID;
        result->response.length = 4U;
        result->response_prebuilt = true;
        result->response_ready = true;
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

static uint32_t read_directed_parameter_word(volatile const uint32_t *value, uint32_t direction)
{
    if (direction == UINT32_C(0x3f800000))
    {
        return *value;
    }
    uint32_t bits;
    __asm__ volatile("vldr s0, [%1]\n\t"
                     "vneg.f32 s0, s0\n\t"
                     "vmov %0, s0"
                     : "=r"(bits)
                     : "r"(value)
                     : "s0", "memory");
    return bits;
}

static float bits_float(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint8_t live_temperature_byte(float value)
{
    int32_t converted;
    __asm__ volatile("vcvt.s32.f32 %1, %1\n\t"
                     "vmov %0, %1"
                     : "=r"(converted), "+t"(value));
    return (uint8_t)converted;
}

static void prepare_live_response(uint8_t selector, const MotorConfig *config,
                                  const MotorController *controller, uint32_t retained_node,
                                  ParameterProtocolResult *result)
{
    (void)config;
    (void)controller;
    result->response.length = 8U;

    if (selector == 0U)
    {
        can_protocol_encode_parameter_feedback_irq(result->references, &result->response);
        result->response_prebuilt = true;
        result->response_ready = true;
        return;
    }
    if ((selector >= 1U) && (selector <= 3U))
    {
        publish_response_byte(result, 0U, (uint8_t)retained_node);
        const volatile uint8_t *const sample_bytes = (const volatile uint8_t *)result->sample_owner;
        const volatile float *const sample = (const volatile float *)result->sample_owner;
        const volatile uint32_t *const motor = result->motor_owner;
        publish_response_byte(result, 1U, sample_bytes[0x80U]);
        const float mos = sample[0x84U / 4U];
        publish_response_byte(result, 2U, live_temperature_byte(mos));
        const float motor_temperature = ((const volatile float *)motor)[0x40U / 4U];
        publish_response_byte(result, 3U, live_temperature_byte(motor_temperature));
        const uint32_t direction_bits = motor[0x34U / 4U];
        uint32_t bits;
#if FIRMWARE_USES_RAW_CAN_FEEDBACK
        if (selector == 1U)
        {
            bits = read_directed_parameter_word(&motor[0x18U / 4U], direction_bits);
        }
        else
        {
            const volatile float *const motor_floats = (const volatile float *)motor;
            const float measurement =
                selector == 2U ? feedback_measurement_velocity(motor_floats)
                               : feedback_measurement_torque(motor_floats, sample);
            bits = float_bits(measurement);
            if (direction_bits != UINT32_C(0x3f800000))
            {
                bits ^= UINT32_C(0x80000000);
            }
        }
#else
        const uint32_t offset = selector == 1U ? 0x18U : selector == 2U ? 0x1CU : 0x30U;
        bits = read_directed_parameter_word(&motor[offset / 4U], direction_bits);
#endif
        publish_response_byte(result, 4U, (uint8_t)bits);
        publish_response_byte(result, 5U, (uint8_t)(bits >> 8U));
        publish_response_byte(result, 6U, (uint8_t)(bits >> 16U));
        publish_response_byte(result, 7U, (uint8_t)(bits >> 24U));
        result->response.id = *(volatile const uint16_t *)(uintptr_t)(result->config_owner + 0x1CU);
        result->response_prebuilt = true;
        result->response_ready = true;
        return;
    }
    if (selector == 4U)
    {
        volatile uint32_t *const motor = result->motor_owner;
        volatile uint32_t *const sample = result->sample_owner;
        const volatile float *const config_floats =
            (const volatile float *)(uintptr_t)result->config_owner;
        const uint32_t direction_bits = motor[0x34U / 4U];
        uint32_t position_bits;
        float maximum;
        if (direction_bits == UINT32_C(0x3f800000))
        {
            /* This path loads the limit before the raw position. */
            maximum = config_floats[0x58U / 4U];
            position_bits = motor[0x18U / 4U];
        }
        else
        {
            /* The negative arm instead loads position through VLDR first. */
            float position = ((volatile float *)motor)[0x18U / 4U];
            maximum = config_floats[0x58U / 4U];
            __asm__ volatile("vneg.f32 %0, %0" : "+t"(position));
            position_bits = float_bits(position);
        }
        float velocity = feedback_measurement_velocity((const volatile float *)motor);
        if (direction_bits != UINT32_C(0x3f800000))
        {
            __asm__ volatile("vneg.f32 %0, %0" : "+t"(velocity));
        }
        const uint16_t encoded_velocity =
            (uint16_t)float_to_uint_helper(velocity, -maximum, maximum, 12U);
        volatile uint16_t *const velocity_scratch =
            (volatile uint16_t *)(uintptr_t)(result->scratch + 4U);
        *velocity_scratch = encoded_velocity;
        float current = ((volatile float *)sample)[0x64U / 4U];
        const float scale = 10000.0f;
        int32_t encoded_current;
        __asm__ volatile("vmul.f32 %1, %1, %2\n\t"
                         "vcvt.s32.f32 %1, %1\n\t"
                         "vmov %0, %1"
                         : "=r"(encoded_current), "+&t"(current)
                         : "t"(scale)
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
        result->response.id = *(volatile const uint16_t *)(uintptr_t)(result->config_owner + 0x1CU);
        result->response_prebuilt = true;
        result->response_ready = true;
    }
}

static uint32_t parameter_float_zero_flags(float value);

static void publish_torque_conversion(uint8_t address, uint32_t value,
                                      const ParameterProtocolResult *result)
{
    volatile const float *const record = (volatile const float *)(uintptr_t)result->config_owner;
    volatile float *const conversion =
        (volatile float *)(uintptr_t)&result->motor_owner[0x44U / 4U];
    const float scale = bits_float(APP_PROFILE_CURRENT_FULL_SCALE_BITS);
    const float torque = address == 0x01U ? bits_float(value) : record[1];
    /* Torque WRITE already normalized its value using VCMPE; don't compare
     * it again. Efficiency WRITE performs its own signaling NE check. */
    const bool explicit_torque =
        address == 0x01U ? (value & UINT32_C(0x7fffffff)) != 0U
                         : (parameter_float_zero_flags(torque) & UINT32_C(0x40000000)) == 0U;
    if (explicit_torque)
    {
        __asm__ volatile("vmul.f32 s0, %2, %3\n\t"
                         "vstr s0, [%0]\n\t"
                         "vmov.f32 s1, #1.0\n\t"
                         "vdiv.f32 s2, s1, s0\n\t"
                         "vstr s2, [%0, #4]"
                         :
                         : "r"(conversion), "r"(record), "t"(torque), "t"(scale)
                         : "s0", "s1", "s2", "memory");
    }
    else
    {
        /* Firmware fallback: uint pole pairs, 1.5, flux, literal scale,
         * gear ratio, efficiency. Keep every separate rounding step. */
        float factor;
        __asm__ volatile("vldr s0, [%1, #64]\n\t"
                         "vcvt.f32.u32 s0, s0\n\t"
                         "vmov.f32 s1, #1.5\n\t"
                         "vmul.f32 s0, s0, s1\n\t"
                         "vldr s1, [%1, #76]\n\t"
                         "vmul.f32 s0, s0, s1\n\t"
                         "vmul.f32 s0, s0, %2\n\t"
                         "vldr s1, [%1, #80]\n\t"
                         "vmul.f32 s0, s0, s1\n\t"
                         "vmov.f32 %0, s0"
                         : "=t"(factor)
                         : "r"(record), "t"(scale)
                         : "s0", "s1", "memory");
        /* Efficiency WRITE retains its input; only torque WRITE reloads
         * staged efficiency after the factor's gear-ratio multiplication. */
        const float efficiency = address == 0x1EU ? bits_float(value) : record[30];
        __asm__ volatile("vmul.f32 s0, %1, %2\n\t"
                         "vstr s0, [%0]\n\t"
                         "vmov.f32 s1, #1.0\n\t"
                         "vdiv.f32 s2, s1, s0\n\t"
                         "vstr s2, [%0, #4]"
                         :
                         : "r"(conversion), "t"(factor), "t"(efficiency)
                         : "s0", "s1", "s2", "memory");
    }
}

static uint32_t parameter_float_zero_flags(float value)
{
    uint32_t flags;
    __asm volatile("vcmpe.f32 %1, #0.0\n\t"
                   "vmrs %0, fpscr"
                   : "=r"(flags)
                   : "t"(value)
                   : "memory");
    return flags;
}

static bool parameter_float_positive(float value)
{
    const uint32_t flags = parameter_float_zero_flags(value);
    /* GT: Z clear and N == V, including unordered's N=0,V=1. */
    return (flags & UINT32_C(0x40000000)) == 0U && (((flags >> 31U) ^ (flags >> 28U)) & 1U) == 0U;
}
static bool parameter_float_nonnegative(float value)
{
    const uint32_t flags = parameter_float_zero_flags(value);
    return (((flags >> 31U) ^ (flags >> 28U)) & 1U) == 0U;
}
static bool parameter_float_greater(float left, float right)
{
    uint32_t flags;
    __asm volatile("vcmpe.f32 %1, %2\n\t"
                   "vmrs %0, fpscr"
                   : "=r"(flags)
                   : "t"(left), "t"(right)
                   : "memory");
    return (flags & UINT32_C(0x40000000)) == 0U && (((flags >> 31U) ^ (flags >> 28U)) & 1U) == 0U;
}

static bool write_register(uint8_t address, uint32_t raw, MotorConfig *config,
                           MotorController *controller, uint32_t *normalized_value,
                           ParameterProtocolResult *result)
{
    const float value = bits_float(raw);
    (void)controller;
    switch (address)
    {
    case 0x00:
        if ((int32_t)raw <= (int32_t)0x41200000U)
            return false;
        return true;
    case 0x01:
        /* The firmware normalizes non-positive/unordered inputs to zero,
         * then immediately rebuilds torque conversion in the WRITE tail. */
        *normalized_value = parameter_float_positive(value) ? raw : 0U;
        (void)config;
        return true;
    case 0x02:
        if ((raw - 0x42A00000U) >= 0x00A80000U)
            return false;
        return true;
    case 0x03:
        if (((int32_t)raw > (int32_t)0x3F7FFFFFU) || !parameter_float_positive(value))
            return false;
        return true;
    case 0x04:
        if (!parameter_float_positive(value))
            return false;
        return true;
    case 0x05:
        /* This path tests CC (C clear), not signed LT. */
        if ((parameter_float_zero_flags(value) & UINT32_C(0x20000000)) != 0U)
            return false;
        return true;
    case 0x06:
        if (!parameter_float_positive(value))
            return false;
        return true;
    case 0x07:
        /* The reference stores the incoming word AND 0x7ff rather than
         * rejecting an oversized standard ID. */
        return true;
    case 0x08:
        return true;
    case 0x09:
        return true;
    case 0x0A:
    {
        if (raw < MOTOR_MODE_MIT || raw > MOTOR_MODE_HYBRID)
            return false;
        volatile uint32_t *const record = (volatile uint32_t *)(uintptr_t)result->config_owner;
        if (record[0x0A] != raw)
        {
            const float zero = 0.0f;
            volatile float *const command = (volatile float *)(uintptr_t)result->sample_owner;
            __asm__ volatile("vstr %1, [%0, #0]\n\t"
                             "vstr %1, [%0, #4]\n\t"
                             "vstr %1, [%0, #8]\n\t"
                             "vstr %1, [%0, #12]\n\t"
                             "vstr %1, [%0, #16]"
                             :
                             : "r"(command), "t"(zero)
                             : "memory");
            record[0x0A] = raw;
            result->sample_owner[0x3CU / 4U] = raw;
        }
        return true;
    }
    case 0x15:
        if (!parameter_float_positive(value))
            return false;
        return true;
    case 0x16:
        if (!parameter_float_positive(value))
            return false;
        return true;
    case 0x17:
        if (!parameter_float_positive(value))
            return false;
        return true;
    case 0x18:
        if ((raw - 0x42C80000U) > 0x03544000U)
            return false;
        return true;
    case 0x19:
        if (!parameter_float_nonnegative(value))
            return false;
        return true;
    case 0x1A:
        if (!parameter_float_nonnegative(value))
            return false;
        return true;
    case 0x1B:
        if (!parameter_float_nonnegative(value))
            return false;
        return true;
    case 0x1C:
        if (!parameter_float_nonnegative(value))
            return false;
        return true;
    case 0x1D:
    {
        const float undervoltage = *(volatile const float *)(uintptr_t)result->config_owner;
        if (!parameter_float_greater(value, undervoltage) ||
            ((int32_t)raw > (int32_t)APP_PROFILE_STARTUP_BUS_OVERVOLTAGE_BITS))
            return false;
        return true;
    }
    case 0x1E:
        if (!parameter_float_positive(value) || ((int32_t)raw >= (int32_t)0x3F800001U))
            return false;
        return true;
    case 0x1F:
        if ((raw - 0x3F800000U) >= 0x02700001U)
            return false;
        return true;
    case 0x20:
        if (!(parameter_float_positive(value) || ((int32_t)raw >= (int32_t)0x43FA0000U)))
        {
            return false;
        }
        return true;
    case 0x21:
        if ((raw - 0x42C80000U) > 0x03544000U)
            return false;
        return true;
    case 0x22:
        if (!parameter_float_positive(value) || (raw > 0x461C4000U))
            return false;
        return true;
    case 0x23:
        if (raw > 11U)
            return false;
        /* This path stores the selector before publishing the
         * classic/FD initialize and send pair, before the WRITE reply. */
        ((volatile uint32_t *)(uintptr_t)result->config_owner)[0x23] = raw;
        platform_select_mcan_transport_format_irq((uint8_t)raw, result->references);
        return true;
    default:
        return false;
    }
}

static void publish_response_byte(ParameterProtocolResult *result, uint8_t index, uint8_t value)
{
    result->payload[index] = value;
    /* DM replies send the fixed payload directly; no stack mirror belongs
     * to the firmware byte-store sequence. Metadata stays in result. */
    (void)result;
}

static void prepare_response(const MotorConfig *config, uint8_t operation, uint8_t address,
                             uint32_t value, uint32_t retained_node,
                             ParameterProtocolResult *result)
{
    (void)config;
    /* READ/STORE retain r0 from addressed dispatch. Only WRITE reloads
     * the possibly changed node halfword (this path). */
    const uint16_t node_id =
        operation == PARAMETER_WRITE
            ? *(volatile const uint16_t *)(uintptr_t)(result->config_owner + 0x20U)
            : (uint16_t)retained_node;
    result->response.length = operation == PARAMETER_STORE ? 4U : 8U;
    publish_response_byte(result, 0U, (uint8_t)node_id);
    const uint8_t high_mask = operation == PARAMETER_WRITE   ? 0x07U
                              : operation == PARAMETER_STORE ? 0x0FU
                                                             : 0xFFU;
    publish_response_byte(result, 1U, (uint8_t)(node_id >> 8U) & high_mask);
    publish_response_byte(result, 2U, operation);
    if (operation == PARAMETER_WRITE)
    {
        /* Reread the address from the fixed protocol scratch at the WRITE
         * tail rather than retaining the earlier dispatch value. */
        address = *(volatile const uint8_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffcc6b),
                                                                              UINT32_C(0x1fffcbf7));
    }
    publish_response_byte(result, 3U, address);
    if (operation != PARAMETER_STORE)
    {
        publish_response_byte(result, 4U, (uint8_t)value);
        publish_response_byte(result, 5U, (uint8_t)(value >> 8U));
        publish_response_byte(result, 6U, (uint8_t)(value >> 16U));
        publish_response_byte(result, 7U, (uint8_t)(value >> 24U));
    }
    result->response.id = *(volatile const uint16_t *)(uintptr_t)(result->config_owner + 0x1CU);
    result->response_prebuilt = true;
}

static void parameter_protocol_process_impl(const CanFrame *request, MotorConfig *config,
                                            MotorController *controller,
                                            const ParameterRuntime *runtime,
                                            ParameterProtocolResult *result,
                                            McanIrqContext *references)
{
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
    if ((request == NULL) || (config == NULL) || (controller == NULL))
    {
        return;
    }
    uint32_t retained_node = 0U;
    uint32_t parameter_header = 0U;
    if (request->id == PARAMETER_CAN_ID)
    {
        volatile uint8_t *const response_owner =
            (volatile uint8_t *)(uintptr_t)*(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(
                UINT32_C(0x1fff8f90), UINT32_C(0x1fff8950));
        volatile uint8_t *const parameter_scratch =
            (volatile uint8_t *)(uintptr_t)*(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(
                UINT32_C(0x1fff8f9c), UINT32_C(0x1fff895c));
        const volatile uint8_t *const config_owner =
            references != NULL ? (const volatile uint8_t *)references->config
                               : (const volatile uint8_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                     UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558));
        result->payload = response_owner + 8U;
        result->scratch = parameter_scratch;
        result->config_owner = config_owner;
        result->motor_owner = references != NULL
                                  ? (volatile uint32_t *)references->motor
                                  : (volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                        UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
        result->sample_owner = references != NULL
                                   ? (volatile uint32_t *)references->sample
                                   : (volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                         UINT32_C(0x1ffff104), UINT32_C(0x1ffff090));
        result->status_owner = references != NULL
                                   ? references->status
                                   : (volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                         UINT32_C(0x1ffff1f0), UINT32_C(0x1ffff17c));
        result->irq_context = references != NULL;
        result->references = references;
        if (references != NULL)
        {
            references->response_dispatch = response_owner;
            references->parameter_scratch = parameter_scratch;
        }
        /* This path reloads payload word zero, not word one. */
        parameter_header = *(volatile const uint32_t *)(uintptr_t)(response_owner + 0x64U);
        /* This path publishes parser scratch before
         * addressed-node matching, including legacy/nonmatching requests. */
        *(volatile uint16_t *)(uintptr_t)(parameter_scratch + 0x1CU) = (uint16_t)parameter_header;
        parameter_scratch[0x1EU] = (uint8_t)(parameter_header >> 16U);
        parameter_scratch[0x1FU] = (uint8_t)(parameter_header >> 24U);
        /* This path retains this full word through dispatch. */
        retained_node = *(volatile const uint32_t *)(uintptr_t)(config_owner + 0x20U);
    }
    if (((uint16_t)parameter_header != retained_node) &&
        process_legacy_command(request, config, controller, retained_node, parameter_header,
                               result))
    {
        result->handled = true;
        return;
    }
    if ((request->id != PARAMETER_CAN_ID) || ((uint16_t)parameter_header != retained_node))
    {
        return;
    }

    result->scratch = (volatile uint8_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8f9c), UINT32_C(0x1fff895c));
    if (references != NULL)
    {
        references->parameter_scratch = result->scratch;
    }
    const uint8_t operation = (uint8_t)(parameter_header >> 16U);
    const uint8_t address = (uint8_t)(parameter_header >> 24U);
    result->handled = true;

    if (operation == PARAMETER_READ)
    {
        uint32_t value;
        if (address == 0x08U)
        {
            value = retained_node;
        }
        else if (address <= 0x24U)
        {
            /* READ responses expose raw configuration words.  Do not narrow
             * IDs, modes, or selectors through the decoded view. */
            volatile const uint32_t *const record =
                (volatile const uint32_t *)(uintptr_t)result->config_owner;
            value = record[address];
        }
        else if ((address >= 0x32U) && (address <= 0x35U))
        {
            volatile const uint32_t *const calibration = (volatile const uint32_t *)(uintptr_t)*(
                volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff93d0),
                                                                UINT32_C(0x1fff8d90));
            value = calibration[address - 0x32U];
        }
        else if (address == 0x36U)
        {
            const volatile uint32_t *const zero_staging = (volatile const uint32_t *)(uintptr_t)*(
                volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff93d4),
                                                                UINT32_C(0x1fff8d94));
            value = zero_staging[0];
        }
        else if (address == 0x37U)
        {
            const volatile uint32_t *const motor =
                references != NULL ? (const volatile uint32_t *)references->motor
                                   : (const volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                         UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
            value = motor[0x34U / 4U];
        }
        else if (address == 0x50U)
        {
            const volatile uint32_t *const motor =
                references != NULL ? (const volatile uint32_t *)references->motor
                                   : (const volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                         UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
            const uint32_t direction = motor[0x34U / 4U];
            value = read_directed_parameter_word(&motor[0x18U / 4U], direction);
        }
        else if (address == 0x51U)
        {
            const volatile uint32_t *const motor =
                references != NULL ? (const volatile uint32_t *)references->motor
                                   : (const volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                         UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
            const uint32_t direction = motor[0x34U / 4U];
            const volatile uint32_t *const output = (const volatile uint32_t *)(uintptr_t)*(
                volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff93d8),
                                                                UINT32_C(0x1fff8d98));
            value = read_directed_parameter_word(&output[0x3CU / 4U], direction);
        }
        else
        {
            value = 0U;
        }
        prepare_response(config, operation, address, value, retained_node, result);
        result->response_ready = true;
        return;
    }

    if (operation == PARAMETER_LIVE)
    {
        prepare_live_response(address, config, controller, retained_node, result);
        return;
    }

    if (operation == PARAMETER_WRITE)
    {
        const uint32_t incoming_value =
            *(volatile const uint32_t *)(uintptr_t)(result->payload + 0x60U);
        uint32_t normalized_value = incoming_value;
        const bool accepted =
            write_register(address, incoming_value, config, controller, &normalized_value, result);
        if (accepted && (address == 0x20U))
        {
            ((volatile uint32_t *)(uintptr_t)result->config_owner)[0x20] = incoming_value;
            /* Accepted input below the signed 500 threshold already took
             * firmware GT; don't execute another VFP comparison. */
            motor_control_configure_velocity_filter_irq(
                bits_float(incoming_value), (int32_t)incoming_value < (int32_t)UINT32_C(0x43fa0000),
                (volatile float *)(uintptr_t)&result->motor_owner[0x68U / 4U],
                references != NULL ? references->two_pi : bits_float(UINT32_C(0x40c90fdb)));
        }
        uint32_t effective_value = 0U;
        if ((address <= 0x0AU) || ((address >= 0x15U) && (address <= 0x23U)))
        {
            /* Valid but rejected writes return the retained register value.
             * Reserved entries in the write jump table fall through with
             * r9 == 0 and still transmit a response. */
            if (!accepted)
            {
                volatile const uint32_t *const record =
                    (volatile const uint32_t *)(uintptr_t)result->config_owner;
                effective_value = record[address];
            }
            else
            {
                effective_value = normalized_value;
            }
        }
        if (accepted && (address != 0x0AU) && (address != 0x20U) && (address != 0x23U) &&
            (address != 0x18U) && (address != 0x21U) && ((address < 0x19U) || (address > 0x1CU)))
        {
            /* The firmware parameter parser edits the fixed 37-word live
             * record directly.  Keep its corresponding word current so a
             * later STORE only has to post runtime-status +0x0c. */
            ((volatile uint32_t *)(uintptr_t)result->config_owner)[address] =
                ((address == 0x07U) || (address == 0x08U)) ? effective_value & UINT32_C(0x7FF)
                                                           : effective_value;
        }
        if (accepted && ((address == 0x01U) || (address == 0x1EU)))
        {
            publish_torque_conversion(address, effective_value, result);
        }
        if (accepted && (address >= 0x19U) && (address <= 0x1CU))
        {
            /* This path publishes only Kp or Ki.
             * No controller reset or global derivation belongs here. */
            const uintptr_t pool =
                address <= 0x1AU
                    ? MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff9838), UINT32_C(0x1fff91f8))
                    : MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff983c), UINT32_C(0x1fff91fc));
            const uint32_t offset = ((uint32_t)address - 0x19U) & 1U;
            volatile float *const staged =
                (volatile float *)(uintptr_t)(result->config_owner +
                                              (uint32_t)address * sizeof(float));
            const float retained = bits_float(effective_value);
            uintptr_t coefficient;
            __asm__ volatile("vstr %3, [%1]\n\t"
                             "ldr %0, [%2]\n\t"
                             "add %0, %0, %4\n\t"
                             "vstr %3, [%0]"
                             : "=&r"(coefficient)
                             : "r"(staged), "r"(pool), "t"(retained), "r"(offset * sizeof(float))
                             : "memory");
        }
        if (accepted && ((address == 0x18U) || (address == 0x21U) || (address == 0x22U)))
        {
            if (address != 0x22U)
            {
                /* Publish bandwidth or enhancement to the runtime cache before
                 * deriving the dependent controller parameters. */
                volatile uint32_t *const cache = (volatile uint32_t *)(uintptr_t)*(
                    volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff9834),
                                                                    UINT32_C(0x1fff91f4));
                ((volatile uint32_t *)(uintptr_t)result->config_owner)[address] = effective_value;
                cache[address == 0x18U ? 0x38U / 4U : 0x3CU / 4U] = effective_value;
            }
            derive_control_parameters_helper();
        }
        if (accepted && (address == 0x08U))
        {
            /* Firmware filter store precedes fixed reply-byte assembly. */
            platform_update_mcan_node_filter((uint16_t)(effective_value & 0x7FFU));
        }
        prepare_response(config, operation, address, effective_value, retained_node, result);
        result->response_ready = true;
        /* Every WRITE reaches the post-send scratch-address check. The
         * IRQ must decide there, not from this pre-send decoded address. */
        result->transport_reconfigure_requested = true;
        return;
    }

    if ((operation == PARAMETER_STORE) && (result->motor_owner[0x38U / 4U] == 0U))
    {
        /* This path posts the full-word flag before constructing
         * and transmitting its four-byte acknowledgement. */
        result->status_owner[0x0CU / 4U] = 1U;
        prepare_response(config, operation, 0x01U, 0U, retained_node, result);
        result->response.length = 4U;
        result->store_requested = true;
        /* Main performs the blocking Flash operation later, without a
         * duplicate acknowledgement. The flag is already posted above. */
        result->response_ready = true;
    }
}

void parameter_protocol_process(const CanFrame *request, MotorConfig *config,
                                MotorController *controller, const ParameterRuntime *runtime,
                                ParameterProtocolResult *result)
{
    parameter_protocol_process_impl(request, config, controller, runtime, result, NULL);
}

void parameter_protocol_process_irq(const CanFrame *request, ParameterProtocolResult *result,
                                    McanIrqContext *references)
{
    parameter_protocol_process_impl(request, (MotorConfig *)(uintptr_t)references->config,
                                    (MotorController *)(uintptr_t)references->motor, NULL, result,
                                    references);
}
