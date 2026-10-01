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
#include "position_sensor.h"

#define APP_VECTOR_TABLE_ADDRESS (0x00020000UL)

static void service_deferred_events(void)
{
    app_service_motor_state_change();
    app_service_control_status_tick();
#if !defined(DAMIAO_DM4310)
    firmware_control_service();
#endif
#if defined(DAMIAO_DM4310)
    const uint32_t can_error = APP_DEFERRED_EVENTS.can_error;
    if (can_error == 1U) {
        debug_console_printf("CAN Error 1\n\r");
        APP_DEFERRED_EVENTS.can_error = 0U;
    } else if (can_error == 2U) {
        debug_console_printf("CAN Error 2\n\r");
        APP_DEFERRED_EVENTS.can_error = 0U;
    }
#else
    const uint8_t can_error = (uint8_t)APP_DEFERRED_EVENTS.can_error;
    if (can_error != 0U) {
        APP_DEFERRED_EVENTS.can_error = 0U;
        if (can_error == 1U) {
            debug_console_printf("CAN Error 1\n\r");
        } else {
            debug_console_printf("CAN Error 2\n\r");
        }
    }
#endif
#if defined(DAMIAO_DM4310)
    if (APP_DEFERRED_EVENTS.save_staged_parameters) {
        __disable_irq();
        platform_store_staged_parameters();
        APP_DEFERRED_EVENTS.save_staged_parameters = false;
        platform_finish_flash_commit();
        __enable_irq();
    }
    if (APP_DEFERRED_EVENTS.reserved_08 != 0U) {
        APP_DEFERRED_EVENTS.reserved_08 = 0U;
    }
    if (APP_DEFERRED_EVENTS.commission_direction) {
        commissioning_run_direction_and_alignment(&g_app.config);
        APP_DEFERRED_EVENTS.commission_direction = false;
        debug_console_return_to_menu();
    }
#endif
#if defined(DAMIAO_DM4310)
    const uint32_t calibration_request =
        APP_DEFERRED_EVENTS.calibration_commit_request;
#else
    const CalibrationUploadKind calibration_request =
        (CalibrationUploadKind)APP_DEFERRED_EVENTS.calibration_commit_request;
#endif

    if (calibration_request != CALIBRATION_UPLOAD_NONE) {
        __disable_irq();

        if (calibration_request == CALIBRATION_UPLOAD_MOTOR_ENCODER) {
            platform_store_motor_encoder_calibration(calibration_upload_motor_record());
        }
#if defined(DAMIAO_DM4310)
        if (APP_DEFERRED_EVENTS.calibration_commit_request ==
#else
        else if (calibration_request ==
#endif
                CALIBRATION_UPLOAD_OUTPUT_SENSOR) {
#if defined(DAMIAO_DM4310)
            /* Factory captures parameter words only after the table writer
             * returns, directly from their fixed calibration source. */
            platform_store_output_sensor_calibration(
                calibration_upload_output_table(),
                (const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff078), UINT32_C(0x1ffff004)));
#else
            float calibration[4];

            if (platform_commissioning_get_output_calibration(calibration)) {
                platform_store_output_sensor_calibration(
                    calibration_upload_output_table(), calibration);
            }
#endif
        }

        APP_DEFERRED_EVENTS.calibration_commit_request =
            CALIBRATION_UPLOAD_NONE;
        platform_load_motor_calibration(&g_app.motor);
#if defined(DAMIAO_DM4310)
        position_sensor_reset_accumulated_delta();
        platform_finish_flash_commit();
#endif

        __enable_irq();

        debug_console_return_to_menu();
    }
    if (APP_DEFERRED_EVENTS.identify_motor) {
        commissioning_run_motor_identification(&g_app.config);
        APP_DEFERRED_EVENTS.identify_motor = false;
        debug_console_return_to_menu();
    }
#if defined(DAMIAO_DM4310)
    const uint32_t firmware_request =
        APP_DEFERRED_EVENTS.firmware_control_request;
    if (firmware_request != 0U) {
        firmware_control_service((uint8_t)firmware_request);
        APP_DEFERRED_EVENTS.firmware_control_request = 0U;
        debug_console_return_to_menu();
    }
    if (APP_DEFERRED_EVENTS.commission_position_sensor) {
        commissioning_run_output_sensor_calibration();
        APP_DEFERRED_EVENTS.commission_position_sensor = false;
    }
#endif
#if !defined(DAMIAO_DM4310)
    /* DM factory main uses the fixed mode/event pair for its menu; its
     * device-information diagnostic is only called during startup. */
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
#endif
#if !defined(DAMIAO_DM4310)
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
#endif
#if !defined(DAMIAO_DM4310)
    if (g_app.events.reconfigure_mcan) {
        g_app.events.reconfigure_mcan = false;
        platform_reconfigure_mcan();
    }
#endif
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
#if defined(DAMIAO_DM4310)
    /* V5017's configuration loader performs its erased-record writeback
     * internally and then continues with the single Flash-to-SRAM copy. */
    platform_load_parameters(&g_app.config);
#else
    if (!platform_load_parameters(&g_app.config)) {
        __disable_irq();
        platform_store_factory_parameters();
        __enable_irq();

        platform_load_parameters(&g_app.config);
    }
#endif
    /* The reference keeps the selected control mode in the zeroed command
     * object from startup; FC enables that mode even before the first
     * setpoint frame.  Parameter writes already synchronize this field. */
    memset(&g_app.motor.command, 0, sizeof(g_app.motor.command));
    g_app.motor.command.mode = g_app.config.control_mode;
#if defined(DAMIAO_DM4310)
    platform_prepare_runtime_configuration();
#endif
    platform_initialize_runtime();
    /* main@0x252f4 prints this block once after sensor/ADC and MCAN setup. */
    debug_console_print_status();
#if !defined(DAMIAO_DM4310)
    g_app.events.print_menu = true;
#endif
    platform_check_startup_bus_voltage();
    platform_start_control_loop();

    for (;;) {
        service_deferred_events();
#if !defined(DAMIAO_DM4310)
        platform_idle();
#endif
    }
}
