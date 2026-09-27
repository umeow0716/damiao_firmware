#include "update_protocol.h"

#include <string.h>

#include "app_image.h"
#include "crc8_maxim.h"
#include "update_profile.h"

#define UPDATE_CAN_ID 0x7ffU
#define STATUS_CAN_ID 0x7feU
#define UPDATE_MAX_PACKETS \
    ((DM4310_APP_END - DM4310_APP_BASE) / BOOT_UPDATE_MAX_PACKET)

static void send_status(const uint8_t text[8])
{
    boot_platform_transmit(STATUS_CAN_ID, text, 8U);
}

static void fail_fatal(BootUpdateSession *session, const char text[8])
{
    session->state = BOOT_UPDATE_ERROR;
    send_status((const uint8_t *)text);
}

static void reject_packet(BootUpdateSession *session, const char text[8])
{
    /* The recovered loader clears its active packet/CRC counters after
     * CRCERROR and TimERROR, then accepts the same descending sequence again.
     * Keep the already committed sectors and AES counter: neither advances
     * until a complete packet has passed CRC. */
    session->packet_length = 0U;
    session->packet_received = 0U;
    session->crc_pending = false;
    session->inactivity_ms = 0U;
    session->state = BOOT_UPDATE_WAIT_HEADER;
    send_status((const uint8_t *)text);
}

void boot_update_init(BootUpdateSession *session)
{
    memset(session, 0, sizeof(*session));
    session->state = BOOT_UPDATE_WAIT_HEADER;
    flash_writer_init(&session->writer);
}

static bool accept_header(BootUpdateSession *session, const BootCanFrame *frame)
{
    if ((frame->length < 5U) || (frame->length > 8U) ||
        (frame->data[0] != '#') || (frame->data[2] != '#')) {
        return false;
    }

    const uint8_t sequence = frame->data[1];
    const uint16_t length = (uint16_t)frame->data[3] |
                            ((uint16_t)frame->data[4] << 8U);
    if ((length == 0U) || (length > BOOT_UPDATE_MAX_PACKET) ||
        ((sequence != 0U) && (length != BOOT_UPDATE_MAX_PACKET))) {
        reject_packet(session, "TimERROR");
        return false;
    }
    /* The first sequence equals packet_count - 1.  Reject an oversized image
     * before journaling or erasing so an update can never reach the recovered
     * configuration sector at 0x30000. */
    if (!session->sequence_started && (sequence >= UPDATE_MAX_PACKETS)) {
        reject_packet(session, "TimERROR");
        return false;
    }
    if (session->sequence_started && (sequence != session->expected_sequence)) {
        reject_packet(session, "TimERROR");
        return false;
    }
    if (!session->sequence_started) {
        if (!flash_writer_begin(&session->writer)) {
            fail_fatal(session, "EFMERROR");
            return false;
        }
        aes256_ctr_init(&session->cipher, dm4310_update_key,
                        dm4310_update_initial_counter);
        session->cipher_started = true;
        session->sequence_started = true;
    }

    session->current_sequence = sequence;
    session->packet_length = length;
    session->packet_received = 0U;
    session->crc_pending = true;
    session->state = BOOT_UPDATE_RECEIVE_DATA;
    return true;
}

static void finish_packet(BootUpdateSession *session, uint8_t received_crc)
{
    if (received_crc !=
        dm4310_crc8_maxim(session->packet, session->packet_length)) {
        reject_packet(session, "CRCERROR");
        return;
    }
    aes256_ctr_transform(&session->cipher, session->packet,
                         session->packet_length);
    const size_t storage_length =
        ((size_t)session->packet_length + 3U) & ~(size_t)3U;
    memset(&session->packet[session->packet_length], 0xffU,
           storage_length - session->packet_length);
    if (!flash_writer_append(&session->writer, session->packet,
                             session->packet_length, storage_length)) {
        fail_fatal(session, "EFMERROR");
        return;
    }

    ++session->packets_written;
    uint8_t ok[8] = {' ', '0', 't', 'h', ' ', 'O', 'K', '!'};
    if (session->packets_written >= 10U) {
        ok[0] = (uint8_t)('0' + (session->packets_written / 10U));
    }
    ok[1] = (uint8_t)('0' + (session->packets_written % 10U));
    send_status(ok);

    session->crc_pending = false;
    if (session->current_sequence == 0U) {
        /* main validates that Reset_Handler falls inside bytes_written before
         * it emits the final "complete" acknowledgement. */
        session->state = BOOT_UPDATE_COMPLETE;
    } else {
        session->expected_sequence = (uint8_t)(session->current_sequence - 1U);
        session->state = BOOT_UPDATE_WAIT_HEADER;
    }
}

void boot_update_accept(BootUpdateSession *session, const BootCanFrame *frame)
{
    if ((session == NULL) || (frame == NULL) || (frame->id != UPDATE_CAN_ID) ||
        (frame->length == 0U) || (frame->length > 8U) ||
        (session->state == BOOT_UPDATE_COMPLETE) ||
        (session->state == BOOT_UPDATE_ERROR)) {
        return;
    }

    /* The recovered loader aborts an active transfer after its 10 ms counter
     * advances past 0x96.  Treat traffic belonging to the update stream as
     * activity so host-side pacing and flash-program time cannot consume the
     * whole timeout budget. */
    if (session->sequence_started) {
        session->inactivity_ms = 0U;
    }

    uint8_t index = 0U;
    if (session->state == BOOT_UPDATE_WAIT_HEADER) {
        if (!accept_header(session, frame)) {
            return;
        }
        index = 5U;
    }
    while (index < frame->length) {
        if (session->packet_received < session->packet_length) {
            session->packet[session->packet_received++] = frame->data[index++];
        } else if (session->crc_pending) {
            finish_packet(session, frame->data[index++]);
            if (index != frame->length) {
                fail_fatal(session, "TimERROR");
            }
            return;
        } else {
            fail_fatal(session, "TimERROR");
            return;
        }
    }
}

void boot_update_advance_time(BootUpdateSession *session, uint32_t elapsed_ms)
{
    if ((session == NULL) || !session->sequence_started ||
        (session->state == BOOT_UPDATE_COMPLETE) ||
        (session->state == BOOT_UPDATE_ERROR)) {
        return;
    }

    if (elapsed_ms >= (BOOT_UPDATE_INACTIVITY_TIMEOUT_MS -
                       session->inactivity_ms)) {
        session->inactivity_ms = BOOT_UPDATE_INACTIVITY_TIMEOUT_MS;
        reject_packet(session, "TimERROR");
    } else {
        session->inactivity_ms += elapsed_ms;
    }
}

bool boot_update_is_complete(const BootUpdateSession *session)
{
    return session->state == BOOT_UPDATE_COMPLETE;
}

bool boot_update_has_started(const BootUpdateSession *session)
{
    return (session != NULL) && session->sequence_started;
}
