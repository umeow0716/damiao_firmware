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
    const uint32_t can_error = APP_DEFERRED_EVENTS.can_error;
    if (can_error == 1U)
    {
        debug_console_printf("CAN Error 1\n\r");
        APP_DEFERRED_EVENTS.can_error = 0U;
    }
    else if (can_error == 2U)
    {
        debug_console_printf("CAN Error 2\n\r");
        APP_DEFERRED_EVENTS.can_error = 0U;
    }
    if (APP_DEFERRED_EVENTS.save_staged_parameters)
    {
        __disable_irq();
        platform_store_staged_parameters();
        APP_DEFERRED_EVENTS.save_staged_parameters = false;
        platform_finish_flash_commit();
        __enable_irq();
    }
    if (APP_DEFERRED_EVENTS.reserved_08 != 0U)
    {
        APP_DEFERRED_EVENTS.reserved_08 = 0U;
    }
    if (APP_DEFERRED_EVENTS.commission_direction)
    {
        commissioning_run_direction_and_alignment(&g_app.config);
        APP_DEFERRED_EVENTS.commission_direction = false;
        debug_console_return_to_menu();
    }
    const uint32_t calibration_request = APP_DEFERRED_EVENTS.calibration_commit_request;

    if (calibration_request != CALIBRATION_UPLOAD_NONE)
    {
        __disable_irq();

        if (calibration_request == CALIBRATION_UPLOAD_MOTOR_ENCODER)
        {
            platform_store_motor_encoder_calibration(calibration_upload_motor_record());
        }
        if (APP_DEFERRED_EVENTS.calibration_commit_request == CALIBRATION_UPLOAD_OUTPUT_SENSOR)
        {
            /* Firmware captures parameter words only after the table writer
             * returns, directly from their fixed calibration source. */
            platform_store_output_sensor_calibration(
                calibration_upload_output_table(), (const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                                       UINT32_C(0x1ffff078), UINT32_C(0x1ffff004)));
        }

        APP_DEFERRED_EVENTS.calibration_commit_request = CALIBRATION_UPLOAD_NONE;
        platform_load_motor_calibration(&g_app.motor);
        position_sensor_reset_accumulated_delta();
        platform_finish_flash_commit();

        __enable_irq();

        debug_console_return_to_menu();
    }
    if (APP_DEFERRED_EVENTS.identify_motor)
    {
        commissioning_run_motor_identification(&g_app.config);
        APP_DEFERRED_EVENTS.identify_motor = false;
        debug_console_return_to_menu();
    }
    const uint32_t firmware_request = APP_DEFERRED_EVENTS.firmware_control_request;
    if (firmware_request != 0U)
    {
        firmware_control_service((uint8_t)firmware_request);
        APP_DEFERRED_EVENTS.firmware_control_request = 0U;
        debug_console_return_to_menu();
    }
    if (APP_DEFERRED_EVENTS.commission_position_sensor)
    {
        commissioning_run_output_sensor_calibration();
        APP_DEFERRED_EVENTS.commission_position_sensor = false;
    }
}

int main(void)
{
    /* Select the application vector table before touching the clock tree or
     * persistent state. */
    SCB->VTOR = APP_VECTOR_TABLE_ADDRESS;
    platform_early_init();
    /* main performs this write immediately after its clock routine.
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
    /* The configuration loader performs erased-record writeback internally,
     * then continues with the single Flash-to-SRAM copy. */
    platform_load_parameters(&g_app.config);
    /* The reference keeps the selected control mode in the zeroed command
     * object from startup; FC enables that mode even before the first
     * setpoint frame.  Parameter writes already synchronize this field. */
    memset(&g_app.motor.command, 0, sizeof(g_app.motor.command));
    g_app.motor.command.mode = g_app.config.control_mode;
    platform_prepare_runtime_configuration();
    platform_initialize_runtime();
    /* main prints this block once after sensor/ADC and MCAN setup. */
    debug_console_print_status();
    platform_check_startup_bus_voltage();
    platform_start_control_loop();

    for (;;)
    {
        service_deferred_events();
    }
}
