#include "safety.h"

#include "factory_layout.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(DAMIAO_DM4310)
Dm4310RuntimeStatus dm4310_runtime_status
    __attribute__((section(".dm4310_runtime_status")));
_Static_assert(sizeof(Dm4310RuntimeStatus) == 0x4CU,
               "DM4310 runtime status size");
_Static_assert(offsetof(Dm4310RuntimeStatus, save_staged_parameters) == 0x0CU,
               "DM4310 deferred save offset");
_Static_assert(offsetof(Dm4310RuntimeStatus, calibration_commit_request) == 0x1CU,
               "DM4310 calibration commit offset");
_Static_assert(offsetof(Dm4310RuntimeStatus, firmware_control_request) == 0x28U,
               "DM4310 firmware request offset");
_Static_assert(offsetof(Dm4310RuntimeStatus, undervoltage_ticks) == 0x34U,
               "DM4310 fault counter offset");
_Static_assert(offsetof(Dm4310RuntimeStatus, fault_indicator_ticks) == 0x48U,
               "DM4310 fault indicator offset");
#endif

#if defined(DAMIAO_DM4310)
static bool persisted(bool condition, volatile uint32_t *counter, uint32_t threshold,
                      volatile const uint32_t *fault_latched)
#else
static bool persisted(bool condition, uint32_t *counter, uint32_t threshold,
                      bool allow_reset)
#endif
{
    if (!condition) {
#if defined(DAMIAO_DM4310)
        if (*fault_latched == 0U) {
#else
        if (allow_reset) {
#endif
            *counter = 0U;
        }
        return false;
    }
    const uint32_t count = *counter + 1U;
    *counter = count;
    if (count > threshold) {
        /* The recovered monitor writes the threshold back when it trips:
         * 8000/5000/20000, after the 8001st/5001st/20001st bad sample. */
#if !defined(DAMIAO_DM4310)
        *counter = threshold;
#endif
        return true;
    }
    return false;
}

#if !defined(DAMIAO_DM4310)
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
#endif

void safety_init(FaultMonitor *monitor)
{
    memset(monitor, 0, sizeof(*monitor));
    /* Fixed DM4310 status/events are Reset-owned BSS, not decoded state.
     * Initializing a monitor must not erase live factory counters/events. */
}

MotorFault safety_update(FaultMonitor *monitor,
                         const MotorConfig *config,
                         const MotorFeedback *feedback,
                         bool motor_armed)
{
#if defined(DAMIAO_DM4310)
    volatile Dm4310RuntimeStatus *const status =
        (volatile Dm4310RuntimeStatus *)(uintptr_t)
        *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF83F8UL, 0x1FFF970CUL);
    const uintptr_t configuration = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF83FCUL, 0x1FFF9710UL);
    const uintptr_t sample = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8400UL, 0x1FFF9714UL);
#define STATUS_COUNTER(field) (&status->field)
#define STATUS_ALLOW_RESET (&status->fault_latched)
#define STATUS_COMMUNICATION_TICKS status->communication_ticks
#define FAULT_CONFIG(field, offset) \
    (*((const volatile float *)(configuration + (offset))))
#define FAULT_FEEDBACK(field, address) \
    (*((const volatile float *)(sample + ((address) - FACTORY_SRAM_ADDRESS(0x1FFFF104UL, 0x1FFFF090UL)))))
    (void)config;
    (void)feedback;
#else
#define STATUS_COUNTER(field) (&monitor->field)
#define STATUS_ALLOW_RESET (monitor->latched_fault == MOTOR_FAULT_NONE)
#define STATUS_COMMUNICATION_TICKS monitor->communication_ticks
#define FAULT_CONFIG(field, offset) (config->field)
#define FAULT_FEEDBACK(field, address) (feedback->field)
#endif
    /* The fault monitor at original 0x1fff8000 checks CAN age
     * before the analogue limits.  The age is advanced once per 50 us ADC
     * control tick only while the motor state is enabled.  A non-zero fault
     * flag prevents healthy inputs from clearing counters, but the reference
     * still evaluates every bad input and lets a later persistent fault
     * replace the diagnostic code.  "Latched" therefore means that the
     * controller cannot return to no-fault without Clear, not first-fault
     * wins forever. */
#if defined(DAMIAO_DM4310)
    const uint32_t communication_ticks = STATUS_COMMUNICATION_TICKS;
    const uint32_t communication_timeout =
        *((const volatile uint32_t *)(configuration + 0x24U));
    if (communication_ticks > communication_timeout &&
        communication_timeout != 0U) {
#else
    if (motor_armed && config->communication_timeout != 0U &&
        STATUS_COMMUNICATION_TICKS > config->communication_timeout) {
#endif
        monitor->latched_fault = MOTOR_FAULT_COMMUNICATION_LOST;
#if defined(DAMIAO_DM4310)
        status->communication_ticks =
            communication_timeout;
        status->fault_latched = 1U;
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_COMMUNICATION_LOST;
#endif
    }
#if defined(DAMIAO_DM4310)
    const uintptr_t motor = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8404UL, 0x1FFF9718UL);
    const float motor_temperature_limit =
        FAULT_CONFIG(motor_temperature_limit, 0x08UL);
    const float motor_temperature =
        *((const volatile float *)(motor + 0x40U));
    uint32_t motor_temperature_exceeded;
    __asm volatile (
        "vcmpe.f32 %1, %2\n"
        "vmrs APSR_nzcv, fpscr\n"
        "mov.w %0, #0\n"
        "it gt\n"
        "movgt %0, #1"
        : "=r" (motor_temperature_exceeded)
        : "t" (motor_temperature), "t" (motor_temperature_limit) : "cc");
#define MOTOR_TEMPERATURE_EXCEEDED (motor_temperature_exceeded != 0U)
#else
#define MOTOR_TEMPERATURE_EXCEEDED \
    (feedback->motor_temperature > config->motor_temperature_limit)
#endif
    if (persisted(MOTOR_TEMPERATURE_EXCEEDED,
            STATUS_COUNTER(motor_overtemperature_ticks), 8000U,
            STATUS_ALLOW_RESET)) {
        monitor->latched_fault = MOTOR_FAULT_MOTOR_OVERTEMPERATURE;
#if defined(DAMIAO_DM4310)
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_MOTOR_OVERTEMPERATURE;
        status->motor_overtemperature_ticks = 8000U;
        status->fault_latched = 1U;
#endif
    }
#if defined(DAMIAO_DM4310)
    const int32_t mos_limit = *(const volatile int32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8408UL, 0x1FFF971CUL);
    const int32_t mos_temperature = *(const volatile int32_t *)(sample + 0x84U);
    if (persisted(mos_temperature > mos_limit,
#else
    if (persisted(mos_temperature_word_exceeds_limit(
                      FAULT_FEEDBACK(mos_temperature, FACTORY_SRAM_ADDRESS(0x1FFFF188UL, 0x1FFFF114UL))),
#endif
                  STATUS_COUNTER(mos_overtemperature_ticks), 8000U,
                  STATUS_ALLOW_RESET)) {
        monitor->latched_fault = MOTOR_FAULT_MOS_OVERTEMPERATURE;
#if defined(DAMIAO_DM4310)
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_MOS_OVERTEMPERATURE;
        status->mos_overtemperature_ticks = 8000U;
        status->fault_latched = 1U;
#endif
    }
#if defined(DAMIAO_DM4310)
    const float current = FAULT_FEEDBACK(current_q, FACTORY_SRAM_ADDRESS(0x1FFFF168UL, 0x1FFFF0F4UL));
    const float current_limit = FAULT_CONFIG(current_limit, 0x0CUL);
    float absolute_current;
    uint32_t overcurrent;
    __asm volatile (
        "vabs.f32 %0, %2\nvcmpe.f32 %0, %3\n"
        "vmrs APSR_nzcv, FPSCR\nmov.w %1, #0\n"
        "it gt\nmovgt %1, #1"
        : "=&t" (absolute_current), "=r" (overcurrent)
        : "t" (current), "t" (current_limit) : "cc", "memory");
    if (persisted(overcurrent != 0U,
#else
    if (persisted(fabsf(FAULT_FEEDBACK(current_q, FACTORY_SRAM_ADDRESS(0x1FFFF168UL, 0x1FFFF0F4UL))) >
                      FAULT_CONFIG(current_limit, 0x0CUL),
#endif
                  STATUS_COUNTER(overcurrent_ticks), 5000U,
                  STATUS_ALLOW_RESET)) {
        monitor->latched_fault = MOTOR_FAULT_OVERCURRENT;
#if defined(DAMIAO_DM4310)
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_OVERCURRENT;
        status->overcurrent_ticks = 5000U;
        status->fault_latched = 1U;
#endif
    }
#if defined(DAMIAO_DM4310)
    const float bus_voltage = FAULT_FEEDBACK(bus_voltage, FACTORY_SRAM_ADDRESS(0x1FFFF16CUL, 0x1FFFF0F8UL));
    const float undervoltage_limit = FAULT_CONFIG(bus_undervoltage, 0x00UL);
    uint32_t undervoltage;
    /* Preserve factory operand order/NZCV and BHI's unordered exclusion. */
    __asm volatile (
        "vcmpe.f32 %1, %2\nvmrs APSR_nzcv, FPSCR\n"
        "mov.w %0, #0\nit ls\nmovls %0, #1"
        : "=r" (undervoltage)
        : "t" (bus_voltage), "t" (undervoltage_limit) : "cc", "memory");
#define FAULT_BUS_VOLTAGE bus_voltage
#else
#define FAULT_BUS_VOLTAGE feedback->bus_voltage
#endif
#if defined(DAMIAO_DM4310)
    if (persisted(undervoltage != 0U,
#else
    if (persisted(FAULT_BUS_VOLTAGE <=
                      FAULT_CONFIG(bus_undervoltage, 0x00UL),
#endif
                  STATUS_COUNTER(undervoltage_ticks), 5000U,
                  STATUS_ALLOW_RESET)) {
        monitor->latched_fault = MOTOR_FAULT_BUS_UNDERVOLTAGE;
#if defined(DAMIAO_DM4310)
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_BUS_UNDERVOLTAGE;
        status->undervoltage_ticks = 5000U;
        status->fault_latched = 1U;
#endif
    }
#if defined(DAMIAO_DM4310)
    const float overvoltage_limit = FAULT_CONFIG(bus_overvoltage, 0x74UL);
    uint32_t overvoltage;
    bool overvoltage_tripped = false;
    __asm volatile (
        "vcmpe.f32 %1, %2\nvmrs APSR_nzcv, FPSCR\n"
        "mov.w %0, #0\nit gt\nmovgt %0, #1"
        : "=r" (overvoltage)
        : "t" (bus_voltage), "t" (overvoltage_limit) : "cc", "memory");
    if (persisted(overvoltage != 0U,
#else
    if (persisted(FAULT_BUS_VOLTAGE >
                      FAULT_CONFIG(bus_overvoltage, 0x74UL),
#endif
                  STATUS_COUNTER(overvoltage_ticks), 20000U,
                  STATUS_ALLOW_RESET)) {
        monitor->latched_fault = MOTOR_FAULT_BUS_OVERVOLTAGE;
#if defined(DAMIAO_DM4310)
        status->overvoltage_ticks = 20000U;
        status->fault_latched = 1U;
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_BUS_OVERVOLTAGE;
        overvoltage_tripped = true;
#endif
    }

#if defined(DAMIAO_DM4310)
    /* adc_foc_control_irq updates age after this monitor has potentially
     * changed the run state, matching 0x1fff8536..0x1fff85d6. */
    (void)motor_armed;
#else
    if (!motor_armed || monitor->latched_fault != MOTOR_FAULT_NONE) {
        STATUS_COMMUNICATION_TICKS = 0U;
    } else {
        ++STATUS_COMMUNICATION_TICKS;
    }
#endif
#undef STATUS_COUNTER
#undef STATUS_ALLOW_RESET
#undef STATUS_COMMUNICATION_TICKS
#undef FAULT_CONFIG
#undef FAULT_FEEDBACK
#undef FAULT_BUS_VOLTAGE
#undef MOTOR_TEMPERATURE_EXCEEDED
#if defined(DAMIAO_DM4310)
    /* 0x1fff8112 skips the fault reread on the overvoltage trip path. */
    const MotorFault fault = overvoltage_tripped ? MOTOR_FAULT_BUS_OVERVOLTAGE :
        (MotorFault)*((const volatile uint32_t *)(sample + 0x80U));
    if ((uint32_t)fault > 7U) {
        volatile uint32_t *const state =
            (volatile uint32_t *)(motor + 0x38U);
        if (state[0] == 2U) {
            /* 0x1fff812e publishes stopped state and change event here,
             * before returning to the enabled-state test in IRQ002. */
            state[0] = 0U;
            state[1] = 1U;
        }
    }
    monitor->latched_fault = fault;
    return fault;
#else
    return monitor->latched_fault;
#endif
}

#if defined(DAMIAO_DM4310)
void safety_finish_control_tick(bool motor_armed, volatile uint32_t *status)
{
    if (motor_armed) {
        ++status[0];
    } else {
        status[0] = 0U;
    }
}
#endif

void safety_note_control_frame(FaultMonitor *monitor)
{
    /* The original MCAN IRQ resets the age as soon as ID[7:0] addresses this
     * motor, before checking the command family/control mode.  The caller
     * represents a rejected family as FEEDBACK_ONLY, so it reaches here too.
     * An already-latched fault still prevents the reset. */
#if defined(DAMIAO_DM4310)
    (void)monitor;
    if (dm4310_runtime_status.fault_latched == 0U) {
        dm4310_runtime_status.communication_ticks = 0U;
    }
#else
    if (monitor->latched_fault == MOTOR_FAULT_NONE) {
        monitor->communication_ticks = 0U;
    }
#endif
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
#if defined(DAMIAO_DM4310)
    /* FB@0x1fff8950 clears the latch before communication age. */
    __asm volatile ("str %0, [%1, #4]\nstr %0, [%1]"
                    : : "r" (0U), "r" (&dm4310_runtime_status)
                    : "memory");
#endif
    monitor->communication_ticks = 0U;
    monitor->startup_fault = MOTOR_FAULT_NONE;
    monitor->latched_fault = MOTOR_FAULT_NONE;
}
