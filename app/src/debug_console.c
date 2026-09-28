#include "debug_console.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#include "app_state.h"
#include "app_profile.h"
#include "calibration_upload.h"
#include "platform.h"

enum {
    DEBUG_FRAME_CAPACITY = 200U,
};

typedef enum {
    DEBUG_MODE_MENU = 0,
    DEBUG_MODE_MOTOR = 2,
    DEBUG_MODE_SETUP = 4,
} DebugConsoleMode;

static uint8_t frame_buffer[DEBUG_FRAME_CAPACITY];
static size_t frame_length;
static DebugConsoleMode console_mode;

void debug_console_return_to_menu(void)
{
    console_mode = DEBUG_MODE_MENU;
}

static void write_text(const char *text)
{
    size_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    platform_debug_write(text, length);
}

static void write_unsigned_digits(uint32_t value, uint32_t base,
                                  unsigned minimum_width)
{
    char reversed[16];
    size_t length = 0U;
    do {
        const uint32_t digit = value % base;
        reversed[length++] = (char)(digit < 10U ? ('0' + digit) :
                                                  ('a' + digit - 10U));
        value /= base;
    } while (value != 0U);
    while (length < minimum_width) {
        reversed[length++] = '0';
    }
    while (length != 0U) {
        --length;
        platform_debug_write(&reversed[length], 1U);
    }
}

static void write_float(float value, unsigned precision)
{
    static const uint32_t scale_by_precision[] = {
        1U, 10U, 100U, 1000U, 10000U, 100000U, 1000000U,
    };
    if (precision >= sizeof(scale_by_precision) /
                     sizeof(scale_by_precision[0])) {
        return;
    }
    if (value < 0.0f) {
        static const char minus = '-';
        platform_debug_write(&minus, 1U);
        value = -value;
    }
    const uint32_t scale = scale_by_precision[precision];
    /* Every recovered diagnostic value is bounded below 4294 at the maximum
     * six-decimal precision.  A 32-bit accumulator therefore preserves the
     * emitted text without linking the 64-bit divide/conversion runtime. */
    const uint32_t scaled = (uint32_t)(value * (float)scale + 0.5f);
    write_unsigned_digits(scaled / scale, 10U, 1U);
    if (precision != 0U) {
        static const char decimal = '.';
        platform_debug_write(&decimal, 1U);
        write_unsigned_digits(scaled % scale, 10U, precision);
    }
}

/* GCC's path analyzer does not correlate a custom format parser's switch
 * branch with the corresponding format character at each caller.  The
 * declaration in debug_console.h carries the real printf-format contract,
 * so every call is still checked by -Wformat=2.  Suppress only this known
 * interprocedural false positive inside the parser itself. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-va-arg-type-mismatch"
#endif
void debug_console_printf(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);

    const char *cursor = format;
    while (*cursor != '\0') {
        const char *literal = cursor;
        while ((*cursor != '\0') && (*cursor != '%')) {
            ++cursor;
        }
        platform_debug_write(literal, (size_t)(cursor - literal));
        if (*cursor == '\0') {
            break;
        }

        ++cursor;
        unsigned width = 0U;
        while ((*cursor >= '0') && (*cursor <= '9')) {
            width = width * 10U + (unsigned)(*cursor - '0');
            ++cursor;
        }
        unsigned precision = 6U;
        if (*cursor == '.') {
            precision = 0U;
            ++cursor;
            while ((*cursor >= '0') && (*cursor <= '9')) {
                precision = precision * 10U + (unsigned)(*cursor - '0');
                ++cursor;
            }
        }

        switch (*cursor) {
        case 'd': {
            const int value = va_arg(arguments, int);
            if (value < 0) {
                static const char minus = '-';
                platform_debug_write(&minus, 1U);
                write_unsigned_digits((uint32_t)(-(int64_t)value),
                                      10U, width);
            } else {
                write_unsigned_digits((uint32_t)value, 10U, width);
            }
            break;
        }
        case 'x':
            write_unsigned_digits(va_arg(arguments, unsigned int),
                                  16U, width);
            break;
        case 'f':
            write_float((float)va_arg(arguments, double), precision);
            break;
        case 's':
            write_text(va_arg(arguments, const char *));
            break;
        case '%': {
            static const char percent = '%';
            platform_debug_write(&percent, 1U);
            break;
        }
        default:
            break;
        }
        if (*cursor != '\0') {
            ++cursor;
        }
    }
    va_end(arguments);
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

static float can_data_rate_mbps(uint8_t selector)
{
    static const uint8_t time_quanta[] = {40U, 40U, 32U, 25U,
                                          20U, 16U, 12U, 9U};
    if (selector < 4U) {
        selector = 4U;
    }
    const uint8_t index = (uint8_t)(selector - 4U);
    const uint8_t bounded = index < sizeof(time_quanta) ? index : 0U;
    const float prescaler = bounded == 0U ? 2.0f : 1.0f;
    return 80.0f / (prescaler * (float)time_quanta[bounded]);
}

static void write_motor_parameters(void)
{
    uint8_t response[13] = {'e'};
    memcpy(&response[1], &g_app.config.phase_resistance, sizeof(float));
    memcpy(&response[5], &g_app.config.phase_inductance, sizeof(float));
    memcpy(&response[9], &g_app.config.flux_linkage, sizeof(float));
    platform_debug_write(response, sizeof(response));
}

static void write_device_identity(void)
{
    uint32_t device_id;
    uint32_t application_identity;
    platform_read_device_identity(&device_id, &application_identity);
    uint8_t response[9] = {'f'};
    memcpy(&response[1], &device_id, sizeof(device_id));
    memcpy(&response[5], &application_identity,
           sizeof(application_identity));
    platform_debug_write(response, sizeof(response));
    /* The recovered Uf query returns to the top-level menu. */
    console_mode = DEBUG_MODE_MENU;
}

static void process_frame(const uint8_t *data, size_t length)
{
    if ((data == NULL) || (length == 0U)) {
        return;
    }

    /* The framing and state transitions below are reconstructed
     * from debug_uart_receive_irq at 0x22244. USART receiver-timeout IRQ004
     * provides exactly one complete command burst to this function. */
    if (data[0] == 0x1BU) {
        console_mode = DEBUG_MODE_MENU;
        if (g_app.motor.armed) {
            motor_control_disarm(&g_app.motor);
        }
        if (g_app.motor.feedback.fault == MOTOR_STATUS_ENABLED) {
            g_app.motor.feedback.fault = MOTOR_FAULT_NONE;
        }
        g_app.events.motor_state_changed = true;
        return;
    }

    if ((length == 1U) && (data[0] == 'X')) {
        platform_enter_bootloader();
        return;
    }

    if ((length == 3U) && (data[0] == 'P') && (data[1] == 'O') && (data[2] == 'S')) {
        debug_console_printf("Motor Position: %f\r\n", (double) g_app.motor_output_position);
        return;
    }

    if ((length == 5U) &&
        (data[0] == 'S') &&
        (data[1] == 'H') &&
        (data[2] == 'A') &&
        (data[3] == 'F') &&
        (data[4] == 'T')) {

        float calibration[4];

        if (platform_commissioning_get_output_calibration(calibration)) {
            debug_console_printf(
                "Output Shaft Calibration:\r\n"
                "u=%f\r\n"
                "v=%f\r\n"
                "w=%f\r\n"
                "c=%f\r\n",
                (double)calibration[0],
                (double)calibration[1],
                (double)calibration[2],
                (double)calibration[3]);
        } else {
            write_text(
                "Output Shaft Calibration: unavailable\r\n");
        }

        return;
    }

    if (console_mode == DEBUG_MODE_MENU) {
        if (data[0] == 'm') {
            if (g_app.motor.feedback.fault < 2U) {
                console_mode = DEBUG_MODE_MOTOR;
                motor_control_arm(&g_app.motor);
                g_app.motor.feedback.fault = MOTOR_STATUS_ENABLED;
                g_app.events.motor_state_changed = true;
            }
        } else if (data[0] == 's') {
            console_mode = DEBUG_MODE_SETUP;
        }
        return;
    }

    if (console_mode != DEBUG_MODE_SETUP) {
        return;
    }

    if ((length >= 64U) && (data[0] == 'U') &&
        ((data[1] == 'd') || (data[1] == 'M'))) {
        uint8_t acknowledgement[2];
        CalibrationUploadKind completed;
        if (calibration_upload_receive_frame(data, length,
                                     acknowledgement,
                                     &completed)) {
            platform_debug_write(acknowledgement,
                                sizeof(acknowledgement));

            if (completed == CALIBRATION_UPLOAD_MOTOR_ENCODER) {
                calibration_upload_set_motor_direction(
                    g_app.config.direction);
            }

            if (completed != CALIBRATION_UPLOAD_NONE) {
                g_app.events.calibration_commit_request =
                    (uint8_t)completed;
            }
        }

        return;
    }

    if ((length >= 2U) && (data[0] == 'U') && (data[1] == 'c')) {
        g_app.events.commission_direction = true;
        return;
    }
    if ((length >= 2U) && (data[0] == 'U') && (data[1] == 'l')) {
        g_app.events.commission_position_sensor = true;
        return;
    }

    if ((length == 3U) && (data[0] == 'U') && (data[1] == 'e')) {
        if (data[2] == 0xAAU) {
            write_motor_parameters();
        } else if (data[2] == 'U') {
            g_app.events.identify_motor = true;
        }
        return;
    }

    if ((length == 3U) && (data[0] == 'U') &&
        (data[1] == 'f') && (data[2] == 0xAAU)) {
        write_device_identity();
        return;
    }

    if ((length == 3U) && (data[0] == 'U') &&
        (data[1] == 'g') && (data[2] == 0xAAU)) {
        g_app.events.firmware_control_request = 1U;
        return;
    }

    if ((length == 131U) && (data[0] == 'U') && (data[1] == 'g')) {
        memcpy(g_app.firmware_control_payload, &data[3],
               sizeof(g_app.firmware_control_payload));
        g_app.firmware_control_payload_valid = true;
        if (data[2] == 'U') {
            g_app.events.firmware_control_request = 2U;
        } else if (data[2] == 'Z') {
            g_app.events.firmware_control_request = 4U;
        } else if (data[2] == 'Q') {
            g_app.events.firmware_control_request = 6U;
        } else {
            g_app.firmware_control_payload_valid = false;
        }
        return;
    }

    if ((length >= 4U) && (data[0] == 'F') && (data[1] == 'C') &&
        (data[2] == 'B') && (data[3] >= '0') && (data[3] <= ';')) {
        g_app.config.can_data_rate_selector =
            (uint8_t)(data[3] - '0');

        g_app.pending_store_response_valid = false;
        g_app.events.save_parameters = true;
    }
}

void debug_console_reset(void)
{
    frame_length = 0U;
    console_mode = DEBUG_MODE_MENU;
    calibration_upload_reset();
}

void debug_console_receive(uint8_t byte)
{
    if (frame_length < sizeof(frame_buffer)) {
        frame_buffer[frame_length++] = byte;
    }
}

void debug_console_end_frame(void)
{
    process_frame(frame_buffer, frame_length);
    frame_length = 0U;
}

void debug_console_receive_frame(const uint8_t *data, size_t length)
{
    process_frame(data, length);
}

void debug_console_print_banner(void)
{
    write_text("\n\r"
               " Commands:\n\r"
               " m - Motor Mode\n\r"
               " s - Setup Mode\n\r"
               " esc - Exit to Menu\n\r");
}

void debug_console_print_status(void)
{
    write_text("DMBOT Motor Driver");
    switch (platform_read_hardware_variant()) {
    case 0U: write_text("--V2.0"); break;
    case 1U: write_text("--V3.0"); break;
    case 2U: write_text("--V4.0"); break;
    case 3U: write_text("--V1.0"); break;
    default: break;
    }
    write_text("\n\r Debug Info:\n\r");
    debug_console_printf("Firmware Version: %d\r\n",
                         APP_PROFILE_FIRMWARE_VERSION_LITERAL);
    debug_console_printf("Sub Version: %03d\r\n",
                         APP_PROFILE_FIRMWARE_SUBVERSION_LITERAL);
    debug_console_printf("Imax: %f\r\n",
                         (double)g_app.config.maximum_phase_current);
    debug_console_printf(" I_U Offset:     %.4f\r\n",
                         (double)g_app.motor.current_offset_u);
    debug_console_printf(" I_V Offset:     %.4f\r\n",
                         (double)g_app.motor.current_offset_v);
    debug_console_printf(" I_W Offset:     %.4f\r\n",
                         (double)g_app.motor.current_offset_w);
    debug_console_printf(
        " Position Sensor Electrical Offset:   %.4f\n\r",
        (double)g_app.motor.electrical_offset);
    debug_console_printf(" Mechanical Offset:   %.4f\n\r",
                         (double)g_app.motor.motor_output_position_offset);
    debug_console_printf(" Output Position:  %.4f\n\r",
                         (double)g_app.motor.feedback.position);
    debug_console_printf(" CAN ID:     0x%03x\n\r",
                         (unsigned int)g_app.config.can_id);
    debug_console_printf(" MASTER ID:  0x%03x\n\r",
                         (unsigned int)g_app.config.master_id);
    if (g_app.config.can_data_rate_selector < 4U) {
        static const uint16_t classic_kbps[] = {125U, 200U, 250U, 500U};
        debug_console_printf(
            " CAN Baud: %dKbps\n\r",
            (int)classic_kbps[g_app.config.can_data_rate_selector]);
    } else {
        debug_console_printf(
            " CAN Baud: %.2fMbps\n\r",
            (double)can_data_rate_mbps(g_app.config.can_data_rate_selector));
    }
    write_text("\n\r Motor Info:\n\r");
    debug_console_printf(" Rs  = %.4f m\xA6\xB8\n\r",
                         (double)(g_app.config.phase_resistance * 1000.0f));
    debug_console_printf(" Ls  = %.4f \xA6\xCC" "H\n\r",
                         (double)(g_app.config.phase_inductance * 1000000.0f));
    debug_console_printf(" \xA6\xB7" "f = %.4f Wb\n\r",
                         (double)g_app.config.flux_linkage);
    debug_console_printf("V_BUS=%.4f\r\n",
                         (double)g_app.motor.feedback.bus_voltage);
    write_text("\n\r Control Mode : \r\n");
    write_text(g_app.config.control_mode == MOTOR_MODE_MIT ?
                   "1:MIT Mode <----\n\r" : "1:MIT Mode\n\r");
    write_text(g_app.config.control_mode == MOTOR_MODE_POSITION_SPEED ?
                   "2:position-speed cascade Mode <----\n\r" :
                   "2:position-speed cascade Mode\n\r");
    write_text(g_app.config.control_mode == MOTOR_MODE_SPEED ?
                   "3:speed Mode <----\n\r" : "3:speed Mode\n\r");
    write_text(g_app.config.control_mode == MOTOR_MODE_HYBRID ?
                   "4:Hybrid control Mode <----\n\r" :
                   "4:Hybrid control Mode\n\r");
}
