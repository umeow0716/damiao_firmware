#ifndef DAMIAO_MOTOR_CONTROL_H
#define DAMIAO_MOTOR_CONTROL_H

#include <stdbool.h>

#include "motor_math.h"
#include "motor_types.h"

struct PositionSensorScratch;

typedef struct OuterLoopContext
{
    volatile void *sample;
    volatile void *motor;
    uintptr_t temperature_scratch;
    volatile uint32_t *status;
    const volatile float *limits;
    volatile float *speed;
} OuterLoopContext;

typedef struct
{
    float phase_u;
    float phase_v;
    float phase_w;
    float bus_voltage;
    float mos_temperature;
    float motor_temperature;
    /* Motor-side encoder values used by commutation and the speed observer. */
    float rotor_angle;
    float rotor_velocity;
    /* Direct analogue output-encoder position, used for zeroing and setup. */
    float analog_output_position;
    /* Gear-scaled feedback implemented from the motor-side encoder. */
    float output_position;
    float output_velocity;
    bool position_sample_ready;
    uint16_t output_sensor_raw_u;
    uint16_t output_sensor_raw_v;
    bool outer_loop_due;
} AdcSample;

/* Fixed 19-float state shared with the SRAM current-control step.  Semantic
 * field names keep the controller maintainable without hiding its ABI. */
typedef struct
{
    float control_bandwidth;
    float reference;
    float measurement;
    float tracking_error;
    float sample_period;
    float estimated_current;
    float disturbance_state;
    float observer_bandwidth;
    float plant_gain;
    float inverse_plant_gain;
    float plant_gain_dt;
    float observer_l1_dt;
    float observer_l2_dt;
    float control_error;
    float raw_control;
    float control_output;
    float limited_output;
    float output_min;
    float output_max;
} CurrentController;

typedef struct
{
    float measured_velocity;
    float current_q;
    float error;
    float previous_error;
    float error_derivative;
    float estimated_velocity;
    float observer_state;
    float sample_period;
    float observer_bandwidth;
    float observer_state_gain;
    float plant_gain;
    float inverse_plant_gain;
    float disturbance_current;
    float output_min;
    float output_max;
} MotionObserver;

typedef struct
{
    float current_offset_u;
    float current_offset_v;
    float current_offset_w;
    float current_scale;
    float electrical_offset;
    float output_position_offset;
    float motor_output_position_offset;
    float sample_period;
    float torque_to_current;
    float speed_integral;
    float desired_velocity;
    float filtered_current_q;
    float filtered_motor_temperature;
    float current_filter_previous;
    CurrentController current_d;
    CurrentController current_q;
    MotionObserver motion_observer;
    MotorCommand command;
    MotorFeedback feedback;
    uint8_t outer_loop_divider;
    bool armed;
    bool outer_loop_ran;
} MotorController;

void motor_control_init(MotorController *controller);
void motor_control_set_command(MotorController *controller, const MotorCommand *command);
void motor_control_set_current_calibration(MotorController *controller, float offset_u,
                                           float offset_v, float offset_w);
void motor_control_arm(MotorController *controller);
void motor_control_disarm(MotorController *controller);
void motor_control_trip(MotorController *controller);
void motor_control_set_runtime_fault(MotorFault fault);
MotorFault motor_control_runtime_fault(void);
void motor_control_set_raw_rotor_velocity(float raw_rotor_velocity);
void motor_control_set_position_runtime(float rotor_position, int32_t revolutions,
                                        float output_position);
void motion_observer_helper(MotionObserver *observer);
float current_controller_helper(CurrentController *axis);
void motion_observer_state_step(MotionObserver *observer);
float current_controller_state_step(CurrentController *axis);
void reset_control_state_helper(void);
void motor_control_reset_dynamic_state(MotorController *controller);
uint32_t motor_control_console_mode(void);
void motor_control_set_console_mode(uint32_t mode);
void motor_control_post_state_change(void);
bool motor_control_take_state_change(uint32_t *mode);
void motor_control_begin_sample(OuterLoopContext *references);
void motor_control_clear_command(MotorController *controller);
float motor_control_fast_sample_prefix(MotorController *controller, AdcSample *sample,
                                       const volatile uint16_t *raw, OuterLoopContext *references);
float motor_control_fast_transform(MotorController *controller, const MotorConfig *config,
                                   AdcSample *sample, float voltage_scale,
                                   volatile struct PositionSensorScratch *scratch,
                                   OuterLoopContext *references);
CurrentController *motor_control_fast_prepare(MotorController *controller,
                                              const MotorConfig *config, const AdcSample *sample,
                                              float cleared, const OuterLoopContext *references);
void motor_control_fast_apply_state(MotorController *controller,
                                    const OuterLoopContext *references);
AlphaBeta motor_control_fast_finish(volatile CurrentController *current_d,
                                    const OuterLoopContext *references);
void motor_control_configure(MotorController *controller, const MotorConfig *config,
                             float bus_voltage);
void motor_control_configure_velocity_filter(MotorController *controller, float bandwidth);
/* DM WRITE has already selected the firmware comparison branch. */
void motor_control_configure_velocity_filter_selected(MotorController *controller, float bandwidth,
                                                      bool filtered);
void motor_control_configure_velocity_filter_irq(float bandwidth, bool filtered,
                                                 volatile float *coefficients, float radians);
void motor_control_configure_runtime_motor(MotorController *controller, const MotorConfig *config);
float motor_control_current_step(CurrentController *axis, float reference, float measurement);
void motor_control_motion_observer_step(MotionObserver *observer, float measured_velocity,
                                        float current_q);
PhaseDuty motor_control_fast_step(MotorController *controller, const MotorConfig *config,
                                  const AdcSample *sample);

#endif
