#include "parameter_protocol.h"

#include <stddef.h>
#include <string.h>

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

static void prepare_legacy_response(uint16_t id, const uint8_t *data,
                                    uint8_t length,
                                    ParameterProtocolResult *result)
{
    result->response.id = id;
    result->response.length = length;
    memcpy(result->response.data, data, length);
    result->response_ready = true;
}

static bool process_legacy_command(const CanFrame *request,
                                   MotorConfig *config,
                                   const MotorController *controller,
                                   ParameterProtocolResult *result)
{
    if ((request->id != PARAMETER_CAN_ID) || controller->armed) {
        return false;
    }

    const uint8_t *const data = request->data;
    if ((data[0] == LEGACY_DISCOVERY_HEAD) && (data[1] == 0U) &&
        (data[2] == 0U) && (data[3] == LEGACY_DISCOVERY_TAIL)) {
        const uint8_t response[4] = {
            LEGACY_DISCOVERY_HEAD,
            (uint8_t)(config->can_id & 0x7FU),
            (uint8_t)(config->master_id & 0x7FU),
            LEGACY_DISCOVERY_TAIL,
        };
        prepare_legacy_response(PARAMETER_CAN_ID, response,
                                sizeof(response), result);
        return true;
    }

    if ((data[0] == LEGACY_ID_HEAD) &&
        (data[3] == LEGACY_ID_TAIL)) {
        config->can_id = data[1];
        config->master_id = data[2];
        const uint8_t response[4] = {
            LEGACY_ID_HEAD,
            (uint8_t)(config->can_id & 0x7FU),
            (uint8_t)(config->master_id & 0x7FU),
            LEGACY_ID_TAIL,
        };
        prepare_legacy_response(PARAMETER_CAN_ID, response,
                                sizeof(response), result);
        result->persist_requested = true;
        result->filter_update_requested = true;
        return true;
    }

    if ((data[0] == 0x55U) && (data[1] == 0x01U) &&
        (data[2] == 0x02U) && (data[3] == 0xAAU) &&
        (read_u16_le(&data[4]) == config->can_id)) {
        static const uint8_t response[8] = {
            'A', 'u', 'p', 'g', 'r', 'a', 'd', 'e',
        };
        prepare_legacy_response(0x7FEU, response, sizeof(response), result);
        result->bootloader_requested = true;
        return true;
    }

    if ((data[0] == LEGACY_VERSION_HEAD) &&
        (data[3] == LEGACY_VERSION_TAIL)) {
        const uint16_t version = (uint16_t)config->software_version;
        const uint8_t response[4] = {
            LEGACY_VERSION_TAIL,
            (uint8_t)version,
            (uint8_t)(version >> 8U),
            LEGACY_VERSION_HEAD,
        };
        prepare_legacy_response(PARAMETER_CAN_ID, response,
                                sizeof(response), result);
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

static float bits_float(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

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

static uint8_t live_temperature_byte(float value)
{
    return (uint8_t)(int32_t)value;
}

static void prepare_live_response(uint8_t selector,
                                  const MotorConfig *config,
                                  const MotorController *controller,
                                  ParameterProtocolResult *result)
{
    const float direction = config->direction == 1.0f ? 1.0f : -1.0f;
    result->response.id = config->master_id;
    result->response.length = 8U;
    memset(result->response.data, 0, sizeof(result->response.data));

    if (selector == 0U) {
        can_protocol_encode_feedback(&controller->feedback, config,
                                     &result->response);
        result->response_ready = true;
        return;
    }
    if ((selector >= 1U) && (selector <= 3U)) {
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
        result->response_ready = true;
        return;
    }
    if (selector == 4U) {
        const float position = controller->feedback.position * direction;
        memcpy(&result->response.data[0], &position, sizeof(position));
        const uint16_t velocity = (uint16_t)motor_float_to_uint(
            controller->feedback.velocity * direction,
            config->velocity_min, config->velocity_max, 12U);
        result->response.data[4] = (uint8_t)velocity;
        result->response.data[5] = (uint8_t)(velocity >> 8U);
        const int16_t current = (int16_t)(controller->feedback.current_q *
                                          direction * 10000.0f);
        result->response.data[6] = (uint8_t)current;
        result->response.data[7] = (uint8_t)((uint16_t)current >> 8U);
        result->response_ready = true;
    }
}

static bool write_register(uint8_t address, uint32_t raw, MotorConfig *config,
                           MotorController *controller)
{
    const float value = bits_float(raw);
    switch (address) {
    case 0x00:
        if ((int32_t)raw <= (int32_t)0x41200000U) return false;
        config->bus_undervoltage = value;
        return true;
    case 0x01:
        if (!(value >= 0.0f)) return false;
        config->torque_constant = value;
        return true;
    case 0x02:
        if ((raw - 0x42A00000U) >= 0x00A80000U) return false;
        config->motor_temperature_limit = value;
        return true;
    case 0x03:
        if (((int32_t)raw > (int32_t)0x3F7FFFFFU) ||
            (value <= 0.0f)) return false;
        config->current_limit = value;
        return true;
    case 0x04:
        if (!(value > 0.0f)) return false;
        config->acceleration_limit = value;
        return true;
    case 0x05:
        if (!(value < 0.0f)) return false;
        config->deceleration_limit = value;
        return true;
    case 0x06:
        if (!(value > 0.0f)) return false;
        config->speed_limit = value;
        return true;
    case 0x07:
        /* The reference stores the incoming word AND 0x7ff rather than
         * rejecting an oversized standard ID. */
        config->master_id = (uint16_t)(raw & 0x7FFU);
        return true;
    case 0x08:
        config->can_id = (uint16_t)(raw & 0x7FFU);
        return true;
    case 0x09:
        config->communication_timeout = raw;
        return true;
    case 0x0A:
        if (raw < MOTOR_MODE_MIT || raw > MOTOR_MODE_HYBRID) return false;
        if (config->control_mode != (MotorControlMode)raw) {
            memset(&controller->command, 0, sizeof(controller->command));
            controller->command.mode = (MotorControlMode)raw;
            config->control_mode = (MotorControlMode)raw;
        }
        return true;
    case 0x15:
        if (!(value > 0.0f)) return false;
        config->position_min = -value;
        config->position_max = value;
        return true;
    case 0x16:
        if (!(value > 0.0f)) return false;
        config->velocity_min = -value;
        config->velocity_max = value;
        return true;
    case 0x17:
        if (!(value > 0.0f)) return false;
        config->torque_min = -value;
        config->torque_max = value;
        return true;
    case 0x18:
        if ((raw - 0x42C80000U) > 0x03544000U) return false;
        config->current_loop_bandwidth = value;
        return true;
    case 0x19:
        if (value < 0.0f) return false;
        config->speed_kp = value;
        return true;
    case 0x1A:
        if (value < 0.0f) return false;
        config->speed_ki = value;
        return true;
    case 0x1B:
        if (!(value >= 0.0f)) return false;
        config->position_kp = value;
        return true;
    case 0x1C:
        if (!(value >= 0.0f)) return false;
        config->position_ki = value;
        return true;
    case 0x1D:
        if ((value <= config->bus_undervoltage) ||
            ((int32_t)raw > (int32_t)0x42000000U)) return false;
        config->bus_overvoltage = value;
        return true;
    case 0x1E:
        if (!(value > 0.0f) ||
            ((int32_t)raw >= (int32_t)0x3F800001U)) return false;
        config->gear_torque_efficiency = value;
        return true;
    case 0x1F:
        if ((raw - 0x3F800000U) >= 0x02700001U) return false;
        config->speed_loop_damping = value;
        return true;
    case 0x20:
        if ((int32_t)raw <= 0) return false;
        config->velocity_filter_bandwidth = value;
        return true;
    case 0x21:
        if ((raw - 0x42C80000U) > 0x03544000U) return false;
        config->current_loop_enhancement = value;
        return true;
    case 0x22:
        if (!(value > 0.0f) || (raw > 0x461C4000U)) return false;
        config->velocity_loop_enhancement = value;
        return true;
    case 0x23:
        if (raw > 11U) return false;
        config->can_data_rate_selector = (uint8_t)raw;
        return true;
    default:
        return false;
    }
}

static void prepare_response(const MotorConfig *config, uint8_t operation,
                             uint8_t address, uint32_t value,
                             ParameterProtocolResult *result)
{
    result->response.id = config->master_id;
    result->response.length = 8U;
    result->response.data[0] = (uint8_t)config->can_id;
    result->response.data[1] = (uint8_t)(config->can_id >> 8U);
    result->response.data[2] = operation;
    result->response.data[3] = address;
    write_u32_le(&result->response.data[4], value);
}

void parameter_protocol_process(const CanFrame *request,
                                MotorConfig *config,
                                MotorController *controller,
                                const ParameterRuntime *runtime,
                                ParameterProtocolResult *result)
{
    memset(result, 0, sizeof(*result));
    if ((request == NULL) || (config == NULL) || (controller == NULL) ||
        (runtime == NULL)) {
        return;
    }
    if (process_legacy_command(request, config, controller, result)) {
        result->handled = true;
        return;
    }
    if ((request->id != PARAMETER_CAN_ID) ||
        (read_u16_le(request->data) != config->can_id)) {
        return;
    }

    const uint8_t operation = request->data[2];
    const uint8_t address = request->data[3];
    result->handled = true;

    if (operation == PARAMETER_READ) {
        uint32_t value;
        if (read_register(address, config, runtime, &value)) {
            prepare_response(config, operation, address, value, result);
            result->response_ready = true;
        }
        return;
    }

    if (operation == PARAMETER_LIVE) {
        prepare_live_response(address, config, controller, result);
        return;
    }

    if (operation == PARAMETER_WRITE) {
        const uint16_t old_node_id = config->can_id;
        const uint8_t old_rate = config->can_data_rate_selector;
        const bool accepted = write_register(address, read_u32_le(&request->data[4]),
                                             config, controller);
        if (accepted && ((address == 0x01U) || (address == 0x18U) ||
                         (address == 0x1EU) || (address == 0x21U) ||
                         (address == 0x22U))) {
            motor_control_configure(controller, config,
                                    controller->feedback.bus_voltage);
        }
        uint32_t effective_value;
        if (read_register(address, config, runtime, &effective_value)) {
            prepare_response(config, operation, address, effective_value, result);
            result->response_ready = true;
        }
        result->filter_update_requested = accepted &&
            (config->can_id != old_node_id);
        result->transport_reconfigure_requested = accepted &&
            (config->can_data_rate_selector != old_rate);
        return;
    }

    if ((operation == PARAMETER_STORE) && !controller->armed) {
        prepare_response(config, operation, 0x01U, 0U, result);
        result->response.length = 4U;
        result->store_requested = true;
        /* Main context sends this response after the blocking Flash routine. */
    }
}
