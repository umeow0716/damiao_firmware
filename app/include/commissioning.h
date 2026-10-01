#ifndef DAMIAO_COMMISSIONING_H
#define DAMIAO_COMMISSIONING_H

#include <stdbool.h>
#include <stdint.h>

#include "motor_control.h"

typedef enum {
    COMMISSIONING_OK = 0,
    COMMISSIONING_POWER_DISABLED,
    COMMISSIONING_INVALID_MEASUREMENT,
    COMMISSIONING_TIMEOUT,
} CommissioningStatus;

typedef struct {
    float direction_code;
#if defined(DAMIAO_DM4310)
    uint32_t pole_pairs;
#else
    uint8_t pole_pairs;
#endif
} CommissioningDirectionResult;

/* Exact nine-float state layout of the original RAM helper at 0x1fffa160.
 * The estimator fits i[k] = a*i[k-1] + b*v[k-1], from which the original
 * identification routine derives R=(1-a)/b and L=Ts/b. */
typedef struct {
    float measurement;
    float previous_measurement;
    float applied_voltage;
    float coefficient_a;
    float coefficient_b;
    float covariance_00;
    float covariance_01;
    float covariance_10;
    float covariance_11;
} CommissioningRls2;

/* Original 10-float saturated integrator helper at 0x1fff9e44. */
typedef struct {
    float input;
    float integral_gain;
    float input_gain;
    float proportional_output;
    float integral_state;
    float unlimited_output;
    float limited_output;
    float saturation_error;
    float output_min;
    float output_max;
} CommissioningIdentificationFilter;

/* Original 23-float coupled rotating-frame d/q observer at 0x1fffa234. */
typedef struct {
    float inductance;
    float resistance_adaptation_gain;
    float flux_adaptation_gain;
    float measured_current_d;
    float measured_current_q;
    float voltage_d;
    float voltage_q;
    float electrical_speed;
    float estimated_current_d;
    float estimated_current_q;
    float current_error_d;
    float current_error_q;
    float resistance_adaptation_state;
    float flux_adaptation_state;
    float resistance_over_inductance;
    float flux_over_inductance;
    float inverse_inductance;
    float base_resistance_over_inductance;
    float base_flux_over_inductance;
    float base_inverse_inductance;
    float sample_period;
    float resistance;
    float flux_linkage;
} CommissioningFluxObserver;

/* The original mechanical stage accumulates one sinusoidal excitation cycle
 * in double precision, then recovers phase and impedance magnitude from the
 * current/speed correlation. */
typedef struct {
    double current_energy;
    double current_speed_cross;
    double speed_energy;
    uint32_t sample_count;
} CommissioningSineRegression;

bool commissioning_analyze_direction(float first_quarter_turn_delta,
                                     float electrical_travel,
                                     CommissioningDirectionResult *result);
void commissioning_rls2_init(CommissioningRls2 *estimator);
void commissioning_rls2_step(CommissioningRls2 *estimator,
                             float measurement, float applied_voltage);
#if defined(DAMIAO_DM4310)
void commissioning_rls2_state_step(CommissioningRls2 *estimator);
void dm4310_rls2_helper(CommissioningRls2 *estimator);
#endif
bool commissioning_rls2_motor_parameters(const CommissioningRls2 *estimator,
                                          float sample_period,
                                          float *resistance,
                                          float *inductance);
void commissioning_identification_filter_step(
    CommissioningIdentificationFilter *filter);
#if defined(DAMIAO_DM4310)
void dm4310_identification_filter_helper(
    CommissioningIdentificationFilter *filter);
#endif
void commissioning_initialize_runtime_drive_states(void);
void commissioning_initialize_scatter_defaults(void);
void commissioning_configure_runtime_drive_states(const MotorConfig *config,
                                                   float bus_voltage);
void commissioning_configure_runtime_loop_states(const MotorConfig *config);
void commissioning_reset_runtime_loop_states(volatile float *speed, float value);
#if defined(DAMIAO_DM4310)
void commissioning_clear_runtime_loop_states(void);
void commissioning_derive_runtime_controller_states(void);
void dm4310_derive_runtime_controller_states_helper(void);
void dm4310_clear_runtime_loop_states_helper(void);
#endif
void commissioning_initialize_runtime_parameter_cache(const MotorConfig *config);
void commissioning_update_runtime_parameter_cache_ugq(const MotorConfig *config);
void commissioning_update_runtime_parameter_cache_motor_id(
    const MotorConfig *config);
bool commissioning_flux_observer_init(CommissioningFluxObserver *observer,
                                       float inductance, float resistance,
                                       float sample_period);
void commissioning_flux_observer_step(CommissioningFluxObserver *observer);
#if defined(DAMIAO_DM4310)
void dm4310_flux_observer_helper(CommissioningFluxObserver *observer);
#endif
void commissioning_sine_regression_init(
    CommissioningSineRegression *regression);
void commissioning_sine_regression_step(
    CommissioningSineRegression *regression,
    float phase_current, float mechanical_speed);
bool commissioning_sine_regression_motor_parameters(
    const CommissioningSineRegression *regression,
    float torque_per_amp, float excitation_angular_frequency,
    float *rotor_inertia, float *viscous_damping);
#if defined(DAMIAO_DM4310)
void commissioning_sine_regression_projection(
    const CommissioningSineRegression *regression, float torque_scale,
    float *phase, float *magnitude);
void commissioning_sine_response_motor_parameters(
    float phase, float magnitude, float excitation_frequency,
    bool frequency_in_hz,
    volatile float *rotor_inertia, volatile float *viscous_damping);
#endif

CommissioningStatus commissioning_run_direction_and_alignment(
    MotorConfig *config);
CommissioningStatus commissioning_run_output_sensor_calibration(void);
CommissioningStatus commissioning_run_motor_identification(MotorConfig *config);

#endif
