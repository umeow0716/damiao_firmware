#ifndef DM4310_UPDATE_PROTOCOL_H
#define DM4310_UPDATE_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "boot_platform.h"
#include "flash_writer.h"
#include "aes256_ctr.h"

#define BOOT_UPDATE_MAX_PACKET 0x2000U
#define BOOT_UPDATE_INACTIVITY_TIMEOUT_MS 1510U

typedef enum {
    BOOT_UPDATE_WAIT_HEADER = 0,
    BOOT_UPDATE_RECEIVE_DATA,
    BOOT_UPDATE_COMPLETE,
    BOOT_UPDATE_ERROR,
} BootUpdateState;

typedef struct {
    BootUpdateState state;
    FlashWriter writer;
    /* Three spare bytes allow a legal non-word-sized final packet to be
     * padded with erased bytes before HC32 word programming. */
    uint8_t packet[BOOT_UPDATE_MAX_PACKET + 3U];
    uint16_t packet_length;
    uint16_t packet_received;
    Aes256Ctr cipher;
    uint8_t current_sequence;
    uint8_t expected_sequence;
    uint8_t packets_written;
    uint32_t inactivity_ms;
    bool crc_pending;
    bool sequence_started;
    bool cipher_started;
} BootUpdateSession;

void boot_update_init(BootUpdateSession *session);
void boot_update_accept(BootUpdateSession *session, const BootCanFrame *frame);
void boot_update_advance_time(BootUpdateSession *session, uint32_t elapsed_ms);
bool boot_update_is_complete(const BootUpdateSession *session);
bool boot_update_has_started(const BootUpdateSession *session);

#endif
