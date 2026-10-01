/* Isolated host test for the recovered DM4310 fault monitor.
 * Compile manually; this file is intentionally absent from CMake/make. */

#include <assert.h>
#include <math.h>
#include <string.h>

#include "safety.h"

static MotorConfig safe_config(void)
{
    MotorConfig config;
    memset(&config, 0, sizeof(config));
    config.communication_timeout = 10U;
    config.motor_temperature_limit = 100.0f;
    config.current_limit = 20.0f;
    config.bus_undervoltage = 12.0f;
    config.bus_overvoltage = 32.0f;
    return config;
}

static MotorFeedback safe_feedback(void)
{
    MotorFeedback feedback;
    memset(&feedback, 0, sizeof(feedback));
    feedback.bus_voltage = 24.0f;
    feedback.motor_temperature = 25.0f;
    feedback.mos_temperature = 25.0f;
    return feedback;
}

int main(void)
{
    FaultMonitor monitor;
    MotorConfig config = safe_config();
    MotorFeedback feedback = safe_feedback();

    safety_init(&monitor);
    safety_set_startup_fault(&monitor,
                             MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING);
    assert(monitor.latched_fault == MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING);
    assert(dm4310_runtime_status.fault_latched == 0U);

    /* The factory monitor checks entering age even when the motor is no
     * longer enabled; final-state age reset happens separately in the IRQ. */
    dm4310_runtime_status.communication_ticks = 11U;
    assert(safety_update(&monitor, &config, &feedback, false) ==
           MOTOR_FAULT_COMMUNICATION_LOST);
    assert(dm4310_runtime_status.communication_ticks == 10U);
    assert(dm4310_runtime_status.fault_latched == 1U);

    safety_clear(&monitor);
    safety_finish_control_tick(true);
    assert(dm4310_runtime_status.communication_ticks == 1U);
    safety_finish_control_tick(false);
    assert(dm4310_runtime_status.communication_ticks == 0U);

    /* Ordinary VFP comparisons are unordered for NaN, so none of these
     * analogue checks trips. MOS temperature is a signed raw-word compare;
     * the usual quiet NaN encoding is positive and therefore does trip only
     * after the recovered 8,001-sample persistence interval. */
    feedback.motor_temperature = NAN;
    feedback.current_q = NAN;
    feedback.bus_voltage = NAN;
    feedback.mos_temperature = 25.0f;
    assert(safety_update(&monitor, &config, &feedback, false) ==
           MOTOR_FAULT_NONE);

    feedback = safe_feedback();
    feedback.mos_temperature = NAN;
    for (unsigned int tick = 0U; tick < 8000U; ++tick) {
        assert(safety_update(&monitor, &config, &feedback, false) ==
               MOTOR_FAULT_NONE);
    }
    assert(safety_update(&monitor, &config, &feedback, false) ==
           MOTOR_FAULT_MOS_OVERTEMPERATURE);
    assert(dm4310_runtime_status.mos_overtemperature_ticks == 8000U);

    return 0;
}
