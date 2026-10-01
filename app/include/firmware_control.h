#ifndef DAMIAO_FIRMWARE_CONTROL_H
#define DAMIAO_FIRMWARE_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "motor_types.h"

#define FIRMWARE_CONTROL_BLOCK_SIZE 128U

void firmware_control_export_block(
    const MotorConfig *config,
    uint8_t block[FIRMWARE_CONTROL_BLOCK_SIZE]);
bool firmware_control_import_block(
    MotorConfig *config,
    const uint8_t block[FIRMWARE_CONTROL_BLOCK_SIZE]);
#if defined(DAMIAO_DM4310)
void firmware_control_service(uint8_t request);
#else
void firmware_control_service(void);
#endif

#endif
