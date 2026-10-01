#include "calibration_upload.h"

#include <string.h>
#include "runtime_compat.h"

enum {
    CHUNK_BYTES = 60U,
    DM4310_SCRATCH_BYTES = 64U,
    FRAME_PAYLOAD_OFFSET = 2U,
    FRAME_LENGTH_OFFSET = 62U,
    FRAME_INDEX_OFFSET = 63U,
    FRAME_MINIMUM_SIZE = 64U,
};

#if defined(DAMIAO_DM4310)
#define MOTOR_RECORD_SECTION \
    __attribute__((section(".dm4310_motor_record")))
#define OUTPUT_TABLE_SECTION \
    __attribute__((section(".dm4310_output_table")))
static uint8_t calibration_frame_scratch[DM4310_SCRATCH_BYTES]
    __attribute__((section(".dm4310_calibration_frame_scratch"), aligned(1)));
#else
#define MOTOR_RECORD_SECTION
#define OUTPUT_TABLE_SECTION
#endif

static uint32_t motor_record[MOTOR_ENCODER_CALIBRATION_WORD_COUNT]
    MOTOR_RECORD_SECTION;
static OutputSensorCalibrationUploadEntry
    output_table[OUTPUT_SENSOR_CORRECTION_COUNT] OUTPUT_TABLE_SECTION;
#if !defined(DAMIAO_DM4310)
static uint16_t next_motor_chunk;
static uint16_t next_output_chunk;
#endif

void calibration_upload_reset(void)
{
#if !defined(DAMIAO_DM4310)
    next_motor_chunk = 0U;
    next_output_chunk = 0U;
#endif
}

static bool receive_motor_chunk(const uint8_t *frame, uint8_t count,
                                uint8_t index,
                                CalibrationUploadKind *completed)
{
#if defined(DAMIAO_DM4310)
    /* Vector_20 performs this raw slot write with no visible count or index
     * bounds check.  The GUI's regular packets happen to use 60-byte slots,
     * but malformed frames retain the factory write semantics. */
    const size_t byte_count = (size_t)count * sizeof(uint32_t);
    const size_t offset = (size_t)index * CHUNK_BYTES;
    (void)frame;
    dm4310_runtime_copy_bytes((void *)((uintptr_t)motor_record + offset),
           calibration_frame_scratch + 1U, byte_count);
    if (index == 0x11U) {
        *completed = CALIBRATION_UPLOAD_MOTOR_ENCODER;
    }
    return true;
#else
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

    if (offset + byte_count > upload_total) {
        return false;
    }
    if (index != next_motor_chunk) {
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
#endif
}

static bool receive_output_chunk(const uint8_t *frame, uint8_t count,
                                 uint8_t index,
                                 CalibrationUploadKind *completed)
{
#if defined(DAMIAO_DM4310)
    const size_t byte_count = (size_t)count * sizeof(uint16_t);
    const size_t offset = (size_t)index * CHUNK_BYTES;
    (void)frame;
    dm4310_runtime_copy_bytes((void *)((uintptr_t)output_table + offset),
           calibration_frame_scratch + 1U, byte_count);
    if (index == 0x88U) {
        *completed = CALIBRATION_UPLOAD_OUTPUT_SENSOR;
    }
    return true;
#else
    /* Factory DM8009 GUI uploads the 256-float O-sensor table emitted by the
     * 'H' result stream.  Full packets carry 15 floats = 60 bytes; the final
     * packet carries one float. */
    const size_t byte_count = (size_t)count * sizeof(float);
    if (count > 15U) {
        return false;
    }
    const size_t offset = (size_t)index * CHUNK_BYTES;
    const size_t total = sizeof(output_table);
    if ((offset + byte_count > total) ||
        ((offset + byte_count < total) && (byte_count != CHUNK_BYTES))) {
        return false;
    }
    if (index != next_output_chunk) {
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
#endif
}

#if defined(DAMIAO_DM4310)
static bool receive_frame_irq(const uint8_t *frame, size_t length,
                              uint8_t subtype,
                              uint8_t acknowledgement[2],
                              CalibrationUploadKind *completed)
{
    *completed = CALIBRATION_UPLOAD_NONE;
    /* The caller has applied the live expected-length gate. Vector_20
     * copies rx+1 through the full received length before reading
     * count/index at scratch+0x3d/+0x3e. */
    dm4310_runtime_copy_bytes(calibration_frame_scratch, frame + 1U, length);
    volatile const uint8_t *const scratch = calibration_frame_scratch;
    const uint8_t count = scratch[0x3DU];
    const uint8_t index = scratch[0x3EU];
    bool accepted = false;
    if (subtype == 'd') {
        accepted = receive_motor_chunk(frame, count, index, completed);
        acknowledgement[0] = 'd';
    } else if (subtype == 'M') {
        accepted = receive_output_chunk(frame, count, index, completed);
        acknowledgement[0] = 'M';
    }
    /* Vector_20 reloads the live index for the ACK rather than retaining the
     * value used to calculate the destination slot. */
    acknowledgement[1] = scratch[0x3EU];
    return accepted;
}

bool calibration_upload_receive_frame_irq(const uint8_t *frame, size_t length,
                                          uint8_t subtype,
                                          uint8_t acknowledgement[2],
                                          CalibrationUploadKind *completed)
{
    if ((frame == NULL) || (acknowledgement == NULL) ||
        (completed == NULL)) {
        return false;
    }
    return receive_frame_irq(frame, length, subtype,
                             acknowledgement, completed);
}

CalibrationUploadKind calibration_upload_finish_frame_irq(uint8_t subtype)
{
    /* This read occurs after the two-byte UART acknowledgement in
     * Vector_20@0x223a2/0x223f4. */
    const uint8_t index =
        *(volatile const uint8_t *)&calibration_frame_scratch[0x3EU];
    if ((subtype == 'd') && (index == 0x11U)) {
        return CALIBRATION_UPLOAD_MOTOR_ENCODER;
    }
    if ((subtype == 'M') && (index == 0x88U)) {
        return CALIBRATION_UPLOAD_OUTPUT_SENSOR;
    }
    return CALIBRATION_UPLOAD_NONE;
}
#endif

bool calibration_upload_receive_frame(const uint8_t *frame, size_t length,
                                      uint8_t acknowledgement[2],
                                      CalibrationUploadKind *completed)
{
    if ((frame == NULL) || (acknowledgement == NULL) ||
        (completed == NULL) ||
#if !defined(DAMIAO_DM4310)
        (length < FRAME_MINIMUM_SIZE) ||
#endif
        (frame[0] != 'U')) {
        return false;
    }
#if defined(DAMIAO_DM4310)
    const uint8_t subtype = frame[1];
    const bool accepted = receive_frame_irq(frame, length, subtype,
                                            acknowledgement, completed);
    if (accepted) {
        *completed = calibration_upload_finish_frame_irq(subtype);
    }
    return accepted;
#else
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
#endif
}

void calibration_upload_set_motor_direction(float direction)
{
    /* Vector_20 stores parser_state+0x34 only at motor_record+0x408.
     * The intervening word[257] is preserved verbatim from the upload. */
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

#if defined(DAMIAO_DM4310)
OutputSensorCalibrationUploadEntry *calibration_upload_output_table_storage(void)
{
    return output_table;
}
#endif
