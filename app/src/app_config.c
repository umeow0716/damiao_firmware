#include "app_config.h"

#include <string.h>

#define OFFICIAL_SOFTWARE_VERSION_WORD   0x37313035UL
#define OFFICIAL_FIRMWARE_SUBVERSION     0x00000034UL
#define FACTORY_STORED_SOFTWARE_VERSION  0x00000000UL
#define FACTORY_STORED_SUBVERSION_WORD   0x30303031UL

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

void app_config_load_defaults(MotorConfig *config)
{
    *config = (MotorConfig) {
        .position_min = -12.5f,
        .position_max = 12.5f,
        .velocity_min = -30.0f,
        .velocity_max = 30.0f,
        .torque_min = -10.0f,
        .torque_max = 10.0f,
        .kp_min = 0.0f,
        .kp_max = 500.0f,
        .kd_min = 0.0f,
        .kd_max = 5.0f,
        .bus_undervoltage = 15.0f,
        .bus_overvoltage = 32.0f,
        .torque_constant = 0.0f,
        .acceleration_limit = 2.0f,
        .deceleration_limit = -2.0f,
        .speed_limit = 600.0f,
        .current_limit = 0.8f,
        .mos_temperature_limit = 120.0f,
        .motor_temperature_limit = 100.0f,
        .phase_resistance = 0x1.b33334p-1f,
        .phase_inductance = 0x1.69c23cp-12f,
        .flux_linkage = 0x1.26e978p-8f,
        .viscous_damping = 0.0f,
        .rotor_inertia = 0x1.2dfd6ap-16f,
        .gear_ratio = 10.0f,
        .current_loop_bandwidth = 1000.0f,
        .speed_kp = 0.00371999992f,
        .speed_ki = 0.00200000009f,
        .position_kp = 54.0f,
        .position_ki = 0.0f,
        .gear_torque_efficiency = 1.0f,
        .speed_loop_damping = 4.0f,
        .velocity_filter_bandwidth = 40.0f,
        .current_loop_enhancement = 2500.0f,
        .velocity_loop_enhancement = 100.0f,
        /* The recovered calibration record uses 1=inverted, 2=normal. */
        .direction = 2.0f,
        .maximum_phase_current = 2.0f,
        .position_sensor_scale = 0.1f,
        .communication_timeout = 0U,
        .hardware_version = 0x56303033UL,
        .software_version = OFFICIAL_SOFTWARE_VERSION_WORD,
        .serial_number = 0x54303035UL,
        .firmware_subversion = OFFICIAL_FIRMWARE_SUBVERSION,
        .bootloader_version = 0U,
        .can_id = 1U,
        .master_id = 0U,
        .pole_pairs = 14U,
        .can_data_rate_selector = 4U,
        .control_mode = MOTOR_MODE_MIT,
        .sensor_inverted = false,
    };
}

void app_config_encode_persistent(const MotorConfig *config,
                                  uint32_t words[APP_CONFIG_WORD_COUNT])
{
    words[0x00] = float_bits(config->bus_undervoltage);
    words[0x01] = float_bits(config->torque_constant);
    words[0x02] = float_bits(config->motor_temperature_limit);
    words[0x03] = float_bits(config->current_limit);
    words[0x04] = float_bits(config->acceleration_limit);
    words[0x05] = float_bits(config->deceleration_limit);
    words[0x06] = float_bits(config->speed_limit);
    words[0x07] = config->master_id;
    words[0x08] = config->can_id;
    words[0x09] = config->communication_timeout;
    words[0x0A] = config->control_mode;
    words[0x0B] = float_bits(config->viscous_damping);
    words[0x0C] = float_bits(config->rotor_inertia);
    words[0x0D] = config->hardware_version;
    words[0x0E] = config->software_version;
    words[0x0F] = config->serial_number;
    words[0x10] = config->pole_pairs;
    words[0x11] = float_bits(config->phase_resistance);
    words[0x12] = float_bits(config->phase_inductance);
    words[0x13] = float_bits(config->flux_linkage);
    words[0x14] = float_bits(config->gear_ratio);
    words[0x15] = float_bits(config->position_max);
    words[0x16] = float_bits(config->velocity_max);
    words[0x17] = float_bits(config->torque_max);
    words[0x18] = float_bits(config->current_loop_bandwidth);
    words[0x19] = float_bits(config->speed_kp);
    words[0x1A] = float_bits(config->speed_ki);
    words[0x1B] = float_bits(config->position_kp);
    words[0x1C] = float_bits(config->position_ki);
    words[0x1D] = float_bits(config->bus_overvoltage);
    words[0x1E] = float_bits(config->gear_torque_efficiency);
    words[0x1F] = float_bits(config->speed_loop_damping);
    words[0x20] = float_bits(config->velocity_filter_bandwidth);
    words[0x21] = float_bits(config->current_loop_enhancement);
    words[0x22] = float_bits(config->velocity_loop_enhancement);
    words[0x23] = config->can_data_rate_selector;
    words[0x24] = config->firmware_subversion;
}

void app_config_encode_factory_persistent(
    uint32_t words[APP_CONFIG_WORD_COUNT])
{
    MotorConfig defaults;
    app_config_load_defaults(&defaults);
    app_config_encode_persistent(&defaults, words);
    /* load_motor_configuration@0x228c0 writes the scatter-loaded template to
     * erased Flash before replacing these two fields in the live RAM copy. */
    words[0x0E] = FACTORY_STORED_SOFTWARE_VERSION;
    words[0x24] = FACTORY_STORED_SUBVERSION_WORD;
}

static bool official_float_is_nan(uint32_t bits)
{
    /* Match the original integer test exactly: infinity is not NaN. */
    return (bits & 0x7FFFFFFFUL) > 0x7F800000UL;
}

static void decode_words(MotorConfig *config,
                         const uint32_t words[APP_CONFIG_WORD_COUNT])
{
    config->bus_undervoltage = bits_float(words[0x00]);
    config->torque_constant = bits_float(words[0x01]);
    config->motor_temperature_limit = bits_float(words[0x02]);
    config->current_limit = bits_float(words[0x03]);
    config->acceleration_limit = bits_float(words[0x04]);
    config->deceleration_limit = bits_float(words[0x05]);
    config->speed_limit = bits_float(words[0x06]);
    config->master_id = (uint16_t)words[0x07];
    config->can_id = (uint16_t)words[0x08];
    config->communication_timeout = words[0x09];
    config->control_mode = (MotorControlMode)words[0x0A];
    config->viscous_damping = bits_float(words[0x0B]);
    config->rotor_inertia = bits_float(words[0x0C]);
    config->hardware_version = words[0x0D];
    config->software_version = words[0x0E];
    config->serial_number = words[0x0F];
    config->pole_pairs = (uint8_t)words[0x10];
    config->phase_resistance = bits_float(words[0x11]);
    config->phase_inductance = bits_float(words[0x12]);
    config->flux_linkage = bits_float(words[0x13]);
    config->gear_ratio = bits_float(words[0x14]);
    config->position_sensor_scale = 1.0f / config->gear_ratio;
    config->position_min = -bits_float(words[0x15]);
    config->position_max = bits_float(words[0x15]);
    config->velocity_min = -bits_float(words[0x16]);
    config->velocity_max = bits_float(words[0x16]);
    config->torque_min = -bits_float(words[0x17]);
    config->torque_max = bits_float(words[0x17]);
    config->current_loop_bandwidth = bits_float(words[0x18]);
    config->speed_kp = bits_float(words[0x19]);
    config->speed_ki = bits_float(words[0x1A]);
    config->position_kp = bits_float(words[0x1B]);
    config->position_ki = bits_float(words[0x1C]);
    config->bus_overvoltage = bits_float(words[0x1D]);
    config->gear_torque_efficiency = bits_float(words[0x1E]);
    config->speed_loop_damping = bits_float(words[0x1F]);
    config->velocity_filter_bandwidth = bits_float(words[0x20]);
    config->current_loop_enhancement = bits_float(words[0x21]);
    config->velocity_loop_enhancement = bits_float(words[0x22]);
    config->can_data_rate_selector = (uint8_t)words[0x23];
    config->firmware_subversion = words[0x24];
}

void app_config_decode_runtime(MotorConfig *config,
                               const uint32_t words[APP_CONFIG_WORD_COUNT])
{
    decode_words(config, words);
}

bool app_config_decode_persistent(MotorConfig *config,
                                  const uint32_t words[APP_CONFIG_WORD_COUNT])
{
    /* load_motor_configuration@0x228c0 treats the record as absent only when
     * one of these three identity fields is erased.  It does not apply the
     * broad source-only range policy which used to live here. */
    if ((words[0x0F] == UINT32_MAX) ||
        (words[0x0E] == UINT32_MAX) ||
        (words[0x08] == UINT32_MAX)) {
        return false;
    }

    decode_words(config, words);
    uint32_t velocity_bw_bits = words[0x20];
    uint32_t current_enhancement_bits = words[0x21];
    uint32_t velocity_enhancement_bits = words[0x22];
    if (official_float_is_nan(velocity_bw_bits)) {
        velocity_bw_bits = float_bits(50.0f);
    }
    if ((int32_t)velocity_bw_bits >= (int32_t)0x43FA0000L) {
        velocity_bw_bits = float_bits(500.0f);
    }
    if (official_float_is_nan(current_enhancement_bits)) {
        current_enhancement_bits = float_bits(2500.0f);
    }
    if (official_float_is_nan(velocity_enhancement_bits)) {
        velocity_enhancement_bits = float_bits(200.0f);
    }
    const float velocity_bw = bits_float(velocity_bw_bits);
    const float current_enhancement = bits_float(current_enhancement_bits);
    const float velocity_enhancement = bits_float(velocity_enhancement_bits);

    config->control_mode = (words[0x0A] >= MOTOR_MODE_MIT &&
                            words[0x0A] <= MOTOR_MODE_HYBRID) ?
                           (MotorControlMode)words[0x0A] : MOTOR_MODE_MIT;
    config->software_version = OFFICIAL_SOFTWARE_VERSION_WORD;
    config->velocity_filter_bandwidth = velocity_bw;
    config->current_loop_enhancement = current_enhancement;
    config->velocity_loop_enhancement = velocity_enhancement;
    config->can_data_rate_selector = words[0x23] <= 11U ?
                                     (uint8_t)words[0x23] : 4U;
    config->firmware_subversion = OFFICIAL_FIRMWARE_SUBVERSION;
    return true;
}
