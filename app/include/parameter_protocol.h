#ifndef DAMIAO_PARAMETER_PROTOCOL_H
#define DAMIAO_PARAMETER_PROTOCOL_H

#include <stdbool.h>

#include "can_protocol.h"
#include "motor_control.h"

typedef struct {
    float motor_position;
    float output_position;
    float output_sensor_calibration[4];
    float output_position_offset;
} ParameterRuntime;

typedef struct {
    bool handled;
    bool response_ready;
    bool store_requested;
    bool persist_requested;
    bool filter_update_requested;
    bool transport_reconfigure_requested;
    bool bootloader_requested;
    CanFrame response;
} ParameterProtocolResult;

/* Decode the standard-ID 0x7ff register protocol. Writes update validated
 * RAM values. Store is reported separately and is acknowledged only after a
 * future persistent-Flash implementation has committed the record. */
void parameter_protocol_process(const CanFrame *request,
                                MotorConfig *config,
                                MotorController *controller,
                                const ParameterRuntime *runtime,
                                ParameterProtocolResult *result);

#endif
