#include "app_state.h"

#include <string.h>

#include "app_config.h"

#if defined(DAMIAO_DM4310)
AppState g_app __attribute__((section(".dm4310_app_state")));
#else
AppState g_app;
#endif

void app_state_init(void)
{
    memset(&g_app, 0, sizeof(g_app));

    app_config_load_defaults(&g_app.config);

    motor_control_init(&g_app.motor);
    g_app.motor.command.mode = g_app.config.control_mode;
    safety_init(&g_app.safety);
#if !defined(DAMIAO_DM4310)
    g_app.events.print_menu = true;
#endif
}
