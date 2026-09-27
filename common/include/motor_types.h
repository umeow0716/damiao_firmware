#ifndef DM4310_MOTOR_TYPES_H
#define DM4310_MOTOR_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MOTOR_MODE_DISABLED = 0,
    MOTOR_MODE_MIT = 1,
    MOTOR_MODE_POSITION_SPEED = 2,
    MOTOR_MODE_SPEED = 3,
    MOTOR_MODE_HYBRID = 4,
} MotorControlMode;

typedef enum {
    MOTOR_FAULT_NONE = 0,
    MOTOR_STATUS_ENABLED = 1,
    MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING = 2,
    MOTOR_FAULT_OUTPUT_CALIBRATION = 3,
    MOTOR_FAULT_OUTPUT_SENSOR = 4,
    MOTOR_FAULT_BUS_OVERVOLTAGE = 8,
    MOTOR_FAULT_BUS_UNDERVOLTAGE = 9,
    MOTOR_FAULT_OVERCURRENT = 0xAU,
    MOTOR_FAULT_MOS_OVERTEMPERATURE = 0xBU,
    MOTOR_FAULT_MOTOR_OVERTEMPERATURE = 0xCU,
    MOTOR_FAULT_COMMUNICATION_LOST = 0xDU,
    MOTOR_FAULT_OVERLOAD = 0xEU,
} MotorFault;

typedef struct {
    float position;
    float velocity;
    float kp;
    float kd;
    float torque;
    MotorControlMode mode;
} MotorCommand;

typedef struct {
    float position;
    float velocity;
    float current_d;
    float current_q;
    float output_torque;
    float bus_voltage;
    float mos_temperature;
    float motor_temperature;
    MotorFault fault;
} MotorFeedback;

typedef struct {
    float position_min;
    float position_max;
    float velocity_min;
    float velocity_max;
    float torque_min;
    float torque_max;
    float kp_min;
    float kp_max;
    float kd_min;
    float kd_max;
    float bus_undervoltage;
    float bus_overvoltage;
    float torque_constant;
    float acceleration_limit;
    float deceleration_limit;
    float speed_limit;
    float current_limit;
    /* Fixed at 120 C by the official fault monitor; retained in the typed
     * runtime view for readable diagnostics, not as a persistent setting. */
    float mos_temperature_limit;
    float motor_temperature_limit;
    float phase_resistance;
    float phase_inductance;
    float flux_linkage;
    float viscous_damping;
    float rotor_inertia;
    float gear_ratio;
    float current_loop_bandwidth;
    float speed_kp;
    float speed_ki;
    float position_kp;
    float position_ki;
    float gear_torque_efficiency;
    float speed_loop_damping;
    float velocity_filter_bandwidth;
    float current_loop_enhancement;
    float velocity_loop_enhancement;
    float direction;
    float maximum_phase_current;
    float position_sensor_scale;
    uint32_t communication_timeout;
    uint32_t hardware_version;
    uint32_t software_version;
    uint32_t serial_number;
    uint32_t firmware_subversion;
    uint32_t bootloader_version;
    uint16_t can_id;
    uint16_t master_id;
    uint8_t pole_pairs;
    uint8_t can_data_rate_selector;
    MotorControlMode control_mode;
    bool sensor_inverted;
} MotorConfig;

#endif
