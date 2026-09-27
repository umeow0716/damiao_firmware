#include "app_commands.h"

#include "app_state.h"
#include "platform.h"

static void write_text(const char *text)
{
    size_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    platform_debug_write(text, length);
}

void app_apply_can_command(CanCommandKind kind, const MotorCommand *command)
{
    switch (kind) {
    case CAN_COMMAND_SETPOINT:
        if (command != NULL) {
            motor_control_set_command(&g_app.motor, command);
        }
        break;
    case CAN_COMMAND_ENABLE:
        /* The original accepts FC only while the feedback fault code is 0
         * or 1.  GPIO/console changes are deferred to the main loop through
         * the same state-change flag used by FD and FB. */
        if ((uint32_t)g_app.motor.feedback.fault < 2U) {
            motor_control_arm(&g_app.motor);
            g_app.motor.feedback.fault = MOTOR_STATUS_ENABLED;
            g_app.events.motor_state_changed = true;
        }
        break;
    case CAN_COMMAND_DISABLE:
        motor_control_disarm(&g_app.motor);
        if (g_app.motor.feedback.fault == MOTOR_STATUS_ENABLED) {
            g_app.motor.feedback.fault = MOTOR_FAULT_NONE;
        }
        g_app.events.motor_state_changed = true;
        break;
    case CAN_COMMAND_SET_ZERO:
        /* Both independent encoders are zeroed by the original FE command.
         * The displayed positions already include the previous offsets, so
         * add the current residual rather than replacing the raw offset. */
        g_app.motor.output_position_offset += g_app.position;
        g_app.motor.motor_output_position_offset +=
            g_app.motor_output_position;
        platform_set_position_zero_offsets(
            g_app.motor.output_position_offset,
            g_app.motor.motor_output_position_offset);
        g_app.position = 0.0f;
        g_app.motor_output_position = 0.0f;
        g_app.motor.command.position = 0.0f;
        g_app.events.save_zero_position = true;
        break;
    case CAN_COMMAND_CLEAR_FAULT:
        safety_clear(&g_app.safety);
        g_app.motor.feedback.fault = MOTOR_FAULT_NONE;
        motor_control_disarm(&g_app.motor);
        g_app.events.motor_state_changed = true;
        break;
    case CAN_COMMAND_FEEDBACK_ONLY:
        break;
    case CAN_COMMAND_NONE:
    default:
        break;
    }
}

void app_service_motor_state_change(void)
{
    if (!g_app.events.motor_state_changed) {
        return;
    }
    g_app.events.motor_state_changed = false;

    if (g_app.motor.armed) {
        write_text("\n\r Entering Motor Mode \n\r");
        platform_set_status_led(PLATFORM_LED_GREEN);
        return;
    }

    /* main@0x2541e clears the five command floats when leaving motor mode;
     * CTRL_MODE remains selected for the next FC command. */
    g_app.motor.command.position = 0.0f;
    g_app.motor.command.velocity = 0.0f;
    g_app.motor.command.kp = 0.0f;
    g_app.motor.command.kd = 0.0f;
    g_app.motor.command.torque = 0.0f;
    write_text("\n\r"
               " Commands:\n\r"
               " m - Motor Mode\n\r"
               " s - Setup Mode\n\r"
               " esc - Exit to Menu\n\r");
    platform_set_status_led(PLATFORM_LED_RED);
}

void app_service_control_status_tick(void)
{
    if (!g_app.events.control_status_tick) {
        return;
    }
    g_app.events.control_status_tick = false;

    /* main@0x254a0 consumes the 1 kHz IRQ flag even when no fault is active.
     * Its counter is retained across healthy periods and trips after the
     * incremented value exceeds 250. */
    if ((uint32_t)g_app.motor.feedback.fault <= MOTOR_STATUS_ENABLED) {
        return;
    }
    ++g_app.fault_indicator_ticks;
    if (g_app.fault_indicator_ticks > 250U) {
        g_app.fault_indicator_ticks = 0U;
        platform_toggle_fault_indicator();
    }
}
