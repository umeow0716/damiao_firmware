#include "board_power_stage.h"

#include "board_delay.h"
#include "hc32f448.h"

#define POWER_STAGE_ALL_A_MASK  ((uint16_t)0x0700U)
#define POWER_STAGE_ALL_B_MASK  ((uint16_t)0xE000U)

static void configure_test_gpio(void)
{
    /* Exact register order from the original APP at 0x22ca8.  PCR=0x0010
     * selects medium drive but intentionally leaves POUTE clear.  The test
     * changes the output data latches; it does not enable six GPIO drivers.
     * Enabling POERA/POERB here changes the electrical test and made every
     * channel fail on the target. */
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PCRB13 = GPIO_PCR_DRV_0;
    CM_GPIO->PCRB14 = GPIO_PCR_DRV_0;
    CM_GPIO->PCRB15 = GPIO_PCR_DRV_0;
    CM_GPIO->PCRA8 = GPIO_PCR_DRV_0;
    CM_GPIO->PCRA9 = GPIO_PCR_DRV_0;
    CM_GPIO->PCRA10 = GPIO_PCR_DRV_0;
    CM_GPIO->PORRB |= POWER_STAGE_ALL_B_MASK;
    CM_GPIO->PORRA |= POWER_STAGE_ALL_A_MASK;
    CM_GPIO->PWPR = 0xA500U;
}

static void drive_step(const BoardPowerStageTestStep *step, bool high)
{
    if (step->port == BOARD_POWER_STAGE_PORT_A) {
        if (high) {
            CM_GPIO->POSRA |= step->pin_mask;
        } else {
            CM_GPIO->PORRA |= step->pin_mask;
        }
    } else {
        if (high) {
#if defined(DAMIAO_DM4310)
            /* Factory 0x22e7a uniquely replaces POSRB for the PB14 test. */
            if (step->pin_mask == (uint16_t)(1U << 14U)) {
                CM_GPIO->POSRB = step->pin_mask;
                return;
            }
#endif
            CM_GPIO->POSRB |= step->pin_mask;
        } else {
            CM_GPIO->PORRB |= step->pin_mask;
        }
    }
}

static uint16_t trigger_and_read(uint8_t adc_index)
{
    const CM_ADC_TypeDef *const adc_units[] = {CM_ADC1, CM_ADC2, CM_ADC3};
    const CM_ADC_TypeDef *const selected = adc_units[adc_index];

    CM_ADC1->STR = ADC_STR_STRT;
    CM_ADC2->STR = ADC_STR_STRT;
    CM_ADC3->STR = ADC_STR_STRT;

    while ((selected->ISR & ADC_ISR_EOCAF) == 0U) {
    }
#if defined(DAMIAO_DM4310)
    /* switch_response_test clears all three EOCA flags before loading the
     * selected DR0 halfword.  Preserve that MMIO order. */
    CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
    CM_ADC2->ISCLRR = ADC_ISR_EOCAF;
    CM_ADC3->ISCLRR = ADC_ISR_EOCAF;
    const uint16_t sample = selected->DR0;
#else
    const uint16_t sample = selected->DR0;
    CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
    CM_ADC2->ISCLRR = ADC_ISR_EOCAF;
    CM_ADC3->ISCLRR = ADC_ISR_EOCAF;
#endif
    return sample;
}

bool board_power_stage_self_test(uint8_t *failure_mask)
{
    if (failure_mask == NULL) {
        return false;
    }

    BoardPowerStageTestStep steps[BOARD_POWER_STAGE_TEST_STEP_COUNT];
    board_power_stage_build_self_test_sequence(steps);
    *failure_mask = 0U;
    configure_test_gpio();
    board_delay_us(20U);

    for (size_t index = 0U; index < BOARD_POWER_STAGE_TEST_STEP_COUNT;
         ++index) {
        drive_step(&steps[index], true);
        board_delay_us(10U);
        const uint16_t sample = trigger_and_read(steps[index].adc_index);
        if (!board_power_stage_adc_sample_valid(sample)) {
            *failure_mask |= steps[index].failure_mask;
        }
        drive_step(&steps[index], false);
        board_delay_us(20U);
    }

    return *failure_mask == 0U;
}
