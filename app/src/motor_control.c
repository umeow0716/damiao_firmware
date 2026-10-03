#include "motor_control.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "app_profile.h"
#include "memory_layout.h"
#include "safety.h"
#include "board_adc.h"
#include "position_sensor.h"
#include "temperature_table.h"

#define CURRENT_ADC_SCALE (0x1p-11f)
#define CONTROL_SAMPLE_PERIOD (0.00005f)
#define OUTER_SAMPLE_PERIOD (0.001f)
#define OUTER_SAMPLE_FREQUENCY (1000.0f)
#define VOLTAGE_NORMALIZATION APP_PROFILE_CURRENT_FULL_SCALE_A
#define INV_SQRT3_F (0.5773502588272095f)
#define INV_TWO_PI_F (0.15915493667125702f)
#define TWO_PI_F (6.2831854820251465f)
#define MODULATION_VECTOR_LIMIT (0.9800000190734863f)
#define MOTION_OBSERVER_BANDWIDTH (1000.0f)
#define MOTION_OBSERVER_STATE_GAIN (600.0f)
#define MOTOR_TEMPERATURE_FILTER_OLD (0.9995002746582031f)
#define MOTOR_TEMPERATURE_FILTER_NEW (0.000499725341796875f)

/* These adjacent firmware objects are shared by startup, commissioning,
 * motor identification and the 20 kHz control IRQ.  Keep the proven object
 * boundaries fixed while individual words are promoted to semantic fields
 * as their complete read/write lifetimes are established. */
typedef struct
{
    float rotor_position;
    float rotor_velocity;
    int32_t revolutions;
    float electrical_angle;
    uint32_t reserved_10;
    float electrical_offset;
    float output_position;
    float output_velocity;
    float raw_rotor_velocity;
    float motor_output_position_offset;
    uint32_t reserved_28[2];
    float output_torque;
    float direction;
    uint32_t console_mode;
    uint32_t motor_state_changed;
    float filtered_motor_temperature;
    float output_torque_constant;
    float inverse_output_torque_constant;
    float gear_ratio;
    uint32_t reserved_50;
    uint32_t pole_pairs;
    float pole_pairs_per_radian;
    float position_sensor_scale;
    float motor_to_output_scale;
    float inverse_pole_pairs;
    float velocity_filter_previous;
    float velocity_filter_new;
    float filtered_current_q;
    float motor_temperature_filter_old;
    float motor_temperature_filter_new;
} MotorRuntimeState;

typedef struct
{
    float command_position;
    float command_velocity;
    float command_torque;
    float command_kp;
    float command_kd;
    uint32_t reserved_14[2];
    float desired_velocity;
    float velocity_target;
    float position_error;
    uint32_t reserved_28;
    float current_q_reference;
    float current_limit;
    uint32_t reserved_34;
    uint32_t outer_loop_divider;
    uint32_t control_mode;
    float current_offset_u;
    float current_offset_v;
    float current_offset_w;
    float current_u;
    float current_v;
    float current_w;
    float current_alpha;
    float current_beta;
    float current_d;
    float current_q;
    float bus_voltage;
    float normalized_bus_voltage;
    float voltage_alpha;
    float voltage_beta;
    float electrical_sine;
    float electrical_cosine;
    uint32_t fault;
    float mos_temperature;
    uint32_t reserved_88;
    uint16_t raw_position;
    uint16_t reserved_8e;
    uint32_t position_sample_ready;
    float wrapped_position;
    float previous_wrapped_position;
    float accumulated_position_delta;
    float position_delta;
} SampleRuntimeState;

static volatile MotorRuntimeState motor_runtime_state
    __attribute__((section(".motor_runtime_state"), used));
static volatile SampleRuntimeState sample_runtime_state
    __attribute__((section(".sample_runtime_state"), used));
static CurrentController d_axis_controller
    __attribute__((section(MEMORY_LAYOUT_SECTION(".current_d", ".alternate_current_d")), used));
static CurrentController q_axis_controller
    __attribute__((section(MEMORY_LAYOUT_SECTION(".current_q", ".alternate_current_q")), used));
static MotionObserver motion_observer __attribute__((section(".motion_observer"), used));

SRAM_ABI_ASSERT_SIZE(MotorRuntimeState, 0x7CU);
SRAM_ABI_ASSERT_OFFSET(MotorRuntimeState, electrical_offset, 0x14U);
SRAM_ABI_ASSERT_OFFSET(MotorRuntimeState, motor_output_position_offset, 0x24U);
SRAM_ABI_ASSERT_OFFSET(MotorRuntimeState, direction, 0x34U);
SRAM_ABI_ASSERT_OFFSET(MotorRuntimeState, output_torque_constant, 0x44U);
SRAM_ABI_ASSERT_OFFSET(MotorRuntimeState, velocity_filter_previous, 0x68U);
SRAM_ABI_ASSERT_OFFSET(MotorRuntimeState, filtered_current_q, 0x70U);
SRAM_ABI_ASSERT_OFFSET(MotorRuntimeState, console_mode, 0x38U);
SRAM_ABI_ASSERT_SIZE(SampleRuntimeState, 0xA4U);
SRAM_ABI_ASSERT_OFFSET(SampleRuntimeState, bus_voltage, 0x68U);
SRAM_ABI_ASSERT_OFFSET(SampleRuntimeState, outer_loop_divider, 0x38U);
SRAM_ABI_ASSERT_OFFSET(SampleRuntimeState, command_kd, 0x10U);
SRAM_ABI_ASSERT_OFFSET(SampleRuntimeState, current_u, 0x4CU);
SRAM_ABI_ASSERT_OFFSET(SampleRuntimeState, electrical_sine, 0x78U);
SRAM_ABI_ASSERT_OFFSET(SampleRuntimeState, voltage_alpha, 0x70U);
SRAM_ABI_ASSERT_OFFSET(SampleRuntimeState, fault, 0x80U);
SRAM_ABI_ASSERT_OFFSET(SampleRuntimeState, current_q_reference, 0x2CU);
SRAM_ABI_ASSERT_OFFSET(SampleRuntimeState, raw_position, 0x8CU);
SRAM_ABI_ASSERT_SIZE(CurrentController, 0x4CU);
SRAM_ABI_ASSERT_SIZE(MotionObserver, 0x3CU);

/* Owned by commissioning.c because the two 10-word loop states live beside
 * its other fixed runtime drive objects. */
void commissioning_reset_runtime_loop_states(volatile float *speed, float value);
void commissioning_configure_runtime_loop_states(const MotorConfig *config);

static CurrentController *current_d_state(MotorController *controller)
{
    (void)controller;
    return &d_axis_controller;
}

static MotionObserver *motion_observer_state(MotorController *controller)
{
    (void)controller;
    return &motion_observer;
}

static float configure_current_axis(CurrentController *axis, const MotorConfig *config,
                                    float plant_gain)
{
    (void)config;
    const volatile float *const cache =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    volatile CurrentController *const state = axis;
    const float period = cache[3];
    state->sample_period = period;
    const float current_scale = cache[2];
    const float inductance = cache[18];
    /* The firmware passes the retained sample voltage here. Do not reread the
     * IRQ-owned sample. */
    const float voltage = plant_gain;
    float denominator;
    float numerator;
    __asm volatile("vmul.f32 %0, %2, %3\n"
                   "vmul.f32 %1, %4, %5\n"
                   "vdiv.f32 %0, %1, %0"
                   : "=&t"(denominator), "=&t"(numerator)
                   : "t"(current_scale), "t"(inductance), "t"(voltage), "t"(INV_SQRT3_F)
                   : "memory");
    plant_gain = denominator;
    state->plant_gain = plant_gain;
    float inverse_gain;
    float plant_gain_dt;
    /* Firmware publishes gain before evaluating its inverse and dt product. */
    __asm volatile("vdiv.f32 %0, %2, %3\n"
                   "vmul.f32 %1, %3, %4"
                   : "=&t"(inverse_gain), "=&t"(plant_gain_dt)
                   : "t"(1.0f), "t"(plant_gain), "t"(period)
                   : "memory");
    state->inverse_plant_gain = inverse_gain;
    state->plant_gain_dt = plant_gain_dt;
    const float bandwidth = cache[14];
    state->control_bandwidth = bandwidth;
    const float enhancement = cache[15];
    state->observer_bandwidth = enhancement;
    float doubled_enhancement;
    float observer_l1_dt;
    float observer_l2_dt;
    /* Operand order selects NaN payloads when DN is clear.  Keep the square
     * between doubling and the two period products too. */
    __asm volatile("vmul.f32 %0, %3, %4\n"
                   "vmul.f32 %2, %3, %3\n"
                   "vmul.f32 %1, %0, %5\n"
                   "vmul.f32 %2, %2, %5"
                   : "=&t"(doubled_enhancement), "=&t"(observer_l1_dt), "=&t"(observer_l2_dt)
                   : "t"(enhancement), "t"(2.0f), "t"(period)
                   : "memory");
    state->observer_l1_dt = observer_l1_dt;
    state->observer_l2_dt = observer_l2_dt;
    state->output_min = -1.0f;
    state->output_max = 1.0f;
    /* Firmware configures Q from the retained D coefficients, without
     * re-reading cache or the D state. */
    volatile CurrentController *const q = &q_axis_controller;
    q->sample_period = period;
    q->plant_gain = plant_gain;
    q->inverse_plant_gain = inverse_gain;
    q->plant_gain_dt = plant_gain_dt;
    q->control_bandwidth = bandwidth;
    q->observer_bandwidth = enhancement;
    q->observer_l1_dt = observer_l1_dt;
    q->observer_l2_dt = observer_l2_dt;
    q->output_min = -1.0f;
    q->output_max = 1.0f;
    return current_scale;
}

static void configure_motion_observer(MotionObserver *observer, const MotorConfig *config,
                                      float current_scale)
{
    (void)config;
    const volatile float *const cache =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL);
    volatile MotionObserver *const state = observer;
    const float pole_pairs = cache[16];
    const float inertia = cache[20];
    /* Evaluate the pole product before loading flux.  The memory clobber
     * prevents the shared-cache read from moving earlier. */
    float scaled_poles;
    __asm volatile("vmul.f32 %0, %1, %2"
                   : "=t"(scaled_poles)
                   : "t"(pole_pairs), "t"(1.5f)
                   : "memory");
    const float flux = cache[19];
    float mechanical_gain;
    float torque;
    __asm volatile("vmul.f32 %0, %2, %3\n"
                   "vmul.f32 %1, %0, %4\n"
                   "vdiv.f32 %0, %1, %5"
                   : "=&t"(mechanical_gain), "=&t"(torque)
                   : "t"(scaled_poles), "t"(flux), "t"(current_scale), "t"(inertia)
                   : "memory");
    state->plant_gain = mechanical_gain;
    float inverse_gain;
    /* Publish gain before evaluating its inverse. */
    __asm volatile("vdiv.f32 %0, %1, %2"
                   : "=t"(inverse_gain)
                   : "t"(1.0f), "t"(mechanical_gain)
                   : "memory");
    state->inverse_plant_gain = inverse_gain;
    state->sample_period = OUTER_SAMPLE_PERIOD;
    state->error = 0.0f;
    state->estimated_velocity = 0.0f;
    state->observer_state = 0.0f;
    state->observer_bandwidth = cache[22];
    state->observer_state_gain = cache[23];
    state->output_min = -1.0f;
    state->output_max = 1.0f;
}

void motor_control_configure_velocity_filter_irq(float bandwidth, bool filtered,
                                                 volatile float *coefficients, float radians)
{
    /* DM control consumes only the fixed runtime coefficients. Keep the
     * generic field/layout for other models, but do not mirror stores. */
    if (filtered)
    {
        float denominator = 1000.0f;
        const float numerator = 1000.0f;
        float previous;
        __asm__ volatile("vmla.f32 %0, %2, %3\n\t"
                         "vdiv.f32 %1, %4, %0"
                         : "+&t"(denominator), "=&t"(previous)
                         : "t"(bandwidth), "t"(radians), "t"(numerator)
                         : "memory");
        coefficients[0] = previous;
        const float one = 1.0f;
        float next;
        __asm__ volatile("vsub.f32 %0, %1, %2" : "=t"(next) : "t"(one), "t"(previous) : "memory");
        coefficients[1] = next;
    }
    else
    {
        /* The firmware write path saturates all positive encodings at or
         * above 500 Hz, including +Inf and positive-payload NaNs. */
        const float zero = 0.0f;
        const float one = 1.0f;
        __asm__ volatile("vstr %1, [%0]\n\t"
                         "vstr %2, [%0, #4]"
                         :
                         : "r"(coefficients), "t"(zero), "t"(one)
                         : "memory");
    }
}

void motor_control_configure_velocity_filter_selected(MotorController *controller, float bandwidth,
                                                      bool filtered)
{
    (void)controller;
    motor_control_configure_velocity_filter_irq(
        bandwidth, filtered, &motor_runtime_state.velocity_filter_previous, TWO_PI_F);
}

void motor_control_configure_velocity_filter(MotorController *controller, float bandwidth)
{
    uint32_t raw;
    memcpy(&raw, &bandwidth, sizeof(raw));
    motor_control_configure_velocity_filter_selected(controller, bandwidth,
                                                     (int32_t)raw < (int32_t)UINT32_C(0x43FA0000));
}

static float control_vmla_f32(float accumulator, float left, float right)
{
#if defined(__arm__) || defined(__thumb__)
    __asm volatile("vmla.f32 %0, %1, %2" : "+t"(accumulator) : "t"(left), "t"(right));
    return accumulator;
#else
    return accumulator + left * right;
#endif
}

void motion_observer_state_step(MotionObserver *observer)
{
    const float error = observer->measured_velocity - observer->estimated_velocity;
    observer->error = error;
    const float derivative = (error - observer->previous_error) * OUTER_SAMPLE_FREQUENCY;
    observer->error_derivative = derivative;
    observer->previous_error = error;
    float velocity_derivative =
        control_vmla_f32(observer->observer_state, observer->observer_bandwidth, error);
    velocity_derivative =
        control_vmla_f32(velocity_derivative, observer->plant_gain, observer->current_q);
    observer->estimated_velocity = control_vmla_f32(observer->estimated_velocity,
                                                    observer->sample_period, velocity_derivative);
    float correction = derivative;
    correction = control_vmla_f32(correction, observer->observer_bandwidth, error);
    observer->observer_state =
        control_vmla_f32(observer->observer_state,
                         observer->sample_period * observer->observer_state_gain, correction);
    /* motion_observer uses ordinary ordered comparisons, so an
     * unordered result survives both upper- and lower-limit tests. */
    observer->disturbance_current =
        motor_clampf(observer->observer_state * observer->inverse_plant_gain, observer->output_min,
                     observer->output_max);
}

void motor_control_motion_observer_step(MotionObserver *observer, float measured_velocity,
                                        float current_q)
{
    observer->measured_velocity = measured_velocity;
    observer->current_q = current_q;
    motion_observer_helper(observer);
}

void motor_control_init(MotorController *controller)
{
    memset(controller, 0, sizeof(*controller));
    controller->current_scale = CURRENT_ADC_SCALE;
    controller->sample_period = CONTROL_SAMPLE_PERIOD;
    controller->command.mode = MOTOR_MODE_DISABLED;
}

void motor_control_configure(MotorController *controller, const MotorConfig *config,
                             float bus_voltage)
{
    /* derive_control_parameters computes these coefficients with raw
     * IEEE-754 division. Low bus voltage is handled by the fault state, not
     * by replacing the current-controller plant gain with zero. */
    const float current_scale =
        configure_current_axis(current_d_state(controller), config, bus_voltage);
    configure_motion_observer(motion_observer_state(controller), config, current_scale);
}

void motor_control_configure_runtime_motor(MotorController *controller, const MotorConfig *config)
{
    const volatile uint32_t *const staging_words =
        (const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    const volatile float *const staging =
        (const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C8UL, 0x1FFFA558UL);
    const uint32_t pole_count = staging_words[16];
    const float pole_pairs = (float)pole_count;
    const float flux = staging[19];
    /* Firmware retains cache word 2 in s20 across the derivation helper; it
     * does not reread fixed cache after returning. */
    const float current_scale = VOLTAGE_NORMALIZATION;
    float base_torque;
    __asm volatile("vmul.f32 %0, %1, %2\n"
                   "vmul.f32 %0, %0, %3\n"
                   "vmul.f32 %0, %0, %4"
                   : "=&t"(base_torque)
                   : "t"(pole_pairs), "t"(1.5f), "t"(flux), "t"(current_scale)
                   : "memory");
    commissioning_configure_runtime_loop_states(config);
    const float override = staging[1];
    uint32_t override_present;
    __asm volatile("vcmpe.f32 %1, #0.0\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "mov %0, #0\n"
                   "it ne\n"
                   "movne %0, #1"
                   : "=r"(override_present)
                   : "t"(override)
                   : "cc");
    float torque_constant;
    if (override_present != 0U)
    {
        __asm volatile("vmul.f32 %0, %1, %2"
                       : "=t"(torque_constant)
                       : "t"(override), "t"(current_scale));
    }
    else
    {
        const float ratio = staging[20];
        const float correction = staging[30];
        __asm volatile("vmul.f32 %0, %1, %2\n"
                       "vmul.f32 %0, %0, %3"
                       : "=&t"(torque_constant)
                       : "t"(ratio), "t"(base_torque), "t"(correction));
    }
    motor_runtime_state.output_torque_constant = torque_constant;
    const float inverse_torque = 1.0f / torque_constant;
    motor_runtime_state.inverse_output_torque_constant = inverse_torque;
    controller->torque_to_current = inverse_torque;
    motor_runtime_state.pole_pairs = pole_count;
    motor_runtime_state.pole_pairs_per_radian = pole_pairs / TWO_PI_F;
    motor_runtime_state.inverse_pole_pairs = 1.0f / pole_pairs;
    const float gear_ratio = staging[20];
    motor_runtime_state.position_sensor_scale = 1.0f / gear_ratio;
    motor_runtime_state.motor_to_output_scale = pole_pairs * gear_ratio;
    motor_runtime_state.gear_ratio = gear_ratio;
    uint32_t mode = staging_words[10];
    if ((mode - 1U) >= 4U)
    {
        mode = 1U;
        *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA5F0UL, 0x1FFFA580UL)) = mode;
#if !APP_PROFILE_RELOAD_NORMALIZED_MODE
        sample_runtime_state.control_mode = mode;
    }
    else
    {
        sample_runtime_state.control_mode = staging_words[10];
#endif
    }
#if APP_PROFILE_RELOAD_NORMALIZED_MODE
    sample_runtime_state.control_mode = staging_words[10];
#endif
    motor_runtime_state.motor_temperature_filter_old = MOTOR_TEMPERATURE_FILTER_OLD;
    motor_runtime_state.motor_temperature_filter_new = MOTOR_TEMPERATURE_FILTER_NEW;
    /* load_motor_configuration installs the analogue
     * encoder weights and clears these words after motor constants. */
    *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1B0UL, 0x1FFFF13CUL)) = 0x3F7C0A7BU;
    *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF1B4UL, 0x1FFFF140UL)) = 0x3C7D6140U;
    *((volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFF220UL, 0x1FFFF1ACUL)) = 0U;
    sample_runtime_state.outer_loop_divider = 0U;
    motor_runtime_state.raw_rotor_velocity = 0.0f;
    controller->command.mode = (MotorControlMode)mode;
}

float current_controller_state_step(CurrentController *axis)
{
    const float tracking_error = axis->measurement - axis->estimated_current;
    axis->tracking_error = tracking_error;
    /* Keep the three additions in the firmware VMLA order.  Regrouping this
     * expression changes the observer state after sustained 20 kHz use. */
    float estimated_current =
        control_vmla_f32(axis->estimated_current, axis->sample_period, axis->disturbance_state);
    estimated_current = control_vmla_f32(estimated_current, axis->observer_l1_dt, tracking_error);
    estimated_current =
        control_vmla_f32(estimated_current, axis->plant_gain_dt, axis->control_output);
    axis->estimated_current = estimated_current;
    float disturbance_state =
        control_vmla_f32(axis->disturbance_state, axis->observer_l2_dt, tracking_error);
    axis->disturbance_state = disturbance_state;
    axis->control_error = axis->reference - axis->estimated_current;
    axis->raw_control = axis->control_bandwidth * axis->control_error;
    axis->control_output = (axis->raw_control - axis->disturbance_state) * axis->inverse_plant_gain;
    /* current_controller likewise preserves unordered output. */
    axis->control_output = motor_clampf(axis->control_output, axis->output_min, axis->output_max);
    axis->limited_output = axis->control_output;
    return axis->limited_output;
}

float motor_control_current_step(CurrentController *axis, float reference, float measurement)
{
    axis->reference = reference;
    axis->measurement = measurement;
    return current_controller_helper(axis);
}

void motor_control_set_command(MotorController *controller, const MotorCommand *command)
{
    /* CAN publishes fixed words during decoding in firmware order. This
     * setter only mirrors each family's fields; CTRL_MODE is untouched. */
    if (command->mode != MOTOR_MODE_SPEED)
    {
        controller->command.position = command->position;
    }
    controller->command.velocity = command->velocity;
    if (command->mode == MOTOR_MODE_MIT)
    {
        controller->command.torque = command->torque;
        controller->command.kp = command->kp;
        controller->command.kd = command->kd;
    }
    else if (command->mode == MOTOR_MODE_HYBRID)
    {
        controller->command.torque = command->torque;
    }
    controller->command.mode = command->mode;
}

void motor_control_set_current_calibration(MotorController *controller, float offset_u,
                                           float offset_v, float offset_w)
{
    controller->current_offset_u = offset_u;
    controller->current_offset_v = offset_v;
    controller->current_offset_w = offset_w;
    sample_runtime_state.current_offset_u = offset_u;
    sample_runtime_state.current_offset_v = offset_v;
    sample_runtime_state.current_offset_w = offset_w;
}

void motor_control_clear_command(MotorController *controller)
{
    /* main leaves the fixed control-mode word untouched. */
    sample_runtime_state.command_position = 0.0f;
    sample_runtime_state.command_velocity = 0.0f;
    sample_runtime_state.command_torque = 0.0f;
    sample_runtime_state.command_kp = 0.0f;
    sample_runtime_state.command_kd = 0.0f;
    __asm volatile("" : : : "memory");
    controller->command.position = 0.0f;
    controller->command.velocity = 0.0f;
    controller->command.torque = 0.0f;
    controller->command.kp = 0.0f;
    controller->command.kd = 0.0f;
}

void motor_control_arm(MotorController *controller)
{
    motor_runtime_state.console_mode = 2U;
    controller->armed = true;
}

void motor_control_reset_dynamic_state(MotorController *controller)
{
    /* This path clears the sample first, then interleaves the
     * two current observers. Volatile stores preserve that SRAM order. */
    (void)controller;
    const float cleared =
        *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFFA118UL, 0x1FFFA150UL);
    volatile float *const sample = (volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA11CUL, 0x1FFFA154UL);
    volatile CurrentController *const current_q = (volatile CurrentController *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA134UL, 0x1FFFA16CUL);
    sample[7] = cleared;
    sample[8] = cleared;
    sample[11] = cleared;
    volatile CurrentController *const current_d = (volatile CurrentController *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA130UL, 0x1FFFA168UL);
    current_d->tracking_error = cleared;
    current_q->tracking_error = cleared;
    current_d->estimated_current = cleared;
    current_q->estimated_current = cleared;
    current_d->disturbance_state = cleared;
    current_q->disturbance_state = cleared;
    current_d->control_error = cleared;
    current_q->control_error = cleared;
    current_d->raw_control = cleared;
    current_q->raw_control = cleared;
    current_d->limited_output = cleared;
    current_q->limited_output = cleared;
    current_d->control_output = cleared;
    volatile float *const speed = (volatile float *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA128UL, 0x1FFFA160UL);
    current_q->control_output = cleared;
    commissioning_reset_runtime_loop_states(speed, cleared);
}

void motor_control_disarm(MotorController *controller)
{
    motor_runtime_state.console_mode = 0U;
    controller->armed = false;
}

void motor_control_trip(MotorController *controller)
{
    motor_runtime_state.console_mode = 0U;
    controller->armed = false;
}

void motor_control_set_runtime_fault(MotorFault fault)
{
    sample_runtime_state.fault = (uint32_t)fault;
}

MotorFault motor_control_runtime_fault(void)
{
    return (MotorFault)sample_runtime_state.fault;
}

void motor_control_set_raw_rotor_velocity(float raw_rotor_velocity)
{
    motor_runtime_state.raw_rotor_velocity = raw_rotor_velocity;
}

void motor_control_set_position_runtime(float rotor_position, int32_t revolutions,
                                        float output_position)
{
    motor_runtime_state.rotor_position = rotor_position;
    motor_runtime_state.revolutions = revolutions;
    motor_runtime_state.output_position = output_position;
}

uint32_t motor_control_console_mode(void)
{
    return motor_runtime_state.console_mode;
}

void motor_control_set_console_mode(uint32_t mode)
{
    motor_runtime_state.console_mode = mode;
}

void motor_control_post_state_change(void)
{
    motor_runtime_state.motor_state_changed = 1U;
}

bool motor_control_take_state_change(uint32_t *retained_mode)
{
    if (motor_runtime_state.motor_state_changed != 1U)
    {
        return false;
    }
    const uint32_t mode = motor_runtime_state.console_mode;
    if ((mode != 0U) && (mode != 2U))
    {
        /* main preserves a pending event in setup/other modes. */
        return false;
    }
    *retained_mode = mode;
    motor_runtime_state.motor_state_changed = 0U;
    return true;
}

static float slew_velocity(float current, float target, const MotorConfig *config,
                           const volatile float *limits, volatile float *state)
{
    (void)current;
    (void)target;
    (void)config;
    float result;
    /* +0x1c is the target, +0x20 the slewed value; +0x24 also holds
     * the slew error. BCS deliberately accepts unordered comparisons. */
    __asm volatile("vldr s3, [%1]\n"
                   "vldr s1, [%1, #4]\n"
                   "vsub.f32 s0, s3, s1\n"
                   "vstr s0, [%1, #8]\n"
                   "vldr s2, [%2, #16]\n"
                   "vcmpe.f32 s0, s2\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bgt 1f\n"
                   "vldr s2, [%2, #20]\n"
                   "vcmpe.f32 s0, s2\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bcs 2f\n"
                   "1: vadd.f32 s3, s1, s2\n"
                   "2: vstr s3, [%1, #4]\n"
                   "vmov.f32 %0, s3"
                   : "=t"(result)
                   : "r"(state), "r"(limits)
                   : "s0", "s1", "s2", "s3", "cc", "memory");
    return result;
}

static bool control_outer_step(MotorController *controller, const MotorConfig *config,
                               float rotor_velocity, bool outer_loop_due,
                               const OuterLoopContext *references)
{
    (void)controller;
    if (!outer_loop_due)
    {
        return false;
    }
    volatile SampleRuntimeState *const sample_state =
        (volatile SampleRuntimeState *)references->sample;
    volatile MotorRuntimeState *const motor_state = (volatile MotorRuntimeState *)references->motor;

    volatile float *const position_loop = references->speed + 10;
    const float command_position = sample_state->command_position;
    const float output_position = motor_state->output_position;
    float position_error;
    __asm volatile("vsub.f32 %0, %1, %2"
                   : "=t"(position_error)
                   : "t"(command_position), "t"(output_position));
    position_loop[2] = position_error;
    const float position_gain = position_loop[0];
    float position_output;
    __asm volatile("vmul.f32 %0, %1, %2"
                   : "=t"(position_output)
                   : "t"(position_gain), "t"(position_error));
    position_loop[6] = position_output;
    const uint32_t mode = sample_state->control_mode;
    if ((mode == MOTOR_MODE_POSITION_SPEED) || (mode == MOTOR_MODE_HYBRID))
    {
        const float limit = sample_state->command_velocity;
        sample_state->desired_velocity = clamp_helper(position_output, -limit, limit);
    }
    else if (mode == MOTOR_MODE_SPEED)
    {
        const float limit = references->limits[6];
        const float command_velocity = sample_state->command_velocity;
        sample_state->desired_velocity = clamp_helper(command_velocity, -limit, limit);
    }
    /* MIT/unknown retain the target, but still publish position-loop state
     * and advance the fixed slew state before current filtering. */
    (void)slew_velocity(0.0f, 0.0f, config, references->limits, &sample_state->desired_velocity);

    /* This path executes after target slewing. Its observer
     * input is raw Q current, not the torque-reporting filtered value. */
    const float old_weight = motor_state->velocity_filter_previous;
    const float previous_current = motor_state->filtered_current_q;
    const float new_weight = motor_state->velocity_filter_new;
    /* Retain this observer pointer through the observer call and subsequent
     * speed-loop reads.  Load it before filtering begins. */
    MotionObserver *const observer = (MotionObserver *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8630UL, 0x1FFF9978UL);
    float filtered_current;
    __asm volatile("vmul.f32 %0, %1, %2"
                   : "=t"(filtered_current)
                   : "t"(old_weight), "t"(previous_current)
                   : "memory");
    const float current_q = sample_state->current_q;
    __asm volatile("vmla.f32 %0, %1, %2"
                   : "+t"(filtered_current)
                   : "t"(new_weight), "t"(current_q)
                   : "memory");
    motor_state->filtered_current_q = filtered_current;
    const float torque_constant = motor_state->output_torque_constant;
    float torque;
    __asm volatile("vmul.f32 %0, %1, %2"
                   : "=t"(torque)
                   : "t"(torque_constant), "t"(filtered_current)
                   : "memory");
    motor_state->output_torque = torque;
    /* Firmware reloads the live filtered velocity after torque publication;
     * the raw Q value is retained from the preceding VMLA. */
    float measured_velocity;
    __asm volatile("vldr %0, [%1]\nvstr %0, [%2]"
                   : "=&t"(measured_velocity)
                   : "r"(&motor_state->rotor_velocity), "r"(&observer->measured_velocity)
                   : "memory");
    ((volatile MotionObserver *)observer)->current_q = current_q;
    (void)rotor_velocity;
    motion_observer_helper(observer);
    volatile float *const speed_loop = references->speed;
    const float velocity = sample_state->velocity_target;
    const float estimated_velocity =
        ((const volatile MotionObserver *)observer)->estimated_velocity;
    float velocity_error;
    __asm volatile("vsub.f32 %0, %1, %2"
                   : "=t"(velocity_error)
                   : "t"(velocity), "t"(estimated_velocity));
    speed_loop[2] = velocity_error;
    const float disturbance = ((const volatile MotionObserver *)observer)->disturbance_current;
    const float speed_gain = speed_loop[0];
    float speed_output;
    __asm volatile("vnmls.f32 %0, %1, %2"
                   : "=&t"(speed_output)
                   : "t"(speed_gain), "t"(velocity_error), "0"(disturbance));
    speed_loop[6] = speed_output;
    return true;
}

static void update_fast_current_transform(MotorController *controller, const MotorConfig *config,
                                          const AdcSample *sample, float voltage_scale,
                                          volatile PositionSensorScratch *scratch,
                                          OuterLoopContext *references)
{
    /* Expose gear-scaled position and velocity as feedback.  The
     * non-MIT speed observer deliberately uses the unscaled motor-side
     * velocity instead; position_sensor_scale at 0x1ffff088+0x5c is applied
     * only when producing the output velocity at +0x1c. */
    controller->feedback.position = sample->output_position;
    (void)config;
    volatile SampleRuntimeState *const sample_state =
        (volatile SampleRuntimeState *)references->sample;
    volatile MotorRuntimeState *const motor_state = (volatile MotorRuntimeState *)references->motor;
    float sine;
    float cosine;
    if (sample->position_sample_ready)
    {
        const uint32_t pole_pairs = motor_state->pole_pairs;
        const float angle = scratch->wrapped_angle;
        const float per_radian = motor_state->pole_pairs_per_radian;
        float electrical_angle;
        float turns;
        __asm volatile("vcvt.f32.u32 %0, %0\n"
                       "vmul.f32 %0, %2, %0\n"
                       "vmul.f32 %1, %2, %3\n"
                       "vcvt.u32.f32 %1, %1\n"
                       "vcvt.f32.u32 %1, %1"
                       : "=&t"(electrical_angle), "=&t"(turns)
                       : "t"(angle), "t"(per_radian), "0"(pole_pairs)
                       : "memory");
        const float two_pi =
            *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8448UL, 0x1FFF975CUL);
        __asm volatile("vmls.f32 %0, %1, %2"
                       : "+t"(electrical_angle)
                       : "t"(turns), "t"(two_pi)
                       : "memory");
        const float offset = motor_state->electrical_offset;
        __asm volatile("vadd.f32 %0, %0, %1" : "+t"(electrical_angle) : "t"(offset));
        motor_state->electrical_angle = electrical_angle;
        const float maximum =
            *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF844CUL, 0x1FFF9760UL);
        const float minimum =
            *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8450UL, 0x1FFF9764UL);
        wrap_helper(&motor_state->electrical_angle, minimum, maximum);
        motor_target_sincos(motor_state->electrical_angle, &sample_state->electrical_sine,
                            &sample_state->electrical_cosine);
    }
    const float current_u = sample_state->current_u;
    sample_state->current_alpha = current_u;
    float beta;
    const float two = 2.0f;
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
    (void)voltage_scale;
    const float phase_for_beta = sample_state->current_w;
    const float inverse_sqrt_three = *(const volatile float *)(uintptr_t)UINT32_C(0x1fff9768);
#else
    const float phase_for_beta = sample_state->current_v;
    const float inverse_sqrt_three = voltage_scale;
#endif
    references->status = (volatile uint32_t *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF83F8UL, 0x1FFF970CUL);
    __asm volatile("vmla.f32 %0, %1, %2"
                   : "=&t"(beta)
                   : "t"(phase_for_beta), "t"(two), "0"(current_u)
                   : "memory");
    references->limits = (const volatile float *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF83FCUL, 0x1FFF9710UL);
    references->speed = (volatile float *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8454UL, 0x1FFF976CUL);
    __asm volatile("vmul.f32 %0, %0, %1" : "+t"(beta) : "t"(inverse_sqrt_three) : "memory");
    sample_state->current_beta = beta;
    cosine = sample_state->electrical_cosine;
    sine = sample_state->electrical_sine;
    DirectQuadrature current_dq;
    __asm volatile("vmul.f32 %0, %2, %3\n"
                   "vmla.f32 %0, %4, %5\n"
                   "vmul.f32 %1, %2, %5\n"
                   "vmls.f32 %1, %4, %3"
                   : "=&t"(current_dq.d), "=&t"(current_dq.q)
                   : "t"(cosine), "t"(current_u), "t"(sine), "t"(beta));
    sample_state->current_d = current_dq.d;
    sample_state->current_q = current_dq.q;
}

/* Firmware IRQ002 selects the hybrid current-limit source with an explicit
 * branch.  Thumb IT conversion around the VFP loads changes the live S0 value
 * observed by the fixed SRAM clamp helper, so keep this fixed-layout boundary
 * branch-based even under the firmware size optimization. */
__attribute__((optimize("no-if-conversion", "no-if-conversion2"))) static CurrentController *
prepare_current_controllers(MotorController *controller, const MotorConfig *config,
                            const AdcSample *sample, float cleared,
                            const OuterLoopContext *references)
{
    volatile SampleRuntimeState *const sample_state =
        (volatile SampleRuntimeState *)references->sample;
    volatile MotorRuntimeState *const motor_state = (volatile MotorRuntimeState *)references->motor;
    (void)control_outer_step(controller, config, sample->rotor_velocity, sample->outer_loop_due,
                             references);
    const uint32_t mode = sample_state->control_mode;
    float requested_q;
    if (mode == MOTOR_MODE_MIT)
    {
        const float command_position = sample_state->command_position;
        const float position = motor_state->output_position;
        const float velocity = motor_state->output_velocity;
        float requested_torque;
        __asm volatile("vsub.f32 %0, %1, %2"
                       : "=t"(requested_torque)
                       : "t"(command_position), "t"(position));
        const float kp = sample_state->command_kp;
        __asm volatile("vmul.f32 %0, %0, %1" : "+t"(requested_torque) : "t"(kp));
        const float command_velocity = sample_state->command_velocity;
        float velocity_error;
        __asm volatile("vsub.f32 %0, %1, %2"
                       : "=t"(velocity_error)
                       : "t"(command_velocity), "t"(velocity));
        const float kd = sample_state->command_kd;
        __asm volatile("vmla.f32 %0, %1, %2"
                       : "+t"(requested_torque)
                       : "t"(kd), "t"(velocity_error));
        const float feed_forward = sample_state->command_torque;
        __asm volatile("vadd.f32 %0, %0, %1" : "+t"(requested_torque) : "t"(feed_forward));
        *(volatile float *)(references->temperature_scratch + 4U) = requested_torque;
        const float inverse_torque = motor_state->inverse_output_torque_constant;
        __asm volatile("vmul.f32 %0, %1, %2"
                       : "=t"(requested_q)
                       : "t"(inverse_torque), "t"(requested_torque));
    }
    else if ((mode == MOTOR_MODE_POSITION_SPEED) || (mode == MOTOR_MODE_SPEED) ||
             (mode == MOTOR_MODE_HYBRID))
    {
        requested_q = references->speed[6];
    }
    else
    {
        requested_q = cleared;
    }
    /* Known modes publish the unbounded reference before reading the
     * limit. Unknown modes write zero and skip the clamp entirely. */
    sample_state->current_q_reference = requested_q;
    if ((mode >= MOTOR_MODE_MIT) && (mode <= MOTOR_MODE_HYBRID))
    {
        const float current_limit =
            (mode == MOTOR_MODE_HYBRID) ? sample_state->current_limit : references->limits[3];
        requested_q = clamp_helper(requested_q, -current_limit, current_limit);
        sample_state->current_q_reference = requested_q;
    }

    /* Preserve VFP-based state publication.  Plain float copies otherwise
     * become integer LDR/STR pairs and change floating-point side effects. */
    CurrentController *const current_d = (CurrentController *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8634UL, 0x1FFF997CUL);
    CurrentController *const current_q = current_d + 1;
    float publication;
    __asm volatile("vldr %0, [%1]\nvstr %0, [%2]\n"
                   "vstr %7, [%3]\n"
                   "vldr %0, [%4]\nvstr %0, [%5]\n"
                   "vldr %0, [%6]\nvstr %0, [%8]"
                   : "=&t"(publication)
                   : "r"(&sample_state->current_d), "r"(&current_d->measurement),
                     "r"(&current_d->reference), "r"(&sample_state->current_q),
                     "r"(&current_q->measurement), "r"(&sample_state->current_q_reference),
                     "t"(cleared), "r"(&current_q->reference)
                   : "memory");
    (void)current_controller_helper(current_d);
    (void)current_controller_helper(current_q);
    return current_d;
}

void motor_control_begin_sample(OuterLoopContext *references)
{
    volatile SampleRuntimeState *const sample_state = (volatile SampleRuntimeState *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8400UL, 0x1FFF9714UL);
    references->sample = sample_state;
    const uint32_t divider = sample_state->outer_loop_divider + 1U;
    sample_state->outer_loop_divider = divider;
}

float motor_control_fast_sample_prefix(MotorController *controller, AdcSample *sample,
                                       const volatile uint16_t *raw, OuterLoopContext *references)
{
    volatile SampleRuntimeState *const sample_state =
        (volatile SampleRuntimeState *)references->sample;
    const uint16_t mos_raw = raw[6];
    const volatile float *const temperatures = (const volatile float *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8430UL, 0x1FFF9744UL);
    volatile uint8_t *const temperature_index = (volatile uint8_t *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF842CUL, 0x1FFF9740UL);
    references->temperature_scratch = (uintptr_t)temperature_index;
    const float mos_temperature = temperatures[(mos_raw >> 4U) & 0xFFU];
    volatile MotorRuntimeState *const motor = (volatile MotorRuntimeState *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8404UL, 0x1FFF9718UL);
    references->motor = motor;
    sample->mos_temperature = mos_temperature;
    controller->feedback.mos_temperature = sample->mos_temperature;
    sample_state->mos_temperature = sample->mos_temperature;
    const uint8_t motor_temperature_index = (uint8_t)(raw[7] >> 4U);
    /* Firmware byte scratch aliases the last word of the Q drive object. */
    *temperature_index = motor_temperature_index;
    float filtered_temperature;
    const float previous_temperature = motor->filtered_motor_temperature;
    const float temperature_old = motor->motor_temperature_filter_old;
    __asm volatile("vmul.f32 %0, %1, %2"
                   : "=t"(filtered_temperature)
                   : "t"(previous_temperature), "t"(temperature_old));
    const float temperature_new = motor->motor_temperature_filter_new;
    sample->motor_temperature = temperatures[motor_temperature_index];
    __asm volatile("vmla.f32 %0, %1, %2"
                   : "+t"(filtered_temperature)
                   : "t"(temperature_new), "t"(sample->motor_temperature));
    controller->filtered_motor_temperature = filtered_temperature;
    controller->feedback.motor_temperature = controller->filtered_motor_temperature;
    motor->filtered_motor_temperature = controller->filtered_motor_temperature;
    const uint16_t bus_raw = raw[3];
    const float bus_scale =
        *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8434UL, 0x1FFF9748UL);
    sample->bus_voltage = (float)bus_raw * bus_scale;
    controller->feedback.bus_voltage = sample->bus_voltage;
    sample_state->bus_voltage = sample->bus_voltage;
    const float voltage_scale =
        *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8438UL, 0x1FFF974CUL);
    sample_state->normalized_bus_voltage = sample->bus_voltage * voltage_scale;
    sample->phase_u = (float)raw[0];
    const float offset_u = sample_state->current_offset_u;
    float current_u;
    __asm volatile("vsub.f32 %0, %1, %2"
                   : "=t"(current_u)
                   : "t"(offset_u), "t"(sample->phase_u)
                   : "memory");
    const float current_scale =
        *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF843CUL, 0x1FFF9750UL);
    current_u *= current_scale;
    sample_state->current_u = current_u;
    sample->phase_v = (float)raw[1];
    const float current_v = (sample_state->current_offset_v - sample->phase_v) * current_scale;
    sample_state->current_v = current_v;
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
    sample->phase_w = (float)raw[2];
    const float current_w = (sample_state->current_offset_w - sample->phase_w) * current_scale;
    sample_state->current_w = current_w;
#endif
    sample_state->current_u = clamp_helper(current_u, -1.0f, 1.0f);
    sample_state->current_v = clamp_helper(sample_state->current_v, -1.0f, 1.0f);
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
    sample_state->current_w = clamp_helper(sample_state->current_w, -1.0f, 1.0f);
#endif
    return voltage_scale;
}

CurrentController *motor_control_fast_prepare(MotorController *controller,
                                              const MotorConfig *config, const AdcSample *sample,
                                              float cleared, const OuterLoopContext *references)
{
    return prepare_current_controllers(controller, config, sample, cleared, references);
}

float motor_control_fast_transform(MotorController *controller, const MotorConfig *config,
                                   AdcSample *sample, float voltage_scale,
                                   volatile PositionSensorScratch *scratch,
                                   OuterLoopContext *references)
{
    update_fast_current_transform(controller, config, sample, voltage_scale, scratch, references);
    const float cleared =
        *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8458UL, 0x1FFF9770UL);
    /* IRQ002 tests the divider after publishing D/Q currents. */
    const volatile SampleRuntimeState *const sample_state =
        (const volatile SampleRuntimeState *)references->sample;
    sample->outer_loop_due = sample_state->outer_loop_divider == 20U;
    return cleared;
}

void motor_control_fast_apply_state(MotorController *controller, const OuterLoopContext *references)
{
    const volatile MotorRuntimeState *const motor_state =
        (const volatile MotorRuntimeState *)references->motor;
    const bool enabled = motor_state->console_mode == 2U;
    controller->armed = enabled;
    /* IRQ002 clears/increments communication age before selective reset,
     * using the same retained enabled-state decision. */
    safety_finish_control_tick(enabled, references->status);
    if (!enabled)
    {
        /* IRQ002 tests the enabled state only after fault monitoring. */
        reset_control_state_helper();
    }
}

AlphaBeta motor_control_fast_finish(volatile CurrentController *current_d,
                                    const OuterLoopContext *references)
{
    /* IRQ002 retains its 8506 pool pointer through fault monitoring/reset. */
    volatile CurrentController *const current_q = current_d + 1;
    const float limit = *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8638UL, 0x1FFF9980UL);
    limit_vector_helper(limit, &current_d->limited_output, &current_q->limited_output);
    volatile SampleRuntimeState *const sample_state =
        (volatile SampleRuntimeState *)references->sample;
    const float cosine = sample_state->electrical_cosine;
    const float voltage_d = current_d->limited_output;
    const float sine = sample_state->electrical_sine;
    const float voltage_q = current_q->limited_output;
    AlphaBeta stationary_voltage;
    __asm volatile("vmul.f32 %0, %2, %3\n"
                   "vmls.f32 %0, %4, %5\n"
                   "vmul.f32 %1, %4, %3\n"
                   "vmla.f32 %1, %2, %5"
                   : "=&t"(stationary_voltage.alpha), "=&t"(stationary_voltage.beta)
                   : "t"(cosine), "t"(voltage_d), "t"(sine), "t"(voltage_q)
                   : "memory");
    sample_state->voltage_alpha = stationary_voltage.alpha;
    sample_state->voltage_beta = stationary_voltage.beta;
    return stationary_voltage;
}

PhaseDuty motor_control_fast_step(MotorController *controller, const MotorConfig *config,
                                  const AdcSample *sample)
{
    AdcSample working_sample = *sample;
    OuterLoopContext references;
    motor_control_begin_sample(&references);
    const float voltage_scale = motor_control_fast_sample_prefix(
        controller, &working_sample,
        (const volatile uint16_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFC778UL, 0x1FFFF348UL), &references);
    const float cleared = motor_control_fast_transform(
        controller, config, &working_sample, voltage_scale,
        (volatile PositionSensorScratch *)MEMORY_LAYOUT_ADDRESS(0x1FFFF190UL, 0x1FFFF11CUL),
        &references);
    CurrentController *const current_d =
        motor_control_fast_prepare(controller, config, &working_sample, cleared, &references);
    motor_control_fast_apply_state(controller, &references);
    return motor_svpwm_modulation(motor_control_fast_finish(current_d, &references));
}
