#ifndef DAMIAO_FEEDBACK_MEASUREMENTS_H
#define DAMIAO_FEEDBACK_MEASUREMENTS_H

#include "firmware_variant.h"

enum
{
    FEEDBACK_MOTOR_OUTPUT_VELOCITY_WORD = 0x1CU / 4U,
    FEEDBACK_MOTOR_RAW_ROTOR_VELOCITY_WORD = 0x20U / 4U,
    FEEDBACK_MOTOR_OUTPUT_TORQUE_WORD = 0x30U / 4U,
    FEEDBACK_MOTOR_OUTPUT_TORQUE_CONSTANT_WORD = 0x44U / 4U,
    FEEDBACK_MOTOR_POSITION_SENSOR_SCALE_WORD = 0x5CU / 4U,
    FEEDBACK_SAMPLE_CURRENT_Q_WORD = 0x64U / 4U,
};

static inline float feedback_measurement_velocity(const volatile float *motor)
{
#if FIRMWARE_USES_RAW_CAN_FEEDBACK
    /* Preserve the output-side rad/s unit while bypassing the reporting
     * filter: raw rotor velocity divided by the gear ratio. */
    return motor[FEEDBACK_MOTOR_RAW_ROTOR_VELOCITY_WORD] *
           motor[FEEDBACK_MOTOR_POSITION_SENSOR_SCALE_WORD];
#else
    return motor[FEEDBACK_MOTOR_OUTPUT_VELOCITY_WORD];
#endif
}

static inline float feedback_measurement_torque(const volatile float *motor,
                                                const volatile float *sample)
{
#if FIRMWARE_USES_RAW_CAN_FEEDBACK
    /* Convert the latest q-axis current directly to output torque, bypassing
     * only the reporting low-pass filter. */
    return sample[FEEDBACK_SAMPLE_CURRENT_Q_WORD] *
           motor[FEEDBACK_MOTOR_OUTPUT_TORQUE_CONSTANT_WORD];
#else
    (void)sample;
    return motor[FEEDBACK_MOTOR_OUTPUT_TORQUE_WORD];
#endif
}

#endif
