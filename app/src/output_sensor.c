#include "output_sensor.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "motor_math.h"
#include "app_profile.h"
#if defined(DAMIAO_DM4310)
#include "calibration_upload.h"
#include "runtime_compat.h"

void dm4310_runtime_errno_set_helper(uint32_t value);

_Static_assert(sizeof(OutputSensorState) == 0x48,
               "DM4310 output sensor state size");
_Static_assert(offsetof(OutputSensorState, centered_u) == 0x10,
               "DM4310 output sensor centered U offset");
_Static_assert(offsetof(OutputSensorState, center_u) == 0x14,
               "DM4310 output sensor U center offset");
_Static_assert(offsetof(OutputSensorState, centered_v) == 0x18,
               "DM4310 output sensor centered V offset");
_Static_assert(offsetof(OutputSensorState, center_v) == 0x1c,
               "DM4310 output sensor V center offset");
_Static_assert(offsetof(OutputSensorState, gain_v) == 0x28,
               "DM4310 output sensor gain offset");
_Static_assert(offsetof(OutputSensorState, phase_sine) == 0x2c,
               "DM4310 output sensor phase sine offset");
_Static_assert(offsetof(OutputSensorState, phase_cosine) == 0x30,
               "DM4310 output sensor phase cosine offset");
_Static_assert(offsetof(OutputSensorState, wrapped_angle) == 0x34,
               "DM4310 output sensor wrapped angle offset");
_Static_assert(offsetof(OutputSensorState, zero_offset) == 0x38,
               "DM4310 output sensor zero offset");
_Static_assert(offsetof(OutputSensorState, continuous_angle) == 0x3c,
               "DM4310 output sensor continuous angle offset");
_Static_assert(offsetof(OutputSensorState, previous_angle) == 0x40,
               "DM4310 output sensor previous angle offset");
_Static_assert(offsetof(OutputSensorState, revolutions) == 0x44,
               "DM4310 output sensor turn count offset");
#endif
/* Exact transforms used by output_sensor_lookup_and_unwrap@0x1fff987c.
 * This is the output-side analogue sin/cos encoder, distinct from the
 * SPI/DMA motor encoder. */
#define TWO_PI_F             0x1.921fb6p+2f
#define TABLE_INDEX_CENTER   2048.0f
#define ANGLE_TO_TABLE_INDEX 0x1.45f306p+9f
#define TABLE_COUNT_TO_RAD   0x1.921fb6p-10f
#define WRAP_THRESHOLD       5.5f
#define FILTER_SAMPLE_RATE   20000.0f

#if defined(DAMIAO_DM4310)
/* Factory 0x237f0, integer-only classification (no VFP exceptions). */
__attribute__((noipa))
static uint32_t factory_binary32_classify(uint32_t bits)
{
    const uint32_t exponent = (bits >> 23U) & UINT32_C(0xff);
    uint32_t classification = (bits << 9U) != 0U ? 4U : 0U;
    if (exponent != 0U) {
        classification |= 1U;
    }
    if (exponent == UINT32_C(0xff)) {
        classification |= 2U;
    }
    return classification == 1U ? 5U : classification;
}

static float vfp_multiply_add(float accumulator, float lhs, float rhs)
{
    __asm volatile ("vmla.f32 %0, %1, %2"
                    : "+t" (accumulator)
                    : "t" (lhs), "t" (rhs));
    return accumulator;
}

static float vfp_multiply_subtract(float accumulator, float lhs, float rhs)
{
    __asm volatile ("vmls.f32 %0, %1, %2"
                    : "+t" (accumulator)
                    : "t" (lhs), "t" (rhs));
    return accumulator;
}

float dm4310_output_atan2f(float y, float x)
{
    uint32_t y_bits;
    uint32_t x_bits;
    memcpy(&y_bits, &y, sizeof(y_bits));
    memcpy(&x_bits, &x, sizeof(x_bits));
    uint32_t y_abs = y_bits & UINT32_C(0x7fffffff);
    uint32_t x_abs = x_bits & UINT32_C(0x7fffffff);
    const bool exceptional_range =
        (y_abs < UINT32_C(0x0c000000)) ||
        (y_abs >= UINT32_C(0x7e800000)) ||
        (x_abs < UINT32_C(0x0c000000)) ||
        (x_abs >= UINT32_C(0x7e800000));
    bool normalized_special = false;

    /* atan2f@0x23be8 is the ARM runtime polynomial reached through the
     * factory SRAM veneer at 0x1fffa4fa.  Its exceptional-input prelude
     * normalizes zero/infinity pairs, propagates NaNs by addition, and scales
     * extreme finite operands before entering the same polynomial path. */
    if ((y_abs > UINT32_C(0x7f800000)) ||
        (x_abs > UINT32_C(0x7f800000))) {
        float result;
        /* Factory 0x242be adds y first: GCC can commute C addition and
         * change which NaN payload survives when FPSCR.DN is clear. */
        __asm volatile ("vadd.f32 %0, %1, %2"
                        : "=t" (result) : "t" (y), "t" (x));
        return result;
    }
    if ((y_abs | x_abs) == 0U) {
        x_bits |= UINT32_C(0x7f800000);
        memcpy(&x, &x_bits, sizeof(x));
        normalized_special = true;
    } else if ((y_abs == UINT32_C(0x7f800000)) &&
               (x_abs == UINT32_C(0x7f800000))) {
        y_bits &= UINT32_C(0xbfffffff);
        x_bits &= UINT32_C(0xbfffffff);
        memcpy(&y, &y_bits, sizeof(y));
        memcpy(&x, &x_bits, sizeof(x));
        normalized_special = true;
    } else if ((y_abs == UINT32_C(0x7f800000)) || (x_abs == 0U)) {
        y_bits |= UINT32_C(0x7f800000);
        x_bits &= UINT32_C(0x80000000);
        normalized_special = true;
    } else if ((x_abs == UINT32_C(0x7f800000)) || (y_abs == 0U)) {
        y_bits &= UINT32_C(0x80000000);
        x_bits |= UINT32_C(0x7f800000);
        normalized_special = true;
    }
    /* Other zero/infinity arms change only R0/R1 classification bits in
     * the factory; S16/S17 still hold the original arithmetic operands. */
    y_abs = y_bits & UINT32_C(0x7fffffff);
    x_abs = x_bits & UINT32_C(0x7fffffff);

    if (exceptional_range && !normalized_special &&
        ((((x_bits << 1U) ^ (y_bits << 1U)) &
          UINT32_C(0x80000000)) == 0U)) {
        const float scale = ((int32_t)(x_bits << 1U) >= 0) ?
            4294967296.0f : 2.3283064365386963e-10f;
        y *= scale;
        x *= scale;
        memcpy(&y_bits, &y, sizeof(y_bits));
        memcpy(&x_bits, &x, sizeof(x_bits));
    }
    y_abs = y_bits & UINT32_C(0x7fffffff);
    x_abs = x_bits & UINT32_C(0x7fffffff);

    const int32_t exponent_difference =
        (int32_t)(y_abs >> 23U) - (int32_t)(x_abs >> 23U);
    if (exponent_difference > 27) {
        return (y_bits & UINT32_C(0x80000000)) != 0U ?
            -0x1.921fb6p+0f : 0x1.921fb6p+0f;
    }
    if (exponent_difference < -26) {
        if ((x_bits & UINT32_C(0x80000000)) == 0U) {
            const float ratio = y / x;
            uint32_t ratio_bits;
            memcpy(&ratio_bits, &ratio, sizeof(ratio_bits));
            if (factory_binary32_classify(ratio_bits) == 4U) {
                (void)dm4310_runtime_force_underflow();
            }
            const uint32_t y_class = factory_binary32_classify(y_bits);
            const uint32_t x_class = factory_binary32_classify(x_bits);
            const uint32_t ratio_class = factory_binary32_classify(ratio_bits);
            if (((y_class == 4U) || (y_class == 5U)) &&
                ((x_class == 4U) || (x_class == 5U)) &&
                (ratio_class == 0U)) {
                dm4310_runtime_errno_set_helper(2U);
            }
            return ratio;
        }
        return (y_bits & UINT32_C(0x80000000)) != 0U ?
            -0x1.921fb6p+1f : 0x1.921fb6p+1f;
    }

    float high;
    float low;
    float numerator = y;
    float denominator = x;
    if (y_abs <= x_abs) {
        high = 0.0f;
        low = 0.0f;
        if ((x_bits & UINT32_C(0x80000000)) != 0U) {
            if ((y_bits & UINT32_C(0x80000000)) != 0U) {
                high = -3.140625f;
                low = -0.0009676535846665502f;
            } else {
                high = 3.140625f;
                low = 0.0009676535846665502f;
            }
        }
    } else {
        numerator = x;
        denominator = -y;
        if ((y_bits & UINT32_C(0x80000000)) != 0U) {
            high = -1.5703125f;
            low = -0.0004838267923332751f;
        } else {
            high = 1.5703125f;
            low = 0.0004838267923332751f;
        }
    }

    uint32_t numerator_bits;
    uint32_t denominator_bits;
    memcpy(&numerator_bits, &numerator, sizeof(numerator_bits));
    memcpy(&denominator_bits, &denominator, sizeof(denominator_bits));
    float ratio;
    if ((uint32_t)((denominator_bits - numerator_bits) << 1U) <
        UINT32_C(0x01000000)) {
        const bool same_sign =
            ((numerator_bits ^ denominator_bits) &
             UINT32_C(0x80000000)) == 0U;
        const float half = same_sign ? 0.5f : -0.5f;
        if (same_sign) {
            high += 0.463623046875f;
            low += 0.000024562124963267595f;
        } else {
            high -= 0.463623046875f;
            low -= 0.000024562124963267595f;
        }
        const float reduced_numerator =
            vfp_multiply_subtract(numerator, half, denominator);
        const float reduced_denominator =
            vfp_multiply_add(denominator, numerator, half);
        ratio = reduced_numerator / reduced_denominator;
    } else {
        ratio = numerator / denominator;
    }

    const float squared = ratio * ratio;
    float polynomial = -0.05607335641980171f;
    polynomial = vfp_multiply_add(
        0.10435592383146286f, squared, polynomial);
    polynomial = vfp_multiply_add(
        -0.14229224622249603f, squared, polynomial);
    polynomial = vfp_multiply_add(
        0.1999831348657608f, squared, polynomial);
    polynomial = vfp_multiply_add(
        -0.33333325386047363f, squared, polynomial);
    float result = vfp_multiply_add(
        low, ratio * squared, polynomial);
    result += ratio;
    result += high;
    return result;
}
#endif

static void apply_calibration(OutputSensorState *state,
                              const float calibration[4],
                              float initial_u, float initial_v)
{
    state->center_u = calibration[0];
    state->center_v = calibration[1];
    state->gain_v = calibration[2];
    motor_target_sincos(calibration[3], &state->phase_sine,
                      &state->phase_cosine);
    state->filtered_u = initial_u;
    state->filtered_v = initial_v;
#if defined(DAMIAO_DM4310)
    state->centered_u = initial_u - state->center_u;
    state->centered_v = (initial_v - state->center_v) * state->gain_v;
#endif
    state->wrapped_angle = 0.0f;
    state->previous_angle = 0.0f;
    state->continuous_angle = 0.0f;
    state->revolutions = 0;
}

#if defined(DAMIAO_DM4310)
void output_sensor_load_persistent_calibration(
    OutputSensorState *state, const float calibration[4])
{
    /* load_and_validate_calibration@0x22658 updates only these live fields.
     * In particular, a post-flash reload must not clear the filter, wrapped
     * angle or revolution state as output_sensor_init does at boot. */
    /* Factory reloads Flash floats and immediately VSTRs live fields. */
    __asm volatile (
        "vldr s0, [%0]\n\t"
        "vstr s0, [%1, #20]\n\t"
        "vldr s0, [%0, #4]\n\t"
        "vstr s0, [%1, #28]\n\t"
        "vldr s0, [%0, #8]\n\t"
        "vstr s0, [%1, #40]"
        : : "r" (calibration), "r" (state) : "s0", "memory");
    motor_target_sincos(calibration[3], &state->phase_sine,
                        &state->phase_cosine);
}
#endif

static float decode_filtered_angle(OutputSensorState *state)
{
    const float uncorrected = output_sensor_uncorrected_angle(state);

#if defined(DAMIAO_DM4310)
    /* 989c/98a0 read the shared SRAM literals in this order.  Keep the
     * center live across lookup, as the factory retains it in S3. */
    const float center = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFF9938UL, 0x1FFF92FCUL);
    const float index_scale = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFF9934UL, 0x1FFF92F8UL);
    int32_t index = (int32_t)vfp_multiply_add(
        center, uncorrected, index_scale);
    if (index < 0) {
        index += 4096;
    }
    if (index >= 4096) {
        index -= 4096;
    }
    /* output_sensor_lookup_and_unwrap@0x1fff987c dereferences the installed
     * table unconditionally.  The startup loader always installs the copied
     * 8 KiB table, including its erased/invalid-status paths. */
    const volatile uint16_t *const table =
        (const volatile uint16_t *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF993CUL, 0x1FFF9300UL);
    const uint16_t sample = table[index];
    const uint32_t positive_wrap =
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9944UL, 0x1FFF9308UL);
    const float centered_sample = (float)sample - center;
    const float count_scale = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFF9940UL, 0x1FFF9304UL);
    const float wrapped =
        centered_sample * count_scale;
#endif

#if defined(DAMIAO_DM4310)
    volatile OutputSensorState *const live = state;
    live->wrapped_angle = wrapped;
    const float delta = wrapped - live->previous_angle;
    uint32_t delta_bits;
    memcpy(&delta_bits, &delta, sizeof(delta_bits));
    if ((int32_t)delta_bits > (int32_t)positive_wrap) {
        live->revolutions = (int32_t)((uint32_t)live->revolutions - 1U);
    }
    const uint32_t negative_wrap =
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9948UL, 0x1FFF930CUL);
    if (delta_bits > negative_wrap) {
        live->revolutions = (int32_t)((uint32_t)live->revolutions + 1U);
    }
#else
    state->wrapped_angle = wrapped;
    const float delta = wrapped - state->previous_angle;
    if (delta > WRAP_THRESHOLD) {
        --state->revolutions;
    } else if (delta < -WRAP_THRESHOLD) {
        ++state->revolutions;
    }
#endif

#if defined(DAMIAO_DM4310)
    live->previous_angle = wrapped;
    const int32_t turns = live->revolutions;
    const float turn_scale = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFF994CUL, 0x1FFF9310UL);
    const float unwrapped = vfp_multiply_add(
        wrapped, (float)turns, turn_scale);
    const float continuous = unwrapped - live->zero_offset;
    live->continuous_angle = continuous;
    return continuous;
#else
    state->previous_angle = wrapped;
    state->continuous_angle = wrapped +
        (float)state->revolutions * TWO_PI_F - state->zero_offset;
    return state->continuous_angle;
#endif
}

#if defined(DAMIAO_DM4310)
float output_sensor_lookup_and_unwrap(OutputSensorState *state)
{
    return decode_filtered_angle(state);
}
#endif

void output_sensor_init(OutputSensorState *state,
                        const float calibration[4],
                        const CorrectionTableEntry correction_table[CORRECTION_TABLE_COUNT],
                        float zero_offset,
                        float initial_u, float initial_v)
{
#if !defined(DAMIAO_DM4310)
    memset(state, 0, sizeof(*state));
#endif
    (void)correction_table;
#if defined(DAMIAO_DM4310)
    ((volatile OutputSensorState *)state)->filtered_u = initial_u;
    ((volatile OutputSensorState *)state)->filtered_v = initial_v;
#else
    state->filtered_u = initial_u;
    state->filtered_v = initial_v;
#endif
    /* load_and_validate_calibration@0x22658 copies these four per-device
     * words verbatim.  Validation belongs to the commissioning producer,
     * not to the startup consumer. */
#if defined(DAMIAO_DM4310)
    (void)calibration;
    (void)zero_offset;
    volatile OutputSensorState *const live = state;
    const float center_u = live->center_u;
    live->centered_u = initial_u - center_u;
    const float center_v = live->center_v;
    const float gain_v = live->gain_v;
    live->centered_v = (initial_v - center_v) * gain_v;
#else
    apply_calibration(state, calibration, initial_u, initial_v);
    /* The 0x36000 loader has already applied the original NaN-only rule. */
    state->zero_offset = zero_offset;
#endif

    /* Captured state is a first-order 50 Hz filter at the 20 kHz ADC rate. */
#if !defined(DAMIAO_DM4310)
    const float sensor_bandwidth = 50.0f;
    state->filter_previous_weight = FILTER_SAMPLE_RATE /
        (FILTER_SAMPLE_RATE + sensor_bandwidth * TWO_PI_F);
    state->filter_new_weight = 1.0f - state->filter_previous_weight;
#endif
    /* validate_current_sensors@0x24ec0 decodes the averaged U/V sample once
     * before the runtime ADC trigger is enabled. */
#if defined(DAMIAO_DM4310)
    dm4310_output_sensor_helper(state);
#else
    decode_filtered_angle(state);
#endif
}

#if defined(DAMIAO_DM4310)
static float update_sensor_sample(OutputSensorState *state,
                                  uint16_t raw_u, uint16_t raw_v,
                                  const volatile uint16_t *fixed_raw)
#else
void output_sensor_update(OutputSensorState *state,
                          uint16_t raw_u, uint16_t raw_v)
#endif
{
#if defined(DAMIAO_DM4310)
    if (fixed_raw != NULL) {
        raw_u = fixed_raw[0];
    }
    volatile OutputSensorState *const live = state;
    float filtered_u = live->filtered_u;
    const float old_weight = live->filter_previous_weight;
    __asm volatile ("vmul.f32 %0, %0, %1"
                    : "+t" (filtered_u) : "t" (old_weight) : "memory");
    float raw_u_float;
    __asm volatile (
        "vmov %0, %1\n"
        "vcvt.f32.u32 %0, %0"
        : "=t" (raw_u_float) : "r" ((uint32_t)raw_u) : "memory");
    const float new_weight = live->filter_new_weight;
    __asm volatile ("vmla.f32 %0, %1, %2"
                    : "+t" (filtered_u)
                    : "t" (raw_u_float), "t" (new_weight) : "memory");
    live->filtered_u = filtered_u;
    float filtered_v = live->filtered_v;
    if (fixed_raw != NULL) {
        raw_v = fixed_raw[1];
    }
    float raw_v_float;
    __asm volatile (
        "vmul.f32 %0, %0, %2\n"
        "vmov %1, %3\n"
        "vcvt.f32.u32 %1, %1\n"
        "vmla.f32 %0, %1, %4"
        : "+t" (filtered_v), "=&t" (raw_v_float)
        : "t" (old_weight), "r" ((uint32_t)raw_v), "t" (new_weight)
        : "memory");
    live->filtered_v = filtered_v;
    live->centered_u = filtered_u - live->center_u;
    const float center_v = live->center_v;
    float centered_v;
    __asm volatile ("vsub.f32 %0, %1, %2"
                    : "=t" (centered_v)
                    : "t" (filtered_v), "t" (center_v) : "memory");
    const float gain_v = live->gain_v;
    __asm volatile ("vmul.f32 %0, %0, %1"
                    : "+t" (centered_v) : "t" (gain_v) : "memory");
    live->centered_v = centered_v;
#else
    state->filtered_u = state->filtered_u * state->filter_previous_weight +
                        (float)raw_u * state->filter_new_weight;
    state->filtered_v = state->filtered_v * state->filter_previous_weight +
                        (float)raw_v * state->filter_new_weight;
#endif

#if defined(DAMIAO_DM4310)
    return dm4310_output_sensor_helper(state);
#else
    decode_filtered_angle(state);
#endif
}

#if defined(DAMIAO_DM4310)
void output_sensor_update(OutputSensorState *state,
                          uint16_t raw_u, uint16_t raw_v)
{
    update_sensor_sample(state, raw_u, raw_v, NULL);
}

float output_sensor_update_control_irq(OutputSensorState *state,
                                       const volatile uint16_t *raw)
{
    return update_sensor_sample(state, 0U, 0U, raw);
}
#endif

float output_sensor_normalize_startup(OutputSensorState *state)
{
    /* validate_current_sensors normalizes the first zero-corrected output
     * angle and adjusts the matching turn count before aligning the SPI
     * encoder's multi-turn state. */
#if defined(DAMIAO_DM4310)
    volatile OutputSensorState *const live = state;
    float angle = live->continuous_angle;
    uint32_t angle_bits;
    memcpy(&angle_bits, &angle, sizeof(angle_bits));
    if ((int32_t)angle_bits > (int32_t)UINT32_C(0x40490fdb)) {
        angle -= TWO_PI_F;
        live->continuous_angle = angle;
        live->revolutions = (int32_t)((uint32_t)live->revolutions - 1U);
    }
    memcpy(&angle_bits, &angle, sizeof(angle_bits));
    if (angle_bits > UINT32_C(0xc0490fdb)) {
        angle += TWO_PI_F;
        live->continuous_angle = angle;
        live->revolutions = (int32_t)((uint32_t)live->revolutions + 1U);
    }
    return angle;
#else
    if (state->continuous_angle > 3.1415927410125732f) {
        state->continuous_angle -= TWO_PI_F;
        --state->revolutions;
    }
    if (state->continuous_angle < -3.1415927410125732f) {
        state->continuous_angle += TWO_PI_F;
        ++state->revolutions;
    }
    return state->continuous_angle;
#endif
}

void output_sensor_set_zero_offset(OutputSensorState *state,
                                   float zero_offset)
{
    state->zero_offset = zero_offset;
}

float output_sensor_zero_current(OutputSensorState *state)
{
#if defined(DAMIAO_DM4310)
    /* FE@0x1fff8a0e persists the current wrapped table angle and resets the
     * analogue encoder's turn counter.  continuous_angle is cleared by the
     * command handler and recomputed on the next ADC sample. */
    volatile OutputSensorState *const live = state;
    /* FE cleared the turn word while folding motor turns, before the
     * motor-angle reload; do not publish it a second time here. */
    const float angle = live->wrapped_angle;
    live->zero_offset = angle;
    return angle;
#else
    state->zero_offset += state->continuous_angle;
    return state->zero_offset;
#endif
}

bool output_sensor_set_calibration(OutputSensorState *state,
                                   const float calibration[4],
                                   float initial_u, float initial_v)
{
    if ((state == NULL) || (calibration == NULL)) {
        return false;
    }
    apply_calibration(state, calibration, initial_u, initial_v);
    return true;
}

float output_sensor_uncorrected_angle(const OutputSensorState *state)
{
#if defined(DAMIAO_DM4310)
    const volatile OutputSensorState *const live = state;
    const float v = live->centered_v;
    const float u = live->centered_u;
    const float cosine = live->phase_cosine;
    float atan_y = v;
    __asm volatile ("vmls.f32 %0, %1, %2"
                    : "+t" (atan_y) : "t" (u), "t" (cosine) : "memory");
    const float atan_x = u * live->phase_sine;
    return dm4310_atan2_helper(atan_y, atan_x);
#else
    const float u = state->filtered_u - state->center_u;
    const float v = (state->filtered_v - state->center_v) * state->gain_v;
    return atan2f(v - u * state->phase_cosine,
                  u * state->phase_sine);
#endif
}
