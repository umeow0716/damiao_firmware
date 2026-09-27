#ifndef DM4310_BOOT_SERVICE_PROTOCOL_H
#define DM4310_BOOT_SERVICE_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "boot_platform.h"

typedef struct {
    uint16_t node_id;
    uint16_t version_word;
    bool launch_requested;
} BootServiceSession;

void boot_service_init(BootServiceSession *session, uint16_t node_id,
                       uint16_t version_word);
bool boot_service_accept(BootServiceSession *session,
                         const BootCanFrame *frame);
bool boot_service_launch_requested(const BootServiceSession *session);

#endif
