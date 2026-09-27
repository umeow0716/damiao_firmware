#include "service_protocol.h"

#include <stddef.h>

#define BOOT_SERVICE_CAN_ID 0x7ffU
#define BOOT_STATUS_CAN_ID  0x7feU

static bool frame_is(const BootCanFrame *frame, uint8_t byte0,
                     uint8_t byte1, uint8_t byte2, uint8_t byte3)
{
    return (frame != NULL) && (frame->id == BOOT_SERVICE_CAN_ID) &&
           (frame->length == 8U) && (frame->data[0] == byte0) &&
           (frame->data[1] == byte1) && (frame->data[2] == byte2) &&
           (frame->data[3] == byte3);
}

void boot_service_init(BootServiceSession *session, uint16_t node_id,
                       uint16_t version_word)
{
    if (session == NULL) {
        return;
    }
    session->node_id = node_id;
    session->version_word = version_word;
    session->launch_requested = false;
}

bool boot_service_accept(BootServiceSession *session,
                         const BootCanFrame *frame)
{
    if ((session == NULL) || (frame == NULL)) {
        return false;
    }

    /* firmware_update_frame_handler@0x6608 answers this broadcast with the
     * loader's configured node ID.  The historical reply deliberately uses
     * four bytes even though the surrounding update transport uses DLC 8. */
    if (frame_is(frame, 0x55U, 0x00U, 0x00U, 0xaaU)) {
        const uint8_t reply[4] = {
            0x55U, (uint8_t)session->node_id, 0x00U, 0xaaU,
        };
        boot_platform_transmit(BOOT_SERVICE_CAN_ID, reply, sizeof(reply));
        return true;
    }

    /* The version bytes are encoded exactly as in the reference: each byte
     * of boot-record word 3 has ASCII '0' added independently. */
    if (frame_is(frame, 0xeeU, 0x00U, 0x00U, 0x11U) &&
        (frame->data[4] == 0U) && (frame->data[5] == 0U) &&
        (frame->data[6] == 0U) && (frame->data[7] == 0U)) {
        const uint8_t reply[4] = {
            0x11U,
            (uint8_t)((uint8_t)session->version_word + (uint8_t)'0'),
            (uint8_t)((uint8_t)(session->version_word >> 8U) +
                      (uint8_t)'0'),
            0xeeU,
        };
        boot_platform_transmit(BOOT_STATUS_CAN_ID, reply, sizeof(reply));
        return true;
    }

    /* U 01 02 AA <node-le16> is the recovered host request to leave the
     * loader.  main performs image validation and the persistent/reset
     * sequence; the parser only authenticates the addressed node. */
    if (frame_is(frame, 0x55U, 0x01U, 0x02U, 0xaaU)) {
        const uint16_t addressed_node = (uint16_t)frame->data[4] |
            ((uint16_t)frame->data[5] << 8U);
        if (addressed_node == session->node_id) {
            session->launch_requested = true;
            return true;
        }
    }
    return false;
}

bool boot_service_launch_requested(const BootServiceSession *session)
{
    return (session != NULL) && session->launch_requested;
}
