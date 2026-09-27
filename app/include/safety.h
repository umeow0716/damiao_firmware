#ifndef DAMIAO_SAFETY_H
#define DAMIAO_SAFETY_H

#include <stdint.h>

#include "motor_types.h"

/* motor_fault_monitor@0x1fff8000 compares the MOS temperature against a
 * literal 120.0f; unlike the motor-temperature limit, it is not stored in
 * the 37-word user configuration record. */
#define SAFETY_MOS_TEMPERATURE_LIMIT_C (120.0f)

typedef struct {
    uint32_t communication_ticks;
    uint32_t undervoltage_ticks;
    uint32_t overvoltage_ticks;
    uint32_t overcurrent_ticks;
    uint32_t mos_overtemperature_ticks;
    uint32_t motor_overtemperature_ticks;
    MotorFault startup_fault;
    MotorFault latched_fault;
} FaultMonitor;

void safety_init(FaultMonitor *monitor);
MotorFault safety_update(FaultMonitor *monitor,
                         const MotorConfig *config,
                         const MotorFeedback *feedback,
                         bool motor_armed);
void safety_note_control_frame(FaultMonitor *monitor);
void safety_latch_runtime_fault(FaultMonitor *monitor, MotorFault fault);
void safety_set_startup_fault(FaultMonitor *monitor, MotorFault fault);
void safety_clear(FaultMonitor *monitor);

#endif
