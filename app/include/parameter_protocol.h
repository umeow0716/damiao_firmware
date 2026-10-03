#ifndef DAMIAO_PARAMETER_PROTOCOL_H
#define DAMIAO_PARAMETER_PROTOCOL_H

#include <stdbool.h>

#include "can_protocol.h"
#include "motor_control.h"

typedef struct McanIrqContext McanIrqContext;

typedef struct
{
    float motor_position;
    float output_position;
    float output_sensor_calibration[4];
    float output_position_offset;
} ParameterRuntime;

typedef struct
{
    bool handled;
    bool response_ready;
    bool response_prebuilt;
    bool store_requested;
    bool persist_requested;
    bool filter_update_requested;
    /* Request the firmware post-send scratch check, not an early decision. */
    bool transport_reconfigure_requested;
    bool bootloader_requested;
    volatile uint8_t *payload;
    volatile uint8_t *scratch;
    const volatile uint8_t *config_owner;
    volatile uint32_t *motor_owner;
    volatile uint32_t *sample_owner;
    volatile uint32_t *status_owner;
    bool irq_context;
    McanIrqContext *references;
    CanFrame response;
} ParameterProtocolResult;

/* Decode the standard-ID 0x7ff register protocol. Writes update validated
 * RAM values. The standard layout reads fixed runtime SRAM (runtime may be
 * NULL) and
 * STORE posts deferred persistence before its IRQ reply. */
void parameter_protocol_process(const CanFrame *request, MotorConfig *config,
                                MotorController *controller, const ParameterRuntime *runtime,
                                ParameterProtocolResult *result);
void parameter_protocol_process_irq(const CanFrame *request, ParameterProtocolResult *result,
                                    McanIrqContext *references);

#endif
