#include "commissioning.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define TWO_PI_F 0x1.921fb6p+2f
#define IDENTIFICATION_RLS_LAMBDA     (0.9900000095367432f)
#define IDENTIFICATION_RLS_INV_LAMBDA (1.0101009607315063f)
#define IDENTIFICATION_RLS_NEG_INV_LAMBDA (-1.0101009607315063f)

static double positive_sqrt(double value)
{
    /* The caller converts a negative radicand to NaN before entering here.
     * Seed Newton-Raphson from the IEEE-754 exponent, then use four steps;
     * the maximum observed relative error over positive normal doubles is
     * one ULP.
     * This avoids newlib's domain/errno wrapper and general libm square-root
     * implementation on a target that has no double-precision FPU. */
    union {
        double value;
        uint64_t bits;
    } estimate = {.value = value};
    estimate.bits = (estimate.bits >> 1U) + UINT64_C(0x1ff8000000000000);
    for (unsigned iteration = 0U; iteration < 4U; ++iteration) {
        estimate.value = 0.5 * (estimate.value + value / estimate.value);
    }
    return estimate.value;
}

void commissioning_rls2_init(CommissioningRls2 *estimator)
{
    if (estimator == NULL) {
        return;
    }
    memset(estimator, 0, sizeof(*estimator));
    estimator->covariance_00 = 1.0f;
    estimator->covariance_11 = 1.0f;
}

void commissioning_rls2_step(CommissioningRls2 *estimator,
                             float measurement, float applied_voltage)
{
    if (estimator == NULL) {
        return;
    }

    estimator->measurement = measurement;
    estimator->applied_voltage = applied_voltage;
    const float phi_current = estimator->previous_measurement;
    const float phi_voltage = estimator->applied_voltage;
    const float old_p00 = estimator->covariance_00;
    const float old_p01 = estimator->covariance_01;
    const float old_p10 = estimator->covariance_10;
    const float old_p11 = estimator->covariance_11;
    const float weighted_current = old_p00 * phi_current +
                                   old_p01 * phi_voltage;
    const float weighted_voltage = old_p10 * phi_current +
                                   old_p11 * phi_voltage;
    const float error = measurement -
        phi_current * estimator->coefficient_a -
        phi_voltage * estimator->coefficient_b;
    const float inverse_denominator = 1.0f /
        (phi_current * weighted_current +
         phi_voltage * weighted_voltage + IDENTIFICATION_RLS_LAMBDA);
    const float gain_current = weighted_current * inverse_denominator;
    const float gain_voltage = weighted_voltage * inverse_denominator;

    /* Keep the recovered operation order and its distinct +/-1/lambda
     * literals.  This is the exact covariance update decompiled from the
     * 212-byte RAM helper, not a generic library RLS substitution. */
    const float left_00 = (1.0f - gain_current * phi_current) *
                          IDENTIFICATION_RLS_INV_LAMBDA;
    const float left_01 = gain_current *
                          IDENTIFICATION_RLS_NEG_INV_LAMBDA * phi_voltage;
    const float left_10 = gain_voltage *
                          IDENTIFICATION_RLS_NEG_INV_LAMBDA * phi_current;
    const float left_11 = (1.0f - gain_voltage * phi_voltage) *
                          IDENTIFICATION_RLS_INV_LAMBDA;

    estimator->covariance_01 = left_00 * old_p01 + left_01 * old_p11;
    estimator->covariance_10 = left_10 * old_p00 + left_11 * old_p10;
    estimator->covariance_11 = left_10 * old_p01 + left_11 * old_p11;
    estimator->previous_measurement = measurement;
    estimator->coefficient_a += gain_current * error;
    estimator->coefficient_b += gain_voltage * error;
    estimator->covariance_00 = left_00 * old_p00 + left_01 * old_p10;
}

bool commissioning_rls2_motor_parameters(const CommissioningRls2 *estimator,
                                          float sample_period,
                                          float *resistance,
                                          float *inductance)
{
    if ((estimator == NULL) || (resistance == NULL) ||
        (inductance == NULL)) {
        return false;
    }
    const float estimated_resistance =
        (1.0f - estimator->coefficient_a) / estimator->coefficient_b;
    const float estimated_inductance =
        sample_period / estimator->coefficient_b;
    *resistance = estimated_resistance;
    *inductance = estimated_inductance;
    /* The original caller accepts zero and rejects negative or unordered
     * results with two ordered VFP comparisons. */
    return (estimated_resistance >= 0.0f) &&
           (estimated_inductance >= 0.0f);
}

void commissioning_identification_filter_step(
    CommissioningIdentificationFilter *filter)
{
    if (filter == NULL) {
        return;
    }
    const float proportional = filter->input * filter->input_gain;
    filter->proportional_output = proportional;
    filter->integral_state += filter->integral_gain * proportional +
                              filter->saturation_error * 0.2f;
    const float unlimited = proportional + filter->integral_state;
    filter->unlimited_output = unlimited;
    float limited = unlimited;
    if (limited < filter->output_min) {
        limited = filter->output_min;
    } else if (limited > filter->output_max) {
        limited = filter->output_max;
    }
    filter->limited_output = limited;
    filter->saturation_error = limited - unlimited;
}

bool commissioning_flux_observer_init(CommissioningFluxObserver *observer,
                                       float inductance, float resistance,
                                       float sample_period)
{
    if (observer == NULL) {
        return false;
    }
    memset(observer, 0, sizeof(*observer));
    observer->inductance = inductance;
    observer->resistance_adaptation_gain = 15.199999809265137f;
    observer->flux_adaptation_gain = 0.10000000149011612f;
    observer->resistance_over_inductance = resistance / inductance;
    observer->flux_over_inductance = 1.0e-6f / inductance;
    observer->inverse_inductance = 1.0f / inductance;
    observer->base_resistance_over_inductance =
        observer->resistance_over_inductance;
    observer->base_flux_over_inductance =
        observer->flux_over_inductance;
    observer->base_inverse_inductance = observer->inverse_inductance;
    observer->sample_period = sample_period;
    observer->resistance = resistance;
    observer->flux_linkage = 1.0e-6f;
    return true;
}

void commissioning_flux_observer_step(CommissioningFluxObserver *observer)
{
    if (observer == NULL) {
        return;
    }
    const float speed = observer->electrical_speed;
    const float dt = observer->sample_period;
    const float decay = 1.0f - observer->resistance_over_inductance * dt;
    const float estimated_d =
        (observer->inverse_inductance * observer->voltage_d +
         speed * observer->estimated_current_q) * dt +
        decay * observer->estimated_current_d;
    observer->estimated_current_d = estimated_d;
    const float estimated_q =
        ((observer->inverse_inductance * observer->voltage_q -
          speed * estimated_d) -
         observer->flux_over_inductance * speed) * dt +
        decay * observer->estimated_current_q;
    observer->estimated_current_q = estimated_q;

    const float error_d = observer->measured_current_d - estimated_d;
    const float error_q = observer->measured_current_q - estimated_q;
    observer->current_error_d = error_d;
    observer->current_error_q = error_q;
    observer->resistance_adaptation_state += dt *
        (estimated_d * error_d + estimated_q * error_q);
    observer->flux_adaptation_state += dt * speed * error_q;
    observer->resistance_over_inductance =
        observer->base_resistance_over_inductance -
        observer->resistance_adaptation_gain *
        observer->resistance_adaptation_state;
    observer->flux_over_inductance =
        observer->base_flux_over_inductance -
        observer->flux_adaptation_gain * observer->flux_adaptation_state;
    observer->resistance = observer->inductance *
                           observer->resistance_over_inductance;
    observer->flux_linkage = observer->inductance *
                             observer->flux_over_inductance;
}

void commissioning_sine_regression_init(
    CommissioningSineRegression *regression)
{
    if (regression != NULL) {
        memset(regression, 0, sizeof(*regression));
    }
}

void commissioning_sine_regression_step(
    CommissioningSineRegression *regression,
    float phase_current, float mechanical_speed)
{
    if (regression == NULL) {
        return;
    }
    const double current = phase_current;
    const double speed = mechanical_speed;
    regression->current_energy += current * current;
    regression->current_speed_cross += current * speed;
    regression->speed_energy += speed * speed;
    ++regression->sample_count;
}

bool commissioning_sine_regression_motor_parameters(
    const CommissioningSineRegression *regression,
    float torque_per_amp, float excitation_angular_frequency,
    float *rotor_inertia, float *viscous_damping)
{
    if ((regression == NULL) || (rotor_inertia == NULL) ||
        (viscous_damping == NULL)) {
        return false;
    }

    const double energy_product = regression->current_energy *
                                  regression->speed_energy;
    /* The recovered implementation evaluates acos(correlation), tan(phase)
     * and an impedance magnitude.  In the accepted physical domain the same
     * B/J decomposition reduces exactly to the normal-equation form below:
     *
     *   B = Kt * abs(sum(i*w)) / sum(w^2)
     *   J = Kt * sqrt(sum(i^2)*sum(w^2)-sum(i*w)^2)
     *          * sign(sum(i*w)) / (sum(w^2) * excitation_frequency)
     *
     * Keep the recovered double accumulators, but avoid pulling general
     * acos/tan and argument-reduction code into a Cortex-M4F image.  Negative
     * The original acos/tan path keeps damping non-negative but transfers
     * the correlation sign to inertia. */
    const double determinant = energy_product -
        regression->current_speed_cross *
        regression->current_speed_cross;
    const double absolute_cross =
        regression->current_speed_cross < 0.0 ?
        -regression->current_speed_cross : regression->current_speed_cross;
    const double damping = (double)torque_per_amp *
        absolute_cross / regression->speed_energy;
    const double magnitude = determinant < 0.0 ? (double)NAN :
                             positive_sqrt(determinant);
    const double signed_magnitude =
        regression->current_speed_cross < 0.0 ? -magnitude : magnitude;
    const double inertia = (double)torque_per_amp * signed_magnitude /
        (regression->speed_energy *
         (double)excitation_angular_frequency);
    *rotor_inertia = (float)inertia;
    *viscous_damping = (float)damping;
    return true;
}

bool commissioning_analyze_direction(float first_quarter_turn_delta,
                                     float electrical_travel,
                                     CommissioningDirectionResult *result)
{
    if (result == NULL) {
        return false;
    }
    const uint32_t estimated =
        (uint32_t)(electrical_travel / TWO_PI_F + 0.5f);
    /* Original internal encoding: 1 reverses SPI counts, 2 keeps them. */
    result->direction_code = first_quarter_turn_delta > 0.0f ? 1.0f : 2.0f;
    result->pole_pairs = (uint8_t)estimated;
    return true;
}
