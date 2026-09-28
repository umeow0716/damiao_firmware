#include "commissioning.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "app_profile.h"
#include "app_state.h"
#include "debug_console.h"
#include "motor_math.h"
#include "platform.h"
#include "sensor_calibration.h"

#define TWO_PI_F                 0x1.921fb6p+2f
#define PI_OVER_TWO_F            0x1.921fb6p+0f
#define ALIGNMENT_VOLTAGE_D      0.1f
#define DIRECTION_ANGLE_STEP     0.002f
#define DIRECTION_LOCK_STEPS     10000U
#define ALIGNMENT_LOCK_STEPS     20000U
#define ALIGNMENT_INNER_STEPS    40U
#define ALIGNMENT_POINTS_PER_PAIR 256U
#define OUTPUT_CALIBRATION_VOLTAGE_Q 0.2f
#define OUTPUT_CALIBRATION_REPORT_DIVIDER 20U
#define OUTPUT_TABLE_POINTS 256U
#define OUTPUT_TABLE_POINTS_PER_MOTOR_TURN 4096U
#define INV_SQRT3_F             0.5773502588272095f
#define CURRENT_FULL_SCALE_A    APP_PROFILE_CURRENT_FULL_SCALE_A
#define IDENTIFICATION_LOCK_STEPS 6284U
#define IDENTIFICATION_ELECTRICAL_STEPS 60000U
#define IDENTIFICATION_RLS_START_STEP 20000U
#define IDENTIFICATION_TARGET_RAMP_STEPS 10000U
#define IDENTIFICATION_TARGET_RAMP_DIVIDER 100U
#define IDENTIFICATION_TARGET_UPDATE_DIVIDER 10U
/* The recovered routine multiplies a 100 Hz target by the phase base step,
 * producing the electrical injection at the 20 kHz sample cadence. */
#define IDENTIFICATION_TARGET_FREQUENCY_HZ 100.0f
#define IDENTIFICATION_PHASE_BASE_STEP 0.00031415926059708f
#define IDENTIFICATION_TARGET_CURRENT_A 5.0f
#define IDENTIFICATION_CURRENT_GAIN 2.0f
#define IDENTIFICATION_VOLTAGE_LIMIT 0.30000001192092896f
#define IDENTIFICATION_FLUX_STEPS 40000U
#define IDENTIFICATION_MECHANICAL_DRIVE_STEPS 80000U
#define IDENTIFICATION_MECHANICAL_COAST_STEPS 20000U
#define IDENTIFICATION_MECHANICAL_CURRENT_A 1.0f
#define IDENTIFICATION_MECHANICAL_FREQUENCY_HZ 2.0f
#define IDENTIFICATION_OBSERVER_WINDOW 20U
#define IDENTIFICATION_OBSERVER_RAMP_STEP 0.0010000000474974513f
#define IDENTIFICATION_OBSERVER_Q_VOLTAGE_LIMIT 0.20000000298023224f
#define IDENTIFICATION_OBSERVER_D_GAIN 10.0f
#define IDENTIFICATION_FILTER_D_LIMIT 0.4000000059604645f
#define IDENTIFICATION_FILTER_Q_LIMIT 0.8999999761581421f
#define IDENTIFICATION_LPF_OLD 0.7991513609886169f
#define IDENTIFICATION_LPF_NEW 0.20084863901138306f

static float wrap_signed(float angle)
{
    while (angle > 0x1.921fb6p+1f) {
        angle -= TWO_PI_F;
    }
    while (angle < -0x1.921fb6p+1f) {
        angle += TWO_PI_F;
    }
    return angle;
}

static float alignment_unwrap(float current, float previous_unwrapped)
{
    float adjusted = current;
    if ((adjusted - previous_unwrapped) > 4.0f) {
        adjusted -= TWO_PI_F;
    }
    if ((adjusted - previous_unwrapped) < -4.0f) {
        adjusted += TWO_PI_F;
    }
    return adjusted;
}

static uint32_t crc32_mpeg2(const uint8_t *data, size_t length)
{
    uint32_t crc = UINT32_MAX;
    for (size_t index = 0U; index < length; ++index) {
        crc ^= (uint32_t)data[index] << 24U;
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x80000000UL) != 0U ?
                (crc << 1U) ^ 0x04C11DB7UL : crc << 1U;
        }
    }
    return crc;
}

static void put_float(uint8_t *destination, float value)
{
    memcpy(destination, &value, sizeof(value));
}

static void send_alignment_sample(float commanded_mechanical_angle,
                                  float measured_angle, float error,
                                  uint16_t raw, uint16_t sample_index)
{
    uint8_t frame[21] = {0x89U};
    put_float(&frame[1], commanded_mechanical_angle);
    put_float(&frame[5], measured_angle);
    put_float(&frame[9], error);
    frame[13] = (uint8_t)raw;
    frame[14] = (uint8_t)(raw >> 8U);
    frame[15] = (uint8_t)sample_index;
    frame[16] = (uint8_t)(sample_index >> 8U);
    const uint32_t crc = crc32_mpeg2(frame, 17U);
    memcpy(&frame[17], &crc, sizeof(crc));
    platform_debug_write(frame, sizeof(frame));
}

static void send_sensor_sample(uint8_t marker, float first, float second,
                               float third, uint16_t raw_u,
                               uint16_t raw_v)
{
    uint8_t frame[21] = {0};
    frame[0] = marker;
    put_float(&frame[1], first);
    put_float(&frame[5], second);
    put_float(&frame[9], third);
    frame[13] = (uint8_t)raw_u;
    frame[14] = (uint8_t)(raw_u >> 8U);
    frame[15] = (uint8_t)raw_v;
    frame[16] = (uint8_t)(raw_v >> 8U);
    const uint32_t crc = crc32_mpeg2(frame, 17U);
    memcpy(&frame[17], &crc, sizeof(crc));
    platform_debug_write(frame, sizeof(frame));
}


static void send_output_sensor_raw_sample(uint16_t raw_u, uint16_t raw_v)
{
    /* Factory DM8009 output calibration sends marker 'h' as a 13-byte raw
     * sensor snapshot: 'h', two zero float slots, then raw U/raw V.  It does
     * not use the generic 21-byte float sample frame here. */
    uint8_t frame[13] = {'h'};
    frame[9] = (uint8_t)raw_u;
    frame[10] = (uint8_t)(raw_u >> 8U);
    frame[11] = (uint8_t)raw_v;
    frame[12] = (uint8_t)(raw_v >> 8U);
    platform_debug_write(frame, sizeof(frame));
}

static void send_output_sensor_result_table(const float table[OUTPUT_TABLE_POINTS])
{
    const uint8_t marker = 'H';
    platform_debug_write(&marker, 1U);
    platform_debug_write((const uint8_t *)table,
                         OUTPUT_TABLE_POINTS * sizeof(float));
    const uint32_t crc = crc32_mpeg2((const uint8_t *)table,
                                     OUTPUT_TABLE_POINTS * sizeof(float));
    platform_debug_write((const uint8_t *)&crc, sizeof(crc));
}

static void refresh_commissioning_position(void)
{
    float rotor_position;
    float rotor_angle;
    float motor_output_position;
    uint16_t raw_position;
    if (platform_read_position(&rotor_position, &rotor_angle,
                               &motor_output_position, &raw_position)) {
        g_app.rotor_position = rotor_position;
        g_app.rotor_angle = rotor_angle;
        g_app.motor_output_position = motor_output_position;
        g_app.raw_position = raw_position;
    }
}

static DirectQuadrature commissioning_current_dq(
    const PlatformCommissioningSample *sample, float electrical_angle)
{
    float sine;
    float cosine;
    motor_fast_sincos(motor_wrapf(electrical_angle,
                                  -0x1.921fb6p+1f,
                                  0x1.921fb6p+1f),
                      &sine, &cosine);
    return motor_park(motor_clarke(sample->current_u, sample->current_v),
                      sine, cosine);
}

static bool identify_electrical_parameters(float electrical_angle,
                                           float *resistance,
                                           float *inductance,
                                           PlatformCommissioningSample *sample)
{
    CommissioningRls2 estimator;
    commissioning_rls2_init(&estimator);
    float target_amplitude = 0.0f;
    float target_current = 0.0f;
    float applied_voltage = 0.0f;
    const float amplitude_step = IDENTIFICATION_TARGET_CURRENT_A /
        (CURRENT_FULL_SCALE_A *
         (float)IDENTIFICATION_TARGET_RAMP_DIVIDER);

    for (uint32_t count = 1U;
         count <= IDENTIFICATION_ELECTRICAL_STEPS; ++count) {
        if (!platform_commissioning_read_sample(sample)) {
            return false;
        }
        if ((count <= IDENTIFICATION_TARGET_RAMP_STEPS) &&
            ((count % IDENTIFICATION_TARGET_RAMP_DIVIDER) == 0U)) {
            target_amplitude += amplitude_step;
        }
        if ((count % IDENTIFICATION_TARGET_UPDATE_DIVIDER) == 0U) {
            const float unwrapped_phase =
                (IDENTIFICATION_TARGET_FREQUENCY_HZ * (float)count) *
                IDENTIFICATION_PHASE_BASE_STEP;
            const float phase = motor_wrapf(
                unwrapped_phase,
                -0x1.921fb6p+1f, 0x1.921fb6p+1f);
            float sine;
            float cosine;
            motor_fast_sincos(phase, &sine, &cosine);
            (void)cosine;
            target_current = target_amplitude * sine;
        }

        const DirectQuadrature current =
            commissioning_current_dq(sample, electrical_angle);
        if (count > IDENTIFICATION_RLS_START_STEP) {
            commissioning_rls2_step(
                &estimator, current.d * CURRENT_FULL_SCALE_A,
                sample->bus_voltage * INV_SQRT3_F * applied_voltage);
        }
        applied_voltage = motor_clampf(
            IDENTIFICATION_CURRENT_GAIN * (target_current - current.d),
            -IDENTIFICATION_VOLTAGE_LIMIT,
            IDENTIFICATION_VOLTAGE_LIMIT);
        platform_commissioning_drive(applied_voltage, 0.0f,
                                     electrical_angle);
        platform_commissioning_finish_sample();
    }
    platform_commissioning_delay_us(5000U);
    platform_commissioning_drive(0.0f, 0.0f, electrical_angle);
    return commissioning_rls2_motor_parameters(
        &estimator, 0.00005f, resistance, inductance);
}

static float identification_electrical_angle(const MotorConfig *config)
{
    return g_app.rotor_angle * (float)config->pole_pairs +
           g_app.motor.electrical_offset;
}

static bool identification_filter_pair_init(
    const MotorConfig *config, float bus_voltage,
    CommissioningIdentificationFilter *axis_d,
    CommissioningIdentificationFilter *axis_q)
{
    if ((config == NULL) || (axis_d == NULL) || (axis_q == NULL)) {
        return false;
    }
    float proportional_gain = 0.0f;
    float integral_gain = 0.0f;
    /* derive_control_parameters@0x25138 zeros both commissioning filters
     * when the sampled bus voltage is not above the stored UV threshold. */
    if (bus_voltage > config->bus_undervoltage) {
        proportional_gain =
            config->current_loop_bandwidth * config->phase_inductance *
            CURRENT_FULL_SCALE_A * (1.0f / INV_SQRT3_F) / bus_voltage;
        integral_gain =
            (config->phase_resistance / config->phase_inductance) * 0.00005f;
    }
    *axis_d = (CommissioningIdentificationFilter) {
        .integral_gain = integral_gain,
        .input_gain = proportional_gain,
        .output_min = -IDENTIFICATION_FILTER_D_LIMIT,
        .output_max = IDENTIFICATION_FILTER_D_LIMIT,
    };
    *axis_q = (CommissioningIdentificationFilter) {
        .integral_gain = integral_gain,
        .input_gain = proportional_gain,
        .output_min = -IDENTIFICATION_FILTER_Q_LIMIT,
        .output_max = IDENTIFICATION_FILTER_Q_LIMIT,
    };
    return true;
}

static void identification_current_filter_step(
    CommissioningIdentificationFilter *axis_d,
    CommissioningIdentificationFilter *axis_q,
    DirectQuadrature measured_current, float target_d, float target_q,
    float electrical_angle, float *voltage_d, float *voltage_q)
{
    axis_d->input = target_d - measured_current.d;
    axis_q->input = target_q - measured_current.q;
    commissioning_identification_filter_step(axis_d);
    commissioning_identification_filter_step(axis_q);
    *voltage_d = axis_d->limited_output;
    *voltage_q = axis_q->limited_output;
    platform_commissioning_drive(*voltage_d, *voltage_q, electrical_angle);
}

static bool identify_flux_linkage(
    const MotorConfig *config, PlatformCommissioningSample *sample,
    float *flux_linkage)
{
    CommissioningFluxObserver observer;
    if ((config == NULL) || (sample == NULL) || (flux_linkage == NULL) ||
        !commissioning_flux_observer_init(
            &observer, config->phase_inductance,
            config->phase_resistance, 0.00005f)) {
        return false;
    }
    float previous_rotor_position = g_app.rotor_position;
    float electrical_speed = 0.0f;
    float voltage_d = 0.0f;
    float voltage_q = 0.0f;
    for (uint32_t count = 1U; count <= IDENTIFICATION_FLUX_STEPS; ++count) {
        if (!platform_commissioning_read_sample(sample)) {
            return false;
        }
        if ((count % IDENTIFICATION_OBSERVER_WINDOW) == 0U) {
            const float rotor_position = g_app.rotor_position;
            const float mechanical_speed = wrap_signed(
                rotor_position - previous_rotor_position) / 0.001f;
            const float raw_electrical_speed =
                mechanical_speed * (float)config->pole_pairs;
            electrical_speed = IDENTIFICATION_LPF_OLD * electrical_speed +
                IDENTIFICATION_LPF_NEW * raw_electrical_speed;
            previous_rotor_position = rotor_position;
            voltage_q += IDENTIFICATION_OBSERVER_RAMP_STEP;
            if (voltage_q > IDENTIFICATION_OBSERVER_Q_VOLTAGE_LIMIT) {
                voltage_q = IDENTIFICATION_OBSERVER_Q_VOLTAGE_LIMIT;
            }
        }

        const float electrical_angle =
            identification_electrical_angle(config);
        const DirectQuadrature current =
            commissioning_current_dq(sample, electrical_angle);
        observer.measured_current_d = current.d * CURRENT_FULL_SCALE_A;
        observer.measured_current_q = current.q * CURRENT_FULL_SCALE_A;
        observer.voltage_d =
            voltage_d * sample->bus_voltage * INV_SQRT3_F;
        observer.voltage_q =
            voltage_q * sample->bus_voltage * INV_SQRT3_F;
        observer.electrical_speed = electrical_speed;
        commissioning_flux_observer_step(&observer);

        voltage_d = motor_clampf(
            voltage_d - IDENTIFICATION_OBSERVER_D_GAIN * 0.00005f *
                        current.d,
            -IDENTIFICATION_VOLTAGE_LIMIT,
            IDENTIFICATION_VOLTAGE_LIMIT);
        platform_commissioning_drive(voltage_d, voltage_q,
                                     electrical_angle);
        platform_commissioning_finish_sample();
    }
    platform_commissioning_delay_us(1000U);
    platform_commissioning_drive(0.0f, 0.0f,
                                 identification_electrical_angle(config));
    *flux_linkage = observer.flux_linkage;
    return true;
}

static bool identify_mechanical_parameters(
    const MotorConfig *config, float flux_linkage,
    PlatformCommissioningSample *sample,
    float *rotor_inertia, float *viscous_damping)
{
    if ((config == NULL) || (sample == NULL) ||
        (rotor_inertia == NULL) || (viscous_damping == NULL) ||
        !platform_commissioning_read_sample(sample)) {
        return false;
    }
    CommissioningIdentificationFilter axis_d;
    CommissioningIdentificationFilter axis_q;
    if (!identification_filter_pair_init(
            config, sample->bus_voltage, &axis_d, &axis_q)) {
        return false;
    }
    platform_commissioning_finish_sample();

    const float torque_per_amp = 1.5f * (float)config->pole_pairs *
                                 flux_linkage;
    const float excitation_angular_frequency =
        IDENTIFICATION_MECHANICAL_FREQUENCY_HZ * TWO_PI_F;
    const float target_amplitude =
        IDENTIFICATION_MECHANICAL_CURRENT_A / CURRENT_FULL_SCALE_A;
    uint32_t phase_count = 0U;
    float voltage_d = 0.0f;
    float voltage_q = 0.0f;
    float previous_rotor_position = g_app.rotor_position;
    float filtered_current = 0.0f;
    float filtered_speed = 0.0f;
    uint32_t window_count = 0U;
    bool have_result = false;
    CommissioningSineRegression regression;
    commissioning_sine_regression_init(&regression);

    for (uint32_t count = 0U;
         count < IDENTIFICATION_MECHANICAL_DRIVE_STEPS; ++count) {
        if (!platform_commissioning_read_sample(sample)) {
            return false;
        }
        ++phase_count;
        const float unwrapped_phase =
            (IDENTIFICATION_MECHANICAL_FREQUENCY_HZ *
             (float)phase_count) * IDENTIFICATION_PHASE_BASE_STEP;
        const float phase = motor_wrapf(
            unwrapped_phase, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
        if (unwrapped_phase > TWO_PI_F) {
            float cycle_inertia;
            float cycle_damping;
            if (commissioning_sine_regression_motor_parameters(
                    &regression, torque_per_amp,
                    excitation_angular_frequency,
                    &cycle_inertia, &cycle_damping)) {
                *rotor_inertia = cycle_inertia;
                *viscous_damping = cycle_damping;
                have_result = true;
            }
            commissioning_sine_regression_init(&regression);
            phase_count = 0U;
        }

        const float electrical_angle =
            identification_electrical_angle(config);
        const DirectQuadrature current =
            commissioning_current_dq(sample, electrical_angle);
        identification_current_filter_step(
            &axis_d, &axis_q, current, 0.0f,
            target_amplitude * motor_fast_sin(phase), electrical_angle,
            &voltage_d, &voltage_q);

        ++window_count;
        if (window_count == IDENTIFICATION_OBSERVER_WINDOW) {
            const float rotor_position = g_app.rotor_position;
            const float mechanical_speed = wrap_signed(
                rotor_position - previous_rotor_position) / 0.001f;
            previous_rotor_position = rotor_position;
            filtered_current = IDENTIFICATION_LPF_OLD * filtered_current +
                IDENTIFICATION_LPF_NEW * current.q * CURRENT_FULL_SCALE_A;
            filtered_speed = IDENTIFICATION_LPF_OLD * filtered_speed +
                IDENTIFICATION_LPF_NEW * mechanical_speed;
            commissioning_sine_regression_step(
                &regression, filtered_current, filtered_speed);
            window_count = 0U;
        }
        platform_commissioning_finish_sample();
    }

    for (uint32_t count = 0U;
         count < IDENTIFICATION_MECHANICAL_COAST_STEPS; ++count) {
        if (!platform_commissioning_read_sample(sample)) {
            return false;
        }
        const float electrical_angle =
            identification_electrical_angle(config);
        const DirectQuadrature current =
            commissioning_current_dq(sample, electrical_angle);
        identification_current_filter_step(
            &axis_d, &axis_q, current, 0.0f, 0.0f,
            electrical_angle, &voltage_d, &voltage_q);
        platform_commissioning_finish_sample();
    }
    platform_commissioning_delay_us(1000U);
    platform_commissioning_drive(0.0f, 0.0f,
                                 identification_electrical_angle(config));
    return have_result;
}

static void send_identification_result(const MotorConfig *config)
{
    uint8_t frame[21] = {'e'};
    put_float(&frame[1], config->phase_resistance);
    put_float(&frame[5], config->phase_inductance);
    put_float(&frame[9], config->flux_linkage);
    put_float(&frame[13], config->viscous_damping);
    put_float(&frame[17], config->rotor_inertia);
    platform_debug_write(frame, sizeof(frame));
}

static CommissioningStatus run_alignment_scan(uint8_t pole_pairs)
{
    /* measure_encoder_alignment@0x24b20 evaluates
     * (pole_pairs * 2*pi) / (pole_pairs * 0x2800), so the electrical step
     * is 2*pi/10240.  The outer pole_pairs*256 and inner 40 loops then span
     * exactly pole_pairs electrical turns, or one mechanical turn. */
    const double step = (double)(TWO_PI_F / 10240.0f);
    double electrical_angle = 0.0;
    platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f, 0.0f);
    for (uint32_t count = 0U; count < ALIGNMENT_LOCK_STEPS; ++count) {
        platform_commissioning_delay_us(100U);
    }

    const uint32_t points =
        (uint32_t)pole_pairs * ALIGNMENT_POINTS_PER_PAIR;
    float unwrapped = g_app.rotor_angle;
    for (uint32_t point = 0U; point < points; ++point) {
        for (uint32_t inner = 0U; inner < ALIGNMENT_INNER_STEPS; ++inner) {
            electrical_angle += step;
            platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f,
                                         (float)electrical_angle);
            platform_commissioning_delay_us(100U);
        }
        const float measured = g_app.rotor_angle;
        unwrapped = alignment_unwrap(measured, unwrapped);
        const double commanded_value = electrical_angle /
                                       (double)pole_pairs;
        const float commanded = (float)(electrical_angle /
                                        (double)pole_pairs);
        send_alignment_sample(commanded, measured,
                              (float)(commanded_value -
                                      (double)unwrapped),
                              g_app.raw_position, (uint16_t)point);
    }

    for (uint32_t point = 0U; point < points; ++point) {
        for (uint32_t inner = 0U; inner < ALIGNMENT_INNER_STEPS; ++inner) {
            electrical_angle -= step;
            platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f,
                                         (float)electrical_angle);
            platform_commissioning_delay_us(100U);
        }
        const float measured = g_app.rotor_angle;
        unwrapped = alignment_unwrap(measured, unwrapped);
        const double commanded_value = electrical_angle /
                                       (double)pole_pairs;
        const float commanded = (float)(electrical_angle /
                                        (double)pole_pairs);
        send_alignment_sample(commanded, measured,
                              (float)(commanded_value -
                                      (double)unwrapped),
                              g_app.raw_position,
                              (uint16_t)(point + points));
    }
    platform_commissioning_drive(0.0f, 0.0f, 0.0f);
    return COMMISSIONING_OK;
}

CommissioningStatus commissioning_run_direction_and_alignment(
    MotorConfig *config)
{
    if ((config == NULL) || !platform_commissioning_begin()) {
        return COMMISSIONING_POWER_DISABLED;
    }

    /* detect_motor_direction_and_pole_pairs@0x264a4 clears all 256 live
     * correction entries and starts from direction code 1 before motion. */
    platform_commissioning_reset_motor_encoder();
    config->direction = 1.0f;
    config->sensor_inverted = true;
    platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f, 0.0f);
    for (uint32_t count = 0U; count < DIRECTION_LOCK_STEPS; ++count) {
        platform_commissioning_delay_us(50U);
    }

    const float start = g_app.rotor_angle;
    float electrical_travel = 0.0f;
    float quarter_delta = 0.0f;
    for (;;) {
        electrical_travel += DIRECTION_ANGLE_STEP;
        platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f,
                                     electrical_travel);
        platform_commissioning_delay_us(50U);
        quarter_delta = wrap_signed(g_app.rotor_angle - start);
        if (fabsf(quarter_delta) >= PI_OVER_TWO_F) {
            break;
        }
    }

    for (;;) {
        electrical_travel += DIRECTION_ANGLE_STEP;
        platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f,
                                     electrical_travel);
        platform_commissioning_delay_us(50U);
        if (fabsf(g_app.rotor_angle - start) <= 0.007f) {
            break;
        }
    }

    CommissioningDirectionResult result;
    if (!commissioning_analyze_direction(quarter_delta, electrical_travel,
                                         &result)) {
        platform_commissioning_end();
        return COMMISSIONING_INVALID_MEASUREMENT;
    }

    config->direction = result.direction_code;
    config->sensor_inverted = result.direction_code == 1.0f;
    config->pole_pairs = result.pole_pairs;
    platform_set_motor_encoder_direction(config->sensor_inverted);

    const uint32_t pole_pairs = result.pole_pairs;
    const uint8_t direction_reply[6] = {
        'c',
        (uint8_t)pole_pairs,
        (uint8_t)(pole_pairs >> 8U),
        (uint8_t)(pole_pairs >> 16U),
        (uint8_t)(pole_pairs >> 24U),
        'U',
    };
    platform_debug_write(direction_reply, sizeof(direction_reply));
    const CommissioningStatus alignment = run_alignment_scan(result.pole_pairs);
    platform_commissioning_end();
    return alignment;
}

CommissioningStatus commissioning_run_output_sensor_calibration(void)
{
    if (!platform_commissioning_begin()) {
        return COMMISSIONING_POWER_DISABLED;
    }

    PlatformCommissioningSample sample;
    if (!platform_commissioning_read_sample(&sample)) {
        platform_commissioning_end();
        return COMMISSIONING_TIMEOUT;
    }
    OutputSensorExtrema extrema = {
        .minimum_u = 4096U,
        .maximum_u = 0U,
        .minimum_v = 4096U,
        .maximum_v = 0U,
    };
    const float start_position = g_app.motor_output_position;
    uint32_t report_count = 0U;

    for (;;) {
        if (!platform_commissioning_read_sample(&sample)) {
            platform_commissioning_end();
            return COMMISSIONING_INVALID_MEASUREMENT;
        }
        if (sample.output_raw_u > extrema.maximum_u) {
            extrema.maximum_u = sample.output_raw_u;
            extrema.angle_at_maximum_u = g_app.motor_output_position;
        }
        if (sample.output_raw_u < extrema.minimum_u) {
            extrema.minimum_u = sample.output_raw_u;
            extrema.angle_at_minimum_u = g_app.motor_output_position;
        }
        if (sample.output_raw_v > extrema.maximum_v) {
            extrema.maximum_v = sample.output_raw_v;
            extrema.angle_at_maximum_v = g_app.motor_output_position;
        }
        if (sample.output_raw_v < extrema.minimum_v) {
            extrema.minimum_v = sample.output_raw_v;
            extrema.angle_at_minimum_v = g_app.motor_output_position;
        }
        refresh_commissioning_position();
        if (++report_count == OUTPUT_CALIBRATION_REPORT_DIVIDER) {
            report_count = 0U;
            send_output_sensor_raw_sample(sample.output_raw_u,
                                          sample.output_raw_v);
        }

        const float electrical_angle =
            g_app.rotor_angle * (float)g_app.config.pole_pairs +
            g_app.motor.electrical_offset;
        platform_commissioning_drive(0.0f,
                                     OUTPUT_CALIBRATION_VOLTAGE_Q,
                                     electrical_angle);
        platform_commissioning_finish_sample();
        if (fabsf(g_app.motor_output_position - start_position) >=
            (TWO_PI_F - 0.0010000000474974513f)) {
            break;
        }
    }

    float calibration[4];
    if (!sensor_calibration_analyze_output_extrema(&extrema, calibration) ||
        !platform_commissioning_apply_output_calibration(
            calibration, sample.output_raw_u, sample.output_raw_v)) {
        platform_commissioning_end();
        return COMMISSIONING_INVALID_MEASUREMENT;
    }

    send_sensor_sample('J', 0.0f, 0.0f, 0.0f,
                       g_app.config.pole_pairs,
                       (uint16_t)g_app.config.gear_ratio);
    platform_commissioning_delay_us(1000U);

    const float measurement_points = g_app.config.gear_ratio *
        (float)OUTPUT_TABLE_POINTS_PER_MOTOR_TURN;
    const uint32_t measurement_count = (uint32_t)measurement_points;
    float output_table[OUTPUT_TABLE_POINTS] = {0.0f};
    uint16_t output_table_counts[OUTPUT_TABLE_POINTS] = {0U};
    report_count = 0U;
    /* measure_position_sensor_offset@0x24448 locks the d-axis at phase zero
     * and advances its double-precision phase accumulator from zero.  It
     * does not seed this pass from the live encoder angle. */
    double electrical_angle = 0.0;
    platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f,
                                 (float)electrical_angle);
    for (uint32_t count = 0U; count < ALIGNMENT_LOCK_STEPS; ++count) {
        platform_commissioning_delay_us(100U);
    }
    const double electrical_step = (double)(
        ((float)g_app.config.pole_pairs * TWO_PI_F *
         g_app.config.gear_ratio) / (float)measurement_count);
    for (uint32_t count = 0U; count < measurement_count; ++count) {
        if (!platform_commissioning_read_sample(&sample)) {
            platform_commissioning_end();
            return COMMISSIONING_INVALID_MEASUREMENT;
        }
        refresh_commissioning_position();
        electrical_angle += electrical_step;
        platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f,
                                     (float)electrical_angle);
        platform_commissioning_finish_sample();
        const float commanded_output = (float)(
            electrical_angle /
            ((double)g_app.config.pole_pairs *
             (double)g_app.config.gear_ratio));
        const float measured = sample.output_uncorrected_angle;
        const uint32_t table_index =
            (count * OUTPUT_TABLE_POINTS) / measurement_count;
        if (table_index < OUTPUT_TABLE_POINTS) {
            output_table[table_index] += wrap_signed(commanded_output - measured);
            ++output_table_counts[table_index];
        }
        if (++report_count == OUTPUT_CALIBRATION_REPORT_DIVIDER) {
            report_count = 0U;
            send_output_sensor_raw_sample(sample.output_raw_u,
                                          sample.output_raw_v);
        }
    }

    for (uint32_t index = 0U; index < OUTPUT_TABLE_POINTS; ++index) {
        if (output_table_counts[index] != 0U) {
            output_table[index] /= (float)output_table_counts[index];
        }
    }
    send_output_sensor_result_table(output_table);
    platform_commissioning_delay_us(1000U);
    platform_commissioning_drive(0.0f, 0.0f, 0.0f);
    platform_commissioning_end();
    debug_console_printf(
        "u=%.4f v=%.4f  w=%.4f c=%.4f\r\n",
        (double)calibration[0],
        (double)calibration[1],
        (double)calibration[2],
        (double)calibration[3]);
    return COMMISSIONING_OK;
}

CommissioningStatus commissioning_run_motor_identification(MotorConfig *config)
{
    if ((config == NULL) || (config->pole_pairs == 0U) ||
        !platform_commissioning_begin()) {
        return COMMISSIONING_POWER_DISABLED;
    }

    refresh_commissioning_position();
    const float locked_electrical_angle =
        g_app.rotor_angle * (float)config->pole_pairs +
        g_app.motor.electrical_offset;
    platform_commissioning_drive(0.1f, 0.0f,
                                 locked_electrical_angle);
    for (uint32_t count = 0U; count < IDENTIFICATION_LOCK_STEPS; ++count) {
        platform_commissioning_delay_us(100U);
    }
    platform_commissioning_finish_sample();

    PlatformCommissioningSample sample;
    float resistance;
    float inductance;
    if (!identify_electrical_parameters(locked_electrical_angle,
                                        &resistance, &inductance, &sample) ||
        (resistance < 0.0f)) {
        platform_commissioning_end();
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
    if (inductance < 0.0f) {
        platform_commissioning_end();
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
    platform_commissioning_delay_us(5000U);

    MotorConfig identified = *config;
    identified.phase_resistance = resistance;
    identified.phase_inductance = inductance;
    float flux_linkage;
    if (!identify_flux_linkage(&identified, &sample, &flux_linkage)) {
        platform_commissioning_end();
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
    identified.flux_linkage = flux_linkage;
    float inertia;
    float damping;
    if (!identify_mechanical_parameters(
            &identified, flux_linkage, &sample, &inertia, &damping)) {
        platform_commissioning_end();
        return COMMISSIONING_INVALID_MEASUREMENT;
    }

    identified.rotor_inertia = inertia;
    identified.viscous_damping = damping;
    *config = identified;
    motor_control_configure(&g_app.motor, config, sample.bus_voltage);
    send_identification_result(config);
    platform_commissioning_end();
    return COMMISSIONING_OK;
}
