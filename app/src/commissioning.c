#include "commissioning.h"
#include "output_sensor.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "app_profile.h"
#include "app_state.h"
#if defined(DAMIAO_DM4310)
#include "board_sampling_timer.h"
#endif
#include "debug_console.h"
#include "motor_math.h"
#include "platform.h"
#include "runtime_compat.h"
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
#define SQRT3_F                 0x1.bb67aep+0f
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

#if defined(DAMIAO_DM4310)
#define COMMISSIONING_WORK_SECTION \
    __attribute__((section(".dm4310_commissioning_work")))
#else
#define COMMISSIONING_WORK_SECTION
#endif

#if !defined(DAMIAO_DM4310)
static float g_output_sensor_result_table[OUTPUT_TABLE_POINTS]
    COMMISSIONING_WORK_SECTION;
static uint16_t g_output_sensor_result_counts[OUTPUT_TABLE_POINTS]
    COMMISSIONING_WORK_SECTION;
#endif
#if defined(DAMIAO_DM4310)
typedef struct {
    float word[10];
} RuntimeDriveState;

static RuntimeDriveState runtime_drive_d
    __attribute__((section(FACTORY_LAYOUT_SECTION(
        ".dm4310_runtime_drive_d", ".dm8009_runtime_drive_d"))));
static RuntimeDriveState runtime_drive_q
    __attribute__((section(FACTORY_LAYOUT_SECTION(
        ".dm4310_runtime_drive_q", ".dm8009_runtime_drive_q"))));
static RuntimeDriveState runtime_speed_loop
    __attribute__((section(FACTORY_LAYOUT_SECTION(
        ".dm4310_runtime_speed_loop", ".dm8009_runtime_speed_loop"))));
static RuntimeDriveState runtime_position_loop
    __attribute__((section(FACTORY_LAYOUT_SECTION(
        ".dm4310_runtime_position_loop", ".dm8009_runtime_position_loop"))));
static volatile uint32_t runtime_parameter_cache[24]
    __attribute__((section(".dm4310_runtime_parameter_cache")));
#endif

#if !defined(DAMIAO_DM4310)
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
#endif

#if defined(DAMIAO_DM4310)
static float __attribute__((noinline)) offset_report_error(float error)
{
    /* offset@0x24652..0x2466a adjusts once, with mutually exclusive
     * signed/unsigned raw-word predicates rather than float comparisons. */
    uint32_t bits;
    memcpy(&bits, &error, sizeof(bits));
    if ((int32_t)bits > (int32_t)UINT32_C(0x40490fdb)) {
        error -= TWO_PI_F;
    } else if (bits > UINT32_C(0xc0490fdb)) {
        error += TWO_PI_F;
    }
    return error;
}
#endif

static float alignment_unwrap(float current, float previous_unwrapped)
{
    float adjusted = current;
#if defined(DAMIAO_DM4310)
    /* 0x24c46/0x24c5c compare the subtraction's raw word, not VFP flags.
     * Preserve their signed/unsigned ordering, including NaN operands. */
    float difference = adjusted - previous_unwrapped;
    uint32_t bits;
    memcpy(&bits, &difference, sizeof(bits));
    if ((int32_t)bits > (int32_t)0x40800000U) {
        adjusted -= TWO_PI_F;
    }
    difference = adjusted - previous_unwrapped;
    memcpy(&bits, &difference, sizeof(bits));
    if (bits > 0xC0800000U) {
        adjusted += TWO_PI_F;
    }
#else
    if ((adjusted - previous_unwrapped) > 4.0f) {
        adjusted -= TWO_PI_F;
    }
    if ((adjusted - previous_unwrapped) < -4.0f) {
        adjusted += TWO_PI_F;
    }
#endif
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
#if defined(DAMIAO_DM4310)
    /* extrema@0x24890 emits three zero float slots and the frame CRC. */
    send_sensor_sample('h', 0.0f, 0.0f, 0.0f, raw_u, raw_v);
#else
    /* Factory DM8009 output calibration sends marker 'h' as a 13-byte raw
     * sensor snapshot: 'h', two zero float slots, then raw U/raw V.  It does
     * not use the generic 21-byte float sample frame here. */
    uint8_t frame[13] = {'h'};
    frame[9] = (uint8_t)raw_u;
    frame[10] = (uint8_t)(raw_u >> 8U);
    frame[11] = (uint8_t)raw_v;
    frame[12] = (uint8_t)(raw_v >> 8U);
    platform_debug_write(frame, sizeof(frame));
#endif
}

#if !defined(DAMIAO_DM4310)
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
#endif

#if !defined(DAMIAO_DM4310)
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

#endif

#if defined(DAMIAO_DM4310)
typedef struct {
    float angle;
    float sine;
    float cosine;
} IdentificationCurrentFrame;

static float identification_electrical_angle(const MotorConfig *config);

static void identification_drive(float voltage_d, float voltage_q,
                                 const IdentificationCurrentFrame *frame,
                                 bool mechanical_stage)
{
    float alpha;
    float beta;
    /* 0x25a84..0x25aa0: inverse Park with the existing IRQ frame;
     * neither wrap nor sincos is called on this output path. */
    if (mechanical_stage) {
        /* Excitation/coast use two VMULs before the cross terms. */
        __asm volatile (
        "vmul.f32 %0, %2, %3\n"
        "vmul.f32 %1, %4, %3\n"
        "vmls.f32 %0, %4, %5\n"
        "vmla.f32 %1, %2, %5\n"
        : "=&t" (alpha), "=&t" (beta)
        : "t" (frame->cosine), "t" (voltage_d),
          "t" (frame->sine), "t" (voltage_q));
    } else {
        __asm volatile (
            "vmul.f32 %0, %2, %3\n"
            "vmls.f32 %0, %4, %5\n"
            "vmul.f32 %1, %4, %3\n"
            "vmla.f32 %1, %2, %5\n"
            : "=&t" (alpha), "=&t" (beta)
            : "t" (frame->cosine), "t" (voltage_d),
              "t" (frame->sine), "t" (voltage_q));
    }
    dm4310_svpwm_helper(alpha, beta);
}
#endif

static DirectQuadrature commissioning_current_dq(
    const PlatformCommissioningSample *sample,
#if defined(DAMIAO_DM4310)
    IdentificationCurrentFrame *frame, bool parallel_products,
    float *scale_snapshot)
#else
    float electrical_angle)
#endif
{
    float sine;
    float cosine;
#if defined(DAMIAO_DM4310)
    float beta = sample->current_u;
    __asm volatile ("vmla.f32 %0, %1, %2"
                    : "+t" (beta)
                    : "t" (sample->current_v), "t" (2.0f));
    beta *= 0x1.279a74p-1f;
    __asm volatile ("" : "+t" (beta) : : "memory");
    if (*((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) != 0U) {
        *((volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) = 0U;
        frame->angle = identification_electrical_angle(NULL);
        dm4310_wrap_helper(&frame->angle,
                          -0x1.921fb6p+1f, 0x1.921fb6p+1f);
        motor_target_sincos(frame->angle, &frame->sine, &frame->cosine);
    }
    sine = frame->sine;
    cosine = frame->cosine;
    DirectQuadrature result;
    /* 0x25a2e..0x25a3c uses VMLA/VMLS, preserving both the
     * binary32 intermediate rounding and factory operand order. */
    if (scale_snapshot != NULL) {
        /* Flux@0x25cfe..0x25d18 snapshots the live scale between
         * the two seed products and their cross terms. */
        float scale;
        __asm volatile (
            "vmul.f32 %0, %3, %4\n"
            "vmul.f32 %1, %3, %6\n"
            "vldr %2, [%7]\n"
            "vmla.f32 %0, %5, %6\n"
            "vmls.f32 %1, %5, %4\n"
            : "=&t" (result.d), "=&t" (result.q), "=&t" (scale)
            : "t" (cosine), "t" (sample->current_u),
              "t" (sine), "t" (beta), "r" (FACTORY_SRAM_ADDRESS(0x1FFFF244UL, 0x1FFFF1D0UL))
            : "memory");
        *scale_snapshot = scale;
    } else if (parallel_products) {
        __asm volatile (
            "vmul.f32 %0, %2, %3\n"
            "vmul.f32 %1, %2, %5\n"
            "vmla.f32 %0, %4, %5\n"
            "vmls.f32 %1, %4, %3\n"
            : "=&t" (result.d), "=&t" (result.q)
            : "t" (cosine), "t" (sample->current_u),
              "t" (sine), "t" (beta));
    } else {
        __asm volatile (
        "vmul.f32 %0, %2, %3\n"
        "vmla.f32 %0, %4, %5\n"
        "vmul.f32 %1, %2, %5\n"
        "vmls.f32 %1, %4, %3\n"
        : "=&t" (result.d), "=&t" (result.q)
        : "t" (cosine), "t" (sample->current_u),
          "t" (sine), "t" (beta));
    }
    return result;
#else
    motor_target_sincos(motor_wrapf(electrical_angle,
                                  -0x1.921fb6p+1f,
                                  0x1.921fb6p+1f),
                      &sine, &cosine);
    return motor_park(motor_clarke(sample->current_u, sample->current_v),
                      sine, cosine);
#endif
}

#if defined(DAMIAO_DM4310)
static float commissioning_reduce_float_angle(float travel);

static float identification_target_phase(float frequency, uint32_t count)
{
    float phase;
    __asm volatile (
        "vmul.f32 %0, %1, %2\n"
        "vmul.f32 %0, %0, %3\n"
        : "=&t" (phase)
        : "t" (frequency), "t" ((float)count),
          "t" (IDENTIFICATION_PHASE_BASE_STEP));
    return phase;
}

static float identification_flux_speed_window(float previous_speed)
{
    const uint32_t poles = *((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    const float delta = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL));
    float raw_speed;
    float pole_count;
    __asm volatile (
        "vmov %1, %3\n"
        "vcvt.f32.u32 %1, %1\n"
        "vmul.f32 %0, %2, %1\n"
        "vmul.f32 %0, %0, %4\n"
        : "=&t" (raw_speed), "=&t" (pole_count)
        : "t" (delta), "r" (poles), "t" (1000.0f)
        : "memory");
    const float previous_weight = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0F0UL, 0x1FFFF07CUL));
    float speed;
    __asm volatile ("vmul.f32 %0, %1, %2"
                    : "=t" (speed)
                    : "t" (previous_weight), "t" (previous_speed)
                    : "memory");
    const float new_weight = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0F4UL, 0x1FFFF080UL));
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
    __asm volatile ("vmla.f32 %0, %1, %2"
                    : "+t" (speed) : "t" (raw_speed), "t" (new_weight)
                    : "memory");
    return speed;
}

static bool identification_fit_sign_accepted(float value)
{
    uint32_t accepted;
    __asm volatile ("vcmpe.f32 %1, #0.0\n"
                    "vmrs APSR_nzcv, fpscr\n"
                    "mov.w %0, #1\n"
                    "it cc\n"
                    "movcc %0, #0"
                    : "=r" (accepted) : "t" (value) : "cc");
    return accepted != 0U;
}
#endif

static bool identify_electrical_parameters(float electrical_angle,
                                           float *resistance,
                                           float *inductance,
                                           PlatformCommissioningSample *sample
#if defined(DAMIAO_DM4310)
                                           , IdentificationCurrentFrame *frame,
                                           float *retained_voltage_d
#endif
                                           )
{
    CommissioningRls2 estimator;
#if defined(DAMIAO_DM4310)
    (void)electrical_angle;
#endif
#if !defined(DAMIAO_DM4310)
    commissioning_rls2_init(&estimator);
#endif
    float target_amplitude = 0.0f;
    float target_current = 0.0f;
    float applied_voltage = 0.0f;
#if defined(DAMIAO_DM4310)
    /* 0x25860..0x2586c reads cache current scale before target current;
     * both are runtime values, not immutable profile constants. */
    const volatile float *const parameter_cache =
        (const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    const float current_scale = parameter_cache[2];
    const float target_current_amplitude = parameter_cache[5];
    float amplitude_step;
    float amplitude_divisor;
    __asm volatile ("vmul.f32 %1, %2, %3\n"
                    "vdiv.f32 %0, %4, %1"
                    : "=&t" (amplitude_step), "=&t" (amplitude_divisor)
                    : "t" (current_scale),
                      "t" ((float)IDENTIFICATION_TARGET_RAMP_DIVIDER),
                      "t" (target_current_amplitude) : "memory");
    commissioning_rls2_init(&estimator);
#else
    const float amplitude_step = IDENTIFICATION_TARGET_CURRENT_A /
        (CURRENT_FULL_SCALE_A *
         (float)IDENTIFICATION_TARGET_RAMP_DIVIDER);
#endif

#if defined(DAMIAO_DM4310)
    /* 0x2589c..0x258a6 clears conversions after the ramp snapshot and
     * estimator initialization, immediately before entering the sample loop. */
    platform_commissioning_finish_sample();
#endif
    for (uint32_t count = 1U;
         count <= IDENTIFICATION_ELECTRICAL_STEPS; ++count) {
#if defined(DAMIAO_DM4310)
        platform_commissioning_wait_identification_sample();
#else
        if (!platform_commissioning_read_identification_sample(sample, true)) {
            return false;
        }
#endif
        if ((count <= IDENTIFICATION_TARGET_RAMP_STEPS) &&
            ((count % IDENTIFICATION_TARGET_RAMP_DIVIDER) == 0U)) {
#if defined(DAMIAO_DM4310)
            __asm volatile ("vadd.f32 %0, %0, %1"
                            : "+t" (target_amplitude)
                            : "t" (amplitude_step) : "memory");
#else
            target_amplitude += amplitude_step;
#endif
        }
        if ((count % IDENTIFICATION_TARGET_UPDATE_DIVIDER) == 0U) {
#if defined(DAMIAO_DM4310)
            const float unwrapped_phase = identification_target_phase(
                IDENTIFICATION_TARGET_FREQUENCY_HZ, count);
#else
            const float unwrapped_phase =
                (IDENTIFICATION_TARGET_FREQUENCY_HZ * (float)count) *
                IDENTIFICATION_PHASE_BASE_STEP;
#endif
#if defined(DAMIAO_DM4310)
            float phase = commissioning_reduce_float_angle(unwrapped_phase);
            dm4310_wrap_helper(&phase, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
#else
            const float phase = motor_wrapf(
                unwrapped_phase,
                -0x1.921fb6p+1f, 0x1.921fb6p+1f);
#endif
            float sine;
            float cosine;
            motor_target_sincos(phase, &sine, &cosine);
            (void)cosine;
#if defined(DAMIAO_DM4310)
            __asm volatile ("vmul.f32 %0, %1, %2"
                            : "=t" (target_current)
                            : "t" (target_amplitude), "t" (sine)
                            : "memory");
#else
            target_current = target_amplitude * sine;
#endif
        }

#if defined(DAMIAO_DM4310)
        if (!platform_commissioning_read_identification_sample(sample, true)) {
            return false;
        }
#endif
        const DirectQuadrature current =
            commissioning_current_dq(sample,
#if defined(DAMIAO_DM4310)
                                     frame, false, NULL);
#else
                                     electrical_angle);
#endif
        if (count > IDENTIFICATION_RLS_START_STEP) {
#if defined(DAMIAO_DM4310)
            const float regression_scale = parameter_cache[2];
            float regression_current;
            __asm volatile ("vmul.f32 %0, %1, %2"
                            : "=t" (regression_current)
                            : "t" (regression_scale), "t" (current.d));
            volatile CommissioningRls2 *const regression_state = &estimator;
            regression_state->measurement = regression_current;
            const float projected_bus =
                *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF170UL, 0x1FFFF0FCUL));
            float regression_voltage;
            __asm volatile ("vmul.f32 %0, %1, %2"
                            : "=t" (regression_voltage)
                            : "t" (projected_bus), "t" (applied_voltage));
            regression_state->applied_voltage = regression_voltage;
            dm4310_rls2_helper(&estimator);
#else
            commissioning_rls2_step(
                &estimator, current.d * CURRENT_FULL_SCALE_A,
                sample->bus_voltage * INV_SQRT3_F * applied_voltage);
#endif
        }
#if defined(DAMIAO_DM4310)
        /* 0x25a64/0x25a68 reload gain and voltage bound each tick. */
        float current_error = target_current - current.d;
        __asm volatile ("" : "+t" (current_error) : : "memory");
        const float current_gain = parameter_cache[6];
        const float voltage_limit = parameter_cache[7];
        float requested_voltage;
        float applied_voltage_q;
        /* 0x25a6c/0x25a70 computes both axes before clamping only D.
         * Q is deliberately not clamped; retain VNMUL's NaN/sign rules. */
        __asm volatile ("vmul.f32 %0, %2, %3\n"
                        "vnmul.f32 %1, %2, %4"
                        : "=&t" (requested_voltage), "=&t" (applied_voltage_q)
                        : "t" (current_gain), "t" (current_error),
                          "t" (current.q));
        applied_voltage = dm4310_clamp_helper(
            requested_voltage,
            -voltage_limit, voltage_limit);
#else
        applied_voltage = motor_clampf(
            IDENTIFICATION_CURRENT_GAIN * (target_current - current.d),
            -IDENTIFICATION_VOLTAGE_LIMIT,
            IDENTIFICATION_VOLTAGE_LIMIT);
#endif
#if defined(DAMIAO_DM4310)
        identification_drive(applied_voltage, applied_voltage_q, frame, false);
#else
        platform_commissioning_drive(applied_voltage, 0.0f,
                                     electrical_angle);
#endif
        platform_commissioning_finish_sample();
    }
    platform_commissioning_delay_us(5000U);
#if defined(DAMIAO_DM4310)
    /* 0x25ad0 calls stationary SVPWM directly, with no wrap/sincos. */
    dm4310_svpwm_helper(0.0f, 0.0f);
#else
    platform_commissioning_drive(0.0f, 0.0f, electrical_angle);
#endif
#if defined(DAMIAO_DM4310)
    /* 0x25ae2..0x25b0c publishes both fits before checking their signs.
     * BCC rejects negative values, but not unordered values (C=1). */
    volatile float *const staging = (volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    /* 0x25ad4 reloads the live sample period after the neutral update. */
    const float fitted_sample_period = parameter_cache[3];
    const float fitted_inductance = fitted_sample_period / estimator.coefficient_b;
    staging[18] = fitted_inductance;
    const float fitted_resistance =
        (1.0f - estimator.coefficient_a) / estimator.coefficient_b;
    staging[17] = fitted_resistance;
    *inductance = fitted_inductance;
    *resistance = fitted_resistance;
    /* s23 remains live across 0x25b24 and seeds the flux D-axis command. */
    *retained_voltage_d = applied_voltage;
    if (!identification_fit_sign_accepted(fitted_resistance)) {
        return false;
    }
    return identification_fit_sign_accepted(fitted_inductance);
#else
    return commissioning_rls2_motor_parameters(
        &estimator, 0.00005f, resistance, inductance);
#endif
}

#if defined(DAMIAO_DM4310)
static float identification_electrical_angle_from_poles(uint32_t poles)
{
    const float position = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF198UL, 0x1FFFF124UL));
    const float turns_per_radian =
        *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0E0UL, 0x1FFFF06CUL));
    float angle = position * (float)poles;
    float turns = position * turns_per_radian;
    __asm volatile (
        "vcvt.u32.f32 %1, %1\n"
        "vcvt.f32.u32 %1, %1\n"
        "vmls.f32 %0, %1, %2\n"
        : "+t" (angle), "+t" (turns)
        : "t" (TWO_PI_F));
    const float offset = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF09CUL, 0x1FFFF028UL));
    return angle + offset;
}
#endif

static float identification_electrical_angle(const MotorConfig *config)
{
#if defined(DAMIAO_DM4310)
    (void)config;
    return identification_electrical_angle_from_poles(
        *((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL)));
#else
    return g_app.rotor_angle * (float)config->pole_pairs +
           g_app.motor.electrical_offset;
#endif
}

static bool identification_filter_pair_init(
    const MotorConfig *config, float bus_voltage,
    CommissioningIdentificationFilter *axis_d,
    CommissioningIdentificationFilter *axis_q
#if defined(DAMIAO_DM4310)
    , float *retained_current_scale
#endif
    )
{
    if ((config == NULL) || (axis_d == NULL) || (axis_q == NULL)) {
        return false;
    }
    float proportional_gain = 0.0f;
    float integral_gain = 0.0f;
#if defined(DAMIAO_DM4310)
    /* Mechanical entry@0x25e1e..0x25e6a derives both filters without
     * the UV gating used by the distinct control-parameter helper. */
    (void)bus_voltage;
    const volatile float *const staging = (const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    const volatile float *const cache = (const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    const float resistance = staging[17];
    const float inductance = staging[18];
    float resistance_ratio = resistance / inductance;
    __asm volatile ("" : "+t" (resistance_ratio) : : "memory");
    const float sample_period = cache[3];
    integral_gain = resistance_ratio * sample_period;
    const float bandwidth = staging[24];
    const float current_scale = cache[2];
    *retained_current_scale = current_scale;
    float scaled_bandwidth = (bandwidth * current_scale) * inductance;
    __asm volatile ("" : "+t" (scaled_bandwidth) : : "memory");
    const float sampled_bus = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF16CUL, 0x1FFFF0F8UL));
    proportional_gain = (scaled_bandwidth / sampled_bus) * SQRT3_F;
#else
    /* derive_control_parameters@0x25138 zeros both commissioning filters
     * when the sampled bus voltage is not above the stored UV threshold. */
    if (bus_voltage > config->bus_undervoltage) {
        proportional_gain =
            ((config->current_loop_bandwidth * config->phase_inductance *
              CURRENT_FULL_SCALE_A) / bus_voltage) * SQRT3_F;
        integral_gain =
            (config->phase_resistance / config->phase_inductance) * 0.00005f;
    }
#endif
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

void commissioning_configure_runtime_drive_states(const MotorConfig *config,
                                                   float bus_voltage)
{
#if defined(DAMIAO_DM4310)
    (void)config;
    const volatile float *const staging = (const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    const volatile float *const cache = (const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    const float voltage = bus_voltage;
    const float undervoltage = staging[0];
    /* derive_control_parameters@0x25138 updates only words 0 and 1 of both
     * fixed states.  In particular UgQ must not reset accumulated state. */
    volatile RuntimeDriveState *const d = &runtime_drive_d;
    volatile RuntimeDriveState *const q = &runtime_drive_q;
    if (voltage > undervoltage) {
        const float bandwidth = cache[14];
        const float inductance = cache[18];
        const float current_scale = cache[2];
        const float proportional_gain =
            (((bandwidth * inductance) * current_scale) / voltage) * SQRT3_F;
        d->word[0] = proportional_gain;
        q->word[0] = proportional_gain;
        const float resistance = cache[17];
        float resistance_ratio;
        __asm volatile ("vdiv.f32 %0, %1, %2"
                        : "=t" (resistance_ratio)
                        : "t" (resistance), "t" (inductance) : "memory");
        const float period = cache[3];
        const float integral_gain = resistance_ratio * period;
        d->word[1] = integral_gain;
        q->word[1] = integral_gain;
    } else {
        d->word[0] = 0.0f;
        q->word[0] = 0.0f;
        d->word[1] = 0.0f;
        q->word[1] = 0.0f;
    }
#else
    (void)config;
    (void)bus_voltage;
#endif
}

void commissioning_initialize_runtime_drive_states(void)
{
#if defined(DAMIAO_DM4310)
    memset(&runtime_drive_d, 0, sizeof(runtime_drive_d));
    memset(&runtime_drive_q, 0, sizeof(runtime_drive_q));
    memset(&runtime_speed_loop, 0, sizeof(runtime_speed_loop));
    memset(&runtime_position_loop, 0, sizeof(runtime_position_loop));
#endif
}

#if defined(DAMIAO_DM4310)
void commissioning_initialize_scatter_defaults(void)
{
    /* Factory initialized drive/loop objects before application main. */
    const RuntimeDriveState defaults[4] = {
        {.word = {[0] = 0.8f, [1] = 0.001f, [8] = -1.0f, [9] = 1.0f}},
        {.word = {[0] = 0.8f, [1] = 0.001f, [8] = -1.0f, [9] = 1.0f}},
        {.word = {[0] = 0.4f, [1] = 0.002f, [8] = -1.0f, [9] = 1.0f}},
        {.word = {[0] = 2.0f, [8] = -600.0f, [9] = 600.0f}},
    };
    volatile RuntimeDriveState *const states[4] = {
        &runtime_drive_d, &runtime_drive_q,
        &runtime_speed_loop, &runtime_position_loop,
    };
    for (unsigned int state = 0U; state < 4U; ++state) {
        for (unsigned int word = 0U; word < 10U; ++word) {
            states[state]->word[word] = defaults[state].word[word];
        }
    }
}
#endif

void commissioning_reset_runtime_loop_states(volatile float *speed, float value)
{
#if defined(DAMIAO_DM4310)
    /* reset_control_state@0x2a448 clears only the dynamic words 2, 4, 5, 6
     * and 7 of both outer-loop states.  Gains, limits and configuration
     * words survive every disabled/faulted 20 kHz tick. */
    speed[2] = value;
    speed[4] = value;
    speed[7] = value;
    speed[5] = value;
    speed[6] = value;
    volatile float *const position = (volatile float *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA12CUL, 0x1FFFA164UL);
    position[2] = value;
    position[4] = value;
    position[7] = value;
    position[5] = value;
    position[6] = value;
#else
    (void)speed;
    (void)value;
#endif
}

#if defined(DAMIAO_DM4310)
static inline float commissioning_ordered_multiply(float left, float right)
{
    float result;
    __asm volatile ("vmul.f32 %0, %1, %2"
                    : "=t" (result) : "t" (left), "t" (right));
    return result;
}

void commissioning_derive_runtime_controller_states(void)
{
    /* Standalone factory entry 0x1fff9ea6 reads the fixed staging image,
     * not g_app.config. Preserve its paired stores and cached scale reads.
     * Sample/current-controller/motor-state layouts belong to motor_control.c. */
    volatile float *const sample = (volatile float *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA11CUL, 0x1FFFA154UL);
    const volatile float *const config = (const volatile float *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA13CUL, 0x1FFFA174UL);
    const volatile float *const cache = (const volatile float *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA140UL, 0x1FFFA178UL);
    const float voltage = sample[26];
    const float undervoltage = config[0];
    const float current_scale = cache[2];
    volatile RuntimeDriveState *const d = (volatile RuntimeDriveState *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA120UL, 0x1FFFA158UL);
    volatile RuntimeDriveState *const q = (volatile RuntimeDriveState *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA124UL, 0x1FFFA15CUL);
    volatile float *const current_d = (volatile float *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA130UL, 0x1FFFA168UL);
    volatile float *const current_q = (volatile float *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA134UL, 0x1FFFA16CUL);
    if (voltage > undervoltage) {
        const float bandwidth = config[24];
        const float inductance = config[18];
        float proportional = ((bandwidth * inductance) * current_scale) / voltage;
        __asm volatile ("" : "+t" (proportional) : : "memory");
        const float proportional_scale = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFFA144UL, 0x1FFFA17CUL);
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
        /* 0x1fff9f08..1c publishes both periods before gain arithmetic. */
        __asm volatile ("vmul.f32 %0, %1, %2"
                        : "=t" (denominator)
                        : "t" (current_scale), "t" (inductance) : "memory");
        const float gain_scale = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFFA148UL, 0x1FFFA180UL);
        __asm volatile (
            "vmul.f32 %0, %2, %3\n"
            "vdiv.f32 %0, %0, %1"
            : "=&t" (input_gain)
            : "t" (denominator), "t" (voltage), "t" (gain_scale) : "memory");
        current_d[8] = input_gain;
        float inverse_gain;
        /* 0x1fff9f24..30 divides between the D and Q gain stores. */
        __asm volatile ("vdiv.f32 %0, %1, %2"
                        : "=t" (inverse_gain)
                        : "t" (1.0f), "t" (input_gain) : "memory");
        current_q[8] = input_gain;
        current_d[9] = inverse_gain;
        current_q[9] = inverse_gain;
        current_d[0] = bandwidth;
        current_q[0] = bandwidth;
        const float observer_gain = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFFA14CUL, 0x1FFFA184UL);
        current_d[7] = observer_gain;
        current_q[7] = observer_gain;
        const float observer_gain_twice = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFFA150UL, 0x1FFFA188UL);
        current_d[11] = observer_gain_twice;
        current_q[11] = observer_gain_twice;
        const float observer_gain_squared = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFFA154UL, 0x1FFFA18CUL);
        current_d[12] = observer_gain_squared;
        current_q[12] = observer_gain_squared;
        current_d[17] = -1.0f;
        current_q[17] = -1.0f;
        current_d[18] = 1.0f;
        current_q[18] = 1.0f;
    } else {
        const float cleared = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFFA118UL, 0x1FFFA150UL);
        d->word[0] = cleared;
        q->word[0] = cleared;
        d->word[1] = cleared;
        q->word[1] = cleared;
        current_d[0] = cleared;
        current_q[0] = cleared;
    }
    const uint32_t pole_pairs =
        ((const volatile uint32_t *)config)[16];
    volatile float *const motor = (volatile float *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA158UL, 0x1FFFA190UL);
    float pole_factor;
    /* 0x1fff9fa4 completes this multiply before the flux load at 9fa8. */
    __asm volatile ("vmul.f32 %0, %1, %2"
                    : "=t" (pole_factor)
                    : "t" ((float)pole_pairs), "t" (1.5f) : "memory");
    const float flux = config[19];
    float torque;
    __asm volatile (
        "vmul.f32 %0, %1, %2\n"
        "vmul.f32 %0, %0, %3"
        : "=&t" (torque)
        : "t" (pole_factor), "t" (flux), "t" (current_scale) : "memory");
    /* The factory evaluates this even when the override discards it. Keep
     * the floating-point exceptions and evaluation before the next load. */
    __asm volatile ("" : "+t" (torque) : : "memory");
    const float override = config[1];
    /* VCMPE, unlike GCC's equality VCMP, raises IOC for quiet NaNs too. */
    __asm volatile ("vcmpe.f32 %0, #0.0" : : "t" (override) : "cc");
    if (override == 0.0f) {
        const float ratio = config[20];
        const float correction = config[30];
        motor[17] = commissioning_ordered_multiply(
            commissioning_ordered_multiply(ratio, torque), correction);
    } else {
        /* Keep operand order when FPSCR.DN is clear: two NaN operands can
         * otherwise select a different payload after GCC swaps VMUL inputs. */
        motor[17] = commissioning_ordered_multiply(override, current_scale);
    }
}

void commissioning_clear_runtime_loop_states(void)
{
    /* Unreferenced factory 0x1fff9d5c is distinct from reset@0x1fff9dc8:
     * it clears sample targets plus the D/Q and both outer-loop states, but
     * does not clear the current observers. Sample layout is owned by
     * motor_control.c; these are its fixed words +0x1c, +0x20 and +0x2c. */
    const float cleared = *(volatile const float *)FACTORY_SRAM_ADDRESS(0x1FFFA118UL, 0x1FFFA150UL);
    volatile float *const sample = (volatile float *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA11CUL, 0x1FFFA154UL);
    volatile RuntimeDriveState *const q = (volatile RuntimeDriveState *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA124UL, 0x1FFFA15CUL);
    sample[7] = cleared;
    sample[8] = cleared;
    sample[11] = cleared;
    volatile RuntimeDriveState *const d = (volatile RuntimeDriveState *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA120UL, 0x1FFFA158UL);
    d->word[2] = cleared;
    q->word[2] = cleared;
    d->word[4] = cleared;
    q->word[4] = cleared;
    d->word[7] = cleared;
    q->word[7] = cleared;
    d->word[5] = cleared;
    q->word[5] = cleared;
    d->word[6] = cleared;
    volatile RuntimeDriveState *const speed = (volatile RuntimeDriveState *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA128UL, 0x1FFFA160UL);
    q->word[6] = cleared;
    commissioning_reset_runtime_loop_states(speed->word, cleared);
}
#endif

void commissioning_configure_runtime_loop_states(const MotorConfig *config)
{
#if defined(DAMIAO_DM4310)
    if (config == NULL) {
        return;
    }
    /* The three factory callers copy staging words 25..28 after parameter
     * derivation.  Only the first two words of either 0x28-byte state change. */
    const volatile float *const staging =
        (const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    volatile RuntimeDriveState *const speed = &runtime_speed_loop;
    volatile RuntimeDriveState *const position = &runtime_position_loop;
    __asm volatile ("vldr s0, [%0, #100]\n\t"
                    "vstr s0, [%1]\n\t"
                    "vldr s0, [%0, #104]\n\t"
                    "vstr s0, [%1, #4]\n\t"
                    "vldr s0, [%0, #108]\n\t"
                    "vstr s0, [%2]\n\t"
                    "vldr s0, [%0, #112]\n\t"
                    "vstr s0, [%2, #4]"
                    : : "r" (staging), "r" (speed), "r" (position)
                    : "s0", "memory");
#else
    (void)config;
#endif
}

#if defined(DAMIAO_DM4310)
static uint32_t runtime_float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}
#endif

void commissioning_initialize_runtime_parameter_cache(const MotorConfig *config)
{
#if defined(DAMIAO_DM4310)
    if (config == NULL) {
        return;
    }
    const volatile uint32_t *const staging =
        (const volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558));
    /* 0x229a2..a04 uses this non-linear constant-store order.  Every cache
     * word is published exactly once; words 13..21 come directly from fixed
     * staging rather than through source-only MotorConfig mirrors. */
    runtime_parameter_cache[0] = APP_PROFILE_RUNTIME_CACHE_0_BITS;
    runtime_parameter_cache[1] = UINT32_C(0x42200a9e);
    runtime_parameter_cache[2] = APP_PROFILE_CURRENT_FULL_SCALE_BITS;
    runtime_parameter_cache[3] = UINT32_C(0x3851b717);
    runtime_parameter_cache[4] = UINT32_C(0x3dcccccd);
    runtime_parameter_cache[5] =
        APP_PROFILE_IDENTIFICATION_TARGET_CURRENT_BITS;
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
    __asm volatile (
        "vldr s0, [%0, #64]\n\t"
        "vcvt.f32.u32 s0, s0\n\t"
        "vstr s0, [%1, #64]"
        : : "r" (staging), "r" (runtime_parameter_cache)
        : "s0", "memory");
    runtime_parameter_cache[17] = staging[17];
    runtime_parameter_cache[18] = staging[18];
    runtime_parameter_cache[19] = staging[19];
    runtime_parameter_cache[20] = staging[12];
    runtime_parameter_cache[21] = staging[31];
    runtime_parameter_cache[22] = UINT32_C(0x447a0000);
    runtime_parameter_cache[23] = UINT32_C(0x44160000);
#else
    (void)config;
#endif
}

void commissioning_update_runtime_parameter_cache_ugq(const MotorConfig *config)
{
#if defined(DAMIAO_DM4310)
    if (config != NULL) {
        const volatile uint32_t *const staging =
            (const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
        /* UgQ 0x26d88..8c transports this word through s0 unchanged. */
        __asm volatile ("vldr s0, [%0, #96]\n\t"
                        "vstr s0, [%1, #56]"
                        : : "r" (staging), "r" (runtime_parameter_cache)
                        : "s0", "memory");
    }
#else
    (void)config;
#endif
}

void commissioning_update_runtime_parameter_cache_motor_id(
    const MotorConfig *config)
{
#if defined(DAMIAO_DM4310)
    if (config == NULL) {
        return;
    }
    runtime_parameter_cache[16] = runtime_float_bits((float)config->pole_pairs);
    runtime_parameter_cache[17] = runtime_float_bits(config->phase_resistance);
    runtime_parameter_cache[18] = runtime_float_bits(config->phase_inductance);
    runtime_parameter_cache[19] = runtime_float_bits(config->flux_linkage);
    runtime_parameter_cache[20] = runtime_float_bits(config->rotor_inertia);
    runtime_parameter_cache[21] = runtime_float_bits(config->speed_loop_damping);
#else
    (void)config;
#endif
}

static void identification_current_filter_step(
    CommissioningIdentificationFilter *axis_d,
    CommissioningIdentificationFilter *axis_q,
    DirectQuadrature measured_current, float target_d, float target_q,
    float electrical_angle, float *voltage_d, float *voltage_q
#if defined(DAMIAO_DM4310)
    , const IdentificationCurrentFrame *frame
#endif
    )
{
#if defined(DAMIAO_DM4310)
    (void)target_d;
    float error_d;
    float error_q;
    /* Excitation@0x261c8 and coast@0x26326 negate D directly;
     * Q retains the VSUB target-current operation in both stages. */
    __asm volatile (
        "vneg.f32 %0, %2\n"
        "vsub.f32 %1, %3, %4\n"
        : "=&t" (error_d), "=&t" (error_q)
        : "t" (measured_current.d), "t" (target_q),
          "t" (measured_current.q));
    axis_d->input = error_d;
    axis_q->input = error_q;
    __asm volatile ("" : : : "memory");
#else
    axis_d->input = target_d - measured_current.d;
    axis_q->input = target_q - measured_current.q;
#endif
#if defined(DAMIAO_DM4310)
    dm4310_identification_filter_helper(axis_d);
    dm4310_identification_filter_helper(axis_q);
#else
    commissioning_identification_filter_step(axis_d);
    commissioning_identification_filter_step(axis_q);
#endif
    *voltage_d = axis_d->limited_output;
    *voltage_q = axis_q->limited_output;
#if defined(DAMIAO_DM4310)
    (void)electrical_angle;
    identification_drive(*voltage_d, *voltage_q, frame, true);
#else
    platform_commissioning_drive(*voltage_d, *voltage_q, electrical_angle);
#endif
}

static bool identify_flux_linkage(
    const MotorConfig *config, PlatformCommissioningSample *sample,
    float *flux_linkage
#if defined(DAMIAO_DM4310)
    , IdentificationCurrentFrame *frame, float initial_voltage_d
#endif
    )
{
    CommissioningFluxObserver observer;
    if ((config == NULL) || (sample == NULL) || (flux_linkage == NULL)) {
        return false;
    }
#if defined(DAMIAO_DM4310)
    /* 0x25b26,0x25b4c,0x25b58 snapshot L, sample period, then R from
     * their factory owners, not the source-owned configuration copy. */
    const volatile float *const staging = (const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    const float observer_inductance = staging[18];
    const float observer_sample_period =
        *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF248UL, 0x1FFFF1D4UL));
    const float observer_resistance = staging[17];
    if (!commissioning_flux_observer_init(
            &observer, observer_inductance, observer_resistance,
            observer_sample_period)) {
#else
    if (
        !commissioning_flux_observer_init(
            &observer, config->phase_inductance,
            config->phase_resistance, 0.00005f)) {
#endif
        return false;
    }
#if !defined(DAMIAO_DM4310)
    float previous_rotor_position = g_app.rotor_position;
#endif
    float electrical_speed = 0.0f;
#if defined(DAMIAO_DM4310)
    /* Factory carries electrical-fit s23 into the flux loop. */
    float voltage_d = initial_voltage_d;
#else
    float voltage_d = 0.0f;
#endif
    float voltage_q = 0.0f;
#if defined(DAMIAO_DM4310)
    /* 0x25b80 clears IRQ001's accumulated angle delta before ADC ACK;
     * 0x25b9a then clears sample_ready immediately before polling. */
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
    platform_commissioning_finish_sample();
    *((volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) = 0U;
#endif
    for (uint32_t count = 1U; count <= IDENTIFICATION_FLUX_STEPS; ++count) {
#if defined(DAMIAO_DM4310)
        platform_commissioning_wait_identification_sample();
#endif
        if (!platform_commissioning_read_identification_sample(sample, true)) {
            return false;
        }
#if !defined(DAMIAO_DM4310)
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
#endif

#if defined(DAMIAO_DM4310)
        float observer_current_scale;
        const DirectQuadrature current = commissioning_current_dq(
            sample, frame, true, &observer_current_scale);
        const float electrical_angle = frame->angle;
#else
        const float electrical_angle =
            identification_electrical_angle(config);
        const DirectQuadrature current =
            commissioning_current_dq(sample, electrical_angle);
#endif
#if defined(DAMIAO_DM4310)
        /* The Park path retained the live scale snapshot; voltage feedback
         * still uses the unscaled current.d below, as the factory does. */
        volatile CommissioningFluxObserver *const observer_state = &observer;
        float measured_d;
        float measured_q;
        __asm volatile ("vmul.f32 %0, %2, %3\n"
                        "vmul.f32 %1, %2, %4"
                        : "=&t" (measured_d), "=&t" (measured_q)
                        : "t" (observer_current_scale), "t" (current.d),
                          "t" (current.q));
        observer_state->measured_current_d = measured_d;
        observer_state->measured_current_q = measured_q;
        observer_state->electrical_speed = electrical_speed;
        /* 0x25d28 loads the already-projected sample word, then multiplies
         * D and Q with that operand first. Do not reassociate three factors. */
        const float observer_projected_bus =
            *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF170UL, 0x1FFFF0FCUL));
        float measured_voltage_d;
        float measured_voltage_q;
        __asm volatile ("vmul.f32 %0, %2, %3\n"
                        "vmul.f32 %1, %2, %4"
                        : "=&t" (measured_voltage_d), "=&t" (measured_voltage_q)
                        : "t" (observer_projected_bus), "t" (voltage_d),
                          "t" (voltage_q));
        observer_state->voltage_d = measured_voltage_d;
        observer_state->voltage_q = measured_voltage_q;
#else
        observer.measured_current_d = current.d * CURRENT_FULL_SCALE_A;
        observer.measured_current_q = current.q * CURRENT_FULL_SCALE_A;
        observer.voltage_d =
            voltage_d * sample->bus_voltage * INV_SQRT3_F;
        observer.voltage_q =
            voltage_q * sample->bus_voltage * INV_SQRT3_F;
        observer.electrical_speed = electrical_speed;
#endif
#if defined(DAMIAO_DM4310)
        dm4310_flux_observer_helper(&observer);
#else
        commissioning_flux_observer_step(&observer);
#endif

#if defined(DAMIAO_DM4310)
        /* 0x25d3c executes the observer before 0x25d40's window update. */
        if ((count % IDENTIFICATION_OBSERVER_WINDOW) == 0U) {
            electrical_speed = identification_flux_speed_window(electrical_speed);
            /* 0x25d78 precedes the live limit load; retain VADD operand
             * order and prevent the compiler moving that load earlier. */
            __asm volatile ("vadd.f32 %0, %0, %1"
                            : "+t" (voltage_q)
                            : "t" (IDENTIFICATION_OBSERVER_RAMP_STEP)
                            : "memory");
            const float q_limit = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF25CUL, 0x1FFFF1E8UL));
            /* 0x25d80 compares limit against ramp; BHI also preserves
             * unordered ramp inputs, so a C min/max is not equivalent. */
            __asm volatile ("vcmpe.f32 %1, %0\n"
                            "vmrs APSR_nzcv, fpscr\n"
                            "it ls\n"
                            "vmovls.f32 %0, %1"
                            : "+t" (voltage_q) : "t" (q_limit) : "cc");
        }
#endif

#if defined(DAMIAO_DM4310)
        const float d_sample_period = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF248UL, 0x1FFFF1D4UL));
        float d_gain_step;
        __asm volatile ("vmul.f32 %1, %2, %3\n"
                        "vmls.f32 %0, %1, %4"
                        : "+t" (voltage_d), "=&t" (d_gain_step)
                        : "t" (d_sample_period), "t" (10.0f),
                          "t" (current.d));
        voltage_d = dm4310_clamp_helper(voltage_d,
            -IDENTIFICATION_VOLTAGE_LIMIT, IDENTIFICATION_VOLTAGE_LIMIT);
#else
        voltage_d = motor_clampf(
            voltage_d - IDENTIFICATION_OBSERVER_D_GAIN * 0.00005f *
                        current.d,
            -IDENTIFICATION_VOLTAGE_LIMIT,
            IDENTIFICATION_VOLTAGE_LIMIT);
#endif
#if defined(DAMIAO_DM4310)
        (void)electrical_angle;
        identification_drive(voltage_d, voltage_q, frame, false);
#else
        platform_commissioning_drive(voltage_d, voltage_q, electrical_angle);
#endif
        platform_commissioning_finish_sample();
    }
    platform_commissioning_delay_us(1000U);
#if defined(DAMIAO_DM4310)
    /* 0x25e0a neutralizes stationary PWM, then publishes flux before
     * resetting the IRQ001 accumulated delta for the mechanical stage. */
    dm4310_svpwm_helper(0.0f, 0.0f);
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA614UL, 0x1FFFA5A4UL)) = observer.flux_linkage;
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
#else
    platform_commissioning_drive(0.0f, 0.0f,
                                 identification_electrical_angle(config));
#endif
    *flux_linkage = observer.flux_linkage;
    return true;
}

static bool identify_mechanical_parameters(
    const MotorConfig *config, float flux_linkage,
    PlatformCommissioningSample *sample,
    float *rotor_inertia, float *viscous_damping
#if defined(DAMIAO_DM4310)
    , IdentificationCurrentFrame *frame
#endif
    )
{
    if ((config == NULL) || (sample == NULL) ||
        (rotor_inertia == NULL) || (viscous_damping == NULL)
#if !defined(DAMIAO_DM4310)
        || !platform_commissioning_read_sample(sample)
#endif
        ) {
        return false;
    }
    CommissioningIdentificationFilter axis_d;
    CommissioningIdentificationFilter axis_q;
#if defined(DAMIAO_DM4310)
    float mechanical_current_scale;
#endif
    if (!identification_filter_pair_init(
            config,
#if defined(DAMIAO_DM4310)
            0.0f,
#else
            sample->bus_voltage,
#endif
            &axis_d, &axis_q
#if defined(DAMIAO_DM4310)
            , &mechanical_current_scale
#endif
            )) {
        return false;
    }
#if !defined(DAMIAO_DM4310)
    platform_commissioning_finish_sample();
#endif

#if defined(DAMIAO_DM4310)
    /* 0x25e6e loads integer bits into s0; later loading a pointer into
     * r0 does not change s0. Preserve the torque scale's multiplication
     * chain and publish both motor state words before excitation. */
    const uint32_t mechanical_poles =
        *((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    const float mechanical_gear = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA618UL, 0x1FFFA5A8UL));
    float torque_scale;
    float torque_per_amp;
    const float pole_scale = (float)mechanical_poles;
    __asm volatile (
        "vmul.f32 %0, %2, %3\n"
        "vmul.f32 %0, %0, %4\n"
        "vmul.f32 %0, %0, %5\n"
        "vmul.f32 %0, %0, %6\n"
        "vmul.f32 %1, %2, %3\n"
        : "=&t" (torque_scale), "=&t" (torque_per_amp)
        : "t" (pole_scale), "t" (1.5f), "t" (flux_linkage),
          "t" (mechanical_gear), "t" (mechanical_current_scale));
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0CCUL, 0x1FFFF058UL)) = torque_scale;
    /* 0x25e98 publishes the motor torque before completing the
     * separate regression scale at 0x25e9c/0x25ea0. */
    __asm volatile ("vmul.f32 %0, %0, %1\n"
                    "vmul.f32 %0, %0, %2"
                    : "+t" (torque_per_amp)
                    : "t" (flux_linkage), "t" (mechanical_current_scale)
                    : "memory");
    __asm volatile ("" : "+t" (torque_scale), "+t" (torque_per_amp)
                    : : "memory");
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0D0UL, 0x1FFFF05CUL)) = 1.0f / torque_scale;
#endif
#if !defined(DAMIAO_DM4310)
    const float torque_per_amp = 1.5f * (float)config->pole_pairs *
                                 flux_linkage;
#endif
#if defined(DAMIAO_DM4310)
    /* 0x25eb4..0x25ef2 refreshes this frame once at mechanical entry,
     * independently of sample_ready; only subsequent samples are gated. */
    frame->angle = identification_electrical_angle_from_poles(mechanical_poles);
    dm4310_wrap_helper(&frame->angle, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
    motor_target_sincos(frame->angle, &frame->sine, &frame->cosine);
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
    *((volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) = 0U;
    const volatile float *const mechanical_cache =
        (const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    const float mechanical_target = mechanical_cache[9];
    const float mechanical_scale = mechanical_cache[2];
    float target_amplitude = mechanical_target / mechanical_scale;
    __asm volatile ("" : "+t" (target_amplitude) : : "memory");
    const float mechanical_frequency = mechanical_cache[10];
    platform_commissioning_finish_sample();
#else
    const float mechanical_frequency = IDENTIFICATION_MECHANICAL_FREQUENCY_HZ;
    const float target_amplitude =
        IDENTIFICATION_MECHANICAL_CURRENT_A / CURRENT_FULL_SCALE_A;
#endif
#if !defined(DAMIAO_DM4310)
    const float excitation_angular_frequency = mechanical_frequency * TWO_PI_F;
#endif
    uint32_t phase_count = 0U;
    float voltage_d = 0.0f;
    float voltage_q = 0.0f;
#if !defined(DAMIAO_DM4310)
    float previous_rotor_position = g_app.rotor_position;
#endif
    float filtered_current = 0.0f;
    float filtered_speed = 0.0f;
    uint32_t window_count = 0U;
#if defined(DAMIAO_DM4310)
    float response_phase = 0.0f;
    float response_magnitude = 0.0f;
#else
    bool have_result = false;
#endif
    CommissioningSineRegression regression;
    commissioning_sine_regression_init(&regression);

    for (uint32_t count = 0U;
         count < IDENTIFICATION_MECHANICAL_DRIVE_STEPS; ++count) {
#if defined(DAMIAO_DM4310)
        platform_commissioning_wait_identification_sample();
#else
        if (!platform_commissioning_read_identification_sample(sample, false)) {
            return false;
        }
#endif
        ++phase_count;
#if defined(DAMIAO_DM4310)
        const float unwrapped_phase = identification_target_phase(
            mechanical_frequency, phase_count);
#else
        const float unwrapped_phase =
            (mechanical_frequency *
             (float)phase_count) * IDENTIFICATION_PHASE_BASE_STEP;
#endif
#if defined(DAMIAO_DM4310)
        float phase = commissioning_reduce_float_angle(unwrapped_phase);
        dm4310_wrap_helper(&phase, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
#else
        const float phase = motor_wrapf(
            unwrapped_phase, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
#endif
#if defined(DAMIAO_DM4310)
        /* 0x25f74..0x25f8e evaluates and scales target sine before
         * cycle projection and the measured-current transform. */
        float target_sine;
        float target_cosine;
        motor_target_sincos(phase, &target_sine, &target_cosine);
        float target_q;
        __asm volatile ("vmul.f32 %0, %1, %2"
                        : "=t" (target_q)
                        : "t" (target_amplitude), "t" (target_sine)
                        : "memory");
        int32_t phase_bits;
        memcpy(&phase_bits, &unwrapped_phase, sizeof(phase_bits));
        /* 0x25f88..0x25f94 compares signed float bits, not FPSCR. */
        const bool cycle_complete = phase_bits > (int32_t)0x40C90FDBUL;
#else
        const bool cycle_complete = unwrapped_phase > TWO_PI_F;
#endif
        if (cycle_complete) {
#if defined(DAMIAO_DM4310)
            commissioning_sine_regression_projection(
                &regression, torque_per_amp,
                &response_phase, &response_magnitude);
#else
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
#endif
            commissioning_sine_regression_init(&regression);
            phase_count = 0U;
        }

#if defined(DAMIAO_DM4310)
        if (!platform_commissioning_read_identification_sample(sample, false)) {
            return false;
        }
        const DirectQuadrature current = commissioning_current_dq(
            sample, frame, true, NULL);
        const float electrical_angle = frame->angle;
#else
        const float electrical_angle =
            identification_electrical_angle(config);
        const DirectQuadrature current =
            commissioning_current_dq(sample, electrical_angle);
#endif
#if !defined(DAMIAO_DM4310)
        identification_current_filter_step(
            &axis_d, &axis_q, current, 0.0f,
            target_amplitude * motor_target_sin(phase), electrical_angle,
            &voltage_d, &voltage_q);
#endif

        ++window_count;
        if (window_count == IDENTIFICATION_OBSERVER_WINDOW) {
#if defined(DAMIAO_DM4310)
            /* 0x26104..0x2612e: accumulated IRQ displacement, raw
             * normalized q current, and live motor filter coefficients. */
            const float displacement =
                *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL));
            float mechanical_speed = displacement * 1000.0f;
            __asm volatile ("" : "+t" (mechanical_speed) : : "memory");
            const float old_weight =
                *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0F0UL, 0x1FFFF07CUL));
            const float new_weight =
                *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0F4UL, 0x1FFFF080UL));
            float next_speed;
            __asm volatile (
                "vmul.f32 %0, %2, %0\n"
                "vmul.f32 %1, %2, %4\n"
                "vmla.f32 %0, %5, %3\n"
                "vmla.f32 %1, %6, %3\n"
                : "+&t" (filtered_current), "=&t" (next_speed)
                : "t" (old_weight), "t" (new_weight),
                  "t" (filtered_speed), "t" (current.q),
                  "t" (mechanical_speed));
            *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1A0UL, 0x1FFFF12CUL)) = 0.0f;
            filtered_speed = next_speed;
#else
            const float rotor_position = g_app.rotor_position;
            const float mechanical_speed = wrap_signed(
                rotor_position - previous_rotor_position) / 0.001f;
            previous_rotor_position = rotor_position;
            filtered_current = IDENTIFICATION_LPF_OLD * filtered_current +
                IDENTIFICATION_LPF_NEW * current.q * CURRENT_FULL_SCALE_A;
            filtered_speed = IDENTIFICATION_LPF_OLD * filtered_speed +
                IDENTIFICATION_LPF_NEW * mechanical_speed;
#endif
            commissioning_sine_regression_step(
                &regression, filtered_current, filtered_speed);
            window_count = 0U;
        }
#if defined(DAMIAO_DM4310)
        identification_current_filter_step(
            &axis_d, &axis_q, current, 0.0f, target_q,
            electrical_angle, &voltage_d, &voltage_q, frame);
#endif
        platform_commissioning_finish_sample();
    }

    for (uint32_t count = 0U;
         count < IDENTIFICATION_MECHANICAL_COAST_STEPS; ++count) {
#if defined(DAMIAO_DM4310)
        platform_commissioning_wait_identification_sample();
#endif
        if (!platform_commissioning_read_identification_sample(sample, false)) {
            return false;
        }
#if defined(DAMIAO_DM4310)
        const DirectQuadrature current = commissioning_current_dq(
            sample, frame, true, NULL);
        const float electrical_angle = frame->angle;
#else
        const float electrical_angle =
            identification_electrical_angle(config);
        const DirectQuadrature current =
            commissioning_current_dq(sample, electrical_angle);
#endif
        identification_current_filter_step(
            &axis_d, &axis_q, current, 0.0f, 0.0f,
            electrical_angle, &voltage_d, &voltage_q
#if defined(DAMIAO_DM4310)
            , frame
#endif
            );
        platform_commissioning_finish_sample();
    }
    platform_commissioning_delay_us(1000U);
#if defined(DAMIAO_DM4310)
    dm4310_svpwm_helper(0.0f, 0.0f);
    commissioning_sine_response_motor_parameters(
        response_phase, response_magnitude,
        mechanical_frequency, true, (volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA5F8UL, 0x1FFFA588UL),
        (volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA5F4UL, 0x1FFFA584UL));
    /* 0x26384..0x26478 has no "completed cycle" rejection branch. */
    return true;
#else
    platform_commissioning_drive(0.0f, 0.0f,
                                 identification_electrical_angle(config));
    return have_result;
#endif
}

static void send_identification_result(const MotorConfig *config)
{
    uint8_t frame[21] = {'e'};
#if defined(DAMIAO_DM4310)
    (void)config;
    const volatile uint32_t *const staging =
        (const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
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
#else
    put_float(&frame[1], config->phase_resistance);
    put_float(&frame[5], config->phase_inductance);
    put_float(&frame[9], config->flux_linkage);
    put_float(&frame[13], config->viscous_damping);
    put_float(&frame[17], config->rotor_inertia);
#endif
    platform_debug_write(frame, sizeof(frame));
}

#if defined(DAMIAO_DM4310)
static float __attribute__((noinline)) alignment_drive_angle(double electrical_angle)
{
    /* 0x24bc0 and 0x24d0c use this exact binary64 constant, then convert
     * the integer turn count to float BEFORE multiplying by float two-pi.
     * 0x27754 reverses subtraction operands: angle minus turn offset. */
    const uint32_t turns = dm4310_runtime_double_to_uint(
        electrical_angle * 0x1.45f3060000000p-3);
    const float offset = (float)turns * TWO_PI_F;
    return (float)(electrical_angle - (double)offset);
}
#endif

static float alignment_measured_angle(void)
{
#if defined(DAMIAO_DM4310)
    /* IRQ001-owned position scratch, not the ADC-updated g_app cache. */
    return *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF198UL, 0x1FFFF124UL));
#else
    return g_app.rotor_angle;
#endif
}

static uint16_t alignment_raw_position(void)
{
#if defined(DAMIAO_DM4310)
    return *((const volatile uint16_t *)FACTORY_SRAM_ADDRESS(0x1FFFF190UL, 0x1FFFF11CUL));
#else
    return g_app.raw_position;
#endif
}

#if defined(DAMIAO_DM4310)
static float __attribute__((noinline)) commissioning_reduce_float_angle(float travel)
{
    /* 0x26532..0x26542 and 0x265d8..0x265e8: unsigned VFP
     * truncation (including its exceptional flags), then non-fused VMLS.
     * Retain the float 1/(2*pi) literal, not a double reciprocal. */
    const float inverse_two_pi = 0x1.45f306p-3f;
    const float two_pi = TWO_PI_F;
    float turns;
    __asm volatile ("vmul.f32 %0, %1, %2\n"
                    "vcvt.u32.f32 %0, %0\n"
                    "vcvt.f32.u32 %0, %0"
                    : "=&t" (turns)
                    : "t" (travel), "t" (inverse_two_pi));
    __asm volatile ("vmls.f32 %0, %1, %2"
                    : "+t" (travel) : "t" (turns), "t" (two_pi));
    return travel;
}

static float direction_angle_delta(float measured, float start)
{
    float delta = measured - start;
    if ((int32_t)runtime_float_bits(delta) > (int32_t)0x40490FDBU) {
        delta -= TWO_PI_F;
    }
    if (runtime_float_bits(delta) > 0xC0490FDBU) {
        delta += TWO_PI_F;
    }
    return delta;
}

static void __attribute__((noinline)) direction_publish_pole_count(uint32_t poles)
{
    /* 0x26664..0x26686: integer staging plus division-derived scales. */
    *((volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL)) = poles;
    *((volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA608UL, 0x1FFFA598UL)) = poles;
    const float runtime_poles = (float)poles;
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0E0UL, 0x1FFFF06CUL)) = runtime_poles / TWO_PI_F;
    const float gear_ratio = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA618UL, 0x1FFFA5A8UL));
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0E8UL, 0x1FFFF074UL)) = runtime_poles * gear_ratio;
}

static uint32_t __attribute__((noinline)) direction_finish_count(
    float travel, float direction)
{
    float turns = travel / TWO_PI_F;
    /* 0x26652 VDIV precedes 0x26656 direction STR, then 0x2665a
     * calls the rounding helper. Keep arithmetic across the volatile store. */
    __asm volatile ("" : "+t" (turns) : : "memory");
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0BCUL, 0x1FFFF048UL)) = direction;
    return (uint32_t)dm4310_runtime_round_to_int(turns);
}
#endif

static CommissioningStatus run_alignment_scan(uint32_t pole_pairs)
{
    /* 0x24b20 converts the numerator count to float before multiplying,
     * but wraps the denominator's shifts/add in uint32_t before converting.
     * Cancelling pole_pairs changes rounding, zero and overflow behavior. */
#if defined(DAMIAO_DM4310)
    pole_pairs = *((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    const uint32_t points = pole_pairs * ALIGNMENT_POINTS_PER_PAIR;
    const uint32_t denominator = points * ALIGNMENT_INNER_STEPS;
    const float numerator = (float)pole_pairs * TWO_PI_F;
    const double step = (double)(numerator / (float)denominator);
    /* alignment@0x24b68 authenticates again after deriving the step,
     * before loading its drive-voltage snapshot or publishing PWM. */
    platform_require_device_authentication();
    /* 0x24b6e snapshots cache word 4 (0x1ffff24c) once for the scan. */
    const uint32_t voltage_bits = runtime_parameter_cache[4];
    float alignment_voltage;
    memcpy(&alignment_voltage, &voltage_bits, sizeof(alignment_voltage));
#else
    const uint32_t points = pole_pairs * ALIGNMENT_POINTS_PER_PAIR;
    const double step = (double)(TWO_PI_F / 10240.0f);
    const float alignment_voltage = ALIGNMENT_VOLTAGE_D;
#endif
    double electrical_angle = 0.0;
#if !defined(DAMIAO_DM4310)
    platform_commissioning_drive(alignment_voltage, 0.0f, 0.0f);
#endif
    for (uint32_t count = 0U; count < ALIGNMENT_LOCK_STEPS; ++count) {
#if defined(DAMIAO_DM4310)
        /* 0x24b7c updates PWM on every lock iteration, not just once. */
        dm4310_svpwm_helper(alignment_voltage, 0.0f);
#endif
        platform_commissioning_delay_us(100U);
    }

    float unwrapped = alignment_measured_angle();
    for (uint32_t point = 0U; point < points; ++point) {
        for (uint32_t inner = 0U; inner < ALIGNMENT_INNER_STEPS; ++inner) {
            electrical_angle += step;
#if defined(DAMIAO_DM4310)
            platform_commissioning_drive(alignment_voltage, 0.0f,
                                  alignment_drive_angle(electrical_angle));
#else
            platform_commissioning_drive(alignment_voltage, 0.0f,
                                         (float)electrical_angle);
#endif
            platform_commissioning_delay_us(100U);
        }
        const float measured = alignment_measured_angle();
        unwrapped = alignment_unwrap(measured, unwrapped);
#if defined(DAMIAO_DM4310)
#if defined(__arm__) || defined(__thumb__)
        __asm volatile ("" : "+t" (unwrapped) : : "memory");
#endif
        /* Factory reloads the live motor word once per sample (0x24c76).
         * The scan bounds and step still retain the initial pole count. */
        const uint32_t sample_pole_pairs =
            *((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
#else
        const uint32_t sample_pole_pairs = pole_pairs;
#endif
        const double commanded_value = electrical_angle /
                                       (double)sample_pole_pairs;
        const float commanded = (float)(electrical_angle /
                                        (double)sample_pole_pairs);
        send_alignment_sample(commanded, measured,
                              (float)(commanded_value -
                                      (double)unwrapped),
                              alignment_raw_position(), (uint16_t)point);
    }

    for (uint32_t point = 0U; point < points; ++point) {
        for (uint32_t inner = 0U; inner < ALIGNMENT_INNER_STEPS; ++inner) {
            electrical_angle -= step;
#if defined(DAMIAO_DM4310)
            platform_commissioning_drive(alignment_voltage, 0.0f,
                                  alignment_drive_angle(electrical_angle));
#else
            platform_commissioning_drive(alignment_voltage, 0.0f,
                                         (float)electrical_angle);
#endif
            platform_commissioning_delay_us(100U);
        }
        const float measured = alignment_measured_angle();
        unwrapped = alignment_unwrap(measured, unwrapped);
#if defined(DAMIAO_DM4310)
#if defined(__arm__) || defined(__thumb__)
        __asm volatile ("" : "+t" (unwrapped) : : "memory");
#endif
        const uint32_t sample_pole_pairs =
            *((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
#else
        const uint32_t sample_pole_pairs = pole_pairs;
#endif
        const double commanded_value = electrical_angle /
                                       (double)sample_pole_pairs;
        const float commanded = (float)(electrical_angle /
                                        (double)sample_pole_pairs);
        send_alignment_sample(commanded, measured,
                              (float)(commanded_value -
                                      (double)unwrapped),
                              alignment_raw_position(),
#if defined(DAMIAO_DM4310)
                              /* 0x24e22 uses the per-sample live count,
                               * not the initial scan bound retained in r6. */
                              (uint16_t)(point +
                                  sample_pole_pairs * ALIGNMENT_POINTS_PER_PAIR));
#else
                              (uint16_t)(point + points));
#endif
    }
#if defined(DAMIAO_DM4310)
    /* 0x24e44..0x24e4c neutralizes stationary PWM directly. */
    dm4310_svpwm_helper(0.0f, 0.0f);
#else
    platform_commissioning_drive(0.0f, 0.0f, 0.0f);
#endif
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
#if defined(DAMIAO_DM4310)
    /* 0x264e8 publishes direction before taking the voltage snapshot. */
    *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0BCUL, 0x1FFFF048UL)) = 1.0f;
    const uint32_t voltage_bits = runtime_parameter_cache[4];
    float direction_voltage;
    memcpy(&direction_voltage, &voltage_bits, sizeof(direction_voltage));
#else
    const float direction_voltage = ALIGNMENT_VOLTAGE_D;
    platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f, 0.0f);
#endif
    for (uint32_t count = 0U; count < DIRECTION_LOCK_STEPS; ++count) {
#if defined(DAMIAO_DM4310)
        dm4310_svpwm_helper(direction_voltage, 0.0f);
#endif
        platform_commissioning_delay_us(50U);
    }

    const float start = alignment_measured_angle();
    float electrical_travel = 0.0f;
    float quarter_delta = 0.0f;
    for (;;) {
        electrical_travel += DIRECTION_ANGLE_STEP;
#if defined(DAMIAO_DM4310)
        platform_commissioning_drive(direction_voltage, 0.0f,
                              commissioning_reduce_float_angle(electrical_travel));
#else
        platform_commissioning_drive(direction_voltage, 0.0f,
                                     electrical_travel);
#endif
        platform_commissioning_delay_us(50U);
#if defined(DAMIAO_DM4310)
        quarter_delta = direction_angle_delta(alignment_measured_angle(), start);
        const uint32_t delta_bits = runtime_float_bits(quarter_delta);
        /* 0x265b4 uses signed CMP; 0x265be uses unsigned CMP. */
        if ((int32_t)delta_bits >= (int32_t)0x3FC90FDBU ||
            delta_bits >= 0xBFC90FDBU) {
            break;
        }
#else
        quarter_delta = wrap_signed(alignment_measured_angle() - start);
        if (fabsf(quarter_delta) >= PI_OVER_TWO_F) {
            break;
        }
#endif
    }

#if defined(DAMIAO_DM4310)
    /* 0x265c2 classifies the quarter-turn delta before returning home.
     * VCMPE also signals quiet NaNs, unlike GCC's ordinary comparison. */
    __asm volatile ("vcmpe.f32 %0, #0.0" : : "t" (quarter_delta) : "cc");
    const bool detected_inverted = quarter_delta > 0.0f;
    const float detected_direction = detected_inverted ? 1.0f : 2.0f;
#endif
    for (;;) {
        electrical_travel += DIRECTION_ANGLE_STEP;
#if defined(DAMIAO_DM4310)
        platform_commissioning_drive(direction_voltage, 0.0f,
                              commissioning_reduce_float_angle(electrical_travel));
#else
        platform_commissioning_drive(direction_voltage, 0.0f,
                                     electrical_travel);
#endif
        platform_commissioning_delay_us(50U);
#if defined(DAMIAO_DM4310)
        const float absolute_delta = fabsf(alignment_measured_angle() - start);
        if ((int32_t)runtime_float_bits(absolute_delta) <=
            (int32_t)0x3BE56042U) {
#else
        if (fabsf(alignment_measured_angle() - start) <= 0.007f) {
#endif
            break;
        }
    }

#if defined(DAMIAO_DM4310)
    /* 0x26646 neutralizes PWM before deriving/publishing the pole count. */
    dm4310_svpwm_helper(0.0f, 0.0f);
#endif
    CommissioningDirectionResult result;
#if defined(DAMIAO_DM4310)
    result.direction_code = detected_direction;
    result.pole_pairs = direction_finish_count(electrical_travel,
                                               detected_direction);
#else
    if (!commissioning_analyze_direction(quarter_delta, electrical_travel,
                                         &result)) {
        platform_commissioning_end();
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
#endif

    config->direction = result.direction_code;
#if defined(DAMIAO_DM4310)
    /* Reuse the direction decision made at 0x265c2.  Re-comparing the float
     * here leaves a different FPSCR NZCV image after the worker returns. */
    config->sensor_inverted = detected_inverted;
#else
    config->sensor_inverted = result.direction_code == 1.0f;
#endif
    config->pole_pairs = result.pole_pairs;
#if defined(DAMIAO_DM4310)
    /* detect-direction@0x26664 publishes the full integer motor word
     * before alignment reads it; +0x54 is not a floating-point field. */
    direction_publish_pole_count(result.pole_pairs);
#endif
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
#if defined(DAMIAO_DM4310)
    /* alignment@0x24b20 has already emitted the final neutral PWM update. */
    platform_commissioning_finish_alignment();
#else
    platform_commissioning_end();
#endif
    return alignment;
}

CommissioningStatus commissioning_run_output_sensor_calibration(void)
{
#if defined(DAMIAO_DM4310)
    if (!platform_commissioning_begin_unauthenticated()) {
#else
    if (!platform_commissioning_begin()) {
#endif
        return COMMISSIONING_POWER_DISABLED;
    }

    PlatformCommissioningSample sample;
#if defined(DAMIAO_DM4310)
    platform_commissioning_prime_output_filter();
#else
    if (!platform_commissioning_read_sample(&sample)) {
        platform_commissioning_end();
        return COMMISSIONING_TIMEOUT;
    }
#endif
    OutputSensorExtrema extrema = {
        .minimum_u = 4096U,
        .maximum_u = 0U,
        .minimum_v = 4096U,
        .maximum_v = 0U,
    };
#if defined(DAMIAO_DM4310)
    const float start_position =
        *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
    float extrema_target_position = start_position + TWO_PI_F;
    __asm volatile ("" : "+t" (extrema_target_position) : : "memory");
#else
    const float start_position = g_app.motor_output_position;
#endif
    uint32_t report_count = 0U;
#if defined(DAMIAO_DM4310)
    IdentificationCurrentFrame extrema_frame;
    extrema_frame.angle = 0.0f;
    /* Factory leaves its retained sine/cosine stack slots untouched until
     * the first ready sample. Read via ASM, not an undefined C float read. */
    __asm volatile ("vldr %0, [%2]\nvldr %1, [%2, #4]"
                    : "=t" (extrema_frame.sine), "=t" (extrema_frame.cosine)
                    : "r" (&extrema_frame.sine) : "memory");
#endif

    for (;;) {
#if defined(DAMIAO_DM4310)
        platform_commissioning_read_extrema_sample(&sample);
#else
        if (!platform_commissioning_read_sample(&sample)) {
            platform_commissioning_end();
            return COMMISSIONING_INVALID_MEASUREMENT;
        }
#endif
        if (sample.output_raw_u > extrema.maximum_u) {
            extrema.maximum_u = sample.output_raw_u;
#if defined(DAMIAO_DM4310)
            extrema.angle_at_maximum_u =
                *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
#else
            extrema.angle_at_maximum_u = g_app.motor_output_position;
#endif
        }
        if (sample.output_raw_u < extrema.minimum_u) {
            extrema.minimum_u = sample.output_raw_u;
#if defined(DAMIAO_DM4310)
            extrema.angle_at_minimum_u =
                *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
#else
            extrema.angle_at_minimum_u = g_app.motor_output_position;
#endif
        }
        if (sample.output_raw_v > extrema.maximum_v) {
            extrema.maximum_v = sample.output_raw_v;
#if defined(DAMIAO_DM4310)
            extrema.angle_at_maximum_v =
                *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
#else
            extrema.angle_at_maximum_v = g_app.motor_output_position;
#endif
        }
        if (sample.output_raw_v < extrema.minimum_v) {
            extrema.minimum_v = sample.output_raw_v;
#if defined(DAMIAO_DM4310)
            extrema.angle_at_minimum_v =
                *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
#else
            extrema.angle_at_minimum_v = g_app.motor_output_position;
#endif
        }
#if defined(DAMIAO_DM4310)
        if (*((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) != 0U) {
            *((volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF194UL, 0x1FFFF120UL)) = 0U;
            /* Unlike other commissioning stages, extrema loads all three
             * inputs before converting poles (0x2483c..0x24848). */
            __asm volatile (
                "vldr s1, [%1, #84]\n"
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
                : "=t" (extrema_frame.angle)
                : "r" ((uintptr_t)FACTORY_SRAM_ADDRESS(0x1FFFF088UL, 0x1FFFF014UL)),
                  "r" ((uintptr_t)FACTORY_SRAM_ADDRESS(0x1FFFF190UL, 0x1FFFF11CUL)), "t" (TWO_PI_F)
                : "s0", "s1", "s2", "memory");
            dm4310_wrap_helper(&extrema_frame.angle,
                              -0x1.921fb6p+1f, 0x1.921fb6p+1f);
            motor_target_sincos(extrema_frame.angle,
                                &extrema_frame.sine, &extrema_frame.cosine);
        }
#else
        refresh_commissioning_position();
#endif
        if (++report_count == OUTPUT_CALIBRATION_REPORT_DIVIDER) {
            report_count = 0U;
            send_output_sensor_raw_sample(sample.output_raw_u,
                                          sample.output_raw_v);
        }

#if defined(DAMIAO_DM4310)
        /* Retain the factory distance before drive/PWM and ADC ack. */
        const float output_position =
            *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0A0UL, 0x1FFFF02CUL));
        float target_distance = output_position - extrema_target_position;
        __asm volatile ("" : "+t" (target_distance) : : "memory");
        /* extrema@0x248cc/0x248d6 publishes D/Q controller output words. */
        *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA528UL, 0x1FFFA610UL)) = 0.0f;
        __asm volatile ("vabs.f32 %0, %0"
                        : "+t" (target_distance) : : "memory");
        *((volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA550UL, 0x1FFFA638UL)) = OUTPUT_CALIBRATION_VOLTAGE_Q;
        __asm volatile ("" : "+t" (target_distance) : : "memory");
#endif
#if defined(DAMIAO_DM4310)
        float alpha;
        float beta;
        __asm volatile ("vmul.f32 %0, %2, %4\n"
                        "vmul.f32 %1, %2, %5\n"
                        "vmls.f32 %0, %5, %3\n"
                        "vmla.f32 %1, %4, %3"
                        : "=&t" (alpha), "=&t" (beta)
                        : "t" (0.0f), "t" (OUTPUT_CALIBRATION_VOLTAGE_Q),
                          "t" (extrema_frame.cosine), "t" (extrema_frame.sine));
        dm4310_svpwm_helper(alpha, beta);
#else
        const float electrical_angle =
            g_app.rotor_angle * (float)g_app.config.pole_pairs +
            g_app.motor.electrical_offset;
        platform_commissioning_drive(0.0f,
                                     OUTPUT_CALIBRATION_VOLTAGE_Q,
                                     electrical_angle);
#endif
        platform_commissioning_finish_sample();
#if defined(DAMIAO_DM4310)
        /* extrema@0x248c2..0x24912 compares signed bits of distance to
         * the retained one-turn target, not travel distance from start. */
        if ((int32_t)runtime_float_bits(target_distance) <=
            (int32_t)UINT32_C(0x3a83126f)) {
#else
        if (fabsf(g_app.motor_output_position - start_position) >=
            (TWO_PI_F - 0.0010000000474974513f)) {
#endif
            break;
        }
    }

#if defined(DAMIAO_DM4310)
    platform_commissioning_publish_output_extrema(&extrema);
#else
    float calibration[4];
    if (!sensor_calibration_analyze_output_extrema(&extrema, calibration) ||
        !platform_commissioning_apply_output_calibration(
            calibration, sample.output_raw_u, sample.output_raw_v)) {
        platform_commissioning_end();
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
#endif

#if defined(DAMIAO_DM4310)
    const uint16_t report_poles =
        *((const volatile uint16_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    const float report_gear = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0D4UL, 0x1FFFF060UL));
    uint32_t report_gear_integer;
    float converted_gear;
    __asm volatile ("vcvt.u32.f32 %0, %2\nvmov %1, %0"
                    : "=&t" (converted_gear), "=r" (report_gear_integer)
                    : "t" (report_gear) : "memory");
    send_sensor_sample('J', 0.0f, 0.0f, 0.0f,
                       report_poles, (uint16_t)report_gear_integer);
    /* Factory 0x24a14 uses the millisecond timer, not 1000 us. */
    platform_delay_ms(1U);
#else
    send_sensor_sample('J', 0.0f, 0.0f, 0.0f,
                       (uint16_t)g_app.config.pole_pairs,
                       (uint16_t)g_app.config.gear_ratio);
    platform_commissioning_delay_us(1000U);
#endif

#if defined(DAMIAO_DM4310)
    /* offset@0x24456 retains gear ratio before taking the live pole count. */
    const float offset_gear_ratio =
        *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0D4UL, 0x1FFFF060UL));
    const float measurement_points = offset_gear_ratio *
        (float)OUTPUT_TABLE_POINTS_PER_MOTOR_TURN;
    uint32_t measurement_count = (uint32_t)measurement_points;
    __asm volatile ("" : "+r" (measurement_count) : : "memory");
    const uint32_t offset_pole_pairs =
        *((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
    float offset_step = ((float)offset_pole_pairs * TWO_PI_F *
                         offset_gear_ratio) / (float)measurement_count;
    /* Retain the factory's pre-lock division even when parameters change
     * during the 20,000 delay/PWM iterations. */
    __asm volatile ("" : "+t" (offset_step) : : "memory");
    const double electrical_step = (double)offset_step;
#else
    const float measurement_points = g_app.config.gear_ratio *
        (float)OUTPUT_TABLE_POINTS_PER_MOTOR_TURN;
    const uint32_t measurement_count = (uint32_t)measurement_points;
#endif
#if !defined(DAMIAO_DM4310)
    memset(g_output_sensor_result_table, 0, sizeof(g_output_sensor_result_table));
    memset(g_output_sensor_result_counts, 0, sizeof(g_output_sensor_result_counts));
    report_count = 0U;
#endif
    /* measure_position_sensor_offset@0x24448 locks the d-axis at phase zero
     * and advances its double-precision phase accumulator from zero.  It
     * does not seed this pass from the live encoder angle. */
    double electrical_angle = 0.0;
#if defined(DAMIAO_DM4310)
    /* offset@0x24490 snapshots the live drive cache; 0x2449e..0x244b4
     * publishes stationary PWM on every lock iteration. */
    const uint32_t offset_voltage_bits = runtime_parameter_cache[4];
    float offset_voltage;
    memcpy(&offset_voltage, &offset_voltage_bits, sizeof(offset_voltage));
#else
    platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f,
                                 (float)electrical_angle);
#endif
    for (uint32_t count = 0U; count < ALIGNMENT_LOCK_STEPS; ++count) {
#if defined(DAMIAO_DM4310)
        dm4310_svpwm_helper(offset_voltage, 0.0f);
#endif
        platform_commissioning_delay_us(100U);
    }
#if !defined(DAMIAO_DM4310)
    const double electrical_step = (double)(
        ((float)g_app.config.pole_pairs * TWO_PI_F *
         g_app.config.gear_ratio) / (float)measurement_count);
#endif
    for (uint32_t count = 0U; count < measurement_count; ++count) {
#if defined(DAMIAO_DM4310)
        float offset_atan_y;
        float offset_atan_x;
        platform_commissioning_read_offset_sample(
            &sample, &offset_atan_y, &offset_atan_x);
#else
        if (!platform_commissioning_read_sample(&sample)) {
            platform_commissioning_end();
            return COMMISSIONING_INVALID_MEASUREMENT;
        }
#endif
#if !defined(DAMIAO_DM4310)
        refresh_commissioning_position();
#endif
        electrical_angle += electrical_step;
#if defined(DAMIAO_DM4310)
        platform_commissioning_drive(offset_voltage, 0.0f,
                                     alignment_drive_angle(electrical_angle));
#else
        platform_commissioning_drive(ALIGNMENT_VOLTAGE_D, 0.0f,
                                     (float)electrical_angle);
#endif
#if defined(DAMIAO_DM4310)
        platform_commissioning_finish_output_sample();
        /* offset@0x245f8 multiplies live float poles/gear before widening
         * the denominator, then sends one CRC-bearing H frame per point. */
        const uint32_t live_poles =
            *((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF0DCUL, 0x1FFFF068UL));
        const float live_gear = *((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF0D4UL, 0x1FFFF060UL));
        const float commanded_output = (float)(electrical_angle /
            (double)((float)live_poles * live_gear));
        const float measured = dm4310_atan2_helper(offset_atan_y, offset_atan_x);
        send_sensor_sample('H', commanded_output, measured,
                           offset_report_error(commanded_output - measured),
                           sample.output_raw_u, sample.output_raw_v);
#else
        platform_commissioning_finish_sample();
        const float commanded_output = (float)(
            electrical_angle /
            ((double)g_app.config.pole_pairs *
             (double)g_app.config.gear_ratio));
        const float measured = sample.output_uncorrected_angle;
        const uint32_t table_index =
            (count * OUTPUT_TABLE_POINTS) / measurement_count;
        if (table_index < OUTPUT_TABLE_POINTS) {
            g_output_sensor_result_table[table_index] +=
                wrap_signed(commanded_output - measured);
            ++g_output_sensor_result_counts[table_index];
        }
        if (++report_count == OUTPUT_CALIBRATION_REPORT_DIVIDER) {
            report_count = 0U;
            send_output_sensor_raw_sample(sample.output_raw_u,
                                          sample.output_raw_v);
        }
#endif
    }

#if defined(DAMIAO_DM4310)
    /* This is the inlined 0x24448 child's single terminal neutral update. */
    dm4310_svpwm_helper(0.0f, 0.0f);
#else
    for (uint32_t index = 0U; index < OUTPUT_TABLE_POINTS; ++index) {
        if (g_output_sensor_result_counts[index] != 0U) {
            g_output_sensor_result_table[index] /=
                (float)g_output_sensor_result_counts[index];
        }
    }
    send_output_sensor_result_table(g_output_sensor_result_table);
    platform_commissioning_delay_us(1000U);
    platform_commissioning_drive(0.0f, 0.0f, 0.0f);
#endif
#if defined(DAMIAO_DM4310)
    /* output-calibration@0x246f8 prints before its ADC/INT002 epilogue and
     * does not issue a second neutral PWM update. */
    send_sensor_sample('Z', 0.0f, 0.0f, 0.0f, UINT16_MAX, UINT16_MAX);
    platform_delay_ms(1U);
    /* Factory widens gain, staged phase, live V and live U in this order. */
    const double report_gain =
        (double)*((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1D0UL, 0x1FFFF15CUL));
    __asm volatile ("" : : "r" (&report_gain) : "memory");
    const double report_phase =
        (double)*((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF084UL, 0x1FFFF010UL));
    __asm volatile ("" : : "r" (&report_phase) : "memory");
    const double report_center_v =
        (double)*((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1C4UL, 0x1FFFF150UL));
    __asm volatile ("" : : "r" (&report_center_v) : "memory");
    const double report_center_u =
        (double)*((const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFF1BCUL, 0x1FFFF148UL));
    debug_console_printf(
        "u=%.4f v=%.4f  w=%.4f c=%.4f\r\n",
        report_center_u, report_center_v, report_phase, report_gain);
    platform_commissioning_restore_control_irq();
#else
    platform_commissioning_end();
    debug_console_printf(
        "u=%.4f v=%.4f  w=%.4f c=%.4f\r\n",
        (double)calibration[0],
        (double)calibration[1],
        (double)calibration[2],
        (double)calibration[3]);
#endif
    return COMMISSIONING_OK;
}

CommissioningStatus commissioning_run_motor_identification(MotorConfig *config)
{
    if ((config == NULL)
#if !defined(DAMIAO_DM4310)
        || (config->pole_pairs == 0U)
#endif
        ||
        !platform_commissioning_begin()) {
        return COMMISSIONING_POWER_DISABLED;
    }

#if defined(DAMIAO_DM4310)
    /* 0x25844..0x2585a applies stationary alpha=0.1, beta=0 on every
     * lock tick. It does not derive a rotor-relative electrical angle. */
    const float locked_electrical_angle = 0.0f;
#else
    refresh_commissioning_position();
    const float locked_electrical_angle =
        g_app.rotor_angle * (float)config->pole_pairs +
        g_app.motor.electrical_offset;
    platform_commissioning_drive(0.1f, 0.0f,
                                 locked_electrical_angle);
#endif
    for (uint32_t count = 0U; count < IDENTIFICATION_LOCK_STEPS; ++count) {
#if defined(DAMIAO_DM4310)
        dm4310_svpwm_helper(0.1f, 0.0f);
#endif
        platform_commissioning_delay_us(100U);
    }
#if !defined(DAMIAO_DM4310)
    platform_commissioning_finish_sample();
#endif

    PlatformCommissioningSample sample;
    float resistance;
    float inductance;
#if defined(DAMIAO_DM4310)
    float retained_voltage_d;
    /* Factory stack +0/+4/+8 starts at angle=0, sin=0, cos=1 and
     * survives electrical, flux, excitation, and coast stages. */
    IdentificationCurrentFrame current_frame = {0.0f, 0.0f, 1.0f};
#endif
    if (!identify_electrical_parameters(locked_electrical_angle,
                                        &resistance, &inductance, &sample
#if defined(DAMIAO_DM4310)
                                        , &current_frame,
                                        &retained_voltage_d
#endif
                                        )
#if !defined(DAMIAO_DM4310)
        || (resistance < 0.0f)
#endif
        ) {
#if defined(DAMIAO_DM4310)
        /* Factory literal at 0x25c24 is exactly "error!/r/n". */
        debug_console_printf("error!/r/n");
#else
        platform_commissioning_end();
#endif
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
#if defined(DAMIAO_DM4310)
    /* 0x25b1e..0x25b20 waits a second 5 ms after both fitted values pass
     * their sign checks and before the flux-observer state is initialized. */
    platform_commissioning_delay_us(5000U);
#endif
#if !defined(DAMIAO_DM4310)
    if (inductance < 0.0f) {
#if defined(DAMIAO_DM4310)
        debug_console_printf("error!/r/n");
#else
        platform_commissioning_end();
#endif
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
#endif
#if !defined(DAMIAO_DM4310)
    platform_commissioning_delay_us(5000U);
#endif

#if defined(DAMIAO_DM4310)
    /* 0x25ad4 onward keeps using staging@0x1fffa5c8 in place.  Do not add
     * whole-structure SRAM reads/writes through a temporary stack copy. */
    MotorConfig *const identified = config;
#else
    MotorConfig identified_storage = *config;
    MotorConfig *const identified = &identified_storage;
    identified->phase_resistance = resistance;
    identified->phase_inductance = inductance;
#endif
    float flux_linkage;
    if (!identify_flux_linkage(identified, &sample, &flux_linkage
#if defined(DAMIAO_DM4310)
                              , &current_frame, retained_voltage_d
#endif
                              )) {
#if defined(DAMIAO_DM4310)
        debug_console_printf("error!/r/n");
#else
        platform_commissioning_end();
#endif
        return COMMISSIONING_INVALID_MEASUREMENT;
    }
#if !defined(DAMIAO_DM4310)
    identified->flux_linkage = flux_linkage;
#endif
    float inertia;
    float damping;
    if (!identify_mechanical_parameters(
            identified, flux_linkage, &sample, &inertia, &damping
#if defined(DAMIAO_DM4310)
            , &current_frame
#endif
            )) {
#if defined(DAMIAO_DM4310)
        debug_console_printf("error!/r/n");
        return COMMISSIONING_INVALID_MEASUREMENT;
#else
#error "The recovered DM43xx V3 path requires DAMIAO_DM4310 compatibility"
#endif
    }

#if !defined(DAMIAO_DM4310)
    identified->rotor_inertia = inertia;
    identified->viscous_damping = damping;
#endif
#if defined(DAMIAO_DM4310)
    /* 0x263be/0x263ce were already published during final arithmetic. */
    /* 0x263d2 resets the motion-observer word before constructing 'e'. */
    *((volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF1FCUL, 0x1FFFF188UL)) = 1U;
#else
    *config = *identified;
#endif
#if defined(DAMIAO_DM4310)
    /* 0x263d6..0x263fc emits the result before the cache refresh and
     * second authentication in derive_control_parameters@0x25138. */
    send_identification_result(config);
    /* The factory motor-ID path authenticates both at entry and again through
     * derive_control_parameters@0x25138 before installing the result. */
    const volatile uint32_t *const result_words =
        (const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    __asm volatile ("vldr s0, [%0, #64]\n\t"
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
                    : : "r" (result_words), "r" (runtime_parameter_cache)
                    : "s0", "memory");
    dm4310_derive_control_parameters_helper();
    /* 0x2643a..0x2645a installs outer-loop gains after derivation, with
     * speed Ki preceding Kp. These fixed loop states are independent of
     * the source-owned MotorConfig and must not retain their old gains. */
    /* r4 resolves through 0x26494 to staging 0x1fffa5c8. Derivation
     * can update these words; re-read each after that call, not config. */
    const volatile float *const staging =
        (const volatile float *)FACTORY_SRAM_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    __asm volatile ("vldr s0, [%0, #104]\n\t"
                    "vstr s0, [%1, #4]\n\t"
                    "vldr s0, [%0, #100]\n\t"
                    "vstr s0, [%1]\n\t"
                    "vldr s0, [%0, #108]\n\t"
                    "vstr s0, [%2]\n\t"
                    "vldr s0, [%0, #112]\n\t"
                    "vstr s0, [%2, #4]"
                    : : "r" (staging), "r" (&runtime_speed_loop),
                      "r" (&runtime_position_loop) : "s0", "memory");
#else
    motor_control_configure(&g_app.motor, config, sample.bus_voltage);
    commissioning_configure_runtime_drive_states(config, sample.bus_voltage);
    commissioning_configure_runtime_loop_states(config);
    send_identification_result(config);
#endif
#if defined(DAMIAO_DM4310)
    platform_commissioning_restore_control_irq();
#else
    platform_commissioning_end();
#endif
    return COMMISSIONING_OK;
}
