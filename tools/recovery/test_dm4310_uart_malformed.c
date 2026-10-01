/* Isolated host test for Vector_20 persistent-DMA malformed-frame behavior.
 * It is intentionally absent from CMake and the normal make graph. */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_state.h"
#include "calibration_upload.h"
#include "debug_console.h"

AppState g_app;
Dm4310RuntimeStatus dm4310_runtime_status;

static uint32_t console_mode;
static unsigned transport_selects;
static unsigned state_changes;
static unsigned bootloader_entries;
static MotorFault runtime_fault;
static uint8_t debug_output[32];
static size_t debug_output_length;

uint32_t motor_control_console_mode(void) { return console_mode; }
uint16_t board_uart_expected_payload_length(void) { return 63U; }
void motor_control_set_console_mode(uint32_t mode) { console_mode = mode; }
void motor_control_post_state_change(void) { ++state_changes; }
void motor_control_arm(MotorController *controller) { controller->armed = true; }
void motor_control_disarm(MotorController *controller) { controller->armed = false; }
void motor_control_set_runtime_fault(MotorFault fault) { runtime_fault = fault; }
MotorFault motor_control_runtime_fault(void) { return runtime_fault; }
void platform_debug_write(const void *data, size_t length)
{
    assert(debug_output_length + length <= sizeof(debug_output));
    memcpy(debug_output + debug_output_length, data, length);
    debug_output_length += length;
}
void platform_enter_bootloader(void) { ++bootloader_entries; }
void platform_select_mcan_transport_format(uint8_t selector)
{
    (void)selector;
    ++transport_selects;
}
bool platform_read_device_identity(uint32_t *device_id,
                                   uint32_t *application_identity)
{
    *device_id = 0U;
    *application_identity = 0U;
    return true;
}
uint8_t platform_read_hardware_variant(void) { return 0U; }
void calibration_upload_reset(void) {}
bool calibration_upload_receive_frame(
    const uint8_t *frame, size_t length, uint8_t acknowledgement[2],
    CalibrationUploadKind *completed)
{
    (void)frame;
    (void)length;
    (void)acknowledgement;
    *completed = CALIBRATION_UPLOAD_NONE;
    return false;
}
void calibration_upload_set_motor_direction(float direction)
{
    (void)direction;
}
uint32_t *app_config_staging_record(void)
{
    static uint32_t staging[37];
    return staging;
}

int main(void)
{
    memset(&g_app, 0, sizeof(g_app));
    memset(&dm4310_runtime_status, 0, sizeof(dm4310_runtime_status));
    debug_console_reset();

    const uint8_t motor = 'm';
    g_app.motor.feedback.fault = MOTOR_FAULT_BUS_OVERVOLTAGE;
    debug_console_receive_frame(&motor, 1U);
    assert(console_mode == 2U);
    assert(g_app.motor.armed);
    assert(runtime_fault == MOTOR_STATUS_ENABLED);
    assert(state_changes == 1U);

    /* Escape tests the fixed sample-state fault word, not the decoded
     * feedback mirror. */
    g_app.motor.feedback.fault = MOTOR_FAULT_BUS_OVERVOLTAGE;
    const uint8_t escape = 0x1bU;
    debug_console_receive_frame(&escape, 1U);
    assert(console_mode == 0U);
    assert(!g_app.motor.armed);
    assert(runtime_fault == MOTOR_FAULT_NONE);
    assert(g_app.motor.feedback.fault == MOTOR_FAULT_NONE);
    assert(state_changes == 2U);

    const uint8_t reset_bad[] = {'X', 0U};
    debug_console_receive_frame(reset_bad, sizeof(reset_bad));
    assert(bootloader_entries == 0U);
    const uint8_t reset = 'X';
    debug_console_receive_frame(&reset, 1U);
    assert(bootloader_entries == 1U);

    const uint8_t setup = 's';
    debug_console_receive_frame(&setup, 1U);
    assert(console_mode == 4U);

    const uint8_t commission[] = {'U', 'c'};
    debug_console_receive_frame(commission, sizeof(commission));
    assert(dm4310_runtime_status.commission_direction);

    dm4310_runtime_status.commission_direction = false;
    const uint8_t short_u = 'U';
    debug_console_receive_frame(&short_u, 1U);
    assert(dm4310_runtime_status.commission_direction);

    dm4310_runtime_status.commission_position_sensor = false;
    const uint8_t commission_position[] = {'U', 'l'};
    debug_console_receive_frame(commission_position,
                                sizeof(commission_position));
    assert(dm4310_runtime_status.commission_position_sensor);

    const uint8_t parameter_query[] = {'U', 'e', 0xaaU};
    g_app.config.phase_resistance = 1.0f;
    g_app.config.phase_inductance = 2.0f;
    g_app.config.flux_linkage = 3.0f;
    debug_output_length = 0U;
    debug_console_receive_frame(parameter_query, sizeof(parameter_query));
    assert(debug_output_length == 13U && debug_output[0] == 'e');

    dm4310_runtime_status.identify_motor = false;
    const uint8_t identify[] = {'U', 'e', 'U'};
    debug_console_receive_frame(identify, sizeof(identify));
    assert(dm4310_runtime_status.identify_motor);

    const uint8_t firmware_query[] = {'U', 'g', 0xaaU};
    dm4310_runtime_status.firmware_control_request = 0U;
    debug_console_receive_frame(firmware_query, sizeof(firmware_query));
    assert(dm4310_runtime_status.firmware_control_request == 1U);

    uint8_t firmware_frame[131] = {'U', 'g', 'U'};
    for (size_t i = 0U; i < 128U; ++i) {
        firmware_frame[i + 3U] = (uint8_t)i;
    }
    debug_console_receive_frame(firmware_frame, sizeof(firmware_frame));
    assert(dm4310_runtime_status.firmware_control_request == 2U);
    assert(g_app.firmware_control_payload_valid);
    assert(memcmp(g_app.firmware_control_payload, firmware_frame + 3U,
                  128U) == 0);
    firmware_frame[2] = 'Z';
    debug_console_receive_frame(firmware_frame, sizeof(firmware_frame));
    assert(dm4310_runtime_status.firmware_control_request == 4U);
    firmware_frame[2] = 'Q';
    debug_console_receive_frame(firmware_frame, sizeof(firmware_frame));
    assert(dm4310_runtime_status.firmware_control_request == 6U);
    dm4310_runtime_status.firmware_control_request = 0U;
    debug_console_receive_frame(firmware_frame, sizeof(firmware_frame) - 1U);
    assert(dm4310_runtime_status.firmware_control_request == 0U);

    const uint8_t format[] = {'F', 'C', 'B', '5'};
    debug_console_receive_frame(format, sizeof(format));
    assert(g_app.config.can_data_rate_selector == 5U);
    assert(transport_selects == 1U);

    const uint8_t short_f = 'F';
    debug_console_receive_frame(&short_f, 1U);
    assert(transport_selects == 2U);
    assert(dm4310_runtime_status.save_staged_parameters);

    const uint8_t invalid_format[] = {'F', 'C', 'B', '/'};
    dm4310_runtime_status.save_staged_parameters = false;
    debug_console_receive_frame(invalid_format, sizeof(invalid_format));
    assert(!dm4310_runtime_status.save_staged_parameters);
    const uint8_t format_ten[] = {'F', 'C', 'B', ':'};
    debug_console_receive_frame(format_ten, sizeof(format_ten));
    assert(g_app.config.can_data_rate_selector == 10U);
    const uint8_t format_eleven[] = {'F', 'C', 'B', ';'};
    debug_console_receive_frame(format_eleven, sizeof(format_eleven));
    assert(g_app.config.can_data_rate_selector == 11U);
    return 0;
}
