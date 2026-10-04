#ifndef DAMIAO_COMMAND_VALIDATION_H
#define DAMIAO_COMMAND_VALIDATION_H
#include "can_protocol.h"
#include <float.h>
#include <math.h>
#include <string.h>

/* Used before any command state or lease is modified. */
static inline bool command_frame_valid(const CanFrame *frame, uint32_t node)
{
    if (frame == NULL || node == 0U || node > 0xFFU || frame->id > 0x3FFU ||
        (frame->id & 0xFFU) != node) return false;
    const uint32_t family = frame->id >> 8U;
    if (frame->length != 8U && !(family == 2U && frame->length == 4U)) return false;
    bool special = frame->length == 8U;
    for (unsigned i = 0; i < 7U && special; ++i) special = frame->data[i] == 0xFFU;
    if (special && (frame->data[7] == 0xFCU || frame->data[7] == 0xFDU ||
                    frame->data[7] == 0xFEU || frame->data[7] == 0xFBU)) return true;
    if (family != 0U) {
        float first;
        memcpy(&first, frame->data, sizeof(first));
        if (!isfinite(first)) return false;
        if (family == 1U) {
            float speed;
            memcpy(&speed, frame->data + 4U, sizeof(speed));
            if (!isfinite(speed)) return false;
        }
    }
    return true;
}

static inline bool command_scaled_velocity_valid(float velocity, float gear_ratio)
{
    return isfinite(velocity) && isfinite(gear_ratio) && gear_ratio > 0.0f &&
           fabsf(velocity) <= FLT_MAX / fmaxf(gear_ratio, 1.0f);
}

/* DLC 0..8 maps directly to bytes; larger frames are not legacy commands. */
static inline uint8_t command_receive_length(uint32_t header0, uint32_t header1)
{
    const uint8_t dlc = (uint8_t)((header1 >> 16U) & 15U);
    return (header0 & 0x60000000U) == 0U && dlc <= 8U ? dlc : 0U;
}
#endif
