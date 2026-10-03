#include "debug_console.h"
#include "board_uart.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#include "app_config.h"
#include "app_state.h"
#include "app_profile.h"
#include "calibration_upload.h"
#include "firmware_variant.h"
#include "formatter_arithmetic.h"
#include "platform.h"
#include "runtime_compat.h"

enum
{
    DEBUG_FRAME_CAPACITY = 200U,
};

typedef enum
{
    DEBUG_MODE_MENU = 0,
    DEBUG_MODE_MOTOR = 2,
    DEBUG_MODE_SETUP = 4,
} DebugConsoleMode;

static uint8_t frame_buffer[DEBUG_FRAME_CAPACITY];
static size_t frame_length;
#define CONSOLE_MODE_GET() ((DebugConsoleMode)motor_control_console_mode())
#define CONSOLE_MODE_SET(value) motor_control_set_console_mode((uint32_t)(value))
#define POST_MOTOR_STATE_CHANGE() motor_control_post_state_change()
static uint8_t short_response_buffer[20]
    __attribute__((section(".short_response_buffer"), aligned(1)));

void debug_console_return_to_menu(void)
{
    CONSOLE_MODE_SET(DEBUG_MODE_MENU);
}

static void write_text(const char *text)
{
    size_t length = 0U;
    while (text[length] != '\0')
    {
        ++length;
    }
    platform_debug_write(text, length);
}

/* Private state shared by the format parser and field writers. */
typedef struct FormatterStream
{
    uint32_t flags;
    void (*write)(uint32_t character, void *context);
    void *context;
    uint32_t (*read)(volatile struct FormatterStream *stream);
    const char *next;
    uint32_t pending;
    uint32_t width;
    uint32_t precision;
    uint32_t written;
} FormatterStream;

__attribute__((noipa)) static uint32_t stream_read(volatile FormatterStream *stream)
{
    const char *next = stream->next;
    stream->next = (const char *)((uintptr_t)next + 1U);
    return *(const volatile uint8_t *)next;
}

static void stream_uart(uint32_t character, void *context)
{
    (void)context;
    const char byte = (char)character;
    platform_debug_write(&byte, 1U);
}

__attribute__((noipa)) static void stream_left_pad(volatile FormatterStream *stream)
{
    uint32_t count = stream->width;
    const uint32_t flags = stream->flags;
    const uint32_t character = (flags & 16U) != 0U ? '0' : ' ';
    if ((flags & 1U) != 0U)
        return;
    while ((int32_t)(--count) >= 0)
    {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        write(character, context);
        stream->written = stream->written + 1U;
    }
}

__attribute__((noipa)) static void stream_right_pad(volatile FormatterStream *stream)
{
    uint32_t count = stream->width;
    if ((((const volatile uint8_t *)stream)[0] & 1U) == 0U)
        return;
    while ((int32_t)(--count) >= 0)
    {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        write(' ', context);
        stream->written = stream->written + 1U;
    }
}

__attribute__((noipa)) static void stream_string(volatile FormatterStream *stream, const char *text,
                                                 uint32_t limit)
{
    uint32_t length;
    if (limit == 1U)
    {
        length = 1U;
    }
    else
    {
        if ((((const volatile uint8_t *)stream)[0] & 32U) != 0U)
            limit = stream->precision;
        length = 0U;
        while (length < limit && ((const volatile uint8_t *)text)[length] != 0U)
            ++length;
    }
    const uintptr_t end = (uintptr_t)text + length;
    stream->width = stream->width - length;
    stream->written = stream->written + length;
    stream_left_pad(stream);
    while ((uintptr_t)text < end)
    {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        const uint32_t character = *(const volatile uint8_t *)text++;
        write(character, context);
    }
    stream_right_pad(stream);
}

static void write_character(char character, volatile FormatterStream *stream)
{
    void (*write)(uint32_t, void *) = stream->write;
    void *context = stream->context;
    write((uint8_t)character, context);
    stream->written = stream->written + 1U;
}

__attribute__((noipa)) static void stream_nonfinite(volatile FormatterStream *stream,
                                                    uint32_t unused, uint32_t classification,
                                                    uint32_t sign)
{
    (void)unused;
    const uint32_t flags = stream->flags;
    const bool nan = (int32_t)classification >= 7;
    const char *text = (flags & 2048U) != 0U ? (nan ? "NAN" : "INF") : (nan ? "nan" : "inf");
    stream->flags = flags & ~16U;
    uint32_t width = stream->width - 3U;
    stream->width = width;
    if (sign != 0U)
    {
        --width;
        stream->width = width;
    }
    stream_left_pad(stream);
    if (sign != 0U)
    {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        write(sign, context);
        const uint32_t written = stream->written + 1U;
        stream->written = written;
        stream->written = written + 3U;
    }
    else
    {
        stream->written = stream->written + 3U;
    }
    for (unsigned index = 0U; index < 3U; ++index)
    {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        write((uint8_t)text[index], context);
    }
    stream_right_pad(stream);
}

static void write_padding(char character, unsigned count, volatile FormatterStream *stream)
{
    while ((int32_t)(--count) >= 0)
    {
        write_character(character, stream);
    }
}

static void write_integer(uint32_t value, uint32_t base, char sign, unsigned width,
                          unsigned precision, bool precision_set, bool zero, bool left,
                          bool alternate, bool uppercase, volatile FormatterStream *stream)
{
    const bool prefix = alternate && base == 16U && value != 0U;
    char reversed[16];
    unsigned length = 0U;
    /* This path leaves the digit buffer empty for zero;
     * the shared field emitter supplies its default/explicit precision. */
    while (value != 0U)
    {
        const uint32_t digit = value % base;
        reversed[length++] =
            (char)(digit < 10U ? '0' + digit : (uppercase ? 'A' : 'a') + digit - 10U);
        value /= base;
    }
    const unsigned minimum_digits = precision_set ? precision : 1U;
    const unsigned zeros = (int32_t)minimum_digits > (int32_t)length ? minimum_digits - length : 0U;
    const unsigned field = length + zeros + (sign != '\0' ? 1U : 0U) + (prefix ? 2U : 0U);
    const unsigned padding = width - field;
    stream->width = padding;
    if (precision_set)
        stream->flags = stream->flags & ~16U;
    zero = zero && !precision_set && !left;
    if (!left && !zero)
    {
        write_padding(' ', padding, stream);
    }
    if (sign != '\0')
    {
        write_character(sign, stream);
    }
    if (prefix)
    {
        const char prefix_zero = '0';
        const char prefix_base = uppercase ? 'X' : 'x';
        write_character(prefix_zero, stream);
        write_character(prefix_base, stream);
    }
    if (zero)
    {
        write_padding('0', padding, stream);
    }
    /* Precision uses BGT on the requested count, not padding's BPL. */
    for (unsigned remaining = zeros; (int32_t)remaining > 0; --remaining)
    {
        const char character = '0';
        write_character(character, stream);
    }
    while (length != 0U)
    {
        --length;
        write_character(reversed[length], stream);
    }
    if (left)
    {
        write_padding(' ', padding, stream);
    }
}

static void write_fixed_digits(const DecimalDigits *decimal, unsigned precision,
                               volatile FormatterStream *stream)
{
    const int first = decimal->exponent > 0 ? decimal->exponent : 0;
    for (int position = first; position >= 0; --position)
    {
        const int index = decimal->exponent - position;
        const char digit =
            index >= 0 && (unsigned)index < decimal->count ? decimal->digit[index] : '0';
        write_character(digit, stream);
    }
    if (precision != 0U)
    {
        const char point = runtime_decimal_separator();
        write_character(point, stream);
        for (unsigned position = 1U; position <= precision; ++position)
        {
            const int index = decimal->exponent + (int)position;
            const char digit =
                index >= 0 && (unsigned)index < decimal->count ? decimal->digit[index] : '0';
            write_character(digit, stream);
        }
    }
}

static void write_float_field(double value, unsigned precision, unsigned width, bool zero,
                              bool left, bool plus, bool space, bool alternate,
                              volatile FormatterStream *stream)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    const char sign = (bits >> 63U) != 0U ? '-' : plus ? '+' : space ? ' ' : '\0';
    bits &= UINT64_C(0x7FFFFFFFFFFFFFFF);
    const uint32_t classification = binary64_classify(bits);
    const bool nonfinite = (classification & 2U) != 0U;
    if (nonfinite)
    {
        stream_nonfinite(stream, 0U, classification, (uint8_t)sign);
        return;
    }
    DecimalDigits finite;
    fixed_digits(bits, precision, &finite);
    unsigned payload_length = finite.exponent >= 0 ? (unsigned)finite.exponent + 1U : 1U;
    if (precision != 0U)
    {
        payload_length += precision + 1U;
    }
    const bool decimal = precision == 0U && alternate;
    const unsigned length = payload_length + (sign != '\0' ? 1U : 0U) + (decimal ? 1U : 0U);
    const unsigned padding = width - length;
    stream->width = padding;
    zero = zero && !left;
    if (!left && !zero)
    {
        write_padding(' ', padding, stream);
    }
    if (sign != '\0')
    {
        write_character(sign, stream);
    }
    if (zero)
    {
        write_padding('0', padding, stream);
    }
    write_fixed_digits(&finite, precision, stream);
    if (decimal)
    {
        const char point = runtime_decimal_separator();
        write_character(point, stream);
    }
    if (left)
    {
        write_padding(' ', padding, stream);
    }
}

static void write_float(double value, unsigned precision, volatile FormatterStream *stream)
{
    write_float_field(value, precision, 0U, false, false, false, false, false, stream);
}

/* Return the argument footprint, not the number of emitted characters. */
__attribute__((noipa)) static uint32_t dispatch(volatile FormatterStream *stream,
                                                uint32_t conversion, const uint32_t *arguments)
{
    if (conversion == 's')
    {
        stream_string(stream, (const char *)(uintptr_t)*(const volatile uint32_t *)arguments,
                      UINT32_MAX);
        return 1U;
    }
    if (conversion != 'd' && conversion != 'x' && conversion != 'f')
        return 0U;
    const uint32_t flags = stream->flags;
    const unsigned width = stream->width;
    const bool precision_set = (flags & 32U) != 0U;
    const unsigned precision = precision_set ? stream->precision : 6U;
    const bool zero = (flags & 16U) != 0U;
    const bool left = (flags & 1U) != 0U;
    const bool plus = (flags & 2U) != 0U;
    const bool space = (flags & 4U) != 0U;
    const bool alternate = (flags & 8U) != 0U;
    const bool uppercase = (flags & 2048U) != 0U;
    switch (conversion)
    {
    case 'd':
    {
        const int32_t value = (int32_t)*(const volatile uint32_t *)arguments;
        const char sign = value < 0 ? '-' : plus ? '+' : space ? ' ' : '\0';
        const uint32_t magnitude = value < 0 ? 0U - (uint32_t)value : (uint32_t)value;
        write_integer(magnitude, 10U, sign, width, precision, precision_set, zero, left, false,
                      uppercase, stream);
        return 1U;
    }
    case 'x':
        write_integer(*(const volatile uint32_t *)arguments, 16U, '\0', width, precision,
                      precision_set, zero, left, alternate, uppercase, stream);
        return 1U;
    case 'f':
    {
        const uintptr_t aligned = ((uintptr_t)arguments + 7U) & ~(uintptr_t)7U;
        double value;
        memcpy(&value, (const void *)aligned, sizeof(value));
        if (width == 0U && !zero && !left && !plus && !space && !alternate && !uppercase)
        {
            write_float(value, precision, stream);
        }
        else
        {
            write_float_field(value, precision, width, zero, left, plus, space, alternate, stream);
        }
        return 3U;
    }
    default:
        return 0U;
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
/* This path uses an unsigned subtraction/range test. */
__attribute__((noipa)) static bool decimal_digit(uint32_t value)
{
    return value - UINT32_C(0x30) < UINT32_C(10);
}

__attribute__((noipa)) static int parse(volatile FormatterStream *input, va_list arguments)
{
    input->written = 0U;
    uint32_t current = input->read(input);
#define FORMAT_BYTE current
#define FORMAT_NEXT() (current = input->read(input))
    while (FORMAT_BYTE != '\0')
    {
        /* Consume and emit one literal at a time rather than scanning the
         * whole run before the first UART write. */
        while ((FORMAT_BYTE != '\0') && (FORMAT_BYTE != '%'))
        {
            const char character = (char)FORMAT_BYTE;
            write_character(character, input);
            FORMAT_NEXT();
        }
        if (FORMAT_BYTE == '\0')
        {
            break;
        }

        FORMAT_NEXT();
        bool zero = false;
        bool left = false;
        bool plus = false;
        bool space = false;
        bool alternate = false;
        while ((FORMAT_BYTE == '0') || (FORMAT_BYTE == '-') || (FORMAT_BYTE == '+') ||
               (FORMAT_BYTE == ' ') || (FORMAT_BYTE == '#'))
        {
            zero |= FORMAT_BYTE == '0';
            left |= FORMAT_BYTE == '-';
            plus |= FORMAT_BYTE == '+';
            space |= FORMAT_BYTE == ' ';
            alternate |= FORMAT_BYTE == '#';
            FORMAT_NEXT();
        }
        bool precision_set = false;
        input->precision = 0U;
        input->width = 0U;
        unsigned width = 0U;
        if (FORMAT_BYTE == '*')
        {
            const int requested = va_arg(arguments, int);
            /* This path stores the raw argument; its signed
             * normalization occurs only after precision parsing. */
            width = (unsigned)requested;
            input->width = width;
            FORMAT_NEXT();
        }
        else
        {
            while (decimal_digit((uint8_t)FORMAT_BYTE))
            {
                width = width * 10U + (unsigned)(FORMAT_BYTE - '0');
                input->width = width;
                FORMAT_NEXT();
            }
        }
        unsigned precision = 6U;
        if (FORMAT_BYTE == '.')
        {
            precision_set = true;
            precision = 0U;
            FORMAT_NEXT();
            if (FORMAT_BYTE == '*')
            {
                const int requested = va_arg(arguments, int);
                input->precision = (uint32_t)requested;
                precision_set = requested >= 0;
                precision = requested >= 0 ? (unsigned)requested : 6U;
                FORMAT_NEXT();
            }
            else
            {
                while (decimal_digit((uint8_t)FORMAT_BYTE))
                {
                    precision = precision * 10U + (unsigned)(FORMAT_BYTE - '0');
                    input->precision = precision;
                    FORMAT_NEXT();
                }
            }
        }

        /* This path normalizes the signed width word after
         * parsing, including a decimal width that wraps into bit 31. */
        if ((int32_t)width < 0)
        {
            width = 0U - width;
            left = true;
            input->width = width;
        }
        if (left)
            zero = false;
        const bool uppercase = FORMAT_BYTE >= 'A' && FORMAT_BYTE <= 'Z';
        const char conversion = uppercase ? (char)(FORMAT_BYTE + ('a' - 'A')) : (char)FORMAT_BYTE;
        input->flags = (left ? 1U : 0U) | (plus ? 2U : 0U) | (space && !plus ? 4U : 0U) |
                       (alternate ? 8U : 0U) | (zero ? 16U : 0U) | (precision_set ? 32U : 0U) |
                       (uppercase ? 2048U : 0U);
        _Static_assert(sizeof(va_list) == sizeof(uintptr_t),
                       "Firmware parser requires ARM word-sized va_list");
        uintptr_t argument_cursor;
        memcpy(&argument_cursor, &arguments, sizeof(argument_cursor));
        const uint32_t consumed =
            dispatch(input, (uint8_t)conversion, (const uint32_t *)argument_cursor);
        if (consumed == 0U)
        {
            if (FORMAT_BYTE != '\0')
                write_character(conversion, input);
        }
        else
        {
            argument_cursor = consumed == 1U ? argument_cursor + 4U
                                             : ((argument_cursor + 7U) & ~(uintptr_t)7U) + 8U;
            memcpy(&arguments, &argument_cursor, sizeof(argument_cursor));
        }
        if (FORMAT_BYTE != '\0')
        {
            FORMAT_NEXT();
        }
    }
    return (int32_t)input->written;
}
#undef FORMAT_BYTE
#undef FORMAT_NEXT
__attribute__((noipa)) static int format_context(const char *format, void *output,
                                                 va_list arguments, void (*write)(uint32_t, void *))
{
    FormatterStream stream;
    stream.write = write;
    stream.context = output;
    stream.pending = 0U;
    stream.read = stream_read;
    stream.next = format;
    return parse(&stream, arguments);
}

__attribute__((noipa)) static int console_vprintf(const char *format, const volatile void *output,
                                                  va_list arguments)
{
    const int written = format_context(format, (void *)(uintptr_t)output, arguments, stream_uart);
    /* Report a stream failure only after all pending output has been attempted. */
    return runtime_stream_error(output) != 0U ? -1 : written;
}

int debug_console_printf(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    const int written = console_vprintf(
        format,
        (const volatile void *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa670), UINT32_C(0x1fffa6a8)),
        arguments);
    va_end(arguments);
    return written;
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

static void write_motor_parameters(void)
{
    volatile uint8_t *const response = short_response_buffer;
    volatile const uint32_t *const parameters =
        (volatile const uint32_t *)app_config_staging_record();
    response[0] = 'e';
    *(volatile uint32_t *)(uintptr_t)(response + 1U) = parameters[0x11U];
    *(volatile uint32_t *)(uintptr_t)(response + 5U) = parameters[0x12U];
    *(volatile uint32_t *)(uintptr_t)(response + 9U) = parameters[0x13U];
    platform_debug_write((const void *)(uintptr_t)response, 13U);
}

static void write_device_identity(void)
{
    uint32_t device_id;
    uint32_t application_identity;
    platform_read_device_identity(&device_id, &application_identity);
    volatile uint8_t *const response = short_response_buffer;
    response[0] = 'f';
    /* The identity query writes its response in tag/device/application order.
     * Volatile word stores keep GCC from moving the device word ahead of the
     * tag byte when this target is built for size. */
    *(volatile uint32_t *)(uintptr_t)(response + 1U) = device_id;
    *(volatile uint32_t *)(uintptr_t)(response + 5U) = application_identity;
    platform_debug_write((const void *)(uintptr_t)response, 9U);
    /* The identity query returns to the top-level menu. */
    CONSOLE_MODE_SET(DEBUG_MODE_MENU);
}

static void process_frame(const uint8_t *data, int32_t length)
{
    if (data == NULL)
    {
        return;
    }
    const uint8_t command = data[0];

    /* USART receiver-timeout IRQ004 provides exactly one complete command
     * burst to this parser. */
    if (command == 0x1BU)
    {
        CONSOLE_MODE_SET(DEBUG_MODE_MENU);
        /* The firmware handler writes the fixed mode once, independently of
         * source-only mirror state. */
        g_app.motor.armed = false;
        if (motor_control_runtime_fault() == MOTOR_STATUS_ENABLED)
        {
            motor_control_set_runtime_fault(MOTOR_FAULT_NONE);
            g_app.motor.feedback.fault = MOTOR_FAULT_NONE;
        }
        POST_MOTOR_STATE_CHANGE();
        return;
    }

    if ((length == 1U) && (command == 'X'))
    {
        /* Commit the boot record as update-requested and unconfirmed before
         * performing the delayed AIRCR reset sequence. */
        platform_enter_bootloader();
        return;
    }

    const DebugConsoleMode mode = CONSOLE_MODE_GET();
    if (mode == DEBUG_MODE_MENU)
    {
        if (command == 'm')
        {
            if ((uint32_t)motor_control_runtime_fault() < 2U)
            {
                CONSOLE_MODE_SET(DEBUG_MODE_MOTOR);
                /* CONSOLE_MODE_SET above is the one fixed-layout fixed-SRAM
                 * write; keep only the source-level mirror here. */
                g_app.motor.armed = true;
                g_app.motor.feedback.fault = MOTOR_STATUS_ENABLED;
                motor_control_set_runtime_fault(MOTOR_STATUS_ENABLED);
                POST_MOTOR_STATE_CHANGE();
            }
        }
        else if (command == 's')
        {
            CONSOLE_MODE_SET(DEBUG_MODE_SETUP);
        }
        return;
    }

    if (mode != DEBUG_MODE_SETUP)
    {
        return;
    }

    /* Load bytes 1 and 2 before dispatching either the U family or FCB, and
     * retain both values across the branches. */
    const uint8_t subcommand = data[1];
    const uint8_t argument = data[2];

    /* Uc and Ul return before the expected-length halfword read. */
    if ((command == 'U') && (subcommand == 'c'))
    {
        APP_DEFERRED_EVENTS.commission_direction = true;
        return;
    }
    if ((command == 'U') && (subcommand == 'l'))
    {
        APP_DEFERRED_EVENTS.commission_position_sensor = true;
        return;
    }

    /* Read this for every remaining U subtype, not only Ud and UM. */
    const int32_t expected_frame_length =
        command == 'U' ? (int32_t)board_uart_expected_payload_length() + 1 : 0;
    if ((length == expected_frame_length) && (command == 'U') &&
        ((subcommand == 'd') || (subcommand == 'M')))
    {
        uint8_t *const acknowledgement = short_response_buffer;
        CalibrationUploadKind completed;
        if (calibration_upload_receive_frame_irq(data, (size_t)length, subcommand, acknowledgement,
                                                 &completed))
        {
            platform_debug_write(acknowledgement, 2U);

            completed = calibration_upload_finish_frame_irq(subcommand);
            if (completed != CALIBRATION_UPLOAD_NONE)
            {
                APP_DEFERRED_EVENTS.calibration_commit_request = (uint8_t)completed;
                if (completed == CALIBRATION_UPLOAD_MOTOR_ENCODER)
                {
                    const float direction =
                        *(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                            UINT32_C(0x1ffff0bc), UINT32_C(0x1ffff048));
                    calibration_upload_set_motor_direction(direction);
                }
            }
        }

        return;
    }

    if ((command == 'U') && (subcommand == 'e'))
    {
        if ((argument == 0xAAU) && (length == 3))
        {
            write_motor_parameters();
        }
        /* Reload RX byte 2 after the optional parameter reply. */
        if ((data[2] == 'U') && (board_uart_received_length() == 3U))
        {
            APP_DEFERRED_EVENTS.identify_motor = true;
        }
        return;
    }

    if ((length == 3U) && (command == 'U') && (subcommand == 'f') && (argument == 0xAAU))
    {
        write_device_identity();
        return;
    }

    if ((length == 3U) && (command == 'U') && (subcommand == 'g') && (argument == 0xAAU))
    {
        APP_DEFERRED_EVENTS.firmware_control_request = 1U;
        return;
    }

    if ((length == 131U) && (command == 'U') && (subcommand == 'g'))
    {
        const uint8_t subtype = argument;
        if ((subtype != 'U') && (subtype != 'Z') && (subtype != 'Q'))
        {
            return;
        }
        /* Overwrite the first 0x80 bytes of the live 37-word staging record
         * before posting the UgU, UgZ, or UgQ request. */
        runtime_copy_bytes(app_config_staging_record(), &data[3],
                           sizeof(g_app.firmware_control_payload));
        if (subtype == 'U')
        {
            APP_DEFERRED_EVENTS.firmware_control_request = 2U;
        }
        else if (subtype == 'Z')
        {
            APP_DEFERRED_EVENTS.firmware_control_request = 4U;
        }
        else
        {
            APP_DEFERRED_EVENTS.firmware_control_request = 6U;
        }
        return;
    }

    if ((command == 'F') && (subcommand == 'C') && (argument == 'B'))
    {
        /* Decode with byte-'0' followed by an unsigned <= 11 test.  Selectors
         * 10 and 11 are consequently encoded as ':' and ';'. */
        const uint8_t selector = (uint8_t)(data[3] - '0');
        if (selector > 11U)
        {
            return;
        }

        g_app.pending_store_response_valid = false;
        /* The firmware staging record is also its live configuration object;
         * keep the source-level decoded view synchronized with word 0x23. */
        app_config_staging_record()[0x23] = selector;
        g_app.config.can_data_rate_selector = selector;
        platform_select_mcan_transport_format(selector);
        APP_DEFERRED_EVENTS.save_staged_parameters = true;
    }
}

void debug_console_reset(void)
{
    frame_length = 0U;
    calibration_upload_reset();
}

void debug_console_receive(uint8_t byte)
{
    if (frame_length < sizeof(frame_buffer))
    {
        frame_buffer[frame_length++] = byte;
    }
}

void debug_console_end_frame(void)
{
    process_frame(frame_buffer, (int32_t)frame_length);
    frame_length = 0U;
}

void debug_console_process_dma_frame(const uint8_t *data, int16_t length)
{
    process_frame(data, (int32_t)length);
}

void debug_console_receive_frame(const uint8_t *data, size_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return;
    }
    if (length > sizeof(frame_buffer))
    {
        length = sizeof(frame_buffer);
    }
    /* Model the production DMA buffer: only received bytes are overwritten;
     * the unused tail deliberately retains the preceding frame. */
    memcpy(frame_buffer, data, length);
    process_frame(frame_buffer, (int32_t)length);
}

void debug_console_print_banner(void)
{
    write_text("\n\r"
               " Commands:\n\r"
               " m - Motor Mode\n\r"
               " s - Setup Mode\n\r"
               " esc - Exit to Menu\n\r");
}

/* Keep literal diagnostics on the same formatter path as this path. */
#define write_status_text debug_console_printf

void debug_console_print_status(void)
{
    /* Retain the register-derived rate before UART output. Unit selection uses
     * the signed float value rather than the configuration selector. */
    const float data_rate_kbps = platform_read_mcan_data_rate_kbps();
    int32_t data_rate_bits;
    memcpy(&data_rate_bits, &data_rate_kbps, sizeof(data_rate_bits));
    write_status_text(FIRMWARE_STATUS_BANNER);
    switch (platform_read_hardware_variant())
    {
    case 0U:
        write_status_text("--V2.0");
        break;
    case 1U:
        write_status_text("--V3.0");
        break;
    case 2U:
        write_status_text("--V4.0");
        break;
    case 3U:
        write_status_text("--V1.0");
        break;
    default:
        break;
    }
    write_status_text("\n\r Debug Info:\n\r");
    debug_console_printf("Firmware Version: %d\r\n", APP_PROFILE_FIRMWARE_VERSION_LITERAL);
    debug_console_printf("Sub Version: %03d\r\n", APP_PROFILE_FIRMWARE_SUBVERSION_LITERAL);
    debug_console_printf("Imax: %f\r\n", (double)APP_PROFILE_CURRENT_FULL_SCALE_A);
    /* Reload each live operand immediately before printing it. */
    debug_console_printf(" I_U Offset:     %.4f\r\n",
                         (double)*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1ffff144), UINT32_C(0x1ffff0d0)));
    debug_console_printf(" I_V Offset:     %.4f\r\n",
                         (double)*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1ffff148), UINT32_C(0x1ffff0d4)));
    debug_console_printf(" I_W Offset:     %.4f\r\n",
                         (double)*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1ffff14c), UINT32_C(0x1ffff0d8)));
    debug_console_printf(" Position Sensor Electrical Offset:   %.4f\n\r",
                         (double)*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1ffff09c), UINT32_C(0x1ffff028)));
    debug_console_printf(" Mechanical Offset:   %.4f\n\r",
                         (double)*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1ffff0ac), UINT32_C(0x1ffff038)));
    debug_console_printf(" Output Position:  %.4f\n\r",
                         (double)*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1ffff0a0), UINT32_C(0x1ffff02c)));
    debug_console_printf(" CAN ID:     0x%03x\n\r",
                         (unsigned int)*(volatile const uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1fffa5e8), UINT32_C(0x1fffa578)));
    debug_console_printf(" MASTER ID:  0x%03x\n\r",
                         (unsigned int)*(volatile const uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1fffa5e4), UINT32_C(0x1fffa574)));
    if (data_rate_bits < INT32_C(0x447a0000))
    {
        debug_console_printf(" CAN Baud: %dKbps\n\r", (int)(uint32_t)data_rate_kbps);
    }
    else
    {
        debug_console_printf(" CAN Baud: %.2fMbps\n\r", (double)(data_rate_kbps * 0x1.0624dep-10f));
    }
    write_status_text("\n\r Motor Info:\n\r");
    debug_console_printf(" Rs  = %.4f m\xA6\xB8\n\r",
                         (double)(*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                      UINT32_C(0x1fffa60c), UINT32_C(0x1fffa59c)) *
                                  1000.0f));
    debug_console_printf(" Ls  = %.4f \xA6\xCC"
                         "H\n\r",
                         (double)(*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                      UINT32_C(0x1fffa610), UINT32_C(0x1fffa5a0)) *
                                  1000000.0f));
    debug_console_printf(" \xA6\xB7"
                         "f = %.4f Wb\n\r",
                         (double)*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1fffa614), UINT32_C(0x1fffa5a4)));
    debug_console_printf("V_BUS=%.4f\r\n",
                         (double)*(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                             UINT32_C(0x1ffff16c), UINT32_C(0x1ffff0f8)));
    write_status_text("\n\r Control Mode : \r\n");
    /* Four independent mode reads/arms, not four per-line selectors. */
    volatile const uint32_t *const mode =
        (volatile const uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff140),
                                                                    UINT32_C(0x1ffff0cc));
    if (*mode == 1U)
    {
        write_status_text("1:MIT Mode <----\n\r");
        write_status_text("2:position-speed cascade Mode\n\r");
        write_status_text("3:speed Mode\n\r");
        write_status_text("4:Hybrid control Mode\n\r");
    }
    if (*mode == 2U)
    {
        write_status_text("1:MIT Mode\n\r");
        write_status_text("2:position-speed cascade Mode <----\n\r");
        write_status_text("3:speed Mode\n\r");
        write_status_text("4:Hybrid control Mode\n\r");
    }
    if (*mode == 3U)
    {
        write_status_text("1:MIT Mode\n\r");
        write_status_text("2:position-speed cascade Mode\n\r");
        write_status_text("3:speed Mode <----\n\r");
        write_status_text("4:Hybrid control Mode\n\r");
    }
    if (*mode == 4U)
    {
        write_status_text("1:MIT Mode\n\r");
        write_status_text("2:position-speed cascade Mode\n\r");
        write_status_text("3:speed Mode\n\r");
        write_status_text("4:Hybrid control Mode <----\n\r");
    }
}
#undef write_status_text
