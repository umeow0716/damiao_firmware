/* Isolated host test for retained velocity-target behavior in IRQ002.
 * It includes the implementation so the fixed-SRAM model remains private
 * production state and the test stays outside the normal build graph. */

#include <assert.h>
#include <string.h>

#define DAMIAO_DM4310 1
#include "../../app/src/motor_control.c"

void dm4310_motion_observer_helper(MotionObserver *observer)
{
    (void)observer;
}

int main(void)
{
    MotorController controller;
    MotorConfig config;
    memset(&controller, 0, sizeof(controller));
    memset(&config, 0, sizeof(config));

    config.acceleration_limit = 0.25f;
    config.deceleration_limit = -0.5f;
    config.torque_constant = 1.0f;
    controller.current_filter_previous = 1.0f;
    controller.outer_loop_divider = 20U;
    controller.desired_velocity = 1.0f;
    controller.command.mode = MOTOR_MODE_MIT;
    dm4310_sample_runtime_state.velocity_target = 2.0f;

    assert(control_outer_step(&controller, &config, 0.0f));
    assert(controller.desired_velocity == 1.25f);
    assert(dm4310_sample_runtime_state.velocity_target == 2.0f);

    controller.outer_loop_divider = 20U;
    controller.command.mode = (MotorControlMode)99;
    assert(control_outer_step(&controller, &config, 0.0f));
    assert(controller.desired_velocity == 1.5f);
    assert(dm4310_sample_runtime_state.velocity_target == 2.0f);

    controller.outer_loop_divider = 20U;
    controller.command.mode = MOTOR_MODE_SPEED;
    controller.command.velocity = -4.0f;
    config.speed_limit = 3.0f;
    assert(control_outer_step(&controller, &config, 0.0f));
    assert(dm4310_sample_runtime_state.velocity_target == -3.0f);
    assert(controller.desired_velocity == 1.0f);
    return 0;
}
