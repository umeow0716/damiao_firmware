#include "motor_control.h"

#include <string.h>

#define CURRENT_ADC_SCALE               (0x1p-11f)
#define CONTROL_SAMPLE_PERIOD           (0.00005f)
#define OUTER_SAMPLE_PERIOD             (0.001f)
#define OUTER_SAMPLE_FREQUENCY          (1000.0f)
#define VOLTAGE_NORMALIZATION (10.261194229125977f)
#define INV_SQRT3_F                     (0.5773502588272095f)
#define INV_TWO_PI_F                    (0.15915493667125702f)
#define TWO_PI_F                        (6.2831854820251465f)
#define MODULATION_VECTOR_LIMIT         (0.9800000190734863f)
#define MOTION_OBSERVER_BANDWIDTH       (1000.0f)
#define MOTION_OBSERVER_STATE_GAIN      (600.0f)
#define MOTOR_TEMPERATURE_FILTER_OLD     (0.9995002746582031f)
#define MOTOR_TEMPERATURE_FILTER_NEW     (0.000499725341796875f)

static float output_torque_constant(const MotorConfig *config)
{
    if (config->torque_constant != 0.0f) {
        return config->torque_constant * VOLTAGE_NORMALIZATION;
    }
    return config->gear_ratio * (float)config->pole_pairs * 1.5f *
           config->flux_linkage * VOLTAGE_NORMALIZATION *
           config->gear_torque_efficiency;
}

static void configure_current_axis(CurrentController *axis,
                                   const MotorConfig *config,
                                   float plant_gain)
{
    axis->control_bandwidth = config->current_loop_bandwidth;
    axis->sample_period = CONTROL_SAMPLE_PERIOD;
    axis->observer_bandwidth = config->current_loop_enhancement;
    axis->plant_gain = plant_gain;
    axis->inverse_plant_gain = 1.0f / plant_gain;
    axis->plant_gain_dt = plant_gain * CONTROL_SAMPLE_PERIOD;
    axis->observer_l1_dt = 2.0f * config->current_loop_enhancement *
                           CONTROL_SAMPLE_PERIOD;
    axis->observer_l2_dt = config->current_loop_enhancement *
                           config->current_loop_enhancement *
                           CONTROL_SAMPLE_PERIOD;
    axis->output_min = -1.0f;
    axis->output_max = 1.0f;
}

static void configure_motion_observer(MotionObserver *observer,
                                      const MotorConfig *config)
{
    const float mechanical_gain =
        ((float)config->pole_pairs * 1.5f * config->flux_linkage *
         VOLTAGE_NORMALIZATION) /
        config->rotor_inertia;
    observer->sample_period = OUTER_SAMPLE_PERIOD;
    /* load_motor_configuration writes literal 1000 and 600
     * into motor-parameter words 22/23; derive_control_parameters copies
     * them to observer fields 8/9.  They are intentionally not aliases for
     * the user speed limit. */
    observer->observer_bandwidth = MOTION_OBSERVER_BANDWIDTH;
    observer->observer_state_gain = MOTION_OBSERVER_STATE_GAIN;
    observer->plant_gain = mechanical_gain;
    observer->inverse_plant_gain = 1.0f / mechanical_gain;
    observer->output_min = -1.0f;
    observer->output_max = 1.0f;
}

void motor_control_motion_observer_step(MotionObserver *observer,
                                        float measured_velocity,
                                        float current_q)
{
    observer->measured_velocity = measured_velocity;
    observer->current_q = current_q;
    const float error = measured_velocity - observer->estimated_velocity;
    observer->error = error;
    const float derivative = (error - observer->previous_error) *
                             OUTER_SAMPLE_FREQUENCY;
    observer->error_derivative = derivative;
    observer->previous_error = error;
    float velocity_derivative = observer->observer_state;
    velocity_derivative += observer->observer_bandwidth * error;
    velocity_derivative += observer->plant_gain * current_q;
    observer->estimated_velocity += observer->sample_period *
                                    velocity_derivative;
    float correction = derivative;
    correction += observer->observer_bandwidth * error;
    observer->observer_state +=
        (observer->sample_period * observer->observer_state_gain) * correction;
    observer->disturbance_current = motor_clampf(
        observer->observer_state * observer->inverse_plant_gain,
        observer->output_min, observer->output_max);
}

void motor_control_init(MotorController *controller)
{
    memset(controller, 0, sizeof(*controller));
    controller->current_scale = CURRENT_ADC_SCALE;
    controller->sample_period = CONTROL_SAMPLE_PERIOD;
    controller->command.mode = MOTOR_MODE_DISABLED;
}

void motor_control_configure(MotorController *controller,
                             const MotorConfig *config,
                             float bus_voltage)
{
    /* derive_control_parameters@0x25138 computes these coefficients with raw
     * IEEE-754 division. Low bus voltage is handled by the fault state, not
     * by replacing the current-controller plant gain with zero. */
    const float plant_gain = (bus_voltage * INV_SQRT3_F) /
                             (VOLTAGE_NORMALIZATION *
                              config->phase_inductance);
    configure_current_axis(&controller->current_d, config, plant_gain);
    configure_current_axis(&controller->current_q, config, plant_gain);
    configure_motion_observer(&controller->motion_observer, config);

    const float torque_constant = output_torque_constant(config);
    controller->torque_to_current = 1.0f / torque_constant;
}

float motor_control_current_step(CurrentController *axis,
                                 float reference, float measurement)
{
    axis->reference = reference;
    axis->measurement = measurement;
    const float tracking_error = measurement - axis->estimated_current;
    axis->tracking_error = tracking_error;
    /* Keep the three additions in the factory VMLA order.  Regrouping this
     * expression changes the observer state after sustained 20 kHz use. */
    float estimated_current = axis->estimated_current;
    estimated_current += axis->sample_period * axis->disturbance_state;
    estimated_current += axis->observer_l1_dt * tracking_error;
    estimated_current += axis->plant_gain_dt * axis->control_output;
    axis->estimated_current = estimated_current;
    float disturbance_state = axis->disturbance_state;
    disturbance_state += axis->observer_l2_dt * tracking_error;
    axis->disturbance_state = disturbance_state;
    axis->control_error = reference - axis->estimated_current;
    axis->raw_control = axis->control_bandwidth * axis->control_error;
    axis->control_output = (axis->raw_control - axis->disturbance_state) *
                           axis->inverse_plant_gain;
    axis->control_output = motor_clampf(axis->control_output,
                                        axis->output_min,
                                        axis->output_max);
    axis->limited_output = axis->control_output;
    return axis->limited_output;
}

void motor_control_set_command(MotorController *controller,
                               const MotorCommand *command)
{
    controller->command = *command;
}

void motor_control_set_current_calibration(MotorController *controller,
                                           float offset_u, float offset_v,
                                           float offset_w)
{
    controller->current_offset_u = offset_u;
    controller->current_offset_v = offset_v;
    controller->current_offset_w = offset_w;
}

void motor_control_arm(MotorController *controller)
{
    controller->armed = true;
}

static void reset_dynamic_control_state(MotorController *controller)
{
    controller->current_d.tracking_error = 0.0f;
    controller->current_d.estimated_current = 0.0f;
    controller->current_d.disturbance_state = 0.0f;
    controller->current_d.control_error = 0.0f;
    controller->current_d.raw_control = 0.0f;
    controller->current_d.control_output = 0.0f;
    controller->current_d.limited_output = 0.0f;
    controller->current_q.tracking_error = 0.0f;
    controller->current_q.estimated_current = 0.0f;
    controller->current_q.disturbance_state = 0.0f;
    controller->current_q.control_error = 0.0f;
    controller->current_q.raw_control = 0.0f;
    controller->current_q.control_output = 0.0f;
    controller->current_q.limited_output = 0.0f;
    controller->desired_velocity = 0.0f;
}

void motor_control_disarm(MotorController *controller)
{
    controller->armed = false;
}

void motor_control_trip(MotorController *controller)
{
    controller->armed = false;
    reset_dynamic_control_state(controller);
}

static float slew_velocity(float current, float target,
                           const MotorConfig *config)
{
    const float error = target - current;
    if (error > config->acceleration_limit) {
        return current + config->acceleration_limit;
    }
    if (error < config->deceleration_limit) {
        return current + config->deceleration_limit;
    }
    return target;
}

static bool control_outer_step(MotorController *controller,
                               const MotorConfig *config,
                               float rotor_velocity)
{
    if (++controller->outer_loop_divider < 20U) {
        return false;
    }
    controller->outer_loop_divider = 0U;

    const float filter_previous = 1000.0f /
        (1000.0f + config->velocity_filter_bandwidth * TWO_PI_F);
    const float filter_new = 1.0f - filter_previous;
    controller->current_filter_previous = filter_previous;
    controller->filtered_current_q =
        controller->filtered_current_q * filter_previous +
        controller->feedback.current_q * filter_new;
    controller->feedback.output_torque = output_torque_constant(config) *
                                         controller->filtered_current_q;

    if (controller->command.mode != MOTOR_MODE_MIT) {
        float velocity_target = 0.0f;
        if ((controller->command.mode == MOTOR_MODE_POSITION_SPEED) ||
            (controller->command.mode == MOTOR_MODE_HYBRID)) {
            const float velocity_limit = controller->command.velocity;
            velocity_target = config->position_kp *
                (controller->command.position - controller->feedback.position);
            velocity_target = motor_clampf(velocity_target, -velocity_limit,
                                            velocity_limit);
        } else if (controller->command.mode == MOTOR_MODE_SPEED) {
            velocity_target = motor_clampf(controller->command.velocity,
                                            -config->speed_limit,
                                            config->speed_limit);
        }
        controller->desired_velocity = slew_velocity(
            controller->desired_velocity, velocity_target, config);
    }

    motor_control_motion_observer_step(&controller->motion_observer,
                                       rotor_velocity,
                                       controller->filtered_current_q);
    return true;
}

static float non_mit_current_reference(MotorController *controller,
                                       const MotorConfig *config)
{
    return config->speed_kp *
           (controller->desired_velocity -
            controller->motion_observer.estimated_velocity) -
           controller->motion_observer.disturbance_current;
}

PhaseDuty motor_control_fast_step(MotorController *controller,
                                  const MotorConfig *config,
                                  const AdcSample *sample)
{
    const PhaseDuty neutral = {0.5f, 0.5f, 0.5f};
    /* The original exposes gear-scaled position/velocity as feedback.  Its
     * non-MIT speed observer deliberately uses the unscaled motor-side
     * velocity instead; position_sensor_scale at 0x1ffff088+0x5c is applied
     * only when producing the output velocity at +0x1c. */
    controller->feedback.position = sample->output_position;
    controller->feedback.velocity = sample->output_velocity;
    controller->feedback.bus_voltage = sample->bus_voltage;
    controller->feedback.mos_temperature = sample->mos_temperature;
    controller->filtered_motor_temperature =
        controller->filtered_motor_temperature * MOTOR_TEMPERATURE_FILTER_OLD +
        sample->motor_temperature * MOTOR_TEMPERATURE_FILTER_NEW;
    controller->feedback.motor_temperature =
        controller->filtered_motor_temperature;

    const float current_u = motor_clampf(
        (controller->current_offset_u - sample->phase_u) *
        controller->current_scale, -1.0f, 1.0f);
    const float current_v = motor_clampf(
        (controller->current_offset_v - sample->phase_v) *
        controller->current_scale, -1.0f, 1.0f);
    const AlphaBeta current_ab = motor_clarke(current_u, current_v);

    const float pole_pairs = (float)config->pole_pairs;
    const uint32_t electrical_turns = (uint32_t)(
        sample->rotor_angle * (pole_pairs * INV_TWO_PI_F));
    const float electrical_angle = motor_wrapf(
        sample->rotor_angle * pole_pairs -
        (float)electrical_turns * TWO_PI_F + controller->electrical_offset,
        -3.14159265358979323846f,
        3.14159265358979323846f);
    float sine;
    float cosine;
    motor_fast_sincos(electrical_angle, &sine, &cosine);
    const DirectQuadrature current_dq = motor_park(current_ab, sine, cosine);
    controller->feedback.current_d = current_dq.d;
    controller->feedback.current_q = current_dq.q;

    controller->outer_loop_ran = control_outer_step(
        controller, config, sample->rotor_velocity);

    if (!controller->armed) {
        /* reset_control_state at 0x1fff9dc8 runs on every ADC tick for a
         * non-enabled motor. It clears both current controllers and the
         * velocity command, but intentionally leaves the motion observer
         * running so it follows a coasting/external-driven rotor. */
        reset_dynamic_control_state(controller);
        return neutral;
    }

    float requested_q;
    if (controller->command.mode == MOTOR_MODE_MIT) {
        const float requested_torque = controller->command.torque +
            controller->command.kp *
                (controller->command.position - controller->feedback.position) +
            controller->command.kd *
                (controller->command.velocity - controller->feedback.velocity);
        requested_q = requested_torque * controller->torque_to_current;
    } else if ((controller->command.mode == MOTOR_MODE_POSITION_SPEED) ||
               (controller->command.mode == MOTOR_MODE_SPEED) ||
               (controller->command.mode == MOTOR_MODE_HYBRID)) {
        requested_q = non_mit_current_reference(controller, config);
    } else {
        requested_q = 0.0f;
    }
    float current_limit = config->current_limit;
    if (controller->command.mode == MOTOR_MODE_HYBRID) {
        /* The PVT field is an unsigned per-unit current ceiling.  The
         * recovered IRQ clamps to +/-i_des even when i_des is zero. */
        current_limit = controller->command.torque;
    }
    requested_q = motor_clampf(requested_q, -current_limit, current_limit);

    DirectQuadrature voltage = {
        .d = motor_control_current_step(&controller->current_d, 0.0f,
                                        current_dq.d),
        .q = motor_control_current_step(&controller->current_q, requested_q,
                                        current_dq.q),
    };
    motor_limit_vector(MODULATION_VECTOR_LIMIT, &voltage.d, &voltage.q);
    return motor_svpwm(motor_inverse_park(voltage, sine, cosine));
}
