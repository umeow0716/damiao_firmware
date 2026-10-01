#include "app_commands.h"

#include "factory_layout.h"

#if defined(DAMIAO_DM4310)
#include "board_mcan.h"
#endif

#include "app_state.h"
#include "hc32f448.h"
#include "platform.h"

#if defined(DAMIAO_DM4310)
#define POST_MOTOR_STATE_CHANGE() motor_control_post_state_change()
#else
#define POST_MOTOR_STATE_CHANGE() (g_app.events.motor_state_changed = true)
#endif

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
#if defined(DAMIAO_DM4310)
        if ((uint32_t)motor_control_runtime_fault() < 2U) {
#else
        if ((uint32_t)g_app.motor.feedback.fault < 2U) {
#endif
            motor_control_arm(&g_app.motor);
            g_app.motor.feedback.fault = MOTOR_STATUS_ENABLED;
#if defined(DAMIAO_DM4310)
            motor_control_set_runtime_fault(MOTOR_STATUS_ENABLED);
#endif
            POST_MOTOR_STATE_CHANGE();
        }
        break;
    case CAN_COMMAND_DISABLE:
        motor_control_disarm(&g_app.motor);
#if defined(DAMIAO_DM4310)
        if ((uint32_t)motor_control_runtime_fault() == 1U) {
#else
        if (g_app.motor.feedback.fault == MOTOR_STATUS_ENABLED) {
#endif
            g_app.motor.feedback.fault = MOTOR_FAULT_NONE;
#if defined(DAMIAO_DM4310)
            motor_control_set_runtime_fault(MOTOR_FAULT_NONE);
#endif
        }
        POST_MOTOR_STATE_CHANGE();
        break;
    case CAN_COMMAND_SET_ZERO:
        /* FE zeroes both encoder models and persists their raw offsets. */
        platform_zero_current_position(&g_app.motor);
        g_app.position = 0.0f;
        g_app.motor_output_position = 0.0f;
        /* FE clears only position, not the other four command words. */
        g_app.motor.command.position = 0.0f;
#if defined(DAMIAO_DM4310)
        /* FE commits both offset words synchronously inside IRQ003.  The
         * factory masks interrupts around the relocated Flash writer and
         * resumes the same handler to emit feedback afterwards. */
        (void)platform_store_zero_position(&g_app.motor);
#else
        g_app.events.save_zero_position = true;
#endif
        break;
    case CAN_COMMAND_CLEAR_FAULT:
        safety_clear(&g_app.safety);
        g_app.motor.feedback.fault = MOTOR_FAULT_NONE;
#if defined(DAMIAO_DM4310)
        motor_control_set_runtime_fault(MOTOR_FAULT_NONE);
        /* Factory FB@0x1fff895c publishes the adjacent mode/event pair
         * with STRD after clearing latch, age and the fixed fault word. */
        __asm__ volatile (
            "strd %0, %1, [%2]"
            :
            : "r" (0U), "r" (1U),
              "r" ((uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff0c0), UINT32_C(0x1ffff04c)))
            : "memory");
        g_app.motor.armed = false;
#else
        motor_control_disarm(&g_app.motor);
        POST_MOTOR_STATE_CHANGE();
#endif
        break;
    case CAN_COMMAND_FEEDBACK_ONLY:
        break;
    case CAN_COMMAND_NONE:
    default:
        break;
    }
}

#if defined(DAMIAO_DM4310)
void dm4310_app_apply_can_command_irq(CanCommandKind kind,
    const MotorCommand *command,
    const Dm4310McanIrqReferences *references)
{
    (void)command;
    volatile uint32_t *const sample =
        (volatile uint32_t *)references->sample;
    volatile uint32_t *const motor =
        (volatile uint32_t *)references->motor;
    switch (kind) {
    case CAN_COMMAND_ENABLE:
        if (sample[0x80U / 4U] < 2U) {
            motor[0x38U / 4U] = 2U;
            sample[0x80U / 4U] = 1U;
            motor[0x3CU / 4U] = 1U;
        }
        break;
    case CAN_COMMAND_DISABLE:
        motor[0x38U / 4U] = 0U;
        if (sample[0x80U / 4U] == 1U) {
            sample[0x80U / 4U] = 0U;
        }
        motor[0x3CU / 4U] = 1U;
        break;
    case CAN_COMMAND_CLEAR_FAULT:
        references->status[1] = 0U;
        references->status[0] = 0U;
        sample[0x80U / 4U] = 0U;
        motor[0x38U / 4U] = 0U;
        motor[0x3CU / 4U] = 1U;
        /* FB rejoins the ordinary command-state gate before feedback. */
        (void)motor[0x38U / 4U];
        break;
    case CAN_COMMAND_FEEDBACK_ONLY:
        break;
    case CAN_COMMAND_SET_ZERO:
        platform_zero_current_position_irq(references);
        break;
    case CAN_COMMAND_SETPOINT:
        /* IRQ decoding has already published the selected factory words. */
        break;
    case CAN_COMMAND_NONE:
        break;
    }
}
#endif

void app_service_motor_state_change(void)
{
#if defined(DAMIAO_DM4310)
    uint32_t mode;
    if (!motor_control_take_state_change(&mode)) {
        return;
    }
#else
    if (!g_app.events.motor_state_changed) {
        return;
    }
    g_app.events.motor_state_changed = false;
#endif

#if defined(DAMIAO_DM4310)
    if (mode == 2U) {
#else
    if (g_app.motor.armed) {
#endif
        write_text("\n\r Entering Motor Mode \n\r");
        platform_set_status_led(PLATFORM_LED_GREEN);
        return;
    }

    /* main@0x2541e clears the five command floats when leaving motor mode;
     * CTRL_MODE remains selected for the next FC command. */
#if defined(DAMIAO_DM4310)
    dm4310_motor_control_clear_command(&g_app.motor);
#else
    g_app.motor.command.position = 0.0f;
    g_app.motor.command.velocity = 0.0f;
    g_app.motor.command.kp = 0.0f;
    g_app.motor.command.kd = 0.0f;
    g_app.motor.command.torque = 0.0f;
#endif
#if defined(DAMIAO_DM4310)
    write_text("\n\r");
    write_text(" Commands:\n\r");
    /* main 0x25442/4e/5a/66 calls the microsecond helper at 0x21f60. */
    platform_commissioning_delay_us(10U);
    write_text(" m - Motor Mode\n\r");
    platform_commissioning_delay_us(10U);
    write_text(" s - Setup Mode\n\r");
    platform_commissioning_delay_us(10U);
    write_text(" esc - Exit to Menu\n\r");
    platform_commissioning_delay_us(10U);
#else
    write_text("\n\r"
               " Commands:\n\r"
               " m - Motor Mode\n\r"
               " s - Setup Mode\n\r"
               " esc - Exit to Menu\n\r");
#endif
    platform_set_status_led(PLATFORM_LED_RED);
}

void app_service_control_status_tick(void)
{
    if (!APP_DEFERRED_EVENTS.control_status_tick) {
        return;
    }
#if !defined(DAMIAO_DM4310)
    APP_DEFERRED_EVENTS.control_status_tick = false;
#endif

    /* main@0x254a0 consumes the 1 kHz IRQ flag even when no fault is active.
     * Its counter is retained across healthy periods and trips after the
     * incremented value exceeds 250. */
#if defined(DAMIAO_DM4310)
    const uint32_t fault = *((const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFFF184UL, 0x1FFFF110UL));
    if (fault <= MOTOR_STATUS_ENABLED) {
        APP_DEFERRED_EVENTS.control_status_tick = 0U;
#else
    if ((uint32_t)g_app.motor.feedback.fault <= MOTOR_STATUS_ENABLED) {
#endif
        return;
    }
#if defined(DAMIAO_DM4310)
    const uint32_t indicator_ticks = APP_FAULT_INDICATOR_TICKS + 1U;
    APP_FAULT_INDICATOR_TICKS = indicator_ticks;
    if (indicator_ticks > 250U) {
#else
    ++APP_FAULT_INDICATOR_TICKS;
    if (APP_FAULT_INDICATOR_TICKS > 250U) {
#endif
        APP_FAULT_INDICATOR_TICKS = 0U;
        platform_toggle_fault_indicator();
    }
#if defined(DAMIAO_DM4310)
    APP_DEFERRED_EVENTS.control_status_tick = 0U;
#endif
}
