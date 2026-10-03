#ifndef DAMIAO_CAN_PROTOCOL_H
#define DAMIAO_CAN_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "motor_types.h"

typedef struct McanIrqContext McanIrqContext;

typedef struct
{
    uint32_t id;
    uint8_t length;
    uint8_t data[64];
} CanFrame;

typedef enum
{
    CAN_COMMAND_NONE = 0,
    CAN_COMMAND_FEEDBACK_ONLY,
    CAN_COMMAND_SETPOINT,
    CAN_COMMAND_ENABLE,
    CAN_COMMAND_DISABLE,
    CAN_COMMAND_SET_ZERO,
    CAN_COMMAND_CLEAR_FAULT,
} CanCommandKind;

CanCommandKind can_protocol_decode_command(const CanFrame *frame, const MotorConfig *config,
                                           MotorCommand *command);
CanCommandKind can_protocol_decode_command_irq(const CanFrame *frame, MotorCommand *command,
                                               const McanIrqContext *references);
void can_protocol_encode_feedback_irq(McanIrqContext *references, CanFrame *frame);
void can_protocol_encode_parameter_feedback_irq(McanIrqContext *references, CanFrame *frame);
void can_protocol_encode_feedback(const MotorFeedback *feedback, const MotorConfig *config,
                                  CanFrame *frame);

#endif
