#include "commissioning.h"
#include "runtime_compat.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define TWO_PI_F 0x1.921fb6p+2f
#define IDENTIFICATION_RLS_LAMBDA     (0.9900000095367432f)
#define IDENTIFICATION_RLS_INV_LAMBDA (1.0101009607315063f)
#define IDENTIFICATION_RLS_NEG_INV_LAMBDA (-1.0101009607315063f)

#if defined(DAMIAO_DM4310)
#if defined(__arm__) || defined(__thumb__)
/* Keep the factory's r0:r1=new product, r2:r3=previous sum ABI order.
 * A distinct C name prevents treating this call as a commutative expression.
 * The runtime helper itself still requires the separate factory audit. */
extern double commissioning_runtime_double_add(double product, double previous)
    __asm__("__aeabi_dadd") __attribute__((pcs("aapcs")));
#else
static double commissioning_runtime_double_add(double product, double previous)
{
    return product + previous;
}
#endif

static float commissioning_vmul_f32(float left, float right)
{
#if defined(__arm__) || defined(__thumb__)
    float result;
    __asm volatile ("vmul.f32 %0, %1, %2"
                    : "=t" (result) : "t" (left), "t" (right));
    return result;
#else
    return left * right;
#endif
}

static float commissioning_vmla_f32(float accumulator, float left,
                                    float right)
{
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("vmla.f32 %0, %1, %2"
                    : "+t" (accumulator)
                    : "t" (left), "t" (right));
    return accumulator;
#else
    return accumulator + left * right;
#endif
}

static float commissioning_vmls_f32(float accumulator, float left,
                                    float right)
{
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("vmls.f32 %0, %1, %2"
                    : "+t" (accumulator)
                    : "t" (left), "t" (right));
    return accumulator;
#else
    return accumulator - left * right;
#endif
}
#endif

/* Factory large-argument reduction@0x242e4, using the captured 2/pi
 * integer coefficients rather than a host/libm remainder operation. */
#if defined(DAMIAO_DM4310)
static float commissioning_reduce_tangent(uint32_t bits, uint32_t *quadrant)
{
    static const uint32_t two_over_pi[8] = {
        0x00000000U, 0xA2F9836EU, 0x4E441529U, 0xFC2757D1U,
        0xF534DDC0U, 0xDB629599U, 0x3C439041U, 0xFE5163ABU,
    };
    const uint32_t exponent = ((bits >> 23U) & 255U) - 120U;
    const uint32_t index = exponent >> 5U;
    const uint32_t shift = exponent & 31U;
    uint32_t first = two_over_pi[index];
    uint32_t second = two_over_pi[index + 1U];
    uint32_t third = two_over_pi[index + 2U];
    if (shift != 0U) {
        first = (first << shift) | (second >> (32U - shift));
        second = (second << shift) | (third >> (32U - shift));
        third = (third << shift) |
                (two_over_pi[index + 3U] >> (32U - shift));
    }
    const uint32_t significand = UINT32_C(0x80000000) | (bits << 8U);
    const uint64_t product_first = (uint64_t)first * significand;
    const uint64_t product_second = (uint64_t)second * significand;
    const uint64_t product_third = (uint64_t)third * significand;
    const uint64_t low_sum = (uint32_t)product_second +
                            (uint64_t)(product_third >> 32U);
    const uint64_t middle_sum = (product_second >> 32U) +
                               (uint32_t)product_first + (low_sum >> 32U);
    const uint32_t top = (uint32_t)(product_first >> 32U) +
                         (uint32_t)(middle_sum >> 32U);
    const uint32_t middle = (uint32_t)middle_sum;
    const uint32_t signed_bits = (top << 26U) |
                                 ((middle >> 19U) << 13U);
    int32_t signed_word;
    memcpy(&signed_word, &signed_bits, sizeof(signed_word));
    const float integer = (float)signed_word;
    const float fraction = (float)(middle << 13U) * 0x1p-19f;
    const float tail = (float)(uint32_t)low_sum * 0x1p-38f;
    const float sum = (integer + fraction) + tail;
    uint32_t high_bits;
    memcpy(&high_bits, &sum, sizeof(high_bits));
    high_bits = (high_bits + 0x800U) & ~UINT32_C(0xfff);
    float high;
    memcpy(&high, &high_bits, sizeof(high));
    const float residual = tail - ((high - integer) - fraction);
    float reduced = high * 0x1.fb5444p-44f;
    reduced = commissioning_vmla_f32(reduced, residual, 0x1.921fb6p-32f);
    reduced = commissioning_vmla_f32(reduced, high, 0x1.92p-32f);
    const uint32_t turns = (top + 32U) >> 6U;
    if ((bits & UINT32_C(0x80000000)) != 0U) {
        *quadrant = UINT32_C(0x10000000) - turns;
        return -reduced;
    }
    *quadrant = turns;
    return reduced;
}

void dm4310_runtime_errno_set_helper(uint32_t value);

static double commissioning_double_sqrt_core(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    uint32_t low = (uint32_t)bits;
    uint32_t high = (uint32_t)(bits >> 32U);
    if ((int32_t)(high + 0x100000U) < 0x200000) {
        const uint32_t classification = (high + 0x100000U) << 1U;
        if ((classification >> 22U) != 0U) {
            bits = UINT64_C(0x7ff8000000000000);
            memcpy(&value, &bits, sizeof(value));
            return value;
        }
        if ((classification & 0x200000U) != 0U) {
            /* Original 0x278da flushes this class to signed zero, even
             * when the double mantissa is nonzero. Do not use libm here. */
            bits &= UINT64_C(0x8000000000000000);
            memcpy(&value, &bits, sizeof(value));
            return value;
        }
        /* Unary descriptor 0xbeffdb6d at 0x278f0 selects unchanged +Inf
         * or canonical NaN. This integer dispatcher does not raise VFP IOC
         * for a signaling NaN, unlike a floating arithmetic fallback. */
        if (bits != UINT64_C(0x7ff0000000000000)) {
            bits = UINT64_C(0x7ff8000000000000);
            memcpy(&value, &bits, sizeof(value));
        }
        return value;
    }
    static const uint8_t reciprocal_seed[48] = {
        252,245,238,232,226,221,216,211,207,203,199,195,
        192,189,185,182,180,177,174,172,169,167,165,163,
        161,159,157,155,154,152,150,149,147,146,144,143,
        141,140,139,137,136,135,134,133,132,131,130,129,
    };
    const uint32_t exponent = (high >> 20U) + 253U;
    high = (high & 0xfffffU) | 0x100000U;
    if ((exponent & 1U) != 0U) {
        high = (high << 1U) | (low >> 31U);
        low <<= 1U;
    }
    const uint32_t mantissa_high = (high << 10U) | (low >> 22U);
    const uint32_t mantissa_low = low << 10U;
    uint32_t reciprocal = reciprocal_seed[(mantissa_high >> 26U) - 16U];
    uint64_t product = (uint64_t)reciprocal *
        (0xc0000000U - (mantissa_high >> 16U) * reciprocal * reciprocal);
    reciprocal = (uint32_t)(product >> 23U);
    product = (uint64_t)(reciprocal * reciprocal) * mantissa_high;
    uint32_t negative_low = 0U - (uint32_t)product;
    uint32_t error = 0xc0000000U - (uint32_t)(product >> 32U) -
                     (uint32_t)((uint32_t)product != 0U);
    product = (uint64_t)reciprocal * error +
              (((uint64_t)reciprocal * negative_low) >> 32U);
    reciprocal = (uint32_t)(product >> 15U);
    const uint64_t square = (uint64_t)reciprocal * reciprocal;
    product = (uint64_t)(uint32_t)(square >> 32U) * mantissa_high +
        (((uint64_t)mantissa_high * (uint32_t)square) >> 32U) +
        (((uint64_t)(uint32_t)(square >> 32U) * mantissa_low) >> 32U);
    negative_low = 0U - (uint32_t)product;
    error = 0xc0000000U - (uint32_t)(product >> 32U) -
            (uint32_t)((uint32_t)product != 0U);
    const uint64_t refinement = (uint64_t)reciprocal * error +
        (((uint64_t)reciprocal * negative_low) >> 32U);
    const uint32_t refinement_high = (uint32_t)(refinement >> 32U);
    const uint32_t refinement_low = (uint32_t)refinement;
    const uint32_t cross = (uint32_t)(((uint64_t)refinement_high *
                                      refinement_low) >> 32U);
    const uint64_t refined_square = (uint64_t)refinement_high * refinement_high +
                                    (uint64_t)cross + cross;
    product = (uint64_t)(uint32_t)(refined_square >> 32U) * mantissa_high +
        (((uint64_t)mantissa_high * (uint32_t)refined_square) >> 32U) +
        (((uint64_t)(uint32_t)(refined_square >> 32U) * mantissa_low) >> 32U);
    negative_low = 0U - (uint32_t)product;
    error = 0x30000000U - (uint32_t)(product >> 32U) -
            (uint32_t)((uint32_t)product != 0U);
    const uint64_t correction = (uint64_t)refinement_high * error +
        (((uint64_t)refinement_low * error) >> 32U) +
        (((uint64_t)refinement_high * negative_low) >> 32U);
    product = (uint64_t)mantissa_high * (uint32_t)(correction >> 32U) +
        (((uint64_t)mantissa_low * (uint32_t)(correction >> 32U)) >> 32U) +
        (((uint64_t)mantissa_high * (uint32_t)correction) >> 32U);
    uint64_t rounded = (product + 32U) >> 6U;
    if ((((uint32_t)product - 27U) & 63U) <= 10U) {
        const uint32_t root_low = (uint32_t)rounded;
        const uint32_t root_high = (uint32_t)(rounded >> 32U);
        const uint64_t root_square = (uint64_t)root_low * root_low;
        const uint32_t difference_high = (uint32_t)(root_square >> 32U) +
            root_low * root_high * 2U - (mantissa_low << 10U);
        const uint64_t difference = ((uint64_t)difference_high << 32U) |
                                     (uint32_t)root_square;
        if ((int32_t)difference_high < 0) {
            if ((int32_t)((difference + rounded) >> 32U) < 0) {
                ++rounded;
            }
        } else if ((int32_t)((difference - rounded) >> 32U) >= 0) {
            --rounded;
        }
    }
    bits = rounded + ((uint64_t)((exponent >> 1U) + 384U) << 52U);
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static bool commissioning_double_is_nan(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    const uint32_t low = (uint32_t)bits;
    const uint32_t high = (uint32_t)(bits >> 32U);
    return ((high & UINT32_C(0x7fffffff)) | (uint32_t)(low != 0U)) >
           UINT32_C(0x7ff00000);
}

static double __attribute__((noinline)) commissioning_checked_double_sqrt(double value)
{
    /* Factory wrappers 0x23f56/0x26e20 set their fixed runtime errno only
     * when a non-NaN input becomes NaN. The arithmetic core 0x2776c still
     * and its unary exceptional-input dispatch are reconstructed as integer
     * operations, avoiding unrelated newlib error handling. */
    const double result = commissioning_double_sqrt_core(value);
    if (commissioning_double_is_nan(result) &&
        !commissioning_double_is_nan(value)) {
        dm4310_runtime_errno_set_helper(1U);
    }
    return result;
}

static double commissioning_polynomial(const double *coefficients,
                                       unsigned count, double value)
{
    double result = coefficients[count - 1U];
    for (unsigned index = count - 1U; index != 0U; --index) {
        result = result * value + coefficients[index - 1U];
    }
    return result;
}

static double commissioning_acos_ratio(double squared)
{
    /* Ascending coefficients from factory 0x28530 and 0x28560. */
    static const double numerator[6] = {
        0x1.5555555555555p-3, -0x1.4d61203eb6f7dp-2,
        0x1.9c1550e884455p-3, -0x1.48228b5688f3bp-5,
        0x1.9efe07501b288p-11, 0x1.23de10dfdf709p-15,
    };
    static const double denominator[4] = {
        -0x1.33a271c8a2d4bp+1, 0x1.02ae59c598ac8p+1,
        -0x1.6066c1b8d0159p-1, 0x1.3b8c5b12e9282p-4,
    };
    const double p = commissioning_polynomial(numerator, 6U, squared) * squared;
    const double q = commissioning_polynomial(denominator, 4U, squared) *
                     squared + 1.0;
    return p / q;
}

static double __attribute__((noinline)) commissioning_factory_acos(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    const uint32_t high = (uint32_t)(bits >> 32U);
    const uint32_t magnitude = high & UINT32_C(0x7fffffff);
    const uint32_t low = (uint32_t)bits;
    const double pi = 0x1.921fb54442d18p+1;
    const double half_pi = 0x1.921fb54442d18p+0;
    const double half_pi_low = 0x1.1a62633145c07p-54;
    if (magnitude >= UINT32_C(0x3ff00000)) {
        if ((magnitude == UINT32_C(0x3ff00000)) && (low == 0U)) {
            return (high & UINT32_C(0x80000000)) != 0U ? pi : 0.0;
        }
        if (!commissioning_double_is_nan(value)) {
            dm4310_runtime_errno_set_helper(1U);
        }
        bits = UINT64_C(0x7ff8000000000000);
        memcpy(&value, &bits, sizeof(value));
        return value;
    }
    if (magnitude < UINT32_C(0x3fe00000)) {
        if (magnitude <= UINT32_C(0x3c600000)) {
            return half_pi;
        }
        const double ratio = commissioning_acos_ratio(value * value);
        return half_pi - (value - (half_pi_low - value * ratio));
    }
    if ((high & UINT32_C(0x80000000)) != 0U) {
        const double squared = (value + 1.0) * 0.5;
        const double ratio = commissioning_acos_ratio(squared);
        const double root = commissioning_checked_double_sqrt(squared);
        const double correction = ratio * root - half_pi_low;
        return pi - (root + correction) * 2.0;
    }
    const double squared = (1.0 - value) * 0.5;
    const double root = commissioning_checked_double_sqrt(squared);
    memcpy(&bits, &root, sizeof(bits));
    bits &= UINT64_C(0xffffffff00000000);
    double high_root;
    memcpy(&high_root, &bits, sizeof(high_root));
    const double correction = (squared - high_root * high_root) /
                              (root + high_root);
    const double ratio = commissioning_acos_ratio(squared);
    return (high_root + (ratio * root + correction)) * 2.0;
}


/* Factory tanf@0x2400c. */
static float __attribute__((noinline)) dm4310_identification_tanf(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    const uint32_t magnitude = bits & UINT32_C(0x7fffffff);
    if (magnitude >= UINT32_C(0x7f800000)) {
        float result;
        if (magnitude == UINT32_C(0x7f800000)) {
            dm4310_runtime_errno_set_helper(1U);
            const float zero = 0.0f;
            __asm volatile ("vdiv.f32 %0, %1, %1"
                            : "=t" (result) : "t" (zero));
        } else {
            __asm volatile ("vadd.f32 %0, %1, %1"
                            : "=t" (result) : "t" (value));
        }
        return result;
    }
    if (magnitude < UINT32_C(0x39800000)) {
        if ((magnitude != 0U) && (magnitude < UINT32_C(0x00800000))) {
            (void)dm4310_runtime_force_underflow();
        }
        return value;
    }
    uint32_t quadrant = 0U;
    if (magnitude >= UINT32_C(0x46490e49)) {
        value = commissioning_reduce_tangent(bits, &quadrant);
    } else if (magnitude >= UINT32_C(0x3f490fdb)) {
        const float scaled = value * 0x1.45f306p-1f;
        const float bias = (bits & UINT32_C(0x80000000)) != 0U ?
            -0x1p+23f : 0x1p+23f;
        const float rounded = (scaled + bias) - bias;
        quadrant = (uint32_t)(int32_t)rounded & 3U;
        value = commissioning_vmls_f32(value, rounded, 0x1.92p+0f);
        value = commissioning_vmls_f32(value, rounded, 0x1.fb4p-12f);
        value = commissioning_vmls_f32(value, rounded, 0x1.444p-24f);
        value = commissioning_vmls_f32(value, rounded, 0x1.68c234p-39f);
    }
    const float squared = value * value;
    float polynomial = commissioning_vmla_f32(
        0x1.0cda18p-9f, squared, 0x1.45d376p-7f);
    polynomial = commissioning_vmla_f32(
        0x1.9ced14p-6f, squared, polynomial);
    polynomial = commissioning_vmla_f32(
        0x1.b36b58p-5f, squared, polynomial);
    polynomial = commissioning_vmla_f32(
        0x1.11426ap-3f, squared, polynomial);
    polynomial = commissioning_vmla_f32(
        0x1.555452p-2f, squared, polynomial);
    const float tangent = commissioning_vmla_f32(
        value, value, polynomial * squared);
    return (quadrant & 1U) != 0U ? -1.0f / tangent : tangent;
}
#endif

/* These two work objects are stack-resident in run_motor_identification, but
 * their layouts are an ABI: the factory veneers at 0x219ce/0x219e2 pass them
 * to the copied helpers at 0x1fffa160/0x1fffa234. */
_Static_assert(sizeof(CommissioningRls2) == 0x24,
               "DM4310 RLS helper state size");
_Static_assert(offsetof(CommissioningRls2, covariance_00) == 0x14,
               "DM4310 RLS covariance offset");
_Static_assert(sizeof(CommissioningFluxObserver) == 0x5c,
               "DM4310 flux-observer helper state size");
_Static_assert(offsetof(CommissioningFluxObserver, sample_period) == 0x50,
               "DM4310 flux-observer sample-period offset");
_Static_assert(offsetof(CommissioningFluxObserver, flux_linkage) == 0x58,
               "DM4310 flux-observer result offset");

#if !defined(DAMIAO_DM4310)
static double positive_sqrt(double value)
{
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
#endif

void commissioning_rls2_init(CommissioningRls2 *estimator)
{
    if (estimator == NULL) {
        return;
    }
#if defined(DAMIAO_DM4310)
    /* 0x2587a..0x25896: the diagonal seed comes from 0x25bd4
     * (0x42c80000 = 100), not an identity covariance. Measurement
     * is first written by 0x25a4c before the helper is called. */
    volatile CommissioningRls2 *const state = estimator;
    state->previous_measurement = 0.0f;
    state->applied_voltage = 0.0f;
    state->coefficient_a = 0.0f;
    state->coefficient_b = 0.0f;
    state->covariance_00 = 100.0f;
    state->covariance_01 = 0.0f;
    state->covariance_10 = 0.0f;
    state->covariance_11 = 100.0f;
#else
    memset(estimator, 0, sizeof(*estimator));
    estimator->covariance_00 = 1.0f;
    estimator->covariance_11 = 1.0f;
#endif
}

void commissioning_rls2_step(CommissioningRls2 *estimator,
                             float measurement, float applied_voltage)
{
    estimator->measurement = measurement;
    estimator->applied_voltage = applied_voltage;
#if defined(DAMIAO_DM4310)
    commissioning_rls2_state_step(estimator);
#else
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
#endif
}

#if defined(DAMIAO_DM4310)
void commissioning_rls2_state_step(CommissioningRls2 *estimator)
{
    volatile CommissioningRls2 *const state = estimator;
    const float phi_current = state->previous_measurement;
    const float old_p00 = state->covariance_00;
    const float phi_voltage = state->applied_voltage;
    const float old_p01 = state->covariance_01;
    float weighted_current = commissioning_vmul_f32(old_p00, phi_current);
    const float old_p10 = state->covariance_10;
    const float measurement = state->measurement;
    const float coefficient_a = state->coefficient_a;
    weighted_current = commissioning_vmla_f32(
        weighted_current, old_p01, phi_voltage);
    float weighted_voltage = commissioning_vmul_f32(old_p10, phi_current);
    const float old_p11 = state->covariance_11;
    float error = measurement;
    error = commissioning_vmls_f32(
        error, phi_current, coefficient_a);
    weighted_voltage = commissioning_vmla_f32(
        weighted_voltage, old_p11, phi_voltage);
    float denominator = commissioning_vmul_f32(phi_current, weighted_current);
#if defined(__arm__) || defined(__thumb__)
    /* 0x1fffa19c precedes the coefficient-B load at 0x1fffa1a0. */
    __asm volatile ("" : "+t" (denominator) : : "memory");
#endif
    const float coefficient_b = state->coefficient_b;
    error = commissioning_vmls_f32(
        error, phi_voltage, coefficient_b);
    denominator = commissioning_vmla_f32(
        denominator, phi_voltage, weighted_voltage);
    float inverse_denominator;
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("vadd.f32 %0, %0, %2\n"
                    "vdiv.f32 %1, %3, %0"
                    : "+t" (denominator), "=&t" (inverse_denominator)
                    : "t" (IDENTIFICATION_RLS_LAMBDA), "t" (1.0f));
#else
    denominator += IDENTIFICATION_RLS_LAMBDA;
    inverse_denominator = 1.0f / denominator;
#endif
    const float gain_current = commissioning_vmul_f32(
        weighted_current, inverse_denominator);
    const float gain_voltage = commissioning_vmul_f32(
        weighted_voltage, inverse_denominator);

    /* 0x1fffa1c8..0x1fffa1f4 interleaves coefficient updates with
     * the covariance-left products; retain the distinct +/-1/lambda. */
    const float left_00 = commissioning_vmul_f32(
        commissioning_vmls_f32(1.0f, gain_current, phi_current),
        IDENTIFICATION_RLS_INV_LAMBDA);
    float left_11 = commissioning_vmls_f32(1.0f, gain_voltage, phi_voltage);
    const float new_coefficient_a = commissioning_vmla_f32(
        coefficient_a, gain_current, error);
    float left_01 = commissioning_vmul_f32(
        gain_current, IDENTIFICATION_RLS_NEG_INV_LAMBDA);
    float left_10 = commissioning_vmul_f32(
        gain_voltage, IDENTIFICATION_RLS_NEG_INV_LAMBDA);
    const float new_coefficient_b = commissioning_vmla_f32(
        coefficient_b, gain_voltage, error);
    left_01 = commissioning_vmul_f32(left_01, phi_voltage);
    left_10 = commissioning_vmul_f32(left_10, phi_current);
    left_11 = commissioning_vmul_f32(left_11, IDENTIFICATION_RLS_INV_LAMBDA);

    /* Coefficient stores follow covariance 01/10/11 and previous input. */
    float new_covariance_00 = commissioning_vmul_f32(left_00, old_p00);
    float new_covariance_01 = commissioning_vmul_f32(left_00, old_p01);
    float new_covariance_10 = commissioning_vmul_f32(left_10, old_p00);
    float new_covariance_11 = commissioning_vmul_f32(left_10, old_p01);
    new_covariance_00 = commissioning_vmla_f32(
        new_covariance_00, left_01, old_p10);
    new_covariance_01 = commissioning_vmla_f32(
        new_covariance_01, left_01, old_p11);
    new_covariance_10 = commissioning_vmla_f32(
        new_covariance_10, left_11, old_p10);
    new_covariance_11 = commissioning_vmla_f32(
        new_covariance_11, left_11, old_p11);

    state->covariance_01 = new_covariance_01;
    state->covariance_10 = new_covariance_10;
    state->covariance_11 = new_covariance_11;
    state->previous_measurement = measurement;
    state->coefficient_a = new_coefficient_a;
    state->coefficient_b = new_coefficient_b;
    state->covariance_00 = new_covariance_00;
}
#endif

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
#if defined(DAMIAO_DM4310) && (defined(__arm__) || defined(__thumb__))
    /* Volatile state and arithmetic barriers preserve the helper's
     * observable load/store order, including the conditional upper bound. */
    volatile CommissioningIdentificationFilter *const state = filter;
    const float input = state->input;
    const float gain = state->input_gain;
    const float proportional = input * gain;
    state->proportional_output = proportional;
    float integral = state->integral_state;
    const float integral_gain = state->integral_gain;
    integral = commissioning_vmla_f32(integral, integral_gain, proportional);
    __asm volatile ("" : "+t" (integral) : : "memory");
    const float saturation_error = state->saturation_error;
    integral = commissioning_vmla_f32(integral, saturation_error, 0.2f);
    state->integral_state = integral;
    __asm volatile ("" : "+t" (integral) : : "memory");
    float unlimited = proportional + integral;
    state->unlimited_output = unlimited;
    float limited = state->output_min;
    __asm volatile (
        "vcmpe.f32 %1, %0\n"
        "vmrs APSR_nzcv, fpscr\n"
        "bcc 1f\n"
        "vldr %0, [%2, #36]\n"
        "vcmpe.f32 %1, %0\n"
        "vmrs APSR_nzcv, fpscr\n"
        "bgt 1f\n"
        "vmov.f32 %0, %1\n"
        "1:\n"
        : "+&t" (limited)
        : "t" (unlimited), "r" (state)
        : "cc", "memory");
    state->limited_output = limited;
    __asm volatile ("" : "+t" (unlimited) : : "memory");
    state->saturation_error = limited - unlimited;
#else
    const float proportional = filter->input * filter->input_gain;
    filter->proportional_output = proportional;
#if defined(DAMIAO_DM4310)
    float integral = filter->integral_state;
    integral = commissioning_vmla_f32(
        integral, filter->integral_gain, proportional);
    integral = commissioning_vmla_f32(
        integral, filter->saturation_error, 0.2f);
    filter->integral_state = integral;
#else
    filter->integral_state += filter->integral_gain * proportional +
                              filter->saturation_error * 0.2f;
#endif
    const float unlimited = proportional + filter->integral_state;
    filter->unlimited_output = unlimited;
#if defined(DAMIAO_DM4310)
    float limited = filter->output_min;
    if (!(unlimited < filter->output_min)) {
        limited = filter->output_max;
        if (!(unlimited > filter->output_max)) {
            limited = unlimited;
        }
    }
#else
    float limited = unlimited;
    if (limited < filter->output_min) {
        limited = filter->output_min;
    } else if (limited > filter->output_max) {
        limited = filter->output_max;
    }
#endif
    filter->limited_output = limited;
    filter->saturation_error = limited - unlimited;
#endif
}

bool commissioning_flux_observer_init(CommissioningFluxObserver *observer,
                                       float inductance, float resistance,
                                       float sample_period)
{
    if (observer == NULL) {
        return false;
    }
#if defined(DAMIAO_DM4310)
    /* 0x25b2a..0x25b7a initializes selected fields, not the whole
     * scratch object. Inputs/errors and final R/flux are first written
     * by the sampling loop/observer step. */
    volatile CommissioningFluxObserver *const state = observer;
    state->inductance = inductance;
    state->resistance_adaptation_gain = 15.199999809265137f;
    state->flux_adaptation_gain = 0.10000000149011612f;
    state->estimated_current_d = 0.0f;
    state->estimated_current_q = 0.0f;
    state->sample_period = sample_period;
    float resistance_ratio;
    float flux_ratio;
    float inverse_inductance;
#if defined(__arm__) || defined(__thumb__)
    __asm volatile (
        "vdiv.f32 %0, %3, %4\n"
        "vdiv.f32 %1, %5, %4\n"
        "vdiv.f32 %2, %6, %4"
        : "=&t" (resistance_ratio), "=&t" (flux_ratio),
          "=&t" (inverse_inductance)
        : "t" (resistance), "t" (inductance), "t" (1.0e-6f),
          "t" (1.0f) : "memory");
#else
    resistance_ratio = resistance / inductance;
    flux_ratio = 1.0e-6f / inductance;
    inverse_inductance = 1.0f / inductance;
#endif
    state->base_resistance_over_inductance = resistance_ratio;
    state->base_flux_over_inductance = flux_ratio;
    state->base_inverse_inductance = inverse_inductance;
    state->resistance_adaptation_state = 0.0f;
    state->flux_adaptation_state = 0.0f;
    state->resistance_over_inductance = resistance_ratio;
    state->flux_over_inductance = flux_ratio;
    state->inverse_inductance = inverse_inductance;
#else
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
#endif
    return true;
}

void commissioning_flux_observer_step(CommissioningFluxObserver *observer)
{
#if defined(DAMIAO_DM4310)
    volatile CommissioningFluxObserver *const state = observer;
    const float inverse_inductance = state->inverse_inductance;
    const float voltage_d = state->voltage_d;
    const float speed = state->electrical_speed;
    const float previous_q = state->estimated_current_q;
    float d_derivative = commissioning_vmul_f32(inverse_inductance, voltage_d);
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("" : "+t" (d_derivative) : : "memory");
#endif
    const float dt = state->sample_period;
    const float resistance_ratio = state->resistance_over_inductance;
    d_derivative = commissioning_vmla_f32(d_derivative, speed, previous_q);
    const float decay = commissioning_vmls_f32(
        1.0f, resistance_ratio, dt);
    const float previous_d = state->estimated_current_d;
    const float estimated_d = commissioning_vmla_f32(
        commissioning_vmul_f32(d_derivative, dt), decay, previous_d);
    state->estimated_current_d = estimated_d;
    const float voltage_q = state->voltage_q;
    float q_derivative = commissioning_vmul_f32(inverse_inductance, voltage_q);
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("" : "+t" (q_derivative) : : "memory");
#endif
    const float flux_ratio = state->flux_over_inductance;
    q_derivative = commissioning_vmls_f32(
        q_derivative, speed, estimated_d);
    q_derivative = commissioning_vmls_f32(
        q_derivative, flux_ratio, speed);
    const float estimated_q = commissioning_vmla_f32(
        commissioning_vmul_f32(q_derivative, dt), decay, previous_q);
    state->estimated_current_q = estimated_q;

    const float error_d = state->measured_current_d - estimated_d;
    state->current_error_d = error_d;
    const float measured_q = state->measured_current_q;
    float resistance_error = commissioning_vmul_f32(estimated_d, error_d);
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("" : "+t" (resistance_error) : : "memory");
#endif
    const float error_q = measured_q - estimated_q;
    state->current_error_q = error_q;
    resistance_error = commissioning_vmla_f32(
        resistance_error, estimated_q, error_q);
    const float resistance_state = state->resistance_adaptation_state;
    const float speed_error = commissioning_vmul_f32(speed, error_q);
    const float next_resistance_state = commissioning_vmla_f32(
        resistance_state, dt, resistance_error);
    state->resistance_adaptation_state = next_resistance_state;
    const float next_flux_state = commissioning_vmla_f32(
        state->flux_adaptation_state, dt, speed_error);
    state->flux_adaptation_state = next_flux_state;
    const float base_resistance = state->base_resistance_over_inductance;
    const float resistance_gain = state->resistance_adaptation_gain;
    const float next_resistance = commissioning_vmls_f32(
        base_resistance, resistance_gain, next_resistance_state);
    state->resistance_over_inductance = next_resistance;
    const float base_flux = state->base_flux_over_inductance;
    const float flux_gain = state->flux_adaptation_gain;
    const float next_flux = commissioning_vmls_f32(
        base_flux, flux_gain, next_flux_state);
    state->flux_over_inductance = next_flux;
    const float inductance = state->inductance;
    state->resistance = commissioning_vmul_f32(inductance, next_resistance);
    __asm volatile ("" : : : "memory");
    state->flux_linkage = commissioning_vmul_f32(inductance, next_flux);
#else
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
#endif
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
#if defined(DAMIAO_DM4310)
    /* 0x26132/0x26198/0x261ac round each product in binary32 BEFORE
     * conversion to binary64 and addition to the cycle accumulators. */
    const float current_energy = commissioning_vmul_f32(
        phase_current, phase_current);
    regression->current_energy = commissioning_runtime_double_add(
        (double)current_energy, regression->current_energy);
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("" : : : "memory");
#endif
    const float cross = commissioning_vmul_f32(phase_current, mechanical_speed);
    regression->current_speed_cross = commissioning_runtime_double_add(
        (double)cross, regression->current_speed_cross);
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("" : : : "memory");
#endif
    const float speed_energy = commissioning_vmul_f32(
        mechanical_speed, mechanical_speed);
    regression->speed_energy = commissioning_runtime_double_add(
        (double)speed_energy, regression->speed_energy);
#else
    const double current = phase_current;
    const double speed = mechanical_speed;
    regression->current_energy += current * current;
    regression->current_speed_cross += current * speed;
    regression->speed_energy += speed * speed;
#endif
    ++regression->sample_count;
}

#if defined(DAMIAO_DM4310)
void commissioning_sine_regression_projection(
    const CommissioningSineRegression *regression, float torque_scale,
    float *phase, float *magnitude)
{
    const double normalization = commissioning_checked_double_sqrt(
        regression->current_energy * regression->speed_energy);
    *phase = (float)commissioning_factory_acos(
        regression->current_speed_cross / normalization);
    const float amplitude_ratio = (float)commissioning_checked_double_sqrt(
        regression->current_energy / regression->speed_energy);
    *magnitude = commissioning_vmul_f32(amplitude_ratio, torque_scale);
}

void commissioning_sine_response_motor_parameters(
    float phase, float magnitude, float excitation_frequency,
    bool frequency_in_hz,
    volatile float *rotor_inertia, volatile float *viscous_damping)
{
    const float tangent = dm4310_identification_tanf(phase);
    const float tangent_norm = commissioning_vmla_f32(1.0f, tangent, tangent);
    const float damping = magnitude / dm4310_sqrt_helper(tangent_norm);
    *viscous_damping = damping;
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("" : : "t" (damping) : "memory");
#endif
    float numerator = commissioning_vmul_f32(damping, tangent);
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("" : "+t" (numerator) : : "memory");
#endif
    const float angular_frequency = frequency_in_hz ?
        commissioning_vmul_f32(excitation_frequency, TWO_PI_F) :
        excitation_frequency;
    *rotor_inertia = numerator / angular_frequency;
}
#endif

bool commissioning_sine_regression_motor_parameters(
    const CommissioningSineRegression *regression,
    float torque_per_amp, float excitation_angular_frequency,
    float *rotor_inertia, float *viscous_damping)
{
    if ((regression == NULL) || (rotor_inertia == NULL) ||
        (viscous_damping == NULL)) {
        return false;
    }

#if defined(DAMIAO_DM4310)
    /* Preserve the factory's mixed-precision operation order.  The
     * accumulators and correlation are double, then the phase and magnitude
     * are rounded to float before the final tanf/sqrtf decomposition. */
    float phase;
    float magnitude;
    commissioning_sine_regression_projection(
        regression, torque_per_amp, &phase, &magnitude);
    commissioning_sine_response_motor_parameters(
        phase, magnitude, excitation_angular_frequency, false,
        rotor_inertia, viscous_damping);
#else
    const double energy_product = regression->current_energy *
                                  regression->speed_energy;
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
#endif
    return true;
}

bool commissioning_analyze_direction(float first_quarter_turn_delta,
                                     float electrical_travel,
                                     CommissioningDirectionResult *result)
{
    if (result == NULL) {
        return false;
    }
#if defined(DAMIAO_DM4310)
    const uint32_t estimated = (uint32_t)dm4310_runtime_round_to_int(
        electrical_travel / TWO_PI_F);
#else
    const uint32_t estimated =
        (uint32_t)(electrical_travel / TWO_PI_F + 0.5f);
#endif
    /* Original internal encoding: 1 reverses SPI counts, 2 keeps them. */
    result->direction_code = first_quarter_turn_delta > 0.0f ? 1.0f : 2.0f;
#if defined(DAMIAO_DM4310)
    result->pole_pairs = estimated;
#else
    result->pole_pairs = (uint8_t)estimated;
#endif
    return true;
}
