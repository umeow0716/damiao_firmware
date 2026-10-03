/* Isolated host test for the recovered IRQ002 cross-module call order.
 * It is intentionally absent from CMake and the normal make graph. */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_state.h"
#include "interrupts.h"
#include "platform.h"

AppState g_app;
FactoryRuntimeStatus dm4310_runtime_status;

static char trace[32];
static size_t trace_length;
static MotorFault injected_fault;

static void note(char event)
{
    trace[trace_length++] = event;
}

bool platform_read_adc(AdcSample *sample)
{
    memset(sample, 0, sizeof(*sample));
    note('R');
    return true;
}

void dm4310_motor_control_fast_prepare(MotorController *controller,
                                       const MotorConfig *config,
                                       const AdcSample *sample)
{
    (void)controller;
    (void)config;
    (void)sample;
    note('P');
}

void dm4310_motor_control_fast_sample_prefix(MotorController *controller,
                                             AdcSample *sample)
{
    (void)controller;
    (void)sample;
    note('C');
}

void platform_finish_adc_sensor_sample(AdcSample *sample)
{
    (void)sample;
    note('O');
}

void dm4310_motor_control_fast_transform(MotorController *controller,
                                         const MotorConfig *config,
                                         AdcSample *sample)
{
    (void)controller;
    (void)config;
    (void)sample;
    note('K');
}

void platform_finish_adc_velocity_sample(AdcSample *sample)
{
    (void)sample;
    note('D');
}

void dm4310_fault_monitor_helper(void)
{
    note('F');
    g_app.safety.latched_fault = injected_fault;
}

void motor_control_set_runtime_fault(MotorFault fault)
{
    assert(fault == injected_fault);
    note('L');
}

void motor_control_trip(MotorController *controller)
{
    note('T');
    controller->armed = false;
}

void motor_control_post_state_change(void)
{
    note('E');
}

void dm4310_motor_control_fast_apply_state(MotorController *controller)
{
    note(controller->armed ? 'V' : 'X');
}

PhaseDuty dm4310_motor_control_fast_finish(MotorController *controller)
{
    (void)controller;
    note('Q');
    return (PhaseDuty){.a = 0.5f, .b = 0.5f, .c = 0.5f};
}

void safety_finish_control_tick(bool motor_armed)
{
    note(motor_armed ? 'S' : 's');
}

void platform_write_pwm(PhaseDuty duty)
{
    assert(duty.a == 0.5f);
    note('W');
}

void platform_ack_adc_irq(void)
{
    note('A');
}

static void run_case(MotorFault fault, const char *expected)
{
    memset(&g_app, 0, sizeof(g_app));
    memset(trace, 0, sizeof(trace));
    trace_length = 0U;
    injected_fault = fault;
    g_app.motor.armed = true;
    adc_foc_control_irq();
    assert(strcmp(trace, expected) == 0);
}

int main(void)
{
    run_case(MOTOR_FAULT_NONE, "RCOKDPFVSQWA");
    run_case(MOTOR_FAULT_BUS_OVERVOLTAGE, "RCOKDPFLTEXsQWA");
    return 0;
}
