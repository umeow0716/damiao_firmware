#ifndef DM4310_BOARD_SAMPLING_TIMER_H
#define DM4310_BOARD_SAMPLING_TIMER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t period;
    uint16_t neutral_compare;
    uint16_t oc_initial_status;
    uint16_t oc_status;
    uint16_t oc_extended;
    uint32_t oc_mode_low;
    uint16_t pwm_control;
    uint16_t dead_time_rising;
    uint16_t dead_time_falling;
    uint32_t channel_output_enable_mask;
    uint32_t main_output_enable_mask;
    uint16_t special_compare;
    uint16_t special_status;
    uint16_t special_mask;
    uint16_t initial_ccsr;
    uint16_t zero_irq_enable_mask;
    uint16_t running_ccsr;
    uint32_t irq_source;
    uint16_t pwm_pin_function;
} BoardSamplingTimerRegisterImage;

void board_sampling_timer_build_config(
    BoardSamplingTimerRegisterImage *config);
bool board_sampling_timer_init(void);
uint16_t board_sampling_timer_duty_to_compare(float duty, uint16_t period);
void board_sampling_timer_write_pwm(float phase_u, float phase_v, float phase_w);
bool board_sampling_timer_is_initialized(void);

#endif
