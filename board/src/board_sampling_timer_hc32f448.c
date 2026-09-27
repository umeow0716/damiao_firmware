#include "board_sampling_timer.h"

#include "hc32f448.h"

static bool sampling_timer_initialized;
static BoardSamplingTimerRegisterImage sampling_timer_config;

static void configure_pwm_pins(void)
{
    CM_GPIO->PWPR = 0xA501U;
    /* The self-test has already written PCR=0x0010 and left every latch low.
     * pwm_timer_init@0x22fac only selects FSEL=2 here; it does not rewrite
     * PCR or GPIO output-enable registers. */
    CM_GPIO->PFSRA8 = sampling_timer_config.pwm_pin_function;
    CM_GPIO->PFSRA9 = sampling_timer_config.pwm_pin_function;
    CM_GPIO->PFSRA10 = sampling_timer_config.pwm_pin_function;
    CM_GPIO->PFSRB13 = sampling_timer_config.pwm_pin_function;
    CM_GPIO->PFSRB14 = sampling_timer_config.pwm_pin_function;
    CM_GPIO->PFSRB15 = sampling_timer_config.pwm_pin_function;
    CM_GPIO->PWPR = 0xA500U;
}

bool board_sampling_timer_init(void)
{
    BoardSamplingTimerRegisterImage *const config = &sampling_timer_config;
    board_sampling_timer_build_config(config);
    sampling_timer_initialized = false;

    CM_PWC->FCG2 &= ~PWC_FCG2_TMR4_1;

    /* pwm_timer_init@0x22fac connects all six pins to TMR4 before touching
     * the timer registers. */
    configure_pwm_pins();
    CM_TMR4_1->CCSR = config->initial_ccsr;
    CM_TMR4_1->CVPR = 0U;
    CM_TMR4_1->CPSR = config->period;

    CM_TMR4_1->OCSRU = config->oc_initial_status;
    CM_TMR4_1->OCSRV = config->oc_initial_status;
    CM_TMR4_1->OCSRW = config->oc_initial_status;
    CM_TMR4_1->OCERU = config->oc_extended;
    CM_TMR4_1->OCERV = config->oc_extended;
    CM_TMR4_1->OCERW = config->oc_extended;
    CM_TMR4_1->OCCRUL = config->neutral_compare;
    CM_TMR4_1->OCCRVL = config->neutral_compare;
    CM_TMR4_1->OCCRWL = config->neutral_compare;
    CM_TMR4_1->OCMRUL = config->oc_mode_low;
    CM_TMR4_1->OCMRVL = config->oc_mode_low;
    CM_TMR4_1->OCMRWL = config->oc_mode_low;
    CM_TMR4_1->OCSRU |= config->oc_status;
    CM_TMR4_1->OCSRV |= config->oc_status;
    CM_TMR4_1->OCSRW |= config->oc_status;
    CM_TMR4_1->POCRU = config->pwm_control;
    CM_TMR4_1->POCRV = config->pwm_control;
    CM_TMR4_1->POCRW = config->pwm_control;
    CM_TMR4_1->RCSR = 0U;
    CM_TMR4_1->PDARU = config->dead_time_rising;
    CM_TMR4_1->PDARV = config->dead_time_rising;
    CM_TMR4_1->PDARW = config->dead_time_rising;
    CM_TMR4_1->PDBRU = config->dead_time_falling;
    CM_TMR4_1->PDBRV = config->dead_time_falling;
    CM_TMR4_1->PDBRW = config->dead_time_falling;
    CM_TMR4_1->PSCR |= config->channel_output_enable_mask;

    /* Recovered special event 0: channel VH, compare on the count-up pass at
     * tick 120. AOS routes that event to ADC1 trigger A. */
    CM_TMR4_1->SCCRUL = config->special_compare;
    CM_TMR4_1->SCMRUL = config->special_mask;
    CM_TMR4_1->SCSRUL = config->special_status;
    CM_TMR4_1->CCSR |= config->zero_irq_enable_mask;

    CM_INTC->INTSEL0 = config->irq_source;
    NVIC_SetPriority(INT000_IRQn, 0U);
    NVIC_ClearPendingIRQ(INT000_IRQn);
    NVIC_EnableIRQ(INT000_IRQn);
    CM_TMR4_1->PSCR |= config->main_output_enable_mask;
    CM_TMR4_1->CCSR &= (uint16_t)~TMR4_CCSR_STOP;
    sampling_timer_initialized = true;
    return true;
}

void board_sampling_timer_write_pwm(float phase_u, float phase_v, float phase_w)
{
    if (!sampling_timer_initialized) {
        return;
    }
    CM_TMR4_1->OCCRUL = board_sampling_timer_duty_to_compare(
        phase_u, sampling_timer_config.period);
    CM_TMR4_1->OCCRVL = board_sampling_timer_duty_to_compare(
        phase_v, sampling_timer_config.period);
    CM_TMR4_1->OCCRWL = board_sampling_timer_duty_to_compare(
        phase_w, sampling_timer_config.period);
}

bool board_sampling_timer_is_initialized(void)
{
    return sampling_timer_initialized;
}
