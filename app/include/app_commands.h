#ifndef DAMIAO_APP_COMMANDS_H
#define DAMIAO_APP_COMMANDS_H

#include "can_protocol.h"

void app_apply_can_command(CanCommandKind kind, const MotorCommand *command);
void app_service_motor_state_change(void);
void app_service_control_status_tick(void);

#endif
