#include "calibration_upload.h"

#include <string.h>
#include "runtime_compat.h"

enum
{
    CHUNK_BYTES = 60U,
    CALIBRATION_SCRATCH_BYTES = 64U,
    FRAME_PAYLOAD_OFFSET = 2U,
    FRAME_LENGTH_OFFSET = 62U,
    FRAME_INDEX_OFFSET = 63U,
    FRAME_MINIMUM_SIZE = 64U,
};

#define MOTOR_RECORD_SECTION __attribute__((section(".motor_record")))
#define OUTPUT_TABLE_SECTION __attribute__((section(".output_table")))
static uint8_t calibration_frame_scratch[CALIBRATION_SCRATCH_BYTES]
    __attribute__((section(".calibration_frame_scratch"), aligned(1)));

static uint32_t motor_record[MOTOR_ENCODER_CALIBRATION_WORD_COUNT] MOTOR_RECORD_SECTION;
static OutputSensorCalibrationUploadEntry
    output_table[OUTPUT_SENSOR_CORRECTION_COUNT] OUTPUT_TABLE_SECTION;

void calibration_upload_reset(void) {}

static bool receive_motor_chunk(const uint8_t *frame, uint8_t count, uint8_t index,
                                CalibrationUploadKind *completed)
{
    /* This protocol path performs a raw slot write with no count or index
     * bounds check.  Regular packets use 60-byte slots; malformed frames
     * intentionally retain the same write semantics. */
    const size_t byte_count = (size_t)count * sizeof(uint32_t);
    const size_t offset = (size_t)index * CHUNK_BYTES;
    (void)frame;
    runtime_copy_bytes((void *)((uintptr_t)motor_record + offset), calibration_frame_scratch + 1U,
                       byte_count);
    if (index == 0x11U)
    {
        *completed = CALIBRATION_UPLOAD_MOTOR_ENCODER;
    }
    return true;
}

static bool receive_output_chunk(const uint8_t *frame, uint8_t count, uint8_t index,
                                 CalibrationUploadKind *completed)
{
    const size_t byte_count = (size_t)count * sizeof(uint16_t);
    const size_t offset = (size_t)index * CHUNK_BYTES;
    (void)frame;
    runtime_copy_bytes((void *)((uintptr_t)output_table + offset), calibration_frame_scratch + 1U,
                       byte_count);
    if (index == 0x88U)
    {
        *completed = CALIBRATION_UPLOAD_OUTPUT_SENSOR;
    }
    return true;
}

static bool receive_frame_irq(const uint8_t *frame, size_t length, uint8_t subtype,
                              uint8_t acknowledgement[2], CalibrationUploadKind *completed)
{
    *completed = CALIBRATION_UPLOAD_NONE;
    /* The caller has applied the live expected-length gate.  Copy rx+1
     * through the full received length before reading
     * count/index at scratch+0x3d/+0x3e. */
    runtime_copy_bytes(calibration_frame_scratch, frame + 1U, length);
    volatile const uint8_t *const scratch = calibration_frame_scratch;
    const uint8_t count = scratch[0x3DU];
    const uint8_t index = scratch[0x3EU];
    bool accepted = false;
    if (subtype == 'd')
    {
        accepted = receive_motor_chunk(frame, count, index, completed);
        acknowledgement[0] = 'd';
    }
    else if (subtype == 'M')
    {
        accepted = receive_output_chunk(frame, count, index, completed);
        acknowledgement[0] = 'M';
    }
    /* Reload the live index for the acknowledgement rather than retaining the
     * value used to calculate the destination slot. */
    acknowledgement[1] = scratch[0x3EU];
    return accepted;
}

bool calibration_upload_receive_frame_irq(const uint8_t *frame, size_t length, uint8_t subtype,
                                          uint8_t acknowledgement[2],
                                          CalibrationUploadKind *completed)
{
    if ((frame == NULL) || (acknowledgement == NULL) || (completed == NULL))
    {
        return false;
    }
    return receive_frame_irq(frame, length, subtype, acknowledgement, completed);
}

CalibrationUploadKind calibration_upload_finish_frame_irq(uint8_t subtype)
{
    /* This read occurs after the two-byte UART acknowledgement. */
    const uint8_t index = *(volatile const uint8_t *)&calibration_frame_scratch[0x3EU];
    if ((subtype == 'd') && (index == 0x11U))
    {
        return CALIBRATION_UPLOAD_MOTOR_ENCODER;
    }
    if ((subtype == 'M') && (index == 0x88U))
    {
        return CALIBRATION_UPLOAD_OUTPUT_SENSOR;
    }
    return CALIBRATION_UPLOAD_NONE;
}

bool calibration_upload_receive_frame(const uint8_t *frame, size_t length,
                                      uint8_t acknowledgement[2], CalibrationUploadKind *completed)
{
    if ((frame == NULL) || (acknowledgement == NULL) || (completed == NULL) || (frame[0] != 'U'))
    {
        return false;
    }
    const uint8_t subtype = frame[1];
    const bool accepted = receive_frame_irq(frame, length, subtype, acknowledgement, completed);
    if (accepted)
    {
        *completed = calibration_upload_finish_frame_irq(subtype);
    }
    return accepted;
}

void calibration_upload_set_motor_direction(float direction)
{
    /* Store direction only in its designated record word; preserve the
     * intervening uploaded word verbatim. */
    memcpy(&motor_record[MOTOR_ENCODER_DIRECTION_WORD], &direction, sizeof(direction));
}

const uint32_t *calibration_upload_motor_record(void)
{
    return motor_record;
}

const OutputSensorCalibrationUploadEntry *calibration_upload_output_table(void)
{
    return output_table;
}

OutputSensorCalibrationUploadEntry *calibration_upload_output_table_storage(void)
{
    return output_table;
}
