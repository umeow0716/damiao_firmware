#include "board_sampling_timer.h"

#include <string.h>

uint16_t board_sampling_timer_duty_to_compare(float duty, uint16_t period)
{
    if (duty != duty) {
        duty = 0.5f;
    } else if (duty < 0.0f) {
        duty = 0.0f;
    } else if (duty > 1.0f) {
        duty = 1.0f;
    }
    return (uint16_t)((1.0f - duty) * (float)period);
}

void board_sampling_timer_build_config(
    BoardSamplingTimerRegisterImage *config)
{
    memset(config, 0, sizeof(*config));
    config->period = 5000U;
    config->neutral_compare = 2500U;
    config->oc_initial_status = 0xFF00U;
    config->oc_status = 0xFF02U;
    config->oc_extended = 0x0008U;
    config->oc_mode_low = 0x0330033FUL;
    config->pwm_control = 0x0010U;
    config->dead_time_rising = 0x0050U;
    config->dead_time_falling = 0x0050U;
    config->channel_output_enable_mask = 0x000000FFUL;
    config->main_output_enable_mask = 0x00000100UL;
    config->special_compare = 120U;
    config->special_status = 0x4000U;
    config->special_mask = 0xFF00U;
    config->initial_ccsr = 0x0070U;
    config->zero_irq_enable_mask = 0x0400U;
    config->running_ccsr = 0x0420U;
    config->irq_source = 201UL;
    config->pwm_pin_function = 0x0002U;
}
