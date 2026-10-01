/* Isolated host test for the three distinct recovered helper clamp shapes.
 * It is intentionally absent from CMake and the normal make graph. */

#include <assert.h>
#include <math.h>
#include <string.h>

#include "commissioning.h"
#include "motor_control.h"

float motor_clampf(float value, float minimum, float maximum)
{
    if (value > maximum) {
        return maximum;
    }
    if (value < minimum) {
        return minimum;
    }
    return value;
}

int main(void)
{
    CommissioningIdentificationFilter filter;
    memset(&filter, 0, sizeof(filter));
    filter.input = NAN;
    filter.input_gain = 1.0f;
    filter.output_min = -0.25f;
    filter.output_max = 0.25f;
    commissioning_identification_filter_step(&filter);
    assert(filter.limited_output == -0.25f);
    assert(isnan(filter.saturation_error));

    MotionObserver observer;
    memset(&observer, 0, sizeof(observer));
    observer.measured_velocity = NAN;
    observer.sample_period = 0.001f;
    observer.observer_bandwidth = 1000.0f;
    observer.observer_state_gain = 600.0f;
    observer.plant_gain = 1.0f;
    observer.inverse_plant_gain = 1.0f;
    observer.output_min = -1.0f;
    observer.output_max = 1.0f;
    dm4310_motion_observer_state_step(&observer);
    assert(isnan(observer.disturbance_current));

    CurrentController controller;
    memset(&controller, 0, sizeof(controller));
    controller.measurement = NAN;
    controller.sample_period = 0.00005f;
    controller.control_bandwidth = 1.0f;
    controller.inverse_plant_gain = 1.0f;
    controller.output_min = -1.0f;
    controller.output_max = 1.0f;
    assert(isnan(dm4310_current_controller_state_step(&controller)));
    assert(isnan(controller.control_output));
    assert(isnan(controller.limited_output));
    return 0;
}
