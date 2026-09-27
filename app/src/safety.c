#include "safety.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static bool persisted(bool condition, uint32_t *counter, uint32_t threshold,
                      bool allow_reset)
{
    if (!condition) {
        if (allow_reset) {
            *counter = 0U;
        }
        return false;
    }
    ++*counter;
    if (*counter > threshold) {
        /* The recovered monitor writes the threshold back when it trips:
         * 8000/5000/20000, after the 8001st/5001st/20001st bad sample. */
        *counter = threshold;
        return true;
    }
    return false;
}

static bool mos_temperature_word_exceeds_limit(float temperature)
{
    uint32_t bits;
    uint32_t limit_bits;
    const float limit = SAFETY_MOS_TEMPERATURE_LIMIT_C;
    memcpy(&bits, &temperature, sizeof(bits));
    memcpy(&limit_bits, &limit, sizeof(limit_bits));
    /* motor_fault_monitor@0x1fff805a uses LDR/CMP/BLE on these words rather
     * than a VFP comparison.  For ordinary non-negative temperatures this
     * is equivalent to >120 C, while preserving its exact Inf/NaN behavior. */
    return (int32_t)bits > (int32_t)limit_bits;
}

void safety_init(FaultMonitor *monitor)
{
    memset(monitor, 0, sizeof(*monitor));
}

MotorFault safety_update(FaultMonitor *monitor,
                         const MotorConfig *config,
                         const MotorFeedback *feedback,
                         bool motor_armed)
{
    /* The fault monitor at original 0x1fff8000 checks CAN age
     * before the analogue limits.  The age is advanced once per 50 us ADC
     * control tick only while the motor state is enabled.  A non-zero fault
     * flag prevents healthy inputs from clearing counters, but the reference
     * still evaluates every bad input and lets a later persistent fault
     * replace the diagnostic code.  "Latched" therefore means that the
     * controller cannot return to no-fault without Clear, not first-fault
     * wins forever. */
    if (motor_armed && config->communication_timeout != 0U &&
        monitor->communication_ticks > config->communication_timeout) {
        monitor->latched_fault = MOTOR_FAULT_COMMUNICATION_LOST;
    }
    if (persisted(
            feedback->motor_temperature > config->motor_temperature_limit,
            &monitor->motor_overtemperature_ticks, 8000U,
            monitor->latched_fault == MOTOR_FAULT_NONE)) {
        monitor->latched_fault = MOTOR_FAULT_MOTOR_OVERTEMPERATURE;
    }
    if (persisted(mos_temperature_word_exceeds_limit(
                      feedback->mos_temperature),
                  &monitor->mos_overtemperature_ticks, 8000U,
                  monitor->latched_fault == MOTOR_FAULT_NONE)) {
        monitor->latched_fault = MOTOR_FAULT_MOS_OVERTEMPERATURE;
    }
    if (persisted(fabsf(feedback->current_q) > config->current_limit,
                  &monitor->overcurrent_ticks, 5000U,
                  monitor->latched_fault == MOTOR_FAULT_NONE)) {
        monitor->latched_fault = MOTOR_FAULT_OVERCURRENT;
    }
    if (persisted(feedback->bus_voltage <= config->bus_undervoltage,
                  &monitor->undervoltage_ticks, 5000U,
                  monitor->latched_fault == MOTOR_FAULT_NONE)) {
        monitor->latched_fault = MOTOR_FAULT_BUS_UNDERVOLTAGE;
    }
    if (persisted(feedback->bus_voltage > config->bus_overvoltage,
                  &monitor->overvoltage_ticks, 20000U,
                  monitor->latched_fault == MOTOR_FAULT_NONE)) {
        monitor->latched_fault = MOTOR_FAULT_BUS_OVERVOLTAGE;
    }

    if (!motor_armed || monitor->latched_fault != MOTOR_FAULT_NONE) {
        monitor->communication_ticks = 0U;
    } else {
        ++monitor->communication_ticks;
    }
    return monitor->latched_fault;
}

void safety_note_control_frame(FaultMonitor *monitor)
{
    /* The original MCAN IRQ resets the age as soon as ID[7:0] addresses this
     * motor, before checking the command family/control mode.  The caller
     * represents a rejected family as FEEDBACK_ONLY, so it reaches here too.
     * An already-latched fault still prevents the reset. */
    if (monitor->latched_fault == MOTOR_FAULT_NONE) {
        monitor->communication_ticks = 0U;
    }
}

void safety_latch_runtime_fault(FaultMonitor *monitor, MotorFault fault)
{
    if ((monitor->latched_fault == MOTOR_FAULT_NONE) &&
        (fault != MOTOR_FAULT_NONE)) {
        monitor->latched_fault = fault;
    }
}

void safety_set_startup_fault(FaultMonitor *monitor, MotorFault fault)
{
    monitor->startup_fault = fault;
    if ((fault != MOTOR_FAULT_NONE) ||
        (monitor->latched_fault == monitor->startup_fault)) {
        monitor->latched_fault = fault;
    }
}

void safety_clear(FaultMonitor *monitor)
{
    /* FB clears the communication age/latch and the reported fault, but the
     * original leaves the analogue debounce counters intact.  A condition
     * which is still bad can therefore trip again on the next control tick. */
    monitor->communication_ticks = 0U;
    monitor->startup_fault = MOTOR_FAULT_NONE;
    monitor->latched_fault = MOTOR_FAULT_NONE;
}
