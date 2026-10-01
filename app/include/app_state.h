#ifndef DAMIAO_APP_STATE_H
#define DAMIAO_APP_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "motor_control.h"
#include "safety.h"
#include "can_protocol.h"

typedef struct {
    volatile bool print_menu;
    volatile bool print_debug_info;
    volatile bool save_parameters;
    volatile bool save_staged_parameters;
    volatile bool save_zero_position;
    volatile bool reconfigure_mcan;
    volatile bool commission_direction;
    volatile bool commission_position_sensor;
    volatile bool identify_motor;
    volatile bool motor_state_changed;
    volatile bool control_status_tick;
    volatile uint8_t firmware_control_request;
    volatile uint8_t calibration_commit_request;
    volatile uint8_t can_error;
} AppEvents;

typedef struct {
    MotorConfig config;
    MotorController motor;
    FaultMonitor safety;
    AppEvents events;
    volatile float rotor_position;
    volatile float rotor_angle;
    volatile float motor_output_position;
    volatile float position;
    volatile float velocity;
    volatile uint16_t raw_position;
    CanFrame pending_store_response;
    bool pending_store_response_valid;
    uint32_t fault_indicator_ticks;
    uint8_t firmware_control_payload[128];
    bool firmware_control_payload_valid;
} AppState;

extern AppState g_app;
#if defined(DAMIAO_DM4310)
#define APP_DEFERRED_EVENTS dm4310_runtime_status
#define APP_FAULT_INDICATOR_TICKS dm4310_runtime_status.fault_indicator_ticks
#else
#define APP_DEFERRED_EVENTS g_app.events
#define APP_FAULT_INDICATOR_TICKS g_app.fault_indicator_ticks
#endif
void app_state_init(void);

#endif
