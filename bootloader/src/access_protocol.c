#include "access_protocol.h"

#include <string.h>

#define UPDATE_CAN_ID 0x7ffU

static const uint8_t access_suffix[37] = {
    'm','x','p','s','h','e','n','z','h','e','n','s','h','i','d','a','m','i',
    'a','o','k','e','j','i','y','o','u','x','i','a','n','g','o','n','g','s','i'
};

static bool command_matches(const uint8_t *command, size_t length,
                            uint8_t prefix0, uint8_t prefix1)
{
    return (length == BOOT_ACCESS_UART_COMMAND_LENGTH) &&
           (command[0] == prefix0) && (command[1] == prefix1) &&
           (memcmp(&command[2], access_suffix, sizeof(access_suffix)) == 0);
}

static bool apply_command(const uint8_t *command, size_t length)
{
    bool disabled;
    const char *response;
    if (command_matches(command, length, 0x00U, 0x01U)) {
        disabled = true;
        response = "Disable SWD! \r\n";
    } else if (command_matches(command, length, 0xffU, 0xfeU)) {
        disabled = false;
        response = "Enable SWD!  \r\n";
    } else {
        return false;
    }

    if (boot_platform_set_swd_disabled(disabled)) {
        boot_platform_debug_write(response, 15U);
    } else {
        static const char error[] = "SWD Flash Error\r\n";
        boot_platform_debug_write(error, sizeof(error) - 1U);
    }
    return true;
}

void boot_access_init(BootAccessSession *session)
{
    if (session != NULL) {
        memset(session, 0, sizeof(*session));
    }
}

bool boot_access_accept_can(const BootCanFrame *frame)
{
    if ((frame == NULL) || (frame->id != UPDATE_CAN_ID) ||
        (frame->length != 8U)) {
        return false;
    }

    uint8_t command[BOOT_ACCESS_UART_COMMAND_LENGTH];
    memcpy(command, frame->data, frame->length);
    memcpy(&command[frame->length], &access_suffix[frame->length - 2U],
           sizeof(command) - frame->length);
    return apply_command(command, sizeof(command));
}

bool boot_access_accept_uart(BootAccessSession *session, uint8_t byte)
{
    if (session == NULL) {
        return false;
    }

    if (session->received < sizeof(session->window)) {
        session->window[session->received++] = byte;
    } else {
        memmove(session->window, &session->window[1],
                sizeof(session->window) - 1U);
        session->window[sizeof(session->window) - 1U] = byte;
    }

    if ((session->received == sizeof(session->window)) &&
        apply_command(session->window, sizeof(session->window))) {
        session->received = 0U;
        return true;
    }
    return false;
}
