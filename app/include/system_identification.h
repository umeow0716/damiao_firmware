#ifndef DAMIAO_SYSTEM_IDENTIFICATION_H
#define DAMIAO_SYSTEM_IDENTIFICATION_H

#include <stdbool.h>
#include <stdint.h>
#include "identification_scan.h"

#define SYSID_REQUEST_ID 0x6F0U
#define SYSID_RESPONSE_ID 0x6F1U
#define SYSID_SAMPLE_ID 0x6F2U
#define SYSID_PROTOCOL_VERSION 1U

typedef enum
{
    SYSID_CAPTURE_START = 1,
    SYSID_CAPTURE_STOP = 2,
    SYSID_CAPTURE_HEARTBEAT = 3,
    SYSID_INFO = 4,
    SYSID_STATUS = 5,
    SYSID_SCAN_LOWER = 6, SYSID_SCAN_UPPER, SYSID_SCAN_HOME, SYSID_SCAN_SPEED,
    SYSID_SCAN_ACCEL, SYSID_SCAN_TORQUE, SYSID_SCAN_TEMPERATURE, SYSID_SCAN_REPEATS,
    SYSID_SCAN_ERROR, SYSID_SCAN_SECONDS, SYSID_SCAN_START, SYSID_SCAN_STATUS,
    SYSID_SCAN_ABORT, SYSID_DATA_ACK, SYSID_DATA_REPLAY,
    SYSID_SAFETY_STATUS, SYSID_LIVE_START, SYSID_LIVE_STOP,
    SYSID_GUARD_LOWER, SYSID_GUARD_UPPER, SYSID_GUARD_SPEED, SYSID_GUARD_TORQUE, SYSID_GUARD_ARM,
} IdentificationOperation;

typedef enum
{
    SYSID_ACCEPTED = 0,
    SYSID_BAD_VERSION = 1,
    SYSID_BAD_ARGUMENT = 2,
    SYSID_BUSY = 3,
    SYSID_TRANSPORT_NOT_READY = 4,
    SYSID_UNSUPPORTED_OPERATION = 5,
    SYSID_MEASUREMENTS_NOT_READY = 6,
} IdentificationResult;

typedef struct
{
    float position;
    float velocity;
    float iq;
    float torque_constant;
    float command_torque;
    float bus_voltage;
    float motor_temperature;
    float gear_ratio;
    float velocity_previous_weight;
    float current_limit;
    float direction;
    uint32_t position_tick;
    uint8_t node;
    uint8_t mode;
    uint8_t fault;
    bool armed;
    bool current_limited;
    bool voltage_limited;
    bool velocity_updated;
    float mos_temperature;
    float applied_velocity_setpoint; /* Output rad/s, after speed clamp and slew. */
} IdentificationMeasurement;

void identification_request_irq(const uint8_t data[8], uint8_t node);
void identification_command_begin_irq(void);
void identification_command_end_irq(bool accepted);
void identification_command_applied_tick(bool outer_loop_due, uint8_t mode);
void identification_position_updated_irq(void);
uint32_t identification_position_tick(void);
void identification_sample_tick(const IdentificationMeasurement *measurement);
void identification_service(void);
bool identification_scan_control_tick(float position, float velocity, float torque,
    float temperature, bool healthy, float *command);
bool identification_scan_keepalive(void);
float identification_scan_current_limit(float hardware_limit, float torque_constant);
bool identification_deadman_check(bool armed);
void identification_deadman_reset(void);

/* Platform boundary; tests supply deterministic substitutes. */
uint32_t identification_lock(void);
void identification_unlock(uint32_t mask);
bool identification_transport_ready(void);
bool identification_send(uint16_t id, const uint8_t *data, uint8_t length);

#endif
