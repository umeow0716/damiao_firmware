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
#include "dm4310_formatter_arithmetic.h"
#include "platform.h"
#include "runtime_compat.h"

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
#if defined(DAMIAO_DM4310)
#define CONSOLE_MODE_GET() ((DebugConsoleMode)motor_control_console_mode())
#define CONSOLE_MODE_SET(value) \
    motor_control_set_console_mode((uint32_t)(value))
#else
static DebugConsoleMode console_mode;
#define CONSOLE_MODE_GET() console_mode
#define CONSOLE_MODE_SET(value) (console_mode = (value))
#endif
#if defined(DAMIAO_DM4310)
#define POST_MOTOR_STATE_CHANGE() motor_control_post_state_change()
#else
#define POST_MOTOR_STATE_CHANGE() (g_app.events.motor_state_changed = true)
#endif
#if defined(DAMIAO_DM4310)
static uint8_t short_response_buffer[20]
    __attribute__((section(".dm4310_short_response_buffer"), aligned(1)));
#endif

void debug_console_return_to_menu(void)
{
    CONSOLE_MODE_SET(DEBUG_MODE_MENU);
}

static void write_text(const char *text)
{
    size_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    platform_debug_write(text, length);
}

#if !defined(DAMIAO_DM4310)
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
#endif

#if defined(DAMIAO_DM4310)
/* Factory formatter context words used by 2048c/204b8/204da. */
typedef struct FactoryFormatterStream {
    uint32_t flags;
    void (*write)(uint32_t character, void *context);
    void *context;
    uint32_t (*read)(volatile struct FactoryFormatterStream *stream);
    const char *next;
    uint32_t pending;
    uint32_t width;
    uint32_t precision;
    uint32_t written;
} FactoryFormatterStream;

_Static_assert(offsetof(FactoryFormatterStream, read) == 12U &&
               offsetof(FactoryFormatterStream, next) == 16U &&
               offsetof(FactoryFormatterStream, width) == 24U &&
               offsetof(FactoryFormatterStream, precision) == 28U &&
               offsetof(FactoryFormatterStream, written) == 32U,
               "Factory formatter context offsets must not change");

__attribute__((noipa))
static uint32_t factory_stream_read(volatile FactoryFormatterStream *stream)
{
    const char *next = stream->next;
    stream->next = (const char *)((uintptr_t)next + 1U);
    return *(const volatile uint8_t *)next;
}

static void factory_stream_uart(uint32_t character, void *context)
{
    (void)context;
    const char byte = (char)character;
    platform_debug_write(&byte, 1U);
}

__attribute__((noipa))
static void factory_stream_left_pad(volatile FactoryFormatterStream *stream)
{
    uint32_t count = stream->width;
    const uint32_t flags = stream->flags;
    const uint32_t character = (flags & 16U) != 0U ? '0' : ' ';
    if ((flags & 1U) != 0U) return;
    while ((int32_t)(--count) >= 0) {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        write(character, context);
        stream->written = stream->written + 1U;
    }
}

__attribute__((noipa))
static void factory_stream_right_pad(volatile FactoryFormatterStream *stream)
{
    uint32_t count = stream->width;
    if ((((const volatile uint8_t *)stream)[0] & 1U) == 0U) return;
    while ((int32_t)(--count) >= 0) {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        write(' ', context);
        stream->written = stream->written + 1U;
    }
}

__attribute__((noipa))
static void factory_stream_string(volatile FactoryFormatterStream *stream,
                                  const char *text, uint32_t limit)
{
    uint32_t length;
    if (limit == 1U) {
        length = 1U;
    } else {
        if ((((const volatile uint8_t *)stream)[0] & 32U) != 0U)
            limit = stream->precision;
        length = 0U;
        while (length < limit && ((const volatile uint8_t *)text)[length] != 0U)
            ++length;
    }
    const uintptr_t end = (uintptr_t)text + length;
    stream->width = stream->width - length;
    stream->written = stream->written + length;
    factory_stream_left_pad(stream);
    while ((uintptr_t)text < end) {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        const uint32_t character = *(const volatile uint8_t *)text++;
        write(character, context);
    }
    factory_stream_right_pad(stream);
}

static void write_factory_character(char character, volatile FactoryFormatterStream *stream)
{
    void (*write)(uint32_t, void *) = stream->write;
    void *context = stream->context;
    write((uint8_t)character, context);
    stream->written = stream->written + 1U;
}

__attribute__((noipa))
static void factory_stream_nonfinite(volatile FactoryFormatterStream *stream,
                                     uint32_t unused, uint32_t classification,
                                     uint32_t sign)
{
    (void)unused;
    const uint32_t flags = stream->flags;
    const bool nan = (int32_t)classification >= 7;
    const char *text = (flags & 2048U) != 0U ?
                       (nan ? "NAN" : "INF") : (nan ? "nan" : "inf");
    stream->flags = flags & ~16U;
    uint32_t width = stream->width - 3U;
    stream->width = width;
    if (sign != 0U) {
        --width;
        stream->width = width;
    }
    factory_stream_left_pad(stream);
    if (sign != 0U) {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        write(sign, context);
        const uint32_t written = stream->written + 1U;
        stream->written = written;
        stream->written = written + 3U;
    } else {
        stream->written = stream->written + 3U;
    }
    for (unsigned index = 0U; index < 3U; ++index) {
        void (*write)(uint32_t, void *) = stream->write;
        void *context = stream->context;
        write((uint8_t)text[index], context);
    }
    factory_stream_right_pad(stream);
}

static void write_padding(char character, unsigned count, volatile FactoryFormatterStream *stream)
{
    while ((int32_t)(--count) >= 0) {
        write_factory_character(character, stream);
    }
}

static void write_factory_integer(uint32_t value, uint32_t base, char sign,
                                   unsigned width, unsigned precision,
                                   bool precision_set, bool zero, bool left,
                                   bool alternate, bool uppercase,
                                   volatile FactoryFormatterStream *stream)
{
    const bool prefix = alternate && base == 16U && value != 0U;
    char reversed[16];
    unsigned length = 0U;
    /* Factory 0x20582/0x205d8 leaves the digit buffer empty for zero;
     * the shared field emitter supplies its default/explicit precision. */
    while (value != 0U) {
        const uint32_t digit = value % base;
        reversed[length++] = (char)(digit < 10U ? '0' + digit :
                      (uppercase ? 'A' : 'a') + digit - 10U);
        value /= base;
    }
    const unsigned minimum_digits = precision_set ? precision : 1U;
    const unsigned zeros = (int32_t)minimum_digits > (int32_t)length ?
                               minimum_digits - length : 0U;
    const unsigned field = length + zeros + (sign != '\0' ? 1U : 0U) +
                           (prefix ? 2U : 0U);
    const unsigned padding = width - field;
    stream->width = padding;
    if (precision_set) stream->flags = stream->flags & ~16U;
    zero = zero && !precision_set && !left;
    if (!left && !zero) {
        write_padding(' ', padding, stream);
    }
    if (sign != '\0') {
        write_factory_character(sign, stream);
    }
    if (prefix) {
        const char prefix_zero = '0';
        const char prefix_base = uppercase ? 'X' : 'x';
        write_factory_character(prefix_zero, stream);
        write_factory_character(prefix_base, stream);
    }
    if (zero) {
        write_padding('0', padding, stream);
    }
    /* Precision uses BGT on the original count, not padding's BPL. */
    for (unsigned remaining = zeros; (int32_t)remaining > 0; --remaining) {
        const char character = '0';
        write_factory_character(character, stream);
    }
    while (length != 0U) {
        --length;
        write_factory_character(reversed[length], stream);
    }
    if (left) {
        write_padding(' ', padding, stream);
    }
}

static void write_factory_fixed(const Dm4310FactoryDecimal *decimal,
                                 unsigned precision, volatile FactoryFormatterStream *stream)
{
    const int first = decimal->exponent > 0 ? decimal->exponent : 0;
    for (int position = first; position >= 0; --position) {
        const int index = decimal->exponent - position;
        const char digit = index >= 0 && (unsigned)index < decimal->count ?
                           decimal->digit[index] : '0';
        write_factory_character(digit, stream);
    }
    if (precision != 0U) {
        const char point = dm4310_runtime_decimal_separator();
        write_factory_character(point, stream);
        for (unsigned position = 1U; position <= precision; ++position) {
            const int index = decimal->exponent + (int)position;
            const char digit = index >= 0 && (unsigned)index < decimal->count ?
                               decimal->digit[index] : '0';
            write_factory_character(digit, stream);
        }
    }
}

static void write_factory_float_field(double value, unsigned precision,
                                      unsigned width, bool zero, bool left,
                                      bool plus, bool space, bool alternate,
                                      volatile FactoryFormatterStream *stream)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    const char sign = (bits >> 63U) != 0U ? '-' :
                      plus ? '+' : space ? ' ' : '\0';
    bits &= UINT64_C(0x7FFFFFFFFFFFFFFF);
    const uint32_t classification = factory_binary64_classify(bits);
    const bool nonfinite = (classification & 2U) != 0U;
    if (nonfinite) {
        factory_stream_nonfinite(stream, 0U, classification, (uint8_t)sign);
        return;
    }
    Dm4310FactoryDecimal finite;
    dm4310_factory_fixed_digits(bits, precision, &finite);
    unsigned payload_length = finite.exponent >= 0 ?
                              (unsigned)finite.exponent + 1U : 1U;
    if (precision != 0U) {
        payload_length += precision + 1U;
    }
    const bool decimal = precision == 0U && alternate;
    const unsigned length = payload_length + (sign != '\0' ? 1U : 0U) +
                            (decimal ? 1U : 0U);
    const unsigned padding = width - length;
    stream->width = padding;
    zero = zero && !left;
    if (!left && !zero) {
        write_padding(' ', padding, stream);
    }
    if (sign != '\0') {
        write_factory_character(sign, stream);
    }
    if (zero) {
        write_padding('0', padding, stream);
    }
    write_factory_fixed(&finite, precision, stream);
    if (decimal) {
        const char point = dm4310_runtime_decimal_separator();
        write_factory_character(point, stream);
    }
    if (left) {
        write_padding(' ', padding, stream);
    }
}

static void write_factory_float(double value, unsigned precision,
                                 volatile FactoryFormatterStream *stream)
{
    write_factory_float_field(value, precision, 0U, false, false,
                              false, false, false, stream);
}

/* Factory 20328 returns the argument footprint, not a character count. */
__attribute__((noipa))
static uint32_t factory_dispatch(volatile FactoryFormatterStream *stream,
                                 uint32_t conversion, const uint32_t *arguments)
{
    if (conversion == 's') {
        factory_stream_string(stream,
            (const char *)(uintptr_t)*(const volatile uint32_t *)arguments,
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
    switch (conversion) {
    case 'd': {
        const int32_t value = (int32_t)*(const volatile uint32_t *)arguments;
        const char sign = value < 0 ? '-' : plus ? '+' : space ? ' ' : '\0';
        const uint32_t magnitude = value < 0 ? 0U - (uint32_t)value :
                                               (uint32_t)value;
        write_factory_integer(magnitude, 10U, sign, width, precision,
                               precision_set, zero, left, false, uppercase, stream);
        return 1U;
    }
    case 'x':
        write_factory_integer(*(const volatile uint32_t *)arguments, 16U, '\0',
                               width, precision, precision_set, zero, left,
                               alternate, uppercase, stream);
        return 1U;
    case 'f': {
        const uintptr_t aligned = ((uintptr_t)arguments + 7U) & ~(uintptr_t)7U;
        double value;
        memcpy(&value, (const void *)aligned, sizeof(value));
        if (width == 0U && !zero && !left && !plus && !space &&
            !alternate && !uppercase) {
            write_factory_float(value, precision, stream);
        } else {
            write_factory_float_field(value, precision, width, zero, left,
                                       plus, space, alternate, stream);
        }
        return 3U;
    }
    default:
        return 0U;
    }
}
#endif

/* GCC's path analyzer does not correlate a custom format parser's switch
 * branch with the corresponding format character at each caller.  The
 * declaration in debug_console.h carries the real printf-format contract,
 * so every call is still checked by -Wformat=2.  Suppress only this known
 * interprocedural false positive inside the parser itself. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-va-arg-type-mismatch"
#endif
#if defined(DAMIAO_DM4310)
/* Factory 0x24438 uses an unsigned subtraction/range test. */
__attribute__((noipa))
static bool factory_decimal_digit(uint32_t value)
{
    return value - UINT32_C(0x30) < UINT32_C(10);
}
#else
#define factory_decimal_digit(value) ((value) >= '0' && (value) <= '9')
#endif

#if defined(DAMIAO_DM4310)
__attribute__((noipa))
static int factory_parse(volatile FactoryFormatterStream *input,
                          va_list arguments)
#else
void debug_console_printf(const char *format, ...)
#endif
{
#if !defined(DAMIAO_DM4310)
    va_list arguments;
    va_start(arguments, format);
#endif

#if defined(DAMIAO_DM4310)
    input->written = 0U;
    uint32_t current = input->read(input);
#define FORMAT_BYTE current
#define FORMAT_NEXT() (current = input->read(input))
#else
    const char *cursor = format;
#define FORMAT_BYTE (*cursor)
#define FORMAT_NEXT() (++cursor)
#endif
    while (FORMAT_BYTE != '\0') {
#if defined(DAMIAO_DM4310)
        /* Factory parser 0x20612..2c consumes/emits one literal at a time,
         * rather than scanning the whole run before the first UART write. */
        while ((FORMAT_BYTE != '\0') && (FORMAT_BYTE != '%')) {
            const char character = (char)FORMAT_BYTE;
            write_factory_character(character, input);
            FORMAT_NEXT();
        }
#else
        const char *literal = cursor;
        while ((FORMAT_BYTE != '\0') && (FORMAT_BYTE != '%')) {
            FORMAT_NEXT();
        }
        platform_debug_write(literal, (size_t)(cursor - literal));
#endif
        if (FORMAT_BYTE == '\0') {
            break;
        }

        FORMAT_NEXT();
#if defined(DAMIAO_DM4310)
        bool zero = false;
        bool left = false;
        bool plus = false;
        bool space = false;
        bool alternate = false;
        while ((FORMAT_BYTE == '0') || (FORMAT_BYTE == '-') ||
               (FORMAT_BYTE == '+') || (FORMAT_BYTE == ' ') || (FORMAT_BYTE == '#')) {
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
#endif
        unsigned width = 0U;
#if defined(DAMIAO_DM4310)
        if (FORMAT_BYTE == '*') {
            const int requested = va_arg(arguments, int);
            /* Factory 0x20678..82 stores the raw argument; its signed
             * normalization occurs only after precision parsing. */
            width = (unsigned)requested;
            input->width = width;
            FORMAT_NEXT();
        } else
#endif
        {
            while (factory_decimal_digit((uint8_t)FORMAT_BYTE)) {
                width = width * 10U + (unsigned)(FORMAT_BYTE - '0');
#if defined(DAMIAO_DM4310)
                input->width = width;
#endif
                FORMAT_NEXT();
            }
        }
        unsigned precision = 6U;
        if (FORMAT_BYTE == '.') {
#if defined(DAMIAO_DM4310)
            precision_set = true;
#endif
            precision = 0U;
            FORMAT_NEXT();
#if defined(DAMIAO_DM4310)
            if (FORMAT_BYTE == '*') {
                const int requested = va_arg(arguments, int);
                input->precision = (uint32_t)requested;
                precision_set = requested >= 0;
                precision = requested >= 0 ? (unsigned)requested : 6U;
                FORMAT_NEXT();
            } else
#endif
            {
                while (factory_decimal_digit((uint8_t)FORMAT_BYTE)) {
                    precision = precision * 10U + (unsigned)(FORMAT_BYTE - '0');
#if defined(DAMIAO_DM4310)
                    input->precision = precision;
#endif
                    FORMAT_NEXT();
                }
            }
        }

#if defined(DAMIAO_DM4310)
        /* Factory 0x206d6..e2 normalizes the signed width word after
         * parsing, including a decimal width that wraps into bit 31. */
        if ((int32_t)width < 0) {
            width = 0U - width;
            left = true;
            input->width = width;
        }
        if (left) zero = false;
        const bool uppercase = FORMAT_BYTE >= 'A' && FORMAT_BYTE <= 'Z';
        const char conversion = uppercase ? (char)(FORMAT_BYTE + ('a' - 'A')) :
                                           (char)FORMAT_BYTE;
        input->flags = (left ? 1U : 0U) | (plus ? 2U : 0U) |
                      (space && !plus ? 4U : 0U) | (alternate ? 8U : 0U) |
                      (zero ? 16U : 0U) | (precision_set ? 32U : 0U) |
                      (uppercase ? 2048U : 0U);
        _Static_assert(sizeof(va_list) == sizeof(uintptr_t),
                       "Factory parser requires ARM word-sized va_list");
        uintptr_t argument_cursor;
        memcpy(&argument_cursor, &arguments, sizeof(argument_cursor));
        const uint32_t consumed = factory_dispatch(input, (uint8_t)conversion,
            (const uint32_t *)argument_cursor);
        if (consumed == 0U) {
            if (FORMAT_BYTE != '\0') write_factory_character(conversion, input);
        } else {
            argument_cursor = consumed == 1U ? argument_cursor + 4U :
                              ((argument_cursor + 7U) & ~(uintptr_t)7U) + 8U;
            memcpy(&arguments, &argument_cursor, sizeof(argument_cursor));
        }
#else
        switch (FORMAT_BYTE) {
        case 'd': {
            const int value = va_arg(arguments, int);
            if (value < 0) {
                static const char minus = '-';
                platform_debug_write(&minus, 1U);
                write_unsigned_digits((uint32_t)(-(int64_t)value), 10U, width);
            } else {
                write_unsigned_digits((uint32_t)value, 10U, width);
            }
            break;
        }
        case 'x':
            write_unsigned_digits(va_arg(arguments, unsigned int), 16U, width);
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
#endif
        if (FORMAT_BYTE != '\0') {
            FORMAT_NEXT();
        }
    }
#if defined(DAMIAO_DM4310)
    return (int32_t)input->written;
#else
    va_end(arguments);
#endif
}
#undef FORMAT_BYTE
#undef FORMAT_NEXT
#if defined(DAMIAO_DM4310)
__attribute__((noipa))
static int factory_format_context(const char *format, void *output,
                                  va_list arguments,
                                  void (*write)(uint32_t, void *))
{
    FactoryFormatterStream stream;
    stream.write = write;
    stream.context = output;
    stream.pending = 0U;
    stream.read = factory_stream_read;
    stream.next = format;
    return factory_parse(&stream, arguments);
}

__attribute__((noipa))
static int factory_vprintf(const char *format, const volatile void *output,
                            va_list arguments)
{
    const int written = factory_format_context(format,
        (void *)(uintptr_t)output, arguments, factory_stream_uart);
    /* Factory 20de0 tests stream +12 only after all output. */
    return dm4310_runtime_stream_error(output) != 0U ? -1 : written;
}

int debug_console_printf(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    const int written = factory_vprintf(format,
        (const volatile void *)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa670), UINT32_C(0x1fffa6a8)), arguments);
    va_end(arguments);
    return written;
}
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

static void write_motor_parameters(void)
{
#if defined(DAMIAO_DM4310)
    volatile uint8_t *const response = short_response_buffer;
    volatile const uint32_t *const parameters =
        (volatile const uint32_t *)app_config_staging_record();
    response[0] = 'e';
    *(volatile uint32_t *)(uintptr_t)(response + 1U) = parameters[0x11U];
    *(volatile uint32_t *)(uintptr_t)(response + 5U) = parameters[0x12U];
    *(volatile uint32_t *)(uintptr_t)(response + 9U) = parameters[0x13U];
#else
    uint8_t response[13] = {'e'};
    response[0] = 'e';
    memcpy(&response[1], &g_app.config.phase_resistance, sizeof(float));
    memcpy(&response[5], &g_app.config.phase_inductance, sizeof(float));
    memcpy(&response[9], &g_app.config.flux_linkage, sizeof(float));
#endif
    platform_debug_write((const void *)(uintptr_t)response, 13U);
}

static void write_device_identity(void)
{
    uint32_t device_id;
    uint32_t application_identity;
    platform_read_device_identity(&device_id, &application_identity);
#if defined(DAMIAO_DM4310)
    volatile uint8_t *const response = short_response_buffer;
#else
    uint8_t response[9] = {'f'};
#endif
    response[0] = 'f';
#if defined(DAMIAO_DM4310)
    /* Uf writes the fixed response scratch in tag/device/application order.
     * Volatile word stores keep GCC from moving the device word ahead of the
     * tag byte when this target is built for size. */
    *(volatile uint32_t *)(uintptr_t)(response + 1U) = device_id;
    *(volatile uint32_t *)(uintptr_t)(response + 5U) = application_identity;
#else
    memcpy(&response[1], &device_id, sizeof(device_id));
    memcpy(&response[5], &application_identity,
           sizeof(application_identity));
#endif
    platform_debug_write((const void *)(uintptr_t)response, 9U);
    /* The recovered Uf query returns to the top-level menu. */
    CONSOLE_MODE_SET(DEBUG_MODE_MENU);
}

static void process_frame(const uint8_t *data, int32_t length)
{
    if (data == NULL) {
        return;
    }
#if !defined(DAMIAO_DM4310)
    if (length == 0) {
        return;
    }
#endif
    const uint8_t command = data[0];

    /* The framing and state transitions below are reconstructed
     * from debug_uart_receive_irq at 0x22244. USART receiver-timeout IRQ004
     * provides exactly one complete command burst to this function. */
    if (command == 0x1BU) {
        CONSOLE_MODE_SET(DEBUG_MODE_MENU);
#if defined(DAMIAO_DM4310)
        /* The factory handler writes the fixed mode once, independently of
         * source-only mirror state. */
        g_app.motor.armed = false;
#else
        if (g_app.motor.armed) {
            motor_control_disarm(&g_app.motor);
        }
#endif
#if defined(DAMIAO_DM4310)
        if (motor_control_runtime_fault() == MOTOR_STATUS_ENABLED) {
            motor_control_set_runtime_fault(MOTOR_FAULT_NONE);
            g_app.motor.feedback.fault = MOTOR_FAULT_NONE;
        }
#else
        if (g_app.motor.feedback.fault == MOTOR_STATUS_ENABLED) {
            g_app.motor.feedback.fault = MOTOR_FAULT_NONE;
        }
#endif
        POST_MOTOR_STATE_CHANGE();
        return;
    }

    if ((length == 1U) && (command == 'X')) {
        /* Vector_20 calls 0x26e04 first.  That helper writes the boot record
         * to (update_requested=1, application_confirmed=0), commits it, and
         * only then performs the 100 us AIRCR reset sequence. */
        platform_enter_bootloader();
        return;
    }

    const DebugConsoleMode mode = CONSOLE_MODE_GET();
    if (mode == DEBUG_MODE_MENU) {
        if (command == 'm') {
#if defined(DAMIAO_DM4310)
            if ((uint32_t)motor_control_runtime_fault() < 2U) {
#else
            if (g_app.motor.feedback.fault < 2U) {
#endif
                CONSOLE_MODE_SET(DEBUG_MODE_MOTOR);
#if defined(DAMIAO_DM4310)
                /* CONSOLE_MODE_SET above is the one recovered fixed-SRAM
                 * write; keep only the source-level mirror here. */
                g_app.motor.armed = true;
#else
                motor_control_arm(&g_app.motor);
#endif
                g_app.motor.feedback.fault = MOTOR_STATUS_ENABLED;
#if defined(DAMIAO_DM4310)
                motor_control_set_runtime_fault(MOTOR_STATUS_ENABLED);
#endif
                POST_MOTOR_STATE_CHANGE();
            }
        } else if (command == 's') {
            CONSOLE_MODE_SET(DEBUG_MODE_SETUP);
        }
        return;
    }

    if (mode != DEBUG_MODE_SETUP) {
        return;
    }

    /* Vector_20 loads bytes 1 and 2 together at 0x2231a before dispatching
     * either the U family or FCB.  Retain both values across the branches. */
    const uint8_t subcommand = data[1];
    const uint8_t argument = data[2];

    /* Uc/Ul return before the factory's expected-length halfword read. */
    if (
#if !defined(DAMIAO_DM4310)
        (length >= 2) &&
#endif
        (command == 'U') && (subcommand == 'c')) {
        APP_DEFERRED_EVENTS.commission_direction = true;
        return;
    }
    if (
#if !defined(DAMIAO_DM4310)
        (length >= 2) &&
#endif
        (command == 'U') && (subcommand == 'l')) {
        APP_DEFERRED_EVENTS.commission_position_sensor = true;
        return;
    }

#if defined(DAMIAO_DM4310)
    /* 0x22344 reads this for every remaining U subtype, not only Ud/UM. */
    const int32_t expected_frame_length = command == 'U' ?
        (int32_t)board_uart_expected_payload_length() + 1 : 0;
    if ((length == expected_frame_length) && (command == 'U') &&
#else
    if ((length >= 64) && (command == 'U') &&
#endif
        ((subcommand == 'd') || (subcommand == 'M'))) {
#if defined(DAMIAO_DM4310)
        uint8_t *const acknowledgement = short_response_buffer;
#else
        uint8_t acknowledgement[2];
#endif
        CalibrationUploadKind completed;
#if defined(DAMIAO_DM4310)
        if (calibration_upload_receive_frame_irq(data, (size_t)length,
                                     subcommand,
#else
        if (calibration_upload_receive_frame(data, (size_t)length,
#endif
                                     acknowledgement,
                                     &completed)) {
            platform_debug_write(acknowledgement,
                                2U);

#if defined(DAMIAO_DM4310)
            completed = calibration_upload_finish_frame_irq(subcommand);
            if (completed != CALIBRATION_UPLOAD_NONE) {
                APP_DEFERRED_EVENTS.calibration_commit_request =
                    (uint8_t)completed;
                if (completed == CALIBRATION_UPLOAD_MOTOR_ENCODER) {
                    const float direction =
                        *(volatile const float *)(uintptr_t)
                            FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff0bc), UINT32_C(0x1ffff048));
                    calibration_upload_set_motor_direction(direction);
                }
            }
#else
            if (completed == CALIBRATION_UPLOAD_MOTOR_ENCODER) {
                calibration_upload_set_motor_direction(
                    g_app.config.direction);
            }

            if (completed != CALIBRATION_UPLOAD_NONE) {
                APP_DEFERRED_EVENTS.calibration_commit_request =
                    (uint8_t)completed;
            }
#endif
        }

        return;
    }

    if ((command == 'U') && (subcommand == 'e')) {
        if ((argument == 0xAAU) && (length == 3)) {
            write_motor_parameters();
        }
        /* 0x2242e reloads RX byte 2 after the optional parameter reply. */
        if ((data[2] == 'U') &&
#if defined(DAMIAO_DM4310)
            (board_uart_received_length() == 3U)
#else
            (length == 3)
#endif
            ) {
            APP_DEFERRED_EVENTS.identify_motor = true;
        }
        return;
    }

    if ((length == 3U) && (command == 'U') &&
        (subcommand == 'f') && (argument == 0xAAU)) {
        write_device_identity();
        return;
    }

    if ((length == 3U) && (command == 'U') &&
        (subcommand == 'g') && (argument == 0xAAU)) {
        APP_DEFERRED_EVENTS.firmware_control_request = 1U;
        return;
    }

    if ((length == 131U) && (command == 'U') && (subcommand == 'g')) {
        const uint8_t subtype = argument;
        if ((subtype != 'U') && (subtype != 'Z') && (subtype != 'Q')) {
            return;
        }
#if defined(DAMIAO_DM4310)
        /* Vector_20 overwrites the first 0x80 bytes of the live 37-word
         * staging record before posting the UgU/UgZ/UgQ request. */
        dm4310_runtime_copy_bytes(app_config_staging_record(), &data[3],
               sizeof(g_app.firmware_control_payload));
#else
        memcpy(g_app.firmware_control_payload, &data[3],
               sizeof(g_app.firmware_control_payload));
        g_app.firmware_control_payload_valid = true;
#endif
        if (subtype == 'U') {
            APP_DEFERRED_EVENTS.firmware_control_request = 2U;
        } else if (subtype == 'Z') {
            APP_DEFERRED_EVENTS.firmware_control_request = 4U;
        } else {
            APP_DEFERRED_EVENTS.firmware_control_request = 6U;
        }
        return;
    }

    if (
#if !defined(DAMIAO_DM4310)
        (length >= 4) &&
#endif
        (command == 'F') && (subcommand == 'C') &&
        (argument == 'B')) {
        /* Vector_20 performs byte-'0', UXTB, then an unsigned <= 11 test.
         * Consequently selectors 10 and 11 are encoded as ':' and ';'. */
        const uint8_t selector = (uint8_t)(data[3] - '0');
        if (selector > 11U) {
            return;
        }

        g_app.pending_store_response_valid = false;
#if defined(DAMIAO_DM4310)
        /* The factory staging record is also its live configuration object;
         * keep the source-level decoded view synchronized with word 0x23. */
        app_config_staging_record()[0x23] = selector;
        g_app.config.can_data_rate_selector = selector;
        platform_select_mcan_transport_format(selector);
        APP_DEFERRED_EVENTS.save_staged_parameters = true;
#else
        g_app.config.can_data_rate_selector = selector;
        g_app.events.save_parameters = true;
#endif
    }
}

void debug_console_reset(void)
{
    frame_length = 0U;
#if !defined(DAMIAO_DM4310)
    CONSOLE_MODE_SET(DEBUG_MODE_MENU);
#endif
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
    process_frame(frame_buffer, (int32_t)frame_length);
    frame_length = 0U;
}

#if defined(DAMIAO_DM4310)
void debug_console_process_dma_frame(const uint8_t *data, int16_t length)
{
    process_frame(data, (int32_t)length);
}
#endif

void debug_console_receive_frame(const uint8_t *data, size_t length)
{
#if defined(DAMIAO_DM4310)
    if ((data == NULL) || (length == 0U)) {
        return;
    }
    if (length > sizeof(frame_buffer)) {
        length = sizeof(frame_buffer);
    }
    /* Model the production DMA buffer: only received bytes are overwritten;
     * the unused tail deliberately retains the preceding frame. */
    memcpy(frame_buffer, data, length);
    process_frame(frame_buffer, (int32_t)length);
#else
    process_frame(data, (int32_t)length);
#endif
}

void debug_console_print_banner(void)
{
    write_text("\n\r"
               " Commands:\n\r"
               " m - Motor Mode\n\r"
               " s - Setup Mode\n\r"
               " esc - Exit to Menu\n\r");
}

#if defined(DAMIAO_DM4310)
/* Keep literal diagnostics on the same formatter path as factory 0x26758. */
#define write_status_text debug_console_printf
#else
#define write_status_text write_text
#endif

void debug_console_print_status(void)
{
#if defined(DAMIAO_DM4310)
    /* Factory 0x26758 retains the register-derived rate before UART output;
     * 0x268cc selects units by signed float bits, not the config selector. */
    const float data_rate_kbps = platform_read_mcan_data_rate_kbps();
    int32_t data_rate_bits;
    memcpy(&data_rate_bits, &data_rate_kbps, sizeof(data_rate_bits));
#endif
    write_status_text("DMBOT Motor Driver");
    switch (platform_read_hardware_variant()) {
    case 0U: write_status_text("--V2.0"); break;
    case 1U: write_status_text("--V3.0"); break;
    case 2U: write_status_text("--V4.0"); break;
    case 3U: write_status_text("--V1.0"); break;
    default: break;
    }
    write_status_text("\n\r Debug Info:\n\r");
    debug_console_printf("Firmware Version: %d\r\n",
                         APP_PROFILE_FIRMWARE_VERSION_LITERAL);
    debug_console_printf("Sub Version: %03d\r\n",
                         APP_PROFILE_FIRMWARE_SUBVERSION_LITERAL);
#if defined(DAMIAO_DM4310)
    debug_console_printf("Imax: %f\r\n",
                         (double)APP_PROFILE_CURRENT_FULL_SCALE_A);
#else
    debug_console_printf("Imax: %f\r\n",
                         (double)g_app.config.maximum_phase_current);
#endif
#if defined(DAMIAO_DM4310)
    /* Each operand is loaded at its own print, as at 0x26838..8c2. */
    debug_console_printf(" I_U Offset:     %.4f\r\n",
        (double)*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff144), UINT32_C(0x1ffff0d0)));
    debug_console_printf(" I_V Offset:     %.4f\r\n",
        (double)*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff148), UINT32_C(0x1ffff0d4)));
    debug_console_printf(" I_W Offset:     %.4f\r\n",
        (double)*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff14c), UINT32_C(0x1ffff0d8)));
    debug_console_printf(
        " Position Sensor Electrical Offset:   %.4f\n\r",
        (double)*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff09c), UINT32_C(0x1ffff028)));
    debug_console_printf(" Mechanical Offset:   %.4f\n\r",
        (double)*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff0ac), UINT32_C(0x1ffff038)));
    debug_console_printf(" Output Position:  %.4f\n\r",
        (double)*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff0a0), UINT32_C(0x1ffff02c)));
    debug_console_printf(" CAN ID:     0x%03x\n\r",
        (unsigned int)*(volatile const uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa5e8), UINT32_C(0x1fffa578)));
    debug_console_printf(" MASTER ID:  0x%03x\n\r",
        (unsigned int)*(volatile const uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa5e4), UINT32_C(0x1fffa574)));
#else
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
                         (double)g_app.position);
    debug_console_printf(" CAN ID:     0x%03x\n\r",
                         (unsigned int)g_app.config.can_id);
    debug_console_printf(" MASTER ID:  0x%03x\n\r",
                         (unsigned int)g_app.config.master_id);
#endif
#if defined(DAMIAO_DM4310)
    if (data_rate_bits < INT32_C(0x447a0000)) {
#else
    if (g_app.config.can_data_rate_selector < 4U) {
#endif
#if defined(DAMIAO_DM4310)
        debug_console_printf(
            " CAN Baud: %dKbps\n\r",
            (int)(uint32_t)data_rate_kbps);
#else
        static const uint16_t classic_kbps[] = {125U, 200U, 250U, 500U};
        debug_console_printf(
            " CAN Baud: %dKbps\n\r",
            (int)classic_kbps[g_app.config.can_data_rate_selector]);
#endif
    } else {
#if defined(DAMIAO_DM4310)
        debug_console_printf(
            " CAN Baud: %.2fMbps\n\r",
            (double)(data_rate_kbps *
                     0x1.0624dep-10f));
#else
        debug_console_printf(
            " CAN Baud: %.2fMbps\n\r",
            (double)can_data_rate_mbps(g_app.config.can_data_rate_selector));
#endif
    }
    write_status_text("\n\r Motor Info:\n\r");
#if defined(DAMIAO_DM4310)
    debug_console_printf(" Rs  = %.4f m\xA6\xB8\n\r",
        (double)(*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa60c), UINT32_C(0x1fffa59c)) * 1000.0f));
    debug_console_printf(" Ls  = %.4f \xA6\xCC" "H\n\r",
        (double)(*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa610), UINT32_C(0x1fffa5a0)) * 1000000.0f));
    debug_console_printf(" \xA6\xB7" "f = %.4f Wb\n\r",
        (double)*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffa614), UINT32_C(0x1fffa5a4)));
    debug_console_printf("V_BUS=%.4f\r\n",
        (double)*(volatile const float *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff16c), UINT32_C(0x1ffff0f8)));
#else
    debug_console_printf(" Rs  = %.4f m\xA6\xB8\n\r",
                         (double)(g_app.config.phase_resistance * 1000.0f));
    debug_console_printf(" Ls  = %.4f \xA6\xCC" "H\n\r",
                         (double)(g_app.config.phase_inductance * 1000000.0f));
    debug_console_printf(" \xA6\xB7" "f = %.4f Wb\n\r",
                         (double)g_app.config.flux_linkage);
    debug_console_printf("V_BUS=%.4f\r\n",
                         (double)g_app.motor.feedback.bus_voltage);
#endif
    write_status_text("\n\r Control Mode : \r\n");
#if defined(DAMIAO_DM4310)
    /* Four independent mode reads/arms, not four per-line selectors. */
    volatile const uint32_t *const mode =
        (volatile const uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff140), UINT32_C(0x1ffff0cc));
    if (*mode == 1U) {
        write_status_text("1:MIT Mode <----\n\r");
        write_status_text("2:position-speed cascade Mode\n\r");
        write_status_text("3:speed Mode\n\r");
        write_status_text("4:Hybrid control Mode\n\r");
    }
    if (*mode == 2U) {
        write_status_text("1:MIT Mode\n\r");
        write_status_text("2:position-speed cascade Mode <----\n\r");
        write_status_text("3:speed Mode\n\r");
        write_status_text("4:Hybrid control Mode\n\r");
    }
    if (*mode == 3U) {
        write_status_text("1:MIT Mode\n\r");
        write_status_text("2:position-speed cascade Mode\n\r");
        write_status_text("3:speed Mode <----\n\r");
        write_status_text("4:Hybrid control Mode\n\r");
    }
    if (*mode == 4U) {
        write_status_text("1:MIT Mode\n\r");
        write_status_text("2:position-speed cascade Mode\n\r");
        write_status_text("3:speed Mode\n\r");
        write_status_text("4:Hybrid control Mode <----\n\r");
    }
#else
    write_status_text(g_app.config.control_mode == MOTOR_MODE_MIT ?
                   "1:MIT Mode <----\n\r" : "1:MIT Mode\n\r");
    write_status_text(g_app.config.control_mode == MOTOR_MODE_POSITION_SPEED ?
                   "2:position-speed cascade Mode <----\n\r" :
                   "2:position-speed cascade Mode\n\r");
    write_status_text(g_app.config.control_mode == MOTOR_MODE_SPEED ?
                   "3:speed Mode <----\n\r" : "3:speed Mode\n\r");
    write_status_text(g_app.config.control_mode == MOTOR_MODE_HYBRID ?
                   "4:Hybrid control Mode <----\n\r" :
                   "4:Hybrid control Mode\n\r");
#endif
}
#undef write_status_text
