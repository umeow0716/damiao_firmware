#ifndef DAMIAO_PARAMETER_PROTOCOL_H
#define DAMIAO_PARAMETER_PROTOCOL_H

#include <stdbool.h>

#include "can_protocol.h"
#include "motor_control.h"

#if defined(DAMIAO_DM4310)
typedef struct Dm4310McanIrqReferences Dm4310McanIrqReferences;
#endif

typedef struct {
    float motor_position;
    float output_position;
    float output_sensor_calibration[4];
    float output_position_offset;
} ParameterRuntime;

typedef struct {
    bool handled;
    bool response_ready;
#if defined(DAMIAO_DM4310)
    bool response_prebuilt;
#endif
    bool store_requested;
    bool persist_requested;
    bool filter_update_requested;
    /* DM4310: request the post-send scratch check, not an early decision. */
    bool transport_reconfigure_requested;
    bool bootloader_requested;
#if defined(DAMIAO_DM4310)
    volatile uint8_t *payload;
    volatile uint8_t *scratch;
    const volatile uint8_t *config_owner;
    volatile uint32_t *motor_owner;
    volatile uint32_t *sample_owner;
    volatile uint32_t *status_owner;
    bool irq_context;
    Dm4310McanIrqReferences *references;
#endif
    CanFrame response;
} ParameterProtocolResult;

/* Decode the standard-ID 0x7ff register protocol. Writes update validated
 * RAM values. DM4310 reads fixed runtime SRAM (runtime may be NULL) and
 * STORE posts deferred persistence before its IRQ reply. */
void parameter_protocol_process(const CanFrame *request,
                                MotorConfig *config,
                                MotorController *controller,
                                const ParameterRuntime *runtime,
                                ParameterProtocolResult *result);
#if defined(DAMIAO_DM4310)
void dm4310_parameter_protocol_process_irq(const CanFrame *request,
                                ParameterProtocolResult *result,
                                Dm4310McanIrqReferences *references);
#endif

#endif
