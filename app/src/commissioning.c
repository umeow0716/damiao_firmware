#include "commissioning.h"
#include "output_sensor.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "app_profile.h"
#include "app_state.h"
#include "board_sampling_timer.h"
#include "debug_console.h"
#include "motor_math.h"
#include "platform.h"
#include "runtime_compat.h"
#include "sensor_calibration.h"

#define TWO_PI_F 0x1.921fb6p+2f
#define PI_OVER_TWO_F 0x1.921fb6p+0f
#define ALIGNMENT_VOLTAGE_D 0.1f
#define DIRECTION_ANGLE_STEP 0.002f
#define DIRECTION_LOCK_STEPS 10000U
#define ALIGNMENT_LOCK_STEPS 20000U
#define ALIGNMENT_INNER_STEPS 40U
#define ALIGNMENT_POINTS_PER_PAIR 256U
#define OUTPUT_CALIBRATION_VOLTAGE_Q 0.2f
#define OUTPUT_CALIBRATION_REPORT_DIVIDER 20U
#define OUTPUT_TABLE_POINTS 256U
#define OUTPUT_TABLE_POINTS_PER_MOTOR_TURN 4096U
#define INV_SQRT3_F 0.5773502588272095f
#define SQRT3_F 0x1.bb67aep+0f
#define CURRENT_FULL_SCALE_A APP_PROFILE_CURRENT_FULL_SCALE_A
#define IDENTIFICATION_LOCK_STEPS 6284U
#define IDENTIFICATION_ELECTRICAL_STEPS 60000U
#define IDENTIFICATION_RLS_START_STEP 20000U
#define IDENTIFICATION_TARGET_RAMP_STEPS 10000U
#define IDENTIFICATION_TARGET_RAMP_DIVIDER 100U
#define IDENTIFICATION_TARGET_UPDATE_DIVIDER 10U
/* The fixed-layout routine multiplies a 100 Hz target by the phase base step,
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

#define COMMISSIONING_WORK_SECTION __attribute__((section(".commissioning_work")))

typedef struct
{
    float word[10];
} RuntimeDriveState;

static RuntimeDriveState runtime_drive_d __attribute__((
    section(MEMORY_LAYOUT_SECTION(".runtime_drive_d", ".alternate_runtime_drive_d"))));
static RuntimeDriveState runtime_drive_q __attribute__((
    section(MEMORY_LAYOUT_SECTION(".runtime_drive_q", ".alternate_runtime_drive_q"))));
static RuntimeDriveState runtime_speed_loop __attribute__((
    section(MEMORY_LAYOUT_SECTION(".runtime_speed_loop", ".alternate_runtime_speed_loop"))));
static RuntimeDriveState runtime_position_loop __attribute__((
    section(MEMORY_LAYOUT_SECTION(".runtime_position_loop", ".alternate_runtime_position_loop"))));
static volatile uint32_t runtime_parameter_cache[24]
    __attribute__((section(".runtime_parameter_cache")));

static float __attribute__((noinline)) offset_report_error(float error)
{
    /* offset adjusts once, with mutually exclusive
     * signed/unsigned raw-word predicates rather than float comparisons. */
    uint32_t bits;
    memcpy(&bits, &error, sizeof(bits));
    if ((int32_t)bits > (int32_t)UINT32_C(0x40490fdb))
    {
        error -= TWO_PI_F;
    }
    else if (bits > UINT32_C(0xc0490fdb))
    {
        error += TWO_PI_F;
    }
    return error;
}

static float alignment_unwrap(float current, float previous_unwrapped)
{
    float adjusted = current;
    /* Compare the subtraction's raw word rather than VFP flags.  Preserve the
     * signed/unsigned ordering, including NaN operands. */
    float difference = adjusted - previous_unwrapped;
    uint32_t bits;
    memcpy(&bits, &difference, sizeof(bits));
    if ((int32_t)bits > (int32_t)0x40800000U)
    {
        adjusted -= TWO_PI_F;
    }
    difference = adjusted - previous_unwrapped;
    memcpy(&bits, &difference, sizeof(bits));
    if (bits > 0xC0800000U)
    {
        adjusted += TWO_PI_F;
    }
    return adjusted;
}

static uint32_t crc32_mpeg2(const uint8_t *data, size_t length)
{
    uint32_t crc = UINT32_MAX;
    for (size_t index = 0U; index < length; ++index)
    {
        crc ^= (uint32_t)data[index] << 24U;
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            crc = (crc & 0x80000000UL) != 0U ? (crc << 1U) ^ 0x04C11DB7UL : crc << 1U;
        }
    }
    return crc;
}

static void put_float(uint8_t *destination, float value)
{
    memcpy(destination, &value, sizeof(value));
}

static void send_alignment_sample(float commanded_mechanical_angle, float measured_angle,
                                  float error, uint16_t raw, uint16_t sample_index)
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

static void send_sensor_sample(uint8_t marker, float first, float second, float third,
                               uint16_t raw_u, uint16_t raw_v)
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
    /* extrema emits three zero float slots and the frame CRC. */
    send_sensor_sample('h', 0.0f, 0.0f, 0.0f, raw_u, raw_v);
}

typedef struct
{
    float angle;
    float sine;
    float cosine;
} IdentificationCurrentFrame;

static float identification_electrical_angle(const MotorConfig *config);

static void identification_drive(float voltage_d, float voltage_q,
                                 const IdentificationCurrentFrame *frame, bool mechanical_stage)
{
    float alpha;
    float beta;
    /* Apply inverse Park with the existing IRQ frame; this output path does
     * not wrap the angle or recompute sine and cosine. */
    if (mechanical_stage)
    {
        /* Excitation/coast use two VMULs before the cross terms. */
        __asm volatile("vmul.f32 %0, %2, %3\n"
                       "vmul.f32 %1, %4, %3\n"
                       "vmls.f32 %0, %4, %5\n"
                       "vmla.f32 %1, %2, %5\n"
                       : "=&t"(alpha), "=&t"(beta)
                       : "t"(frame->cosine), "t"(voltage_d), "t"(frame->sine), "t"(voltage_q));
    }
    else
    {
        __asm volatile("vmul.f32 %0, %2, %3\n"
                       "vmls.f32 %0, %4, %5\n"
                       "vmul.f32 %1, %4, %3\n"
                       "vmla.f32 %1, %2, %5\n"
                       : "=&t"(alpha), "=&t"(beta)
                       : "t"(frame->cosine), "t"(voltage_d), "t"(frame->sine), "t"(voltage_q));
    }
    svpwm_helper(alpha, beta);
}

static DirectQuadrature commissioning_current_dq(const PlatformCommissioningSample *sample,
                                                 IdentificationCurrentFrame *frame,
                                                 bool parallel_products, float *scale_snapshot)
{
    float sine;
    float cosine;
    float beta = sample->current_u;
    __asm volatile("vmla.f32 %0, %1, %2" : "+t"(beta) : "t"(sample->current_v), "t"(2.0f));
    beta *= 0x1.279a74p-1f;
    __asm volatile("" : "+t"(beta) : : "memory");
    if (*((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) != 0U)
    {
        *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) = 0U;
        frame->angle = identification_electrical_angle(NULL);
        wrap_helper(&frame->angle, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
        motor_target_sincos(frame->angle, &frame->sine, &frame->cosine);
    }
    sine = frame->sine;
    cosine = frame->cosine;
    DirectQuadrature result;
    /* VMLA/VMLS preserve both binary32 intermediate rounding and operand order. */
    if (scale_snapshot != NULL)
    {
        /* Flux snapshots the live scale between
         * the two seed products and their cross terms. */
        float scale;
        __asm volatile("vmul.f32 %0, %3, %4\n"
                       "vmul.f32 %1, %3, %6\n"
                       "vldr %2, [%7]\n"
                       "vmla.f32 %0, %5, %6\n"
                       "vmls.f32 %1, %5, %4\n"
                       : "=&t"(result.d), "=&t"(result.q), "=&t"(scale)
                       : "t"(cosine), "t"(sample->current_u), "t"(sine), "t"(beta),
                         "r"(MEMORY_LAYOUT_ADDRESS(0x1FFFF244UL, 0x1FFFF1D0UL))
                       : "memory");
        *scale_snapshot = scale;
    }
    else if (parallel_products)
    {
        __asm volatile("vmul.f32 %0, %2, %3\n"
                       "vmul.f32 %1, %2, %5\n"
                       "vmla.f32 %0, %4, %5\n"
                       "vmls.f32 %1, %4, %3\n"
                       : "=&t"(result.d), "=&t"(result.q)
                       : "t"(cosine), "t"(sample->current_u), "t"(sine), "t"(beta));
    }
    else
    {
        __asm volatile("vmul.f32 %0, %2, %3\n"
                       "vmla.f32 %0, %4, %5\n"
                       "vmul.f32 %1, %2, %5\n"
                       "vmls.f32 %1, %4, %3\n"
                       : "=&t"(result.d), "=&t"(result.q)
                       : "t"(cosine), "t"(sample->current_u), "t"(sine), "t"(beta));
    }
    return result;
}

static float commissioning_reduce_float_angle(float travel);

static float identification_target_phase(float frequency, uint32_t count)
{
    float phase;
    __asm volatile("vmul.f32 %0, %1, %2\n"
                   "vmul.f32 %0, %0, %3\n"
                   : "=&t"(phase)
                   : "t"(frequency), "t"((float)count), "t"(IDENTIFICATION_PHASE_BASE_STEP));
    return phase;
}

static float identification_flux_speed_window(float previous_speed)
{
    const uint32_t poles =
        *((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    const float delta =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL));
    float raw_speed;
    float pole_count;
    __asm volatile("vmov %1, %3\n"
                   "vcvt.f32.u32 %1, %1\n"
                   "vmul.f32 %0, %2, %1\n"
                   "vmul.f32 %0, %0, %4\n"
                   : "=&t"(raw_speed), "=&t"(pole_count)
                   : "t"(delta), "r"(poles), "t"(1000.0f)
                   : "memory");
    const float previous_weight =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0F0UL, 0x1FFFF07CUL));
    float speed;
    __asm volatile("vmul.f32 %0, %1, %2"
                   : "=t"(speed)
                   : "t"(previous_weight), "t"(previous_speed)
                   : "memory");
    const float new_weight =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0F4UL, 0x1FFFF080UL));
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
    __asm volatile("vmla.f32 %0, %1, %2"
                   : "+t"(speed)
                   : "t"(raw_speed), "t"(new_weight)
                   : "memory");
    return speed;
}

static bool identification_fit_sign_accepted(float value)
{
    uint32_t accepted;
    __asm volatile("vcmpe.f32 %1, #0.0\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "mov.w %0, #1\n"
                   "it cc\n"
                   "movcc %0, #0"
                   : "=r"(accepted)
                   : "t"(value)
                   : "cc");
    return accepted != 0U;
}

static bool identify_electrical_parameters(float electrical_angle, float *resistance,
                                           float *inductance, PlatformCommissioningSample *sample,
                                           IdentificationCurrentFrame *frame,
                                           float *retained_voltage_d)
{
    CommissioningRls2 estimator;
    (void)electrical_angle;
    float target_amplitude = 0.0f;
    float target_current = 0.0f;
    float applied_voltage = 0.0f;
    /* Read the cached current scale before target current; both are runtime
     * values rather than immutable profile constants. */
    const volatile float *const parameter_cache =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    const float current_scale = parameter_cache[2];
    const float target_current_amplitude = parameter_cache[5];
    float amplitude_step;
    float amplitude_divisor;
    __asm volatile("vmul.f32 %1, %2, %3\n"
                   "vdiv.f32 %0, %4, %1"
                   : "=&t"(amplitude_step), "=&t"(amplitude_divisor)
                   : "t"(current_scale), "t"((float)IDENTIFICATION_TARGET_RAMP_DIVIDER),
                     "t"(target_current_amplitude)
                   : "memory");
    commissioning_rls2_init(&estimator);

    /* Clear conversions after the ramp snapshot and estimator initialization,
     * immediately before entering the sample loop. */
    platform_commissioning_finish_sample();
    for (uint32_t count = 1U; count <= IDENTIFICATION_ELECTRICAL_STEPS; ++count)
    {
        platform_commissioning_wait_identification_sample();
        if ((count <= IDENTIFICATION_TARGET_RAMP_STEPS) &&
            ((count % IDENTIFICATION_TARGET_RAMP_DIVIDER) == 0U))
        {
            __asm volatile("vadd.f32 %0, %0, %1"
                           : "+t"(target_amplitude)
                           : "t"(amplitude_step)
                           : "memory");
        }
        if ((count % IDENTIFICATION_TARGET_UPDATE_DIVIDER) == 0U)
        {
            const float unwrapped_phase =
                identification_target_phase(IDENTIFICATION_TARGET_FREQUENCY_HZ, count);
            float phase = commissioning_reduce_float_angle(unwrapped_phase);
            wrap_helper(&phase, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
            float sine;
            float cosine;
            motor_target_sincos(phase, &sine, &cosine);
            (void)cosine;
            __asm volatile("vmul.f32 %0, %1, %2"
                           : "=t"(target_current)
                           : "t"(target_amplitude), "t"(sine)
                           : "memory");
        }

        if (!platform_commissioning_read_identification_sample(sample, true))
        {
            return false;
        }
        const DirectQuadrature current = commissioning_current_dq(sample, frame, false, NULL);
        if (count > IDENTIFICATION_RLS_START_STEP)
        {
            const float regression_scale = parameter_cache[2];
            float regression_current;
            __asm volatile("vmul.f32 %0, %1, %2"
                           : "=t"(regression_current)
                           : "t"(regression_scale), "t"(current.d));
            volatile CommissioningRls2 *const regression_state = &estimator;
            regression_state->measurement = regression_current;
            const float projected_bus =
                *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF170UL, 0x1FFFF0FCUL));
            float regression_voltage;
            __asm volatile("vmul.f32 %0, %1, %2"
                           : "=t"(regression_voltage)
                           : "t"(projected_bus), "t"(applied_voltage));
            regression_state->applied_voltage = regression_voltage;
            rls2_helper(&estimator);
        }
        /* Reload gain and the voltage bound on every tick. */
        float current_error = target_current - current.d;
        __asm volatile("" : "+t"(current_error) : : "memory");
        const float current_gain = parameter_cache[6];
        const float voltage_limit = parameter_cache[7];
        float requested_voltage;
        float applied_voltage_q;
        /* Compute both axes before clamping only D.  Q is deliberately not
         * clamped; retain VNMUL's NaN and sign rules. */
        __asm volatile("vmul.f32 %0, %2, %3\n"
                       "vnmul.f32 %1, %2, %4"
                       : "=&t"(requested_voltage), "=&t"(applied_voltage_q)
                       : "t"(current_gain), "t"(current_error), "t"(current.q));
        applied_voltage = clamp_helper(requested_voltage, -voltage_limit, voltage_limit);
        identification_drive(applied_voltage, applied_voltage_q, frame, false);
        platform_commissioning_finish_sample();
    }
    platform_commissioning_delay_us(5000U);
    /* Neutralize stationary SVPWM directly, without wrap or sincos work. */
    svpwm_helper(0.0f, 0.0f);
    /* Publish both fits before checking their signs.  The carry test rejects
     * negative values but retains unordered values. */
    volatile float *const staging =
        (volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    /* Reload the live sample period after the neutral update. */
    const float fitted_sample_period = parameter_cache[3];
    const float fitted_inductance = fitted_sample_period / estimator.coefficient_b;
    staging[18] = fitted_inductance;
    const float fitted_resistance = (1.0f - estimator.coefficient_a) / estimator.coefficient_b;
    staging[17] = fitted_resistance;
    *inductance = fitted_inductance;
    *resistance = fitted_resistance;
    /* Keep the applied voltage live to seed the flux D-axis command. */
    *retained_voltage_d = applied_voltage;
    if (!identification_fit_sign_accepted(fitted_resistance))
    {
        return false;
    }
    return identification_fit_sign_accepted(fitted_inductance);
}

static float identification_electrical_angle_from_poles(uint32_t poles)
{
    const float position =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF198UL, 0x1FFFF124UL));
    const float turns_per_radian =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0E0UL, 0x1FFFF06CUL));
    float angle = position * (float)poles;
    float turns = position * turns_per_radian;
    __asm volatile("vcvt.u32.f32 %1, %1\n"
                   "vcvt.f32.u32 %1, %1\n"
                   "vmls.f32 %0, %1, %2\n"
                   : "+t"(angle), "+t"(turns)
                   : "t"(TWO_PI_F));
    const float offset =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF09CUL, 0x1FFFF028UL));
    return angle + offset;
}

static float identification_electrical_angle(const MotorConfig *config)
{
    (void)config;
    return identification_electrical_angle_from_poles(
        *((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL)));
}

static bool identification_filter_pair_init(const MotorConfig *config, float bus_voltage,
                                            CommissioningIdentificationFilter *axis_d,
                                            CommissioningIdentificationFilter *axis_q,
                                            float *retained_current_scale)
{
    if ((config == NULL) || (axis_d == NULL) || (axis_q == NULL))
    {
        return false;
    }
    float proportional_gain = 0.0f;
    float integral_gain = 0.0f;
    /* Mechanical entry derives both filters without
     * the UV gating used by the distinct control-parameter helper. */
    (void)bus_voltage;
    const volatile float *const staging =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    const volatile float *const cache =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    const float resistance = staging[17];
    const float inductance = staging[18];
    float resistance_ratio = resistance / inductance;
    __asm volatile("" : "+t"(resistance_ratio) : : "memory");
    const float sample_period = cache[3];
    integral_gain = resistance_ratio * sample_period;
    const float bandwidth = staging[24];
    const float current_scale = cache[2];
    *retained_current_scale = current_scale;
    float scaled_bandwidth = (bandwidth * current_scale) * inductance;
    __asm volatile("" : "+t"(scaled_bandwidth) : : "memory");
    const float sampled_bus =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF16CUL, 0x1FFFF0F8UL));
    proportional_gain = (scaled_bandwidth / sampled_bus) * SQRT3_F;
    *axis_d = (CommissioningIdentificationFilter){
        .integral_gain = integral_gain,
        .input_gain = proportional_gain,
        .output_min = -IDENTIFICATION_FILTER_D_LIMIT,
        .output_max = IDENTIFICATION_FILTER_D_LIMIT,
    };
    *axis_q = (CommissioningIdentificationFilter){
        .integral_gain = integral_gain,
        .input_gain = proportional_gain,
        .output_min = -IDENTIFICATION_FILTER_Q_LIMIT,
        .output_max = IDENTIFICATION_FILTER_Q_LIMIT,
    };
    return true;
}

void commissioning_configure_runtime_drive_states(const MotorConfig *config, float bus_voltage)
{
    (void)config;
    const volatile float *const staging =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    const volatile float *const cache =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    const float voltage = bus_voltage;
    const float undervoltage = staging[0];
    /* derive_control_parameters updates only words 0 and 1 of both
     * fixed states.  In particular UgQ must not reset accumulated state. */
    volatile RuntimeDriveState *const d = &runtime_drive_d;
    volatile RuntimeDriveState *const q = &runtime_drive_q;
    if (voltage > undervoltage)
    {
        const float bandwidth = cache[14];
        const float inductance = cache[18];
        const float current_scale = cache[2];
        const float proportional_gain =
            (((bandwidth * inductance) * current_scale) / voltage) * SQRT3_F;
        d->word[0] = proportional_gain;
        q->word[0] = proportional_gain;
        const float resistance = cache[17];
        float resistance_ratio;
        __asm volatile("vdiv.f32 %0, %1, %2"
                       : "=t"(resistance_ratio)
                       : "t"(resistance), "t"(inductance)
                       : "memory");
        const float period = cache[3];
        const float integral_gain = resistance_ratio * period;
        d->word[1] = integral_gain;
        q->word[1] = integral_gain;
    }
    else
    {
        d->word[0] = 0.0f;
        q->word[0] = 0.0f;
        d->word[1] = 0.0f;
        q->word[1] = 0.0f;
    }
}

void commissioning_initialize_runtime_drive_states(void)
{
    memset(&runtime_drive_d, 0, sizeof(runtime_drive_d));
    memset(&runtime_drive_q, 0, sizeof(runtime_drive_q));
    memset(&runtime_speed_loop, 0, sizeof(runtime_speed_loop));
    memset(&runtime_position_loop, 0, sizeof(runtime_position_loop));
}

void commissioning_initialize_scatter_defaults(void)
{
    /* Initialize drive and loop objects before application main. */
    const RuntimeDriveState defaults[4] = {
        {.word = {[0] = 0.8f, [1] = 0.001f, [8] = -1.0f, [9] = 1.0f}},
        {.word = {[0] = 0.8f, [1] = 0.001f, [8] = -1.0f, [9] = 1.0f}},
        {.word = {[0] = 0.4f, [1] = 0.002f, [8] = -1.0f, [9] = 1.0f}},
        {.word = {[0] = 2.0f, [8] = -600.0f, [9] = 600.0f}},
    };
    volatile RuntimeDriveState *const states[4] = {
        &runtime_drive_d,
        &runtime_drive_q,
        &runtime_speed_loop,
        &runtime_position_loop,
    };
    for (unsigned int state = 0U; state < 4U; ++state)
    {
        for (unsigned int word = 0U; word < 10U; ++word)
        {
            states[state]->word[word] = defaults[state].word[word];
        }
    }
}

void commissioning_reset_runtime_loop_states(volatile float *speed, float value)
{
    /* reset_control_state clears only the dynamic words 2, 4, 5, 6
     * and 7 of both outer-loop states.  Gains, limits and configuration
     * words survive every disabled/faulted 20 kHz tick. */
    speed[2] = value;
    speed[4] = value;
    speed[7] = value;
    speed[5] = value;
    speed[6] = value;
    volatile float *const position = (volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA12CUL, 0x1FFFA164UL);
    position[2] = value;
    position[4] = value;
    position[7] = value;
    position[5] = value;
    position[6] = value;
}

static inline float commissioning_ordered_multiply(float left, float right)
{
    float result;
    __asm volatile("vmul.f32 %0, %1, %2" : "=t"(result) : "t"(left), "t"(right));
    return result;
}

void commissioning_derive_runtime_controller_states(void)
{
    /* This SRAM entry reads the fixed staging image rather than g_app.config.
     * Preserve its paired stores and cached scale reads.
     * Sample/current-controller/motor-state layouts belong to motor_control.c. */
    volatile float *const sample = (volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA11CUL, 0x1FFFA154UL);
    const volatile float *const config = (const volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA13CUL, 0x1FFFA174UL);
    const volatile float *const cache = (const volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA140UL, 0x1FFFA178UL);
    const float voltage = sample[26];
    const float undervoltage = config[0];
    const float current_scale = cache[2];
    volatile RuntimeDriveState *const d = (volatile RuntimeDriveState *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA120UL, 0x1FFFA158UL);
    volatile RuntimeDriveState *const q = (volatile RuntimeDriveState *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA124UL, 0x1FFFA15CUL);
    volatile float *const current_d = (volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA130UL, 0x1FFFA168UL);
    volatile float *const current_q = (volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA134UL, 0x1FFFA16CUL);
    if (voltage > undervoltage)
    {
        const float bandwidth = config[24];
        const float inductance = config[18];
        float proportional = ((bandwidth * inductance) * current_scale) / voltage;
        __asm volatile("" : "+t"(proportional) : : "memory");
        const float proportional_scale =
            *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA144UL, 0x1FFFA17CUL);
        proportional = commissioning_ordered_multiply(proportional, proportional_scale);
        d->word[0] = proportional;
        q->word[0] = proportional;
        const float resistance = config[17];
        const float period = cache[3];
        const float integral = (resistance / inductance) * period;
        d->word[1] = integral;
        q->word[1] = integral;
        current_d[4] = period;
        current_q[4] = period;
        float input_gain;
        float denominator;
        /* Publish both sample periods before gain arithmetic. */
        __asm volatile("vmul.f32 %0, %1, %2"
                       : "=t"(denominator)
                       : "t"(current_scale), "t"(inductance)
                       : "memory");
        const float gain_scale =
            *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA148UL, 0x1FFFA180UL);
        __asm volatile("vmul.f32 %0, %2, %3\n"
                       "vdiv.f32 %0, %0, %1"
                       : "=&t"(input_gain)
                       : "t"(denominator), "t"(voltage), "t"(gain_scale)
                       : "memory");
        current_d[8] = input_gain;
        float inverse_gain;
        /* Perform the division between the D- and Q-axis gain stores. */
        __asm volatile("vdiv.f32 %0, %1, %2"
                       : "=t"(inverse_gain)
                       : "t"(1.0f), "t"(input_gain)
                       : "memory");
        current_q[8] = input_gain;
        current_d[9] = inverse_gain;
        current_q[9] = inverse_gain;
        current_d[0] = bandwidth;
        current_q[0] = bandwidth;
        const float observer_gain =
            *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA14CUL, 0x1FFFA184UL);
        current_d[7] = observer_gain;
        current_q[7] = observer_gain;
        const float observer_gain_twice =
            *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA150UL, 0x1FFFA188UL);
        current_d[11] = observer_gain_twice;
        current_q[11] = observer_gain_twice;
        const float observer_gain_squared =
            *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA154UL, 0x1FFFA18CUL);
        current_d[12] = observer_gain_squared;
        current_q[12] = observer_gain_squared;
        current_d[17] = -1.0f;
        current_q[17] = -1.0f;
        current_d[18] = 1.0f;
        current_q[18] = 1.0f;
    }
    else
    {
        const float cleared =
            *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA118UL, 0x1FFFA150UL);
        d->word[0] = cleared;
        q->word[0] = cleared;
        d->word[1] = cleared;
        q->word[1] = cleared;
        current_d[0] = cleared;
        current_q[0] = cleared;
    }
    const uint32_t pole_pairs = ((const volatile uint32_t *)config)[16];
    volatile float *const motor = (volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA158UL, 0x1FFFA190UL);
    float pole_factor;
    /* Complete the pole-factor multiply before loading flux linkage. */
    __asm volatile("vmul.f32 %0, %1, %2"
                   : "=t"(pole_factor)
                   : "t"((float)pole_pairs), "t"(1.5f)
                   : "memory");
    const float flux = config[19];
    float torque;
    __asm volatile("vmul.f32 %0, %1, %2\n"
                   "vmul.f32 %0, %0, %3"
                   : "=&t"(torque)
                   : "t"(pole_factor), "t"(flux), "t"(current_scale)
                   : "memory");
    /* Evaluate this even when the override discards it.  Keep
     * the floating-point exceptions and evaluation before the next load. */
    __asm volatile("" : "+t"(torque) : : "memory");
    const float override = config[1];
    /* VCMPE, unlike GCC's equality VCMP, raises IOC for quiet NaNs too. */
    __asm volatile("vcmpe.f32 %0, #0.0" : : "t"(override) : "cc");
    if (override == 0.0f)
    {
        const float ratio = config[20];
        const float correction = config[30];
        motor[17] = commissioning_ordered_multiply(commissioning_ordered_multiply(ratio, torque),
                                                   correction);
    }
    else
    {
        /* Keep operand order when FPSCR.DN is clear: two NaN operands can
         * otherwise select a different payload after GCC swaps VMUL inputs. */
        motor[17] = commissioning_ordered_multiply(override, current_scale);
    }
}

void commissioning_clear_runtime_loop_states(void)
{
    /* Unreferenced this path is distinct from reset:
     * it clears sample targets plus the D/Q and both outer-loop states, but
     * does not clear the current observers. Sample layout is owned by
     * motor_control.c; these are its fixed words +0x1c, +0x20 and +0x2c. */
    const float cleared =
        *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA118UL, 0x1FFFA150UL);
    volatile float *const sample = (volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA11CUL, 0x1FFFA154UL);
    volatile RuntimeDriveState *const q = (volatile RuntimeDriveState *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA124UL, 0x1FFFA15CUL);
    sample[7] = cleared;
    sample[8] = cleared;
    sample[11] = cleared;
    volatile RuntimeDriveState *const d = (volatile RuntimeDriveState *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA120UL, 0x1FFFA158UL);
    d->word[2] = cleared;
    q->word[2] = cleared;
    d->word[4] = cleared;
    q->word[4] = cleared;
    d->word[7] = cleared;
    q->word[7] = cleared;
    d->word[5] = cleared;
    q->word[5] = cleared;
    d->word[6] = cleared;
    volatile RuntimeDriveState *const speed = (volatile RuntimeDriveState *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA128UL, 0x1FFFA160UL);
    q->word[6] = cleared;
    commissioning_reset_runtime_loop_states(speed->word, cleared);
}

void commissioning_configure_runtime_loop_states(const MotorConfig *config)
{
    if (config == NULL)
    {
        return;
    }
    /* Callers copy staging words 25..28 after parameter derivation.  Only the
     * first two words of either 0x28-byte state change. */
    const volatile float *const staging =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    volatile RuntimeDriveState *const speed = &runtime_speed_loop;
    volatile RuntimeDriveState *const position = &runtime_position_loop;
    __asm volatile("vldr s0, [%0, #100]\n\t"
                   "vstr s0, [%1]\n\t"
                   "vldr s0, [%0, #104]\n\t"
                   "vstr s0, [%1, #4]\n\t"
                   "vldr s0, [%0, #108]\n\t"
                   "vstr s0, [%2]\n\t"
                   "vldr s0, [%0, #112]\n\t"
                   "vstr s0, [%2, #4]"
                   :
                   : "r"(staging), "r"(speed), "r"(position)
                   : "s0", "memory");
}

static uint32_t runtime_float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void commissioning_initialize_runtime_parameter_cache(const MotorConfig *config)
{
    if (config == NULL)
    {
        return;
    }
    const volatile uint32_t *const staging =
        (const volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa5c8),
                                                                    UINT32_C(0x1fffa558));
    /* Preserve this non-linear constant-store order.  Every cache word is
     * published exactly once; words 13..21 come directly from fixed
     * staging rather than through source-only MotorConfig mirrors. */
    runtime_parameter_cache[0] = APP_PROFILE_RUNTIME_CACHE_0_BITS;
    runtime_parameter_cache[1] = UINT32_C(0x42200a9e);
    runtime_parameter_cache[2] = APP_PROFILE_CURRENT_FULL_SCALE_BITS;
    runtime_parameter_cache[3] = UINT32_C(0x3851b717);
    runtime_parameter_cache[4] = UINT32_C(0x3dcccccd);
    runtime_parameter_cache[5] = APP_PROFILE_IDENTIFICATION_TARGET_CURRENT_BITS;
    runtime_parameter_cache[9] = APP_PROFILE_RUNTIME_CACHE_9_BITS;
    runtime_parameter_cache[10] = UINT32_C(0x40000000);
    runtime_parameter_cache[11] = UINT32_C(0x3dcccccd);
    runtime_parameter_cache[12] = UINT32_C(0x3f800000);
    runtime_parameter_cache[6] = UINT32_C(0x40000000);
    runtime_parameter_cache[7] = UINT32_C(0x3e99999a);
    runtime_parameter_cache[8] = UINT32_C(0x3e4ccccd);
    runtime_parameter_cache[13] = staging[32];
    runtime_parameter_cache[14] = staging[24];
    runtime_parameter_cache[15] = staging[33];
    __asm volatile("vldr s0, [%0, #64]\n\t"
                   "vcvt.f32.u32 s0, s0\n\t"
                   "vstr s0, [%1, #64]"
                   :
                   : "r"(staging), "r"(runtime_parameter_cache)
                   : "s0", "memory");
    runtime_parameter_cache[17] = staging[17];
    runtime_parameter_cache[18] = staging[18];
    runtime_parameter_cache[19] = staging[19];
    runtime_parameter_cache[20] = staging[12];
    runtime_parameter_cache[21] = staging[31];
    runtime_parameter_cache[22] = UINT32_C(0x447a0000);
    runtime_parameter_cache[23] = UINT32_C(0x44160000);
}

void commissioning_update_runtime_parameter_cache_ugq(const MotorConfig *config)
{
    if (config != NULL)
    {
        const volatile uint32_t *const staging =
            (const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
        /* Transport UgQ through s0 without changing its bit pattern. */
        __asm volatile("vldr s0, [%0, #96]\n\t"
                       "vstr s0, [%1, #56]"
                       :
                       : "r"(staging), "r"(runtime_parameter_cache)
                       : "s0", "memory");
    }
}

void commissioning_update_runtime_parameter_cache_motor_id(const MotorConfig *config)
{
    if (config == NULL)
    {
        return;
    }
    runtime_parameter_cache[16] = runtime_float_bits((float)config->pole_pairs);
    runtime_parameter_cache[17] = runtime_float_bits(config->phase_resistance);
    runtime_parameter_cache[18] = runtime_float_bits(config->phase_inductance);
    runtime_parameter_cache[19] = runtime_float_bits(config->flux_linkage);
    runtime_parameter_cache[20] = runtime_float_bits(config->rotor_inertia);
    runtime_parameter_cache[21] = runtime_float_bits(config->speed_loop_damping);
}

static void identification_current_filter_step(CommissioningIdentificationFilter *axis_d,
                                               CommissioningIdentificationFilter *axis_q,
                                               DirectQuadrature measured_current, float target_d,
                                               float target_q, float electrical_angle,
                                               float *voltage_d, float *voltage_q,
                                               const IdentificationCurrentFrame *frame)
{
    (void)target_d;
    float error_d;
    float error_q;
    /* Excitation and coast negate D directly;
     * Q retains the VSUB target-current operation in both stages. */
    __asm volatile("vneg.f32 %0, %2\n"
                   "vsub.f32 %1, %3, %4\n"
                   : "=&t"(error_d), "=&t"(error_q)
                   : "t"(measured_current.d), "t"(target_q), "t"(measured_current.q));
    axis_d->input = error_d;
    axis_q->input = error_q;
    __asm volatile("" : : : "memory");
    identification_filter_helper(axis_d);
    identification_filter_helper(axis_q);
    *voltage_d = axis_d->limited_output;
    *voltage_q = axis_q->limited_output;
    (void)electrical_angle;
    identification_drive(*voltage_d, *voltage_q, frame, true);
}

static bool identify_flux_linkage(const MotorConfig *config, PlatformCommissioningSample *sample,
                                  float *flux_linkage, IdentificationCurrentFrame *frame,
                                  float initial_voltage_d)
{
    CommissioningFluxObserver observer;
    if ((config == NULL) || (sample == NULL) || (flux_linkage == NULL))
    {
        return false;
    }
    /* Snapshot L, sample period, then R from their runtime owners rather than
     * the source-owned configuration copy. */
    const volatile float *const staging =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    const float observer_inductance = staging[18];
    const float observer_sample_period =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF248UL, 0x1FFFF1D4UL));
    const float observer_resistance = staging[17];
    if (!commissioning_flux_observer_init(&observer, observer_inductance, observer_resistance,
                                          observer_sample_period))
    {
        return false;
    }
    float electrical_speed = 0.0f;
    /* Carry the fitted electrical voltage into the flux loop. */
    float voltage_d = initial_voltage_d;
    float voltage_q = 0.0f;
    /* Clear the position IRQ's accumulated angle before acknowledging ADC,
     * then clear sample_ready immediately before polling. */
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
    platform_commissioning_finish_sample();
    *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) = 0U;
    for (uint32_t count = 1U; count <= IDENTIFICATION_FLUX_STEPS; ++count)
    {
        platform_commissioning_wait_identification_sample();
        if (!platform_commissioning_read_identification_sample(sample, true))
        {
            return false;
        }

        float observer_current_scale;
        const DirectQuadrature current =
            commissioning_current_dq(sample, frame, true, &observer_current_scale);
        const float electrical_angle = frame->angle;
        /* The Park path retained the live scale snapshot; voltage feedback
         * still uses the unscaled current.d below. */
        volatile CommissioningFluxObserver *const observer_state = &observer;
        float measured_d;
        float measured_q;
        __asm volatile("vmul.f32 %0, %2, %3\n"
                       "vmul.f32 %1, %2, %4"
                       : "=&t"(measured_d), "=&t"(measured_q)
                       : "t"(observer_current_scale), "t"(current.d), "t"(current.q));
        observer_state->measured_current_d = measured_d;
        observer_state->measured_current_q = measured_q;
        observer_state->electrical_speed = electrical_speed;
        /* Load the already-projected sample word, then multiply D and Q with
         * that operand first.  Do not reassociate the three factors. */
        const float observer_projected_bus =
            *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF170UL, 0x1FFFF0FCUL));
        float measured_voltage_d;
        float measured_voltage_q;
        __asm volatile("vmul.f32 %0, %2, %3\n"
                       "vmul.f32 %1, %2, %4"
                       : "=&t"(measured_voltage_d), "=&t"(measured_voltage_q)
                       : "t"(observer_projected_bus), "t"(voltage_d), "t"(voltage_q));
        observer_state->voltage_d = measured_voltage_d;
        observer_state->voltage_q = measured_voltage_q;
        flux_observer_helper(&observer);

        /* Execute the observer before updating the speed window. */
        if ((count % IDENTIFICATION_OBSERVER_WINDOW) == 0U)
        {
            electrical_speed = identification_flux_speed_window(electrical_speed);
            /* Retain VADD operand order and keep the live limit load after it. */
            __asm volatile("vadd.f32 %0, %0, %1"
                           : "+t"(voltage_q)
                           : "t"(IDENTIFICATION_OBSERVER_RAMP_STEP)
                           : "memory");
            const float q_limit =
                *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF25CUL, 0x1FFFF1E8UL));
            /* Compare the limit against the ramp with the unsigned FP status;
             * this preserves unordered inputs, unlike a C min/max. */
            __asm volatile("vcmpe.f32 %1, %0\n"
                           "vmrs APSR_nzcv, fpscr\n"
                           "it ls\n"
                           "vmovls.f32 %0, %1"
                           : "+t"(voltage_q)
                           : "t"(q_limit)
                           : "cc");
        }

        const float d_sample_period =
            *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF248UL, 0x1FFFF1D4UL));
        float d_gain_step;
        __asm volatile("vmul.f32 %1, %2, %3\n"
                       "vmls.f32 %0, %1, %4"
                       : "+t"(voltage_d), "=&t"(d_gain_step)
                       : "t"(d_sample_period), "t"(10.0f), "t"(current.d));
        voltage_d =
            clamp_helper(voltage_d, -IDENTIFICATION_VOLTAGE_LIMIT, IDENTIFICATION_VOLTAGE_LIMIT);
        (void)electrical_angle;
        identification_drive(voltage_d, voltage_q, frame, false);
        platform_commissioning_finish_sample();
    }
    platform_commissioning_delay_us(1000U);
    /* Neutralize stationary PWM, publish flux, then reset the position IRQ's
     * accumulated delta for the mechanical stage. */
    svpwm_helper(0.0f, 0.0f);
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA614UL, 0x1FFFA5A4UL)) = observer.flux_linkage;
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
    *flux_linkage = observer.flux_linkage;
    return true;
}

static bool identify_mechanical_parameters(const MotorConfig *config, float flux_linkage,
                                           PlatformCommissioningSample *sample,
                                           float *rotor_inertia, float *viscous_damping,
                                           IdentificationCurrentFrame *frame)
{
    if ((config == NULL) || (sample == NULL) || (rotor_inertia == NULL) ||
        (viscous_damping == NULL))
    {
        return false;
    }
    CommissioningIdentificationFilter axis_d;
    CommissioningIdentificationFilter axis_q;
    float mechanical_current_scale;
    if (!identification_filter_pair_init(config, 0.0f, &axis_d, &axis_q, &mechanical_current_scale))
    {
        return false;
    }

    /* Load the integer bits into s0 before reusing r0 for a pointer.  Preserve
     * the torque scale's multiplication
     * chain and publish both motor state words before excitation. */
    const uint32_t mechanical_poles =
        *((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    const float mechanical_gear =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA618UL, 0x1FFFA5A8UL));
    float torque_scale;
    float torque_per_amp;
    const float pole_scale = (float)mechanical_poles;
    __asm volatile("vmul.f32 %0, %2, %3\n"
                   "vmul.f32 %0, %0, %4\n"
                   "vmul.f32 %0, %0, %5\n"
                   "vmul.f32 %0, %0, %6\n"
                   "vmul.f32 %1, %2, %3\n"
                   : "=&t"(torque_scale), "=&t"(torque_per_amp)
                   : "t"(pole_scale), "t"(1.5f), "t"(flux_linkage), "t"(mechanical_gear),
                     "t"(mechanical_current_scale));
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0CCUL, 0x1FFFF058UL)) = torque_scale;
    /* Publish motor torque before completing the separate regression scale. */
    __asm volatile("vmul.f32 %0, %0, %1\n"
                   "vmul.f32 %0, %0, %2"
                   : "+t"(torque_per_amp)
                   : "t"(flux_linkage), "t"(mechanical_current_scale)
                   : "memory");
    __asm volatile("" : "+t"(torque_scale), "+t"(torque_per_amp) : : "memory");
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0D0UL, 0x1FFFF05CUL)) = 1.0f / torque_scale;
    /* Refresh this frame once on mechanical-stage entry, independently of
     * sample_ready; only subsequent samples are gated. */
    frame->angle = identification_electrical_angle_from_poles(mechanical_poles);
    wrap_helper(&frame->angle, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
    motor_target_sincos(frame->angle, &frame->sine, &frame->cosine);
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
    *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) = 0U;
    const volatile float *const mechanical_cache =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    const float mechanical_target = mechanical_cache[9];
    const float mechanical_scale = mechanical_cache[2];
    float target_amplitude = mechanical_target / mechanical_scale;
    __asm volatile("" : "+t"(target_amplitude) : : "memory");
    const float mechanical_frequency = mechanical_cache[10];
    platform_commissioning_finish_sample();
    uint32_t phase_count = 0U;
    float voltage_d = 0.0f;
    float voltage_q = 0.0f;
    float filtered_current = 0.0f;
    float filtered_speed = 0.0f;
    uint32_t window_count = 0U;
    float response_phase = 0.0f;
    float response_magnitude = 0.0f;
    CommissioningSineRegression regression;
    commissioning_sine_regression_init(&regression);

    for (uint32_t count = 0U; count < IDENTIFICATION_MECHANICAL_DRIVE_STEPS; ++count)
    {
        platform_commissioning_wait_identification_sample();
        ++phase_count;
        const float unwrapped_phase =
            identification_target_phase(mechanical_frequency, phase_count);
        float phase = commissioning_reduce_float_angle(unwrapped_phase);
        wrap_helper(&phase, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
        /* Evaluate and scale target sine before cycle projection and the
         * measured-current transform. */
        float target_sine;
        float target_cosine;
        motor_target_sincos(phase, &target_sine, &target_cosine);
        float target_q;
        __asm volatile("vmul.f32 %0, %1, %2"
                       : "=t"(target_q)
                       : "t"(target_amplitude), "t"(target_sine)
                       : "memory");
        int32_t phase_bits;
        memcpy(&phase_bits, &unwrapped_phase, sizeof(phase_bits));
        /* Detect cycle completion from signed float bits rather than FPSCR. */
        const bool cycle_complete = phase_bits > (int32_t)0x40C90FDBUL;
        if (cycle_complete)
        {
            commissioning_sine_regression_projection(&regression, torque_per_amp, &response_phase,
                                                     &response_magnitude);
            commissioning_sine_regression_init(&regression);
            phase_count = 0U;
        }

        if (!platform_commissioning_read_identification_sample(sample, false))
        {
            return false;
        }
        const DirectQuadrature current = commissioning_current_dq(sample, frame, true, NULL);
        const float electrical_angle = frame->angle;

        ++window_count;
        if (window_count == IDENTIFICATION_OBSERVER_WINDOW)
        {
            /* Use accumulated IRQ displacement, raw normalized q current, and
             * live motor filter coefficients. */
            const float displacement =
                *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL));
            float mechanical_speed = displacement * 1000.0f;
            __asm volatile("" : "+t"(mechanical_speed) : : "memory");
            const float old_weight =
                *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0F0UL, 0x1FFFF07CUL));
            const float new_weight =
                *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0F4UL, 0x1FFFF080UL));
            float next_speed;
            __asm volatile("vmul.f32 %0, %2, %0\n"
                           "vmul.f32 %1, %2, %4\n"
                           "vmla.f32 %0, %5, %3\n"
                           "vmla.f32 %1, %6, %3\n"
                           : "+&t"(filtered_current), "=&t"(next_speed)
                           : "t"(old_weight), "t"(new_weight), "t"(filtered_speed), "t"(current.q),
                             "t"(mechanical_speed));
            *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
            filtered_speed = next_speed;
            commissioning_sine_regression_step(&regression, filtered_current, filtered_speed);
            window_count = 0U;
        }
        identification_current_filter_step(&axis_d, &axis_q, current, 0.0f, target_q,
                                           electrical_angle, &voltage_d, &voltage_q, frame);
        platform_commissioning_finish_sample();
    }

    for (uint32_t count = 0U; count < IDENTIFICATION_MECHANICAL_COAST_STEPS; ++count)
    {
        platform_commissioning_wait_identification_sample();
        if (!platform_commissioning_read_identification_sample(sample, false))
        {
            return false;
        }
        const DirectQuadrature current = commissioning_current_dq(sample, frame, true, NULL);
        const float electrical_angle = frame->angle;
        identification_current_filter_step(&axis_d, &axis_q, current, 0.0f, 0.0f, electrical_angle,
                                           &voltage_d, &voltage_q, frame);
        platform_commissioning_finish_sample();
    }
    platform_commissioning_delay_us(1000U);
    svpwm_helper(0.0f, 0.0f);
    commissioning_sine_response_motor_parameters(
        response_phase, response_magnitude, mechanical_frequency, true,
        (volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5F8UL, 0x1FFFA588UL),
        (volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5F4UL, 0x1FFFA584UL));
    /* A partial final cycle is accepted; there is no completion rejection. */
    return true;
}

static void send_identification_result(const MotorConfig *config)
{
    uint8_t frame[21] = {'e'};
    (void)config;
    const volatile uint32_t *const staging =
        (const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    uint32_t value = staging[17];
    memcpy(&frame[1], &value, sizeof(value));
    value = staging[18];
    memcpy(&frame[5], &value, sizeof(value));
    value = staging[19];
    memcpy(&frame[9], &value, sizeof(value));
    value = staging[11];
    memcpy(&frame[13], &value, sizeof(value));
    value = staging[12];
    memcpy(&frame[17], &value, sizeof(value));
    platform_debug_write(frame, sizeof(frame));
}

static float __attribute__((noinline)) alignment_drive_angle(double electrical_angle)
{
    /* Use this exact binary64 constant, then convert the integer turn count to
     * float before multiplying by the single-precision two-pi value. Subtract
     * the resulting turn offset from the original angle. */
    const uint32_t turns = runtime_double_to_uint(electrical_angle * 0x1.45f3060000000p-3);
    const float offset = (float)turns * TWO_PI_F;
    return (float)(electrical_angle - (double)offset);
}

static float alignment_measured_angle(void)
{
    /* IRQ001-owned position scratch, not the ADC-updated g_app cache. */
    return *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF198UL, 0x1FFFF124UL));
}

static uint16_t alignment_raw_position(void)
{
    return *((const volatile uint16_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF190UL, 0x1FFFF11CUL));
}

static float __attribute__((noinline)) commissioning_reduce_float_angle(float travel)
{
    /* Use unsigned VFP truncation, including its exceptional flags, followed
     * by non-fused VMLS.
     * Retain the float 1/(2*pi) literal, not a double reciprocal. */
    const float inverse_two_pi = 0x1.45f306p-3f;
    const float two_pi = TWO_PI_F;
    float turns;
    __asm volatile("vmul.f32 %0, %1, %2\n"
                   "vcvt.u32.f32 %0, %0\n"
                   "vcvt.f32.u32 %0, %0"
                   : "=&t"(turns)
                   : "t"(travel), "t"(inverse_two_pi));
    __asm volatile("vmls.f32 %0, %1, %2" : "+t"(travel) : "t"(turns), "t"(two_pi));
    return travel;
}

static float direction_angle_delta(float measured, float start)
{
    float delta = measured - start;
    if ((int32_t)runtime_float_bits(delta) > (int32_t)0x40490FDBU)
    {
        delta -= TWO_PI_F;
    }
    if (runtime_float_bits(delta) > 0xC0490FDBU)
    {
        delta += TWO_PI_F;
    }
    return delta;
}

static void __attribute__((noinline)) direction_publish_pole_count(uint32_t poles)
{
    /* Publish integer staging followed by the division-derived scales. */
    *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL)) = poles;
    *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA608UL, 0x1FFFA598UL)) = poles;
    const float runtime_poles = (float)poles;
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0E0UL, 0x1FFFF06CUL)) =
        runtime_poles / TWO_PI_F;
    const float gear_ratio =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA618UL, 0x1FFFA5A8UL));
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0E8UL, 0x1FFFF074UL)) =
        runtime_poles * gear_ratio;
}

static uint32_t __attribute__((noinline)) direction_finish_count(float travel, float direction)
{
    float turns = travel / TWO_PI_F;
    /* Divide before publishing direction, then call the rounding helper.  Keep
     * arithmetic values live across the volatile store. */
    __asm volatile("" : "+t"(turns) : : "memory");
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0BCUL, 0x1FFFF048UL)) = direction;
    return (uint32_t)runtime_round_to_int(turns);
}

static CommissioningStatus run_alignment_scan(uint32_t pole_pairs)
{
    /* Convert the numerator count to float before multiplying, but wrap the
     * denominator's shifts and addition in uint32_t before converting.
     * Cancelling pole_pairs changes rounding, zero and overflow behavior. */
    pole_pairs = *((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    const uint32_t points = pole_pairs * ALIGNMENT_POINTS_PER_PAIR;
    const uint32_t denominator = points * ALIGNMENT_INNER_STEPS;
    const float numerator = (float)pole_pairs * TWO_PI_F;
    const double step = (double)(numerator / (float)denominator);
    /* alignment authenticates again after deriving the step,
     * before loading its drive-voltage snapshot or publishing PWM. */
    platform_require_device_authentication();
    /* Snapshot the alignment-voltage cache word once for the scan. */
    const uint32_t voltage_bits = runtime_parameter_cache[4];
    float alignment_voltage;
    memcpy(&alignment_voltage, &voltage_bits, sizeof(alignment_voltage));
    double electrical_angle = 0.0;
    for (uint32_t count = 0U; count < ALIGNMENT_LOCK_STEPS; ++count)
    {
        /* Update PWM on every lock iteration, not just once. */
        svpwm_helper(alignment_voltage, 0.0f);
        platform_commissioning_delay_us(100U);
    }

    float unwrapped = alignment_measured_angle();
    for (uint32_t point = 0U; point < points; ++point)
    {
        for (uint32_t inner = 0U; inner < ALIGNMENT_INNER_STEPS; ++inner)
        {
            electrical_angle += step;
            platform_commissioning_drive(alignment_voltage, 0.0f,
                                         alignment_drive_angle(electrical_angle));
            platform_commissioning_delay_us(100U);
        }
        const float measured = alignment_measured_angle();
        unwrapped = alignment_unwrap(measured, unwrapped);
#if defined(__arm__) || defined(__thumb__)
        __asm volatile("" : "+t"(unwrapped) : : "memory");
#endif
        /* Reload the live motor word once per sample.  The scan bounds and
         * step retain the initial pole count. */
        const uint32_t sample_pole_pairs =
            *((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
        const double commanded_value = electrical_angle / (double)sample_pole_pairs;
        const float commanded = (float)(electrical_angle / (double)sample_pole_pairs);
        send_alignment_sample(commanded, measured, (float)(commanded_value - (double)unwrapped),
                              alignment_raw_position(), (uint16_t)point);
    }

    for (uint32_t point = 0U; point < points; ++point)
    {
        for (uint32_t inner = 0U; inner < ALIGNMENT_INNER_STEPS; ++inner)
        {
            electrical_angle -= step;
            platform_commissioning_drive(alignment_voltage, 0.0f,
                                         alignment_drive_angle(electrical_angle));
            platform_commissioning_delay_us(100U);
        }
        const float measured = alignment_measured_angle();
        unwrapped = alignment_unwrap(measured, unwrapped);
#if defined(__arm__) || defined(__thumb__)
        __asm volatile("" : "+t"(unwrapped) : : "memory");
#endif
        const uint32_t sample_pole_pairs =
            *((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
        const double commanded_value = electrical_angle / (double)sample_pole_pairs;
        const float commanded = (float)(electrical_angle / (double)sample_pole_pairs);
        send_alignment_sample(commanded, measured, (float)(commanded_value - (double)unwrapped),
                              alignment_raw_position(),
                              /* Use the per-sample live count rather than the
                               * initial scan bound. */
                              (uint16_t)(point + sample_pole_pairs * ALIGNMENT_POINTS_PER_PAIR));
    }
    /* Neutralize stationary PWM directly. */
    svpwm_helper(0.0f, 0.0f);
    return COMMISSIONING_OK;
}

CommissioningStatus commissioning_run_direction_and_alignment(MotorConfig *config)
{
    if ((config == NULL) || !platform_commissioning_begin())
    {
        return COMMISSIONING_POWER_DISABLED;
    }

    /* detect_motor_direction_and_pole_pairs clears all 256 live
     * correction entries and starts from direction code 1 before motion. */
    platform_commissioning_reset_motor_encoder();
    config->direction = 1.0f;
    config->sensor_inverted = true;
    /* Publish direction before taking the voltage snapshot. */
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0BCUL, 0x1FFFF048UL)) = 1.0f;
    const uint32_t voltage_bits = runtime_parameter_cache[4];
    float direction_voltage;
    memcpy(&direction_voltage, &voltage_bits, sizeof(direction_voltage));
    for (uint32_t count = 0U; count < DIRECTION_LOCK_STEPS; ++count)
    {
        svpwm_helper(direction_voltage, 0.0f);
        platform_commissioning_delay_us(50U);
    }

    const float start = alignment_measured_angle();
    float electrical_travel = 0.0f;
    float quarter_delta = 0.0f;
    for (;;)
    {
        electrical_travel += DIRECTION_ANGLE_STEP;
        platform_commissioning_drive(direction_voltage, 0.0f,
                                     commissioning_reduce_float_angle(electrical_travel));
        platform_commissioning_delay_us(50U);
        quarter_delta = direction_angle_delta(alignment_measured_angle(), start);
        const uint32_t delta_bits = runtime_float_bits(quarter_delta);
        /* Apply the signed upper and unsigned lower raw-bit comparisons. */
        if ((int32_t)delta_bits >= (int32_t)0x3FC90FDBU || delta_bits >= 0xBFC90FDBU)
        {
            break;
        }
    }

    /* Classify the quarter-turn delta before returning home.  VCMPE also
     * signals quiet NaNs, unlike GCC's ordinary comparison. */
    __asm volatile("vcmpe.f32 %0, #0.0" : : "t"(quarter_delta) : "cc");
    const bool detected_inverted = quarter_delta > 0.0f;
    const float detected_direction = detected_inverted ? 1.0f : 2.0f;
    for (;;)
    {
        electrical_travel += DIRECTION_ANGLE_STEP;
        platform_commissioning_drive(direction_voltage, 0.0f,
                                     commissioning_reduce_float_angle(electrical_travel));
        platform_commissioning_delay_us(50U);
        const float absolute_delta = fabsf(alignment_measured_angle() - start);
        if ((int32_t)runtime_float_bits(absolute_delta) <= (int32_t)0x3BE56042U)
        {
            break;
        }
    }

    /* Neutralize PWM before deriving and publishing the pole count. */
    svpwm_helper(0.0f, 0.0f);
    CommissioningDirectionResult result;
    result.direction_code = detected_direction;
    result.pole_pairs = direction_finish_count(electrical_travel, detected_direction);

    config->direction = result.direction_code;
    /* Reuse the existing direction decision.  Re-comparing the float
     * here leaves a different FPSCR NZCV image after the worker returns. */
    config->sensor_inverted = detected_inverted;
    config->pole_pairs = result.pole_pairs;
    /* detect-direction publishes the full integer motor word
     * before alignment reads it; +0x54 is not a floating-point field. */
    direction_publish_pole_count(result.pole_pairs);
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
    /* alignment has already emitted the final neutral PWM update. */
    platform_commissioning_finish_alignment();
    return alignment;
}

CommissioningStatus commissioning_run_output_sensor_calibration(void)
{
    if (!platform_commissioning_begin_unauthenticated())
    {
        return COMMISSIONING_POWER_DISABLED;
    }

    PlatformCommissioningSample sample;
    platform_commissioning_prime_output_filter();
    OutputSensorExtrema extrema = {
        .minimum_u = 4096U,
        .maximum_u = 0U,
        .minimum_v = 4096U,
        .maximum_v = 0U,
    };
    const float start_position =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
    float extrema_target_position = start_position + TWO_PI_F;
    __asm volatile("" : "+t"(extrema_target_position) : : "memory");
    uint32_t report_count = 0U;
    IdentificationCurrentFrame extrema_frame;
    extrema_frame.angle = 0.0f;
    /* Leave the retained sine/cosine stack slots untouched until
     * the first ready sample. Read via ASM, not an undefined C float read. */
    __asm volatile("vldr %0, [%2]\nvldr %1, [%2, #4]"
                   : "=t"(extrema_frame.sine), "=t"(extrema_frame.cosine)
                   : "r"(&extrema_frame.sine)
                   : "memory");

    for (;;)
    {
        platform_commissioning_read_extrema_sample(&sample);
        if (sample.output_raw_u > extrema.maximum_u)
        {
            extrema.maximum_u = sample.output_raw_u;
            extrema.angle_at_maximum_u =
                *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
        }
        if (sample.output_raw_u < extrema.minimum_u)
        {
            extrema.minimum_u = sample.output_raw_u;
            extrema.angle_at_minimum_u =
                *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
        }
        if (sample.output_raw_v > extrema.maximum_v)
        {
            extrema.maximum_v = sample.output_raw_v;
            extrema.angle_at_maximum_v =
                *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
        }
        if (sample.output_raw_v < extrema.minimum_v)
        {
            extrema.minimum_v = sample.output_raw_v;
            extrema.angle_at_minimum_v =
                *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
        }
        if (*((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) != 0U)
        {
            *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) = 0U;
            /* Unlike other commissioning stages, load all three inputs before
             * converting the pole count. */
            __asm volatile("vldr s1, [%1, #84]\n"
                           "vldr s0, [%2, #8]\n"
                           "vldr s2, [%1, #88]\n"
                           "vcvt.f32.u32 s1, s1\n"
                           "vmul.f32 s1, s0, s1\n"
                           "vmul.f32 s0, s0, s2\n"
                           "vcvt.u32.f32 s0, s0\n"
                           "vcvt.f32.u32 s0, s0\n"
                           "vmls.f32 s1, s0, %3\n"
                           "vldr s0, [%1, #20]\n"
                           "vadd.f32 %0, s1, s0"
                           : "=t"(extrema_frame.angle)
                           : "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(0x1FFFF088UL, 0x1FFFF014UL)),
                             "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(0x1FFFF190UL, 0x1FFFF11CUL)),
                             "t"(TWO_PI_F)
                           : "s0", "s1", "s2", "memory");
            wrap_helper(&extrema_frame.angle, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
            motor_target_sincos(extrema_frame.angle, &extrema_frame.sine, &extrema_frame.cosine);
        }
        if (++report_count == OUTPUT_CALIBRATION_REPORT_DIVIDER)
        {
            report_count = 0U;
            send_output_sensor_raw_sample(sample.output_raw_u, sample.output_raw_v);
        }

        /* Retain the sampled distance before drive/PWM work and ADC acknowledgement. */
        const float output_position =
            *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
        float target_distance = output_position - extrema_target_position;
        __asm volatile("" : "+t"(target_distance) : : "memory");
        /* extrema publishes D/Q controller output words. */
        *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA528UL, 0x1FFFA610UL)) = 0.0f;
        __asm volatile("vabs.f32 %0, %0" : "+t"(target_distance) : : "memory");
        *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA550UL, 0x1FFFA638UL)) =
            OUTPUT_CALIBRATION_VOLTAGE_Q;
        __asm volatile("" : "+t"(target_distance) : : "memory");
        float alpha;
        float beta;
        __asm volatile("vmul.f32 %0, %2, %4\n"
                       "vmul.f32 %1, %2, %5\n"
                       "vmls.f32 %0, %5, %3\n"
                       "vmla.f32 %1, %4, %3"
                       : "=&t"(alpha), "=&t"(beta)
                       : "t"(0.0f), "t"(OUTPUT_CALIBRATION_VOLTAGE_Q), "t"(extrema_frame.cosine),
                         "t"(extrema_frame.sine));
        svpwm_helper(alpha, beta);
        platform_commissioning_finish_sample();
        /* extrema compares signed bits of distance to
         * the retained one-turn target, not travel distance from start. */
        if ((int32_t)runtime_float_bits(target_distance) <= (int32_t)UINT32_C(0x3a83126f))
        {
            break;
        }
    }

    platform_commissioning_publish_output_extrema(&extrema);

    const uint16_t report_poles =
        *((const volatile uint16_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    const float report_gear =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0D4UL, 0x1FFFF060UL));
    uint32_t report_gear_integer;
    float converted_gear;
    __asm volatile("vcvt.u32.f32 %0, %2\nvmov %1, %0"
                   : "=&t"(converted_gear), "=r"(report_gear_integer)
                   : "t"(report_gear)
                   : "memory");
    send_sensor_sample('J', 0.0f, 0.0f, 0.0f, report_poles, (uint16_t)report_gear_integer);
    /* This path uses the millisecond timer, not 1000 us. */
    platform_delay_ms(1U);

    /* offset retains gear ratio before taking the live pole count. */
    const float offset_gear_ratio =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0D4UL, 0x1FFFF060UL));
    const float measurement_points = offset_gear_ratio * (float)OUTPUT_TABLE_POINTS_PER_MOTOR_TURN;
    uint32_t measurement_count = (uint32_t)measurement_points;
    __asm volatile("" : "+r"(measurement_count) : : "memory");
    const uint32_t offset_pole_pairs =
        *((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    float offset_step =
        ((float)offset_pole_pairs * TWO_PI_F * offset_gear_ratio) / (float)measurement_count;
    /* Retain the pre-lock division even when parameters change
     * during the 20,000 delay/PWM iterations. */
    __asm volatile("" : "+t"(offset_step) : : "memory");
    const double electrical_step = (double)offset_step;
    /* measure_position_sensor_offset locks the d-axis at phase zero
     * and advances its double-precision phase accumulator from zero.  It
     * does not seed this pass from the live encoder angle. */
    double electrical_angle = 0.0;
    /* Snapshot the live drive-cache offset and publish stationary PWM on every
     * lock iteration. */
    const uint32_t offset_voltage_bits = runtime_parameter_cache[4];
    float offset_voltage;
    memcpy(&offset_voltage, &offset_voltage_bits, sizeof(offset_voltage));
    for (uint32_t count = 0U; count < ALIGNMENT_LOCK_STEPS; ++count)
    {
        svpwm_helper(offset_voltage, 0.0f);
        platform_commissioning_delay_us(100U);
    }
    for (uint32_t count = 0U; count < measurement_count; ++count)
    {
        float offset_atan_y;
        float offset_atan_x;
        platform_commissioning_read_offset_sample(&sample, &offset_atan_y, &offset_atan_x);
        electrical_angle += electrical_step;
        platform_commissioning_drive(offset_voltage, 0.0f, alignment_drive_angle(electrical_angle));
        platform_commissioning_finish_output_sample();
        /* offset multiplies live float poles/gear before widening
         * the denominator, then sends one CRC-bearing H frame per point. */
        const uint32_t live_poles =
            *((const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
        const float live_gear =
            *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF0D4UL, 0x1FFFF060UL));
        const float commanded_output =
            (float)(electrical_angle / (double)((float)live_poles * live_gear));
        const float measured = atan2_helper(offset_atan_y, offset_atan_x);
        send_sensor_sample('H', commanded_output, measured,
                           offset_report_error(commanded_output - measured), sample.output_raw_u,
                           sample.output_raw_v);
    }

    /* Perform the worker's single terminal neutral update. */
    svpwm_helper(0.0f, 0.0f);
    /* output-calibration prints before its ADC/INT002 epilogue and
     * does not issue a second neutral PWM update. */
    send_sensor_sample('Z', 0.0f, 0.0f, 0.0f, UINT16_MAX, UINT16_MAX);
    platform_delay_ms(1U);
    /* Widen gain, staged phase, live V, and live U in this order. */
    const double report_gain =
        (double)*((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1D0UL, 0x1FFFF15CUL));
    __asm volatile("" : : "r"(&report_gain) : "memory");
    const double report_phase =
        (double)*((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF084UL, 0x1FFFF010UL));
    __asm volatile("" : : "r"(&report_phase) : "memory");
    const double report_center_v =
        (double)*((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1C4UL, 0x1FFFF150UL));
    __asm volatile("" : : "r"(&report_center_v) : "memory");
    const double report_center_u =
        (double)*((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1BCUL, 0x1FFFF148UL));
    debug_console_printf("u=%.4f v=%.4f  w=%.4f c=%.4f\r\n", report_center_u, report_center_v,
                         report_phase, report_gain);
    platform_commissioning_restore_control_irq();
    return COMMISSIONING_OK;
}

CommissioningStatus commissioning_run_motor_identification(MotorConfig *config)
{
    if ((config == NULL) || !platform_commissioning_begin())
    {
        return COMMISSIONING_POWER_DISABLED;
    }

    /* Apply stationary alpha=0.1, beta=0 on every lock tick; this stage does
     * not derive a rotor-relative electrical angle. */
    const float locked_electrical_angle = 0.0f;
    for (uint32_t count = 0U; count < IDENTIFICATION_LOCK_STEPS; ++count)
    {
        svpwm_helper(0.1f, 0.0f);
        platform_commissioning_delay_us(100U);
    }

    PlatformCommissioningSample sample;
    float resistance;
    float inductance;
    float retained_voltage_d;
    /* The retained frame starts at angle=0, sin=0, cos=1 and
     * survives electrical, flux, excitation, and coast stages. */
    IdentificationCurrentFrame current_frame = {0.0f, 0.0f, 1.0f};
    if (!identify_electrical_parameters(locked_electrical_angle, &resistance, &inductance, &sample,
                                        &current_frame, &retained_voltage_d))
    {
        /* This protocol intentionally uses the literal "error!/r/n". */
        debug_console_printf("error!/r/n");
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
    /* Wait a second 5 ms after both fitted values pass their sign checks and
     * before initializing the flux-observer state. */
    platform_commissioning_delay_us(5000U);

    /* Continue using staging in place; do not add whole-structure SRAM
     * reads or writes through a temporary stack copy. */
    MotorConfig *const identified = config;
    float flux_linkage;
    if (!identify_flux_linkage(identified, &sample, &flux_linkage, &current_frame,
                               retained_voltage_d))
    {
        debug_console_printf("error!/r/n");
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
    float inertia;
    float damping;
    if (!identify_mechanical_parameters(identified, flux_linkage, &sample, &inertia, &damping,
                                        &current_frame))
    {
        debug_console_printf("error!/r/n");
        return COMMISSIONING_INVALID_MEASUREMENT;
    }

    /* Mechanical results have already been published during final arithmetic. */
    /* Reset the motion-observer state before constructing the result frame. */
    *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1FCUL, 0x1FFFF188UL)) = 1U;
    /* Emit the result before refreshing the cache and authenticating again in
     * derive_control_parameters. */
    send_identification_result(config);
    /* The motor-ID path authenticates both at entry and again through
     * derive_control_parameters before installing the result. */
    const volatile uint32_t *const result_words =
        (const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    __asm volatile("vldr s0, [%0, #64]\n\t"
                   "vcvt.f32.u32 s0, s0\n\t"
                   "vstr s0, [%1, #64]\n\t"
                   "vldr s0, [%0, #68]\n\t"
                   "vstr s0, [%1, #68]\n\t"
                   "vldr s0, [%0, #72]\n\t"
                   "vstr s0, [%1, #72]\n\t"
                   "vldr s0, [%0, #76]\n\t"
                   "vstr s0, [%1, #76]\n\t"
                   "vldr s0, [%0, #48]\n\t"
                   "vstr s0, [%1, #80]\n\t"
                   "vldr s0, [%0, #124]\n\t"
                   "vstr s0, [%1, #84]"
                   :
                   : "r"(result_words), "r"(runtime_parameter_cache)
                   : "s0", "memory");
    derive_control_parameters_helper();
    /* Install outer-loop gains after derivation, with speed Ki preceding Kp.
     * These fixed loop states are independent of
     * the source-owned MotorConfig and must not retain their old gains. */
    /* Derivation can update the staging words; re-read each one after that
     * call rather than using the source configuration copy. */
    const volatile float *const staging =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    __asm volatile("vldr s0, [%0, #104]\n\t"
                   "vstr s0, [%1, #4]\n\t"
                   "vldr s0, [%0, #100]\n\t"
                   "vstr s0, [%1]\n\t"
                   "vldr s0, [%0, #108]\n\t"
                   "vstr s0, [%2]\n\t"
                   "vldr s0, [%0, #112]\n\t"
                   "vstr s0, [%2, #4]"
                   :
                   : "r"(staging), "r"(&runtime_speed_loop), "r"(&runtime_position_loop)
                   : "s0", "memory");
    platform_commissioning_restore_control_irq();
    return COMMISSIONING_OK;
}
