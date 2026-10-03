#include "board_sampling_timer.h"

#include "memory_layout.h"

#include "hc32f448.h"
#include "motor_math.h"

static bool sampling_timer_initialized;
static BoardSamplingTimerRegisterImage sampling_timer_config;

static void configure_pwm_pins(void)
{
    CM_GPIO->PWPR = 0xA501U;
    /* The self-test has already written PCR=0x0010 and left every latch low.
     * pwm_timer_init only selects FSEL=2 here; it does not rewrite
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

    /* pwm_timer_init connects all six pins to TMR4 before touching
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

    /* Fixed-layout special event 0: channel VH, compare on the count-up pass at
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
    if (!sampling_timer_initialized)
    {
        return;
    }
    CM_TMR4_1->OCCRUL = board_sampling_timer_duty_to_compare(phase_u, sampling_timer_config.period);
    CM_TMR4_1->OCCRVL = board_sampling_timer_duty_to_compare(phase_v, sampling_timer_config.period);
    CM_TMR4_1->OCCRWL = board_sampling_timer_duty_to_compare(phase_w, sampling_timer_config.period);
}

static uint16_t modulation_compare(float phase, float scale)
{
    uint32_t compare;
    float converted;
    const float one = 1.0f;
    __asm volatile("vsub.f32 %0, %2, %3\n"
                   "vmul.f32 %0, %0, %4\n"
                   "vcvt.u32.f32 %0, %0\n"
                   "vmov %1, %0"
                   : "=&t"(converted), "=r"(compare)
                   : "t"(one), "t"(phase), "t"(scale)
                   : "memory");
    return (uint16_t)compare;
}

void board_sampling_timer_write_modulation(float phase_u, float phase_v, float phase_w)
{
    /* svpwm_write_compare uses the literal 2500.0f and writes
     * unconditionally; it does not consult the initialization shadow. */
    /* Firmware writes W (+0x14), V (+0x0c), then U (+0x04), each
     * immediately after its unsigned conversion. Do not reorder by name. */
    CM_TMR4_1->OCCRWL = modulation_compare(phase_w, 2500.0f);
    CM_TMR4_1->OCCRVL = modulation_compare(phase_v, 2500.0f);
    CM_TMR4_1->OCCRUL = modulation_compare(phase_u, 2500.0f);
}

void board_sampling_timer_write_space_vector(float alpha, float beta)
{
    const float projection_scale =
        *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFF9D4CUL, 0x1FFF9D84UL);
    const PhaseDuty duty = motor_svpwm_modulation_scaled(
        (AlphaBeta){
            .alpha = alpha,
            .beta = beta,
        },
        projection_scale);
    /* Firmware caches the scale, converts W, then reads the timer pointer
     * before the W/V/U stores. Do not reload either pool word per phase. */
    const float compare_scale =
        *(volatile const float *)MEMORY_LAYOUT_ADDRESS(0x1FFF9D50UL, 0x1FFF9D88UL);
    const uint16_t compare_w = modulation_compare(duty.modulation_c, compare_scale);
    const uintptr_t timer =
        *(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9D54UL, 0x1FFF9D8CUL);
    *(volatile uint16_t *)(timer + 0x14U) = compare_w;
    *(volatile uint16_t *)(timer + 0x0CU) = modulation_compare(duty.modulation_b, compare_scale);
    *(volatile uint16_t *)(timer + 0x04U) = modulation_compare(duty.modulation_a, compare_scale);
}

bool board_sampling_timer_is_initialized(void)
{
    return sampling_timer_initialized;
}
