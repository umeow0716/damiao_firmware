#ifndef DAMIAO_CALIBRATION_STORE_H
#define DAMIAO_CALIBRATION_STORE_H

#include <stdint.h>

#include "motor_control.h"

#define ZERO_POSITION_RECORD_WORD_COUNT 2U

void calibration_store_decode_zero(MotorController *controller,
                                   const uint32_t words[ZERO_POSITION_RECORD_WORD_COUNT]);
void calibration_store_encode_zero(const MotorController *controller,
                                   uint32_t words[ZERO_POSITION_RECORD_WORD_COUNT]);

#endif
