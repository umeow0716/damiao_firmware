#include "motor_math.h"

#include <math.h>

#include "motor_sine_table.h"

#define INV_SQRT3_F 0.5773502691896258f
#define SINCOS_INDEX_SCALE 325.9493103027344f
#define SINCOS_ZERO_INDEX  1024.0f
#define PI_F                0x1.921fb6p+1f

static float motor_sine_table[2049];

static void motor_math_initialize_sine_table(void) __attribute__((constructor));

static void motor_math_initialize_sine_table(void)
{
    /* The captured 2049-point table is bit-exact quarter-wave symmetric.
     * Expand it once into RAM so the 20 kHz control path retains the original
     * direct lookup timing.  BSS initialization supplies positive zero at the
     * three crossings; the loop fills every non-zero entry. */
    for (uint16_t index = 1U; index <= 512U; ++index) {
        const float value = motor_sine_quarter_table[index];
        motor_sine_table[index] = value;
        motor_sine_table[1024U - index] = value;
        motor_sine_table[1024U + index] = -value;
        motor_sine_table[2048U - index] = -value;
    }
}

float motor_clampf(float value, float minimum, float maximum)
{
    if (value > maximum) {
        return maximum;
    }
    if (value < minimum) {
        return minimum;
    }
    return value;
}

float motor_wrapf(float value, float minimum, float maximum)
{
    const float period = maximum - minimum;
    if (period <= 0.0f) {
        return minimum;
    }
    while (value > maximum) {
        value -= period;
    }
    while (value < minimum) {
        value += period;
    }
    return value;
}

float motor_fast_sin(float angle)
{
    float sine;
    float cosine;
    angle = motor_wrapf(angle, -PI_F, PI_F);
    motor_fast_sincos(angle, &sine, &cosine);
    return sine;
}

void motor_fast_sincos(float angle, float *sine, float *cosine)
{
    const float table_position = angle * SINCOS_INDEX_SCALE +
                                 SINCOS_ZERO_INDEX;
    if ((table_position == 0.0f) || (table_position == 2048.0f)) {
        *sine = 0.0f;
        *cosine = -1.0f;
        return;
    }

    const uint16_t index = (uint16_t)table_position;
    const float fraction = table_position - (float)index;
    *sine = motor_sine_table[index] +
            fraction * (motor_sine_table[index + 1U] -
                        motor_sine_table[index]);

    const uint16_t cosine_index = (uint16_t)((index + 0x200U) & 0x7ffU);
    *cosine = motor_sine_table[cosine_index] +
              fraction * (motor_sine_table[cosine_index + 1U] -
                          motor_sine_table[cosine_index]);
}

uint32_t motor_float_to_uint(float value, float minimum, float maximum, uint8_t bits)
{
    /* float_to_uint_packed@0x1fffa46a extrapolates outside the configured
     * range and converts through a signed integer; it does not clamp. */
    const int32_t full_scale = ((int32_t)1 << bits) - 1;
    return (uint32_t)(int32_t)(((value - minimum) * (float)full_scale) /
                              (maximum - minimum));
}

float motor_uint_to_float(uint32_t value, float minimum, float maximum, uint8_t bits)
{
    const int32_t full_scale = ((int32_t)1 << bits) - 1;
    return ((float)value * (maximum - minimum) / (float)full_scale) + minimum;
}

AlphaBeta motor_clarke(float phase_u, float phase_v)
{
    AlphaBeta result = {
        .alpha = phase_u,
        .beta = (phase_u + (2.0f * phase_v)) * INV_SQRT3_F,
    };
    return result;
}

DirectQuadrature motor_park(AlphaBeta stationary, float sine, float cosine)
{
    DirectQuadrature result = {
        .d = (stationary.alpha * cosine) + (stationary.beta * sine),
        .q = (-stationary.alpha * sine) + (stationary.beta * cosine),
    };
    return result;
}

AlphaBeta motor_inverse_park(DirectQuadrature rotating, float sine, float cosine)
{
    AlphaBeta result = {
        .alpha = (rotating.d * cosine) - (rotating.q * sine),
        .beta = (rotating.d * sine) + (rotating.q * cosine),
    };
    return result;
}

PhaseDuty motor_svpwm(AlphaBeta voltage)
{
    /* Algebraic transcription of svpwm_write_compare at 0x1fff9c40.  The
     * original writes (1 - phase) * 2500 into a 5000-count center-aligned
     * timer; returning (1 + phase) / 2 preserves that exact relation through
     * board_sampling_timer_duty_to_compare(). */
    const float projection = voltage.alpha * 0.8660253882408142f +
                             voltage.beta * 0.5f;
    float phase_a = projection - voltage.beta;
    float phase_b;
    float phase_c;
    int sector = projection <= 0.0f ? 3 : 2;
    if (phase_a > 0.0f) {
        --sector;
    }
    if (voltage.beta < 0.0f) {
        sector = 7 - sector;
    }

    switch (sector) {
    case 1:
    case 4:
        phase_b = voltage.beta - phase_a;
        phase_a = projection;
        phase_c = -projection;
        break;
    case 2:
    case 5:
        phase_a += projection;
        phase_c = -voltage.beta;
        phase_b = voltage.beta;
        break;
    case 3:
    case 6:
        phase_c = -(voltage.beta + projection);
        phase_b = -phase_a;
        break;
    default:
        phase_a = 0.0f;
        phase_b = 0.0f;
        phase_c = 0.0f;
        break;
    }

    const PhaseDuty result = {
        .a = motor_clampf(0.5f * (1.0f + phase_a), 0.0f, 1.0f),
        .b = motor_clampf(0.5f * (1.0f + phase_b), 0.0f, 1.0f),
        .c = motor_clampf(0.5f * (1.0f + phase_c), 0.0f, 1.0f),
    };
    return result;
}

float motor_pi_step(PIController *controller, float error, float dt)
{
    const float proportional = controller->kp * error;
    controller->integral += controller->ki * error * dt;
    controller->integral = motor_clampf(controller->integral,
                                        controller->output_min,
                                        controller->output_max);
    const float unconstrained = proportional + controller->integral;
    const float output = motor_clampf(unconstrained,
                                      controller->output_min,
                                      controller->output_max);
    if (output != unconstrained) {
        controller->integral = output - proportional;
    }
    return output;
}

void motor_limit_vector(float limit, float *x, float *y)
{
    const float magnitude = sqrtf((*x * *x) + (*y * *y));
    if ((limit > 0.0f) && (magnitude > limit)) {
        const float scale = limit / magnitude;
        *x *= scale;
        *y *= scale;
    }
}
