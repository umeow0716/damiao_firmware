#include "app_state.h"

#include <string.h>

#include "app_config.h"

AppState g_app __attribute__((section(".app_state")));

void app_state_init(void)
{
    memset(&g_app, 0, sizeof(g_app));

    app_config_load_defaults(&g_app.config);

    motor_control_init(&g_app.motor);
    g_app.motor.command.mode = g_app.config.control_mode;
    safety_init(&g_app.safety);
}
