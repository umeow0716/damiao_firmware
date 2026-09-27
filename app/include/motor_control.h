#ifndef DM4310_MOTOR_CONTROL_H
#define DM4310_MOTOR_CONTROL_H

#include <stdbool.h>

#include "motor_math.h"
#include "motor_types.h"

typedef struct {
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
    /* Gear-scaled feedback reconstructed from the motor-side encoder. */
    float output_position;
    float output_velocity;
} AdcSample;

/* State layout recovered from current_controller_step at 0x1fffa07a.  Keeping
 * semantic field names here makes the controller maintainable while the
 * implementation remains directly auditable against the 19-float original. */
typedef struct {
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

typedef struct {
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

typedef struct {
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
void motor_control_set_current_calibration(MotorController *controller,
                                           float offset_u, float offset_v,
                                           float offset_w);
void motor_control_arm(MotorController *controller);
void motor_control_disarm(MotorController *controller);
void motor_control_trip(MotorController *controller);
void motor_control_configure(MotorController *controller,
                             const MotorConfig *config,
                             float bus_voltage);
float motor_control_current_step(CurrentController *axis,
                                 float reference, float measurement);
void motor_control_motion_observer_step(MotionObserver *observer,
                                        float measured_velocity,
                                        float current_q);
PhaseDuty motor_control_fast_step(MotorController *controller,
                                  const MotorConfig *config,
                                  const AdcSample *sample);

#endif
