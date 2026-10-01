#include "motor_math.h"
#include <stdbool.h>

#include <math.h>

#include "motor_sine_table.h"

#define INV_SQRT3_F 0.5773502691896258f
#define SINCOS_INDEX_SCALE 325.9493103027344f
#define SINCOS_ZERO_INDEX  1024.0f
#define PI_F                0x1.921fb6p+1f

extern float motor_sine_table[2049];

void motor_math_initialize_sine_table(void) __attribute__((constructor));

void motor_math_initialize_sine_table(void)
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
    /* wrap_once@0x1fffa3aa performs at most one correction in each
     * direction.  Callers advance by less than one period per sample; do not
     * turn this into a general modulo operation because out-of-contract and
     * unordered bounds are observably different in the factory image. */
    if (value > maximum) {
        value -= period;
    }
    if (value < minimum) {
        value += period;
    }
    return value;
}

void motor_wrapf_in_place(float *value, float minimum, float maximum)
{
    *value = motor_wrapf(*value, minimum, maximum);
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
    /* uint_to_float_packed@0x1fffa492 converts r0 with VCVT.F32.S32 even
     * though protocol callers normally supply only 12- or 16-bit values. */
    return ((float)(int32_t)value * (maximum - minimum) /
            (float)full_scale) + minimum;
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

static PhaseDuty svpwm_result(AlphaBeta voltage, bool normalized_duty,
                              float projection_scale)
{
    /* Algebraic transcription of svpwm_write_compare at 0x1fff9c40.  The
     * original writes (1 - phase) * 2500 into a 5000-count center-aligned
     * timer; returning (1 + phase) / 2 preserves that exact relation through
     * board_sampling_timer_duty_to_compare(). */
    float projection = voltage.alpha * projection_scale;
    const float half = 0.5f;
    __asm volatile ("vmla.f32 %0, %1, %2"
                    : "+t" (projection)
                    : "t" (voltage.beta), "t" (half));
    float phase_a = projection - voltage.beta;
    float phase_b;
    float phase_c;
    /* VCMPE followed by BLE selects sector 3 for both <= 0 and unordered
     * inputs.  Writing this as !(projection > 0) preserves that NaN path. */
    int sector;
    if (!normalized_duty) {
        uint32_t projection_status;
        uint32_t phase_status;
        uint32_t beta_status;
        /* The raw modulation API preserves 0x1fff9c56..86: all three
         * VCMPEs execute even when two sectors share a polynomial. C
         * branch folding otherwise drops the beta comparison and changes
         * the FPSCR carried into the timer stores. */
        __asm volatile (
            "vcmpe.f32 %3, #0.0\nvmrs %0, FPSCR\n"
            "vcmpe.f32 %4, #0.0\nvmrs %1, FPSCR\n"
            "vcmpe.f32 %5, #0.0\nvmrs %2, FPSCR"
            : "=&r" (projection_status), "=&r" (phase_status),
              "=&r" (beta_status)
            : "t" (projection), "t" (phase_a), "t" (voltage.beta)
            : "memory");
        sector = ((projection_status & UINT32_C(0x40000000)) == 0U &&
                  (projection_status >> 31U) ==
                  ((projection_status >> 28U) & 1U)) ? 2 : 3;
        if ((phase_status & UINT32_C(0x40000000)) == 0U &&
            (phase_status >> 31U) == ((phase_status >> 28U) & 1U)) {
            --sector;
        }
        if ((beta_status & UINT32_C(0x20000000)) == 0U) {
            sector = 7 - sector;
        }
    } else {
        sector = projection > 0.0f ? 2 : 3;
        if (phase_a > 0.0f) {
            --sector;
        }
        if (voltage.beta < 0.0f) {
            sector = 7 - sector;
        }
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
        /* Preserve factory VADD operand order when both inputs are NaNs. */
        __asm volatile ("vadd.f32 %0, %0, %1"
                        : "+t" (phase_a) : "t" (projection));
        phase_c = -voltage.beta;
        phase_b = voltage.beta;
        break;
    case 3:
    case 6:
        __asm volatile ("vadd.f32 %0, %1, %2\nvneg.f32 %0, %0"
                        : "=t" (phase_c)
                        : "t" (voltage.beta), "t" (projection));
        phase_b = -phase_a;
        break;
    default:
        phase_a = 0.0f;
        phase_b = 0.0f;
        phase_c = 0.0f;
        break;
    }

    /* svpwm_write_compare@0x1fff9c40 does not clamp the three phase values
     * before its unsigned conversion.  Upstream voltage limiting normally
     * keeps them in range; preserving the raw result also preserves factory
     * over-modulation and unordered-input behavior. */
    const PhaseDuty result = {
        .a = normalized_duty ? 0.5f * (1.0f + phase_a) : 0.5f,
        .b = normalized_duty ? 0.5f * (1.0f + phase_b) : 0.5f,
        .c = normalized_duty ? 0.5f * (1.0f + phase_c) : 0.5f,
        .modulation_a = phase_a,
        .modulation_b = phase_b,
        .modulation_c = phase_c,
    };
    return result;
}

PhaseDuty motor_svpwm(AlphaBeta voltage)
{
    return svpwm_result(voltage, true, 0.8660253882408142f);
}

PhaseDuty motor_svpwm_modulation(AlphaBeta voltage)
{
    /* Factory compare writers consume only modulation fields. Avoid
     * additional floating operations (and FPSCR effects) for unused duty. */
    return svpwm_result(voltage, false, 0.8660253882408142f);
}

PhaseDuty motor_svpwm_modulation_scaled(AlphaBeta voltage, float projection_scale)
{
    return svpwm_result(voltage, false, projection_scale);
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
    if (magnitude > limit) {
        const float scale = limit / magnitude;
        *x *= scale;
        *y *= scale;
    }
}
