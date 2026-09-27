#ifndef DM4310_ACCESS_PROTOCOL_H
#define DM4310_ACCESS_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "boot_platform.h"

#define BOOT_ACCESS_UART_COMMAND_LENGTH 39U

typedef struct {
    uint8_t window[BOOT_ACCESS_UART_COMMAND_LENGTH];
    size_t received;
} BootAccessSession;

void boot_access_init(BootAccessSession *session);
bool boot_access_accept_can(const BootCanFrame *frame);
bool boot_access_accept_uart(BootAccessSession *session, uint8_t byte);

#endif
