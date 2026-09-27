#include "board_adc.h"

#include <stddef.h>

#include "board_delay.h"
#include "hc32f448.h"

#define ADC_CALIBRATION_SAMPLE_COUNT 1000U

static bool adc_initialized;
static bool adc_runtime_sampling_configured;
static bool adc_runtime_sampling_enabled;

static void configure_analog_pins(const BoardAdcRegisterImage *config)
{
    /* The eight primary analogue inputs use DDIS.  GPIO_PCR_PIN (0x0100) in
     * the live dump is a read-only reflection of the electrical level and
     * must not be copied into a configuration write.  ADC3 channel 2 is
     * multiplexed from PB10; the original intentionally leaves PB10 alone. */
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PCRA0 = config->analog_pin_control;
    CM_GPIO->PCRA1 = config->analog_pin_control;
    CM_GPIO->PCRA2 = config->analog_pin_control;
    CM_GPIO->PCRA4 = config->analog_pin_control;
    CM_GPIO->PCRA5 = config->analog_pin_control;
    CM_GPIO->PCRA6 = config->analog_pin_control;
    CM_GPIO->PCRA7 = config->analog_pin_control;
    CM_GPIO->PCRB1 = config->analog_pin_control;
    CM_GPIO->PWPR = 0xA500U;
}

static void configure_adc_unit(CM_ADC_TypeDef *adc, uint16_t channel_mux,
                               const BoardAdcRegisterImage *config)
{
    adc->STR = 0U;
    adc->CR0 = 0U;
    adc->CHMUXR0 = channel_mux;
    adc->CHSELRA = config->channel_select;
    adc->SSTR0 = config->sample_time;
    adc->SSTR1 = config->sample_time;
    adc->SSTR2 = config->sample_time;
}

bool board_adc_init(void)
{
    BoardAdcRegisterImage config;
    board_adc_build_config(&config);
    adc_initialized = false;
    adc_runtime_sampling_configured = false;
    adc_runtime_sampling_enabled = false;

    configure_analog_pins(&config);
    /* Exact protected clock sequence from adc_sampling_init@0x21a00:
     * unlock peripheral-clock selection with FPRC bit 1, clear FCG0PC,
     * select 0x80, select ADC3's internal extended channel, relock, then
     * release the three ADC clocks through FCG3. */
    CM_PWC->FPRC = (uint16_t)(CM_PWC->FPRC | 0xA502U);
    CM_PWC->FCG0PC = 0U;
    CM_CMU->PERICKSEL = 0x0080U;
    CM_ADC3->EXCHSELR = 1U;
    CM_PWC->FPRC = (uint16_t)(0xA500U |
                              (CM_PWC->FPRC & (uint16_t)~0x0002U));
    CM_PWC->FCG3 &= ~(PWC_FCG3_ADC1 | PWC_FCG3_ADC2 | PWC_FCG3_ADC3);
    CM_ADC1->SYNCCR = 0U;
    configure_adc_unit(CM_ADC1, config.adc1_channel_mux, &config);
    configure_adc_unit(CM_ADC2, config.adc2_channel_mux, &config);
    configure_adc_unit(CM_ADC3, config.adc3_channel_mux, &config);

    /* The factory image leaves IRQ002 untouched here.  It runs the six-output
     * test and both software-triggered calibration passes before routing the
     * ADC interrupt after PWM timer initialization. */
    adc_initialized = true;
    return true;
}

bool board_adc_configure_runtime_sampling(void)
{
    if (!adc_initialized) {
        return false;
    }

    BoardAdcRegisterImage config;
    board_adc_build_config(&config);

    /* Recovered tail of validate_current_sensors@0x2506c..0x25098.  This is
     * deliberately separate from board_adc_init(): it must occur after the
     * startup software conversions and power-stage test. */
    CM_PWC->FCG0PC = 0xA5A50001UL;
    CM_PWC->FCG0 &= ~PWC_FCG0_AOS;
    CM_AOS->ADC1_TRGSEL0 = config.aos_trigger;
    CM_PWC->FCG0PC = 0xA5A50000UL;
    CM_ADC1->TRGSR = config.trigger_select;
    CM_ADC1->SYNCCR =
        (uint16_t)(config.sync_control & (uint16_t)~ADC_SYNCCR_SYNCEN);
    CM_ADC1->SYNCCR |= ADC_SYNCCR_SYNCEN;
    CM_ADC1->ICR = ADC_ICR_EOCAIEN;

    adc_runtime_sampling_configured = true;
    return true;
}

bool board_adc_enable_runtime_irq(void)
{
    if (!adc_runtime_sampling_configured) {
        return false;
    }

    BoardAdcRegisterImage config;
    board_adc_build_config(&config);

    CM_INTC->INTSEL2 = config.irq_source;
    NVIC_ClearPendingIRQ(INT002_IRQn);
    NVIC_SetPriority(INT002_IRQn, 2U);
    NVIC_EnableIRQ(INT002_IRQn);
    adc_runtime_sampling_enabled = true;
    return true;
}

static void wait_for_eoca(const CM_ADC_TypeDef *adc)
{
    while ((adc->ISR & ADC_ISR_EOCAF) == 0U) {
    }
}

static void start_all_conversions(void)
{
    CM_ADC1->STR = ADC_STR_STRT;
    CM_ADC2->STR = ADC_STR_STRT;
    CM_ADC3->STR = ADC_STR_STRT;
}

bool board_adc_calibrate_startup(BoardAdcStartupCalibration *calibration)
{
    if (!adc_initialized || (calibration == NULL)) {
        return false;
    }

    float phase_u = 0.0f;
    float phase_v = 0.0f;
    float phase_w = 0.0f;
    float bus_raw = 0.0f;
    for (uint32_t sample = 0U; sample < ADC_CALIBRATION_SAMPLE_COUNT;
         ++sample) {
        start_all_conversions();
        wait_for_eoca(CM_ADC1);
        wait_for_eoca(CM_ADC2);
        wait_for_eoca(CM_ADC3);
        phase_u += (float)CM_ADC1->DR0;
        phase_v += (float)CM_ADC2->DR0;
        phase_w += (float)CM_ADC3->DR0;
        bus_raw += (float)CM_ADC1->DR1;
        /* calibrate_adc_offsets@0x21b00 clears ADC1/2 after reading.  It
         * intentionally leaves ADC3 EOCA set until the later output-sensor
         * pass clears all three units. */
        CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
        CM_ADC2->ISCLRR = ADC_ISR_EOCAF;
    }

    const float reciprocal_samples = 0.0010000000474974513f;
    calibration->phase_offset_u = phase_u * reciprocal_samples;
    calibration->phase_offset_v = phase_v * reciprocal_samples;
    calibration->phase_offset_w = phase_w * reciprocal_samples;
    calibration->bus_voltage_raw = bus_raw * reciprocal_samples;
    return true;
}

bool board_adc_calibrate_output_sensor(
    BoardAdcOutputSensorCalibration *calibration)
{
    if (!adc_initialized || (calibration == NULL)) {
        return false;
    }

    /* validate_current_sensors@0x24ec0 deliberately performs a second,
     * delayed 1,000-sample pass.  Each zero-word SPI transfer refreshes the
     * motor encoder while ADC1/2 DR2 capture the output-side analogue pair. */
    board_delay_ms(5U);
    float sensor_u = 0.0f;
    float sensor_v = 0.0f;
    for (uint32_t sample = 0U; sample < ADC_CALIBRATION_SAMPLE_COUNT;
         ++sample) {
        start_all_conversions();
        CM_SPI3->DR = 0U;
        board_delay_us(100U);
        wait_for_eoca(CM_ADC1);
        sensor_u += (float)CM_ADC1->DR2;
        wait_for_eoca(CM_ADC2);
        sensor_v += (float)CM_ADC2->DR2;
        CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
        CM_ADC2->ISCLRR = ADC_ISR_EOCAF;
        CM_ADC3->ISCLRR = ADC_ISR_EOCAF;
    }

    const float reciprocal_samples = 0.0010000000474974513f;
    calibration->mean_u = sensor_u * reciprocal_samples;
    calibration->mean_v = sensor_v * reciprocal_samples;
    return true;
}

bool board_adc_read(BoardAdcRawSample *sample)
{
    if (!adc_runtime_sampling_enabled || (sample == NULL) ||
        ((CM_ADC1->ISR & ADC_ISR_EOCAF) == 0U)) {
        return false;
    }

    sample->phase_u = CM_ADC1->DR0;
    sample->phase_v = CM_ADC2->DR0;
    sample->phase_w = CM_ADC3->DR0;
    sample->bus_voltage = CM_ADC1->DR1;
    sample->temperature = CM_ADC2->DR1;
    sample->auxiliary = CM_ADC3->DR1;
    sample->phase_voltage_u = CM_ADC1->DR2;
    sample->phase_voltage_v = CM_ADC2->DR2;
    sample->phase_voltage_w = CM_ADC3->DR2;
    return true;
}

void board_adc_ack_polling_sample(void)
{
    if (!adc_runtime_sampling_enabled) {
        return;
    }
    CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
    CM_ADC2->ISCLRR = ADC_ISR_EOCAF;
    CM_ADC3->ISCLRR = ADC_ISR_EOCAF;
}

void board_adc_ack_interrupt(void)
{
    board_adc_ack_polling_sample();
    NVIC_ClearPendingIRQ(INT002_IRQn);
    __DSB();
}
