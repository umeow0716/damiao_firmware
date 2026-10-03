#ifndef DAMIAO_SAFETY_H
#define DAMIAO_SAFETY_H

#include <stdbool.h>
#include <stdint.h>

#include "motor_types.h"

/* motor_fault_monitor compares the MOS temperature against a
 * literal 120.0f; unlike the motor-temperature limit, it is not stored in
 * the 37-word user configuration record. */
#define SAFETY_MOS_TEMPERATURE_LIMIT_C (120.0f)

typedef struct
{
    uint32_t communication_ticks;
    uint32_t undervoltage_ticks;
    uint32_t overvoltage_ticks;
    uint32_t overcurrent_ticks;
    uint32_t mos_overtemperature_ticks;
    uint32_t motor_overtemperature_ticks;
    MotorFault startup_fault;
    MotorFault latched_fault;
} FaultMonitor;

typedef struct
{
    volatile uint32_t communication_ticks;        /* +0x00 */
    volatile uint32_t fault_latched;              /* +0x04 */
    volatile uint32_t reserved_08;                /* main clears nonzero +0x08 */
    volatile uint32_t save_staged_parameters;     /* +0x0c */
    volatile uint32_t commission_direction;       /* +0x10 */
    volatile uint32_t commission_position_sensor; /* +0x14 */
    uint32_t reserved_18;
    volatile uint32_t calibration_commit_request; /* +0x1c */
    uint32_t reserved_20;
    volatile uint32_t identify_motor;              /* +0x24 */
    volatile uint32_t firmware_control_request;    /* +0x28 */
    volatile uint32_t control_status_tick;         /* +0x2c */
    volatile uint32_t can_error;                   /* +0x30 */
    volatile uint32_t undervoltage_ticks;          /* +0x34 */
    volatile uint32_t overvoltage_ticks;           /* +0x38 */
    volatile uint32_t overcurrent_ticks;           /* +0x3c */
    volatile uint32_t mos_overtemperature_ticks;   /* +0x40 */
    volatile uint32_t motor_overtemperature_ticks; /* +0x44 */
    volatile uint32_t fault_indicator_ticks;       /* +0x48 */
} RuntimeStatus;

extern RuntimeStatus runtime_status;

void safety_init(FaultMonitor *monitor);
MotorFault safety_update(FaultMonitor *monitor, const MotorConfig *config,
                         const MotorFeedback *feedback, bool motor_armed);
void safety_note_control_frame(FaultMonitor *monitor);
void safety_finish_control_tick(bool motor_armed, volatile uint32_t *status);
void safety_latch_runtime_fault(FaultMonitor *monitor, MotorFault fault);
void safety_set_startup_fault(FaultMonitor *monitor, MotorFault fault);
void safety_clear(FaultMonitor *monitor);

#endif
