#include "calibration_upload.h"

#include <string.h>

enum {
    CHUNK_BYTES = 60U,
    FRAME_PAYLOAD_OFFSET = 2U,
    FRAME_LENGTH_OFFSET = 62U,
    FRAME_INDEX_OFFSET = 63U,
    FRAME_MINIMUM_SIZE = 64U,
};

static uint32_t motor_record[MOTOR_ENCODER_CALIBRATION_WORD_COUNT];
static OutputSensorCalibrationUploadEntry output_table[OUTPUT_SENSOR_CORRECTION_COUNT];
static uint16_t next_motor_chunk;
static uint16_t next_output_chunk;

void calibration_upload_reset(void)
{
    next_motor_chunk = 0U;
    next_output_chunk = 0U;
}

static bool receive_motor_chunk(const uint8_t *frame, uint8_t count,
                                uint8_t index,
                                CalibrationUploadKind *completed)
{
    size_t byte_count;
    const size_t offset = (size_t)index * CHUNK_BYTES;

    /*
     * GUI motor encoder packets use:
     *
     *   index 0..16:
     *     count = 0x10
     *     payload = 15 uint32_t = 60 bytes
     *
     *   index 17:
     *     count = 0x02
     *     payload = 2 uint32_t = 8 bytes
     *
     * The regular packet count includes the packet trailer convention
     * used by the original GUI/firmware protocol.  Do not interpret
     * count=0x10 as 16 payload words.
     */
    if (index < 17U) {
        if (count != 16U) {
            return false;
        }

        byte_count = CHUNK_BYTES;
    } else if (index == 17U) {
        if (count != 2U) {
            return false;
        }

        byte_count = 2U * sizeof(uint32_t);
    } else {
        return false;
    }

    const size_t upload_total =
        (MOTOR_ENCODER_ELECTRICAL_OFFSET_WORD + 1U) *
        sizeof(uint32_t);

    if ((index != next_motor_chunk) ||
        (offset + byte_count > upload_total)) {
        return false;
    }

    memcpy((uint8_t *)motor_record + offset,
           frame + FRAME_PAYLOAD_OFFSET,
           byte_count);

    ++next_motor_chunk;

    if (offset + byte_count == upload_total) {
        *completed = CALIBRATION_UPLOAD_MOTOR_ENCODER;
        next_motor_chunk = 0U;
    }

    return true;
}

static bool receive_output_chunk(const uint8_t *frame, uint8_t count,
                                 uint8_t index,
                                 CalibrationUploadKind *completed)
{
#if defined(DAMIAO_DM8009)
    /* Factory DM8009 GUI uploads the 256-float O-sensor table emitted by the
     * 'H' result stream.  Full packets carry 15 floats = 60 bytes; the final
     * packet carries one float. */
    const size_t byte_count = (size_t)count * sizeof(float);
    if (count > 15U) {
        return false;
    }
#else
    const size_t byte_count = (size_t)count * sizeof(uint16_t);
    if (count > 30U) {
        return false;
    }
#endif
    const size_t offset = (size_t)index * CHUNK_BYTES;
    const size_t total = sizeof(output_table);
    if ((index != next_output_chunk) ||
        (offset + byte_count > total) ||
        ((offset + byte_count < total) && (byte_count != CHUNK_BYTES))) {
        return false;
    }
    memcpy((uint8_t *)output_table + offset,
           frame + FRAME_PAYLOAD_OFFSET, byte_count);
    ++next_output_chunk;
    if (offset + byte_count == total) {
        *completed = CALIBRATION_UPLOAD_OUTPUT_SENSOR;
        next_output_chunk = 0U;
    }
    return true;
}

bool calibration_upload_receive_frame(const uint8_t *frame, size_t length,
                                      uint8_t acknowledgement[2],
                                      CalibrationUploadKind *completed)
{
    if ((frame == NULL) || (acknowledgement == NULL) ||
        (completed == NULL) || (length < FRAME_MINIMUM_SIZE) ||
        (frame[0] != 'U')) {
        return false;
    }
    *completed = CALIBRATION_UPLOAD_NONE;
    const uint8_t count = frame[FRAME_LENGTH_OFFSET];
    const uint8_t index = frame[FRAME_INDEX_OFFSET];
    bool accepted = false;
    if (frame[1] == 'd') {
        if (index == 0U) {
            next_motor_chunk = 0U;
        }
        accepted = receive_motor_chunk(frame, count, index, completed);
        acknowledgement[0] = 'd';
    } else if (frame[1] == 'M') {
        if (index == 0U) {
            next_output_chunk = 0U;
        }
        accepted = receive_output_chunk(frame, count, index, completed);
        acknowledgement[0] = 'M';
    }
    acknowledgement[1] = index;
    return accepted;
}

void calibration_upload_set_motor_direction(float direction)
{
    /* word[257] is unused/reserved. */
    motor_record[MOTOR_ENCODER_ELECTRICAL_OFFSET_WORD + 1U] = 0U;

    /* word[258] stores the direction as float bits: 1.0f or 2.0f. */
    memcpy(&motor_record[MOTOR_ENCODER_DIRECTION_WORD],
           &direction,
           sizeof(direction));
}

const uint32_t *calibration_upload_motor_record(void)
{
    return motor_record;
}

const OutputSensorCalibrationUploadEntry *calibration_upload_output_table(void)
{
    return output_table;
}
