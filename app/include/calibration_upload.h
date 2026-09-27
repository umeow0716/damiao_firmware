#ifndef DM4310_CALIBRATION_UPLOAD_H
#define DM4310_CALIBRATION_UPLOAD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "motor_encoder_calibration.h"

#define MOTOR_ENCODER_CALIBRATION_WORD_COUNT MOTOR_ENCODER_RECORD_WORD_COUNT
#define OUTPUT_SENSOR_CORRECTION_COUNT 4096U

typedef enum {
    CALIBRATION_UPLOAD_NONE = 0,
    CALIBRATION_UPLOAD_MOTOR_ENCODER = 1,
    CALIBRATION_UPLOAD_OUTPUT_SENSOR = 2,
} CalibrationUploadKind;

void calibration_upload_reset(void);

bool calibration_upload_receive_frame(
    const uint8_t *frame,
    size_t length,
    uint8_t acknowledgement[2],
    CalibrationUploadKind *completed);

void calibration_upload_set_motor_direction(float direction);

const uint32_t *calibration_upload_motor_record(void);
const uint16_t *calibration_upload_output_table(void);

#endif
