#ifndef DM4310_MOTOR_ENCODER_CALIBRATION_H
#define DM4310_MOTOR_ENCODER_CALIBRATION_H

#include <stdbool.h>
#include <stdint.h>

#define MOTOR_ENCODER_CORRECTION_COUNT 256U
#define MOTOR_ENCODER_RECORD_WORD_COUNT 259U
#define MOTOR_ENCODER_ELECTRICAL_OFFSET_WORD 256U
#define MOTOR_ENCODER_DIRECTION_WORD 258U

bool sensor_calibration_decode_motor_record(
    const uint32_t record[MOTOR_ENCODER_RECORD_WORD_COUNT],
    float correction[MOTOR_ENCODER_CORRECTION_COUNT],
    float *electrical_offset, float *direction);

#endif
