#include "app_state.h"
#include "app_commands.h"
#include "app_profile.h"
#include <string.h>

#include "commissioning.h"
#include "calibration_upload.h"
#include "debug_console.h"
#include "firmware_control.h"
#include "hc32f448.h"
#include "platform.h"

#define APP_VECTOR_TABLE_ADDRESS (0x00020000UL)

static void service_deferred_events(void)
{
    app_service_motor_state_change();
    app_service_control_status_tick();
    firmware_control_service();
    const uint8_t can_error = g_app.events.can_error;
    if (can_error != 0U) {
        g_app.events.can_error = 0U;
        if (can_error == 1U) {
            debug_console_printf("CAN Error 1\n\r");
        } else {
            debug_console_printf("CAN Error 2\n\r");
        }
    }
    const CalibrationUploadKind calibration_request =
        (CalibrationUploadKind)g_app.events.calibration_commit_request;

    if (calibration_request != CALIBRATION_UPLOAD_NONE) {
        __disable_irq();

        if (calibration_request == CALIBRATION_UPLOAD_MOTOR_ENCODER) {
            platform_store_motor_encoder_calibration(calibration_upload_motor_record());
        } else if (calibration_request ==
                CALIBRATION_UPLOAD_OUTPUT_SENSOR) {
            float calibration[4];

            if (platform_commissioning_get_output_calibration(calibration)) {
                platform_store_output_sensor_calibration(
                    calibration_upload_output_table(), calibration);
            }
        }

        g_app.events.calibration_commit_request = CALIBRATION_UPLOAD_NONE;
        platform_load_motor_calibration(&g_app.motor);

        __enable_irq();

        debug_console_return_to_menu();
    }
    if (g_app.events.commission_direction) {
        commissioning_run_direction_and_alignment(&g_app.config);
        g_app.events.commission_direction = false;
#if defined(DAMIAO_DM8009)
        debug_console_printf("E_OFF = %f\r\n",
                             (double)g_app.motor.electrical_offset);
#endif
        debug_console_return_to_menu();
    }
    if (g_app.events.commission_position_sensor) {
        commissioning_run_output_sensor_calibration();
        g_app.events.commission_position_sensor = false;
    }
    if (g_app.events.identify_motor) {
        commissioning_run_motor_identification(&g_app.config);
        g_app.events.identify_motor = false;
        debug_console_return_to_menu();
    }
    if (g_app.events.print_menu) {
        g_app.events.print_menu = false;
        debug_console_print_banner();
    }
    if (g_app.events.print_debug_info) {
        g_app.events.print_debug_info = false;
        debug_console_print_status();
    }
    if (g_app.events.save_parameters) {
        __disable_irq();

        platform_store_parameters(&g_app.config);
        g_app.events.save_parameters = false;

        __enable_irq();

        if (g_app.pending_store_response_valid) {
            platform_mcan_send(&g_app.pending_store_response);
        }

        g_app.pending_store_response_valid = false;
    }
    if (g_app.events.save_zero_position) {
        g_app.events.save_zero_position = false;
        if (!platform_store_zero_position(&g_app.motor)) {
            safety_set_startup_fault(&g_app.safety,
                                     MOTOR_FAULT_OUTPUT_CALIBRATION);
            g_app.motor.feedback.fault = MOTOR_FAULT_OUTPUT_CALIBRATION;
            motor_control_disarm(&g_app.motor);
            platform_set_status_led(PLATFORM_LED_RED);
        }
    }
    if (g_app.events.reconfigure_mcan) {
        g_app.events.reconfigure_mcan = false;
        platform_reconfigure_mcan();
    }
}

int main(void)
{
    /* The official main@0x252f4 switches VTOR from the SystemInit value 0 to
     * the APP table before it touches the clock tree or persistent state. */
    SCB->VTOR = APP_VECTOR_TABLE_ADDRESS;
    platform_early_init();
    /* main@0x252f4 performs this write immediately after its clock routine.
     * Keep the loader handoff path free of application setup, GPIO writes,
     * peripheral access and delays: after a successful update the installed
     * loader clears the record to (0, 0), and only this write changes it to
     * the normal (0, 1) state. */
    platform_confirm_application_boot();
    /* store_application_identity follows boot confirmation and normalizes
     * boot-record word 3 to the target-specific APP identity. */
    platform_update_application_identity();
    __enable_irq();
    app_state_init();
    debug_console_reset();
    platform_prepare_board_startup();
    platform_initialize_peripherals();
    if (!platform_load_parameters(&g_app.config)) {
        __disable_irq();
        platform_store_factory_parameters();
        __enable_irq();

        platform_load_parameters(&g_app.config);
    }
    /* The reference keeps the selected control mode in the zeroed command
     * object from startup; FC enables that mode even before the first
     * setpoint frame.  Parameter writes already synchronize this field. */
    memset(&g_app.motor.command, 0, sizeof(g_app.motor.command));
    g_app.motor.command.mode = g_app.config.control_mode;
    platform_initialize_runtime();
    /* main@0x252f4 prints this block once after sensor/ADC and MCAN setup. */
    debug_console_print_status();
    g_app.events.print_menu = true;
    platform_check_startup_bus_voltage();
    platform_start_control_loop();

    for (;;) {
        service_deferred_events();
        platform_idle();
    }
}
