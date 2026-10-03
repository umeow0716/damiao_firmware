#include "safety.h"

#include "memory_layout.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

RuntimeStatus runtime_status __attribute__((section(".runtime_status")));
SRAM_ABI_ASSERT_SIZE(RuntimeStatus, 0x4CU);
SRAM_ABI_ASSERT_OFFSET(RuntimeStatus, save_staged_parameters, 0x0CU);
SRAM_ABI_ASSERT_OFFSET(RuntimeStatus, calibration_commit_request, 0x1CU);
SRAM_ABI_ASSERT_OFFSET(RuntimeStatus, firmware_control_request, 0x28U);
SRAM_ABI_ASSERT_OFFSET(RuntimeStatus, undervoltage_ticks, 0x34U);
SRAM_ABI_ASSERT_OFFSET(RuntimeStatus, fault_indicator_ticks, 0x48U);

static bool persisted(bool condition, volatile uint32_t *counter, uint32_t threshold,
                      volatile const uint32_t *fault_latched)
{
    if (!condition)
    {
        if (*fault_latched == 0U)
        {
            *counter = 0U;
        }
        return false;
    }
    const uint32_t count = *counter + 1U;
    *counter = count;
    if (count > threshold)
    {
        /* The fixed-layout monitor writes the threshold back when it trips:
         * 8000/5000/20000, after the 8001st/5001st/20001st bad sample. */
        return true;
    }
    return false;
}

void safety_init(FaultMonitor *monitor)
{
    memset(monitor, 0, sizeof(*monitor));
    /* Fixed firmware status/events are Reset-owned BSS, not decoded state.
     * Initializing a monitor must not erase live firmware counters/events. */
}

MotorFault safety_update(FaultMonitor *monitor, const MotorConfig *config,
                         const MotorFeedback *feedback, bool motor_armed)
{
    volatile RuntimeStatus *const status = (volatile RuntimeStatus *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF83F8UL, 0x1FFF970CUL);
    const uintptr_t configuration =
        *(const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF83FCUL, 0x1FFF9710UL);
    const uintptr_t sample =
        *(const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8400UL, 0x1FFF9714UL);
#define STATUS_COUNTER(field) (&status->field)
#define STATUS_ALLOW_RESET (&status->fault_latched)
#define STATUS_COMMUNICATION_TICKS status->communication_ticks
#define FAULT_CONFIG(field, offset) (*((const volatile float *)(configuration + (offset))))
#define FAULT_FEEDBACK(field, address)                                                             \
    (*((const volatile float *)(sample +                                                           \
                                ((address) - MEMORY_LAYOUT_ADDRESS(0x1FFFF104UL, 0x1FFFF090UL)))))
    (void)config;
    (void)feedback;
    /* Check CAN age before the analogue limits.  The age is advanced once per 50 us ADC
     * control tick only while the motor state is enabled.  A non-zero fault
     * flag prevents healthy inputs from clearing counters, but the reference
     * still evaluates every bad input and lets a later persistent fault
     * replace the diagnostic code.  "Latched" therefore means that the
     * controller cannot return to no-fault without Clear, not first-fault
     * wins forever. */
    const uint32_t communication_ticks = STATUS_COMMUNICATION_TICKS;
    const uint32_t communication_timeout = *((const volatile uint32_t *)(configuration + 0x24U));
    if (communication_ticks > communication_timeout && communication_timeout != 0U)
    {
        monitor->latched_fault = MOTOR_FAULT_COMMUNICATION_LOST;
        status->communication_ticks = communication_timeout;
        status->fault_latched = 1U;
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_COMMUNICATION_LOST;
    }
    const uintptr_t motor =
        *(const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8404UL, 0x1FFF9718UL);
    const float motor_temperature_limit = FAULT_CONFIG(motor_temperature_limit, 0x08UL);
    const float motor_temperature = *((const volatile float *)(motor + 0x40U));
    uint32_t motor_temperature_exceeded;
    __asm volatile("vcmpe.f32 %1, %2\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "mov.w %0, #0\n"
                   "it gt\n"
                   "movgt %0, #1"
                   : "=r"(motor_temperature_exceeded)
                   : "t"(motor_temperature), "t"(motor_temperature_limit)
                   : "cc");
#define MOTOR_TEMPERATURE_EXCEEDED (motor_temperature_exceeded != 0U)
    if (persisted(MOTOR_TEMPERATURE_EXCEEDED, STATUS_COUNTER(motor_overtemperature_ticks), 8000U,
                  STATUS_ALLOW_RESET))
    {
        monitor->latched_fault = MOTOR_FAULT_MOTOR_OVERTEMPERATURE;
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_MOTOR_OVERTEMPERATURE;
        status->motor_overtemperature_ticks = 8000U;
        status->fault_latched = 1U;
    }
    const int32_t mos_limit =
        *(const volatile int32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8408UL, 0x1FFF971CUL);
    const int32_t mos_temperature = *(const volatile int32_t *)(sample + 0x84U);
    if (persisted(mos_temperature > mos_limit, STATUS_COUNTER(mos_overtemperature_ticks), 8000U,
                  STATUS_ALLOW_RESET))
    {
        monitor->latched_fault = MOTOR_FAULT_MOS_OVERTEMPERATURE;
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_MOS_OVERTEMPERATURE;
        status->mos_overtemperature_ticks = 8000U;
        status->fault_latched = 1U;
    }
    const float current =
        FAULT_FEEDBACK(current_q, MEMORY_LAYOUT_ADDRESS(0x1FFFF168UL, 0x1FFFF0F4UL));
    const float current_limit = FAULT_CONFIG(current_limit, 0x0CUL);
    float absolute_current;
    uint32_t overcurrent;
    __asm volatile("vabs.f32 %0, %2\nvcmpe.f32 %0, %3\n"
                   "vmrs APSR_nzcv, FPSCR\nmov.w %1, #0\n"
                   "it gt\nmovgt %1, #1"
                   : "=&t"(absolute_current), "=r"(overcurrent)
                   : "t"(current), "t"(current_limit)
                   : "cc", "memory");
    if (persisted(overcurrent != 0U, STATUS_COUNTER(overcurrent_ticks), 5000U, STATUS_ALLOW_RESET))
    {
        monitor->latched_fault = MOTOR_FAULT_OVERCURRENT;
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_OVERCURRENT;
        status->overcurrent_ticks = 5000U;
        status->fault_latched = 1U;
    }
    const float bus_voltage =
        FAULT_FEEDBACK(bus_voltage, MEMORY_LAYOUT_ADDRESS(0x1FFFF16CUL, 0x1FFFF0F8UL));
    const float undervoltage_limit = FAULT_CONFIG(bus_undervoltage, 0x00UL);
    uint32_t undervoltage;
    /* Preserve firmware operand order/NZCV and BHI's unordered exclusion. */
    __asm volatile("vcmpe.f32 %1, %2\nvmrs APSR_nzcv, FPSCR\n"
                   "mov.w %0, #0\nit ls\nmovls %0, #1"
                   : "=r"(undervoltage)
                   : "t"(bus_voltage), "t"(undervoltage_limit)
                   : "cc", "memory");
#define FAULT_BUS_VOLTAGE bus_voltage
    if (persisted(undervoltage != 0U, STATUS_COUNTER(undervoltage_ticks), 5000U,
                  STATUS_ALLOW_RESET))
    {
        monitor->latched_fault = MOTOR_FAULT_BUS_UNDERVOLTAGE;
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_BUS_UNDERVOLTAGE;
        status->undervoltage_ticks = 5000U;
        status->fault_latched = 1U;
    }
    const float overvoltage_limit = FAULT_CONFIG(bus_overvoltage, 0x74UL);
    uint32_t overvoltage;
    bool overvoltage_tripped = false;
    __asm volatile("vcmpe.f32 %1, %2\nvmrs APSR_nzcv, FPSCR\n"
                   "mov.w %0, #0\nit gt\nmovgt %0, #1"
                   : "=r"(overvoltage)
                   : "t"(bus_voltage), "t"(overvoltage_limit)
                   : "cc", "memory");
    if (persisted(overvoltage != 0U, STATUS_COUNTER(overvoltage_ticks), 20000U, STATUS_ALLOW_RESET))
    {
        monitor->latched_fault = MOTOR_FAULT_BUS_OVERVOLTAGE;
        status->overvoltage_ticks = 20000U;
        status->fault_latched = 1U;
        *((volatile uint32_t *)(sample + 0x80U)) = MOTOR_FAULT_BUS_OVERVOLTAGE;
        overvoltage_tripped = true;
    }

    /* The control IRQ updates age after this monitor has potentially changed
     * the run state. */
    (void)motor_armed;
#undef STATUS_COUNTER
#undef STATUS_ALLOW_RESET
#undef STATUS_COMMUNICATION_TICKS
#undef FAULT_CONFIG
#undef FAULT_FEEDBACK
#undef FAULT_BUS_VOLTAGE
#undef MOTOR_TEMPERATURE_EXCEEDED
    /* The overvoltage trip path already owns the final fault value. */
    const MotorFault fault = overvoltage_tripped
                                 ? MOTOR_FAULT_BUS_OVERVOLTAGE
                                 : (MotorFault) * ((const volatile uint32_t *)(sample + 0x80U));
    if ((uint32_t)fault > 7U)
    {
        volatile uint32_t *const state = (volatile uint32_t *)(motor + 0x38U);
        if (state[0] == 2U)
        {
            /* Publish stopped state and the change event before returning to
             * the enabled-state test in the control IRQ. */
            state[0] = 0U;
            state[1] = 1U;
        }
    }
    monitor->latched_fault = fault;
    return fault;
}

void safety_finish_control_tick(bool motor_armed, volatile uint32_t *status)
{
    if (motor_armed)
    {
        ++status[0];
    }
    else
    {
        status[0] = 0U;
    }
}

void safety_note_control_frame(FaultMonitor *monitor)
{
    /* Reset communication age as soon as ID[7:0] addresses this
     * motor, before checking the command family/control mode.  The caller
     * represents a rejected family as FEEDBACK_ONLY, so it reaches here too.
     * An already-latched fault still prevents the reset. */
    (void)monitor;
    if (runtime_status.fault_latched == 0U)
    {
        runtime_status.communication_ticks = 0U;
    }
}

void safety_latch_runtime_fault(FaultMonitor *monitor, MotorFault fault)
{
    if ((monitor->latched_fault == MOTOR_FAULT_NONE) && (fault != MOTOR_FAULT_NONE))
    {
        monitor->latched_fault = fault;
    }
}

void safety_set_startup_fault(FaultMonitor *monitor, MotorFault fault)
{
    monitor->startup_fault = fault;
    if ((fault != MOTOR_FAULT_NONE) || (monitor->latched_fault == monitor->startup_fault))
    {
        monitor->latched_fault = fault;
    }
}

void safety_clear(FaultMonitor *monitor)
{
    /* FB clears the communication age/latch and the reported fault, but the
     * analogue debounce counters remain intact.  A condition
     * which is still bad can therefore trip again on the next control tick. */
    /* FB clears the latch before communication age. */
    __asm volatile("str %0, [%1, #4]\nstr %0, [%1]" : : "r"(0U), "r"(&runtime_status) : "memory");
    monitor->communication_ticks = 0U;
    monitor->startup_fault = MOTOR_FAULT_NONE;
    monitor->latched_fault = MOTOR_FAULT_NONE;
}
