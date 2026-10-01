#include "board_adc.h"

#include "factory_layout.h"

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
#if defined(DAMIAO_DM8009_V3)
    CM_GPIO->PCRA1 = config->analog_pin_control;
    CM_GPIO->PCRA2 = config->analog_pin_control;
    CM_GPIO->PCRA3 = config->analog_pin_control;
#else
    CM_GPIO->PCRA0 = config->analog_pin_control;
    CM_GPIO->PCRA1 = config->analog_pin_control;
    CM_GPIO->PCRA2 = config->analog_pin_control;
#endif
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
     * unlock peripheral-clock selection with FPRC bit 1, clear its selector,
     * select ADC clock 0x80, select ADC3's extended channel, relock, then
     * release the three ADC clocks through FCG3. */
    CM_PWC->FPRC = (uint16_t)(CM_PWC->FPRC | 0xA502U);
#if defined(DAMIAO_DM4310)
    /* Factory 0x21a4a/50: halfword zero at 0x40054010, then byte
     * clock selector 0x80 at 0x4004cc10, not FCG0PC/PERICKSEL fields. */
    *(volatile uint16_t *)(uintptr_t)UINT32_C(0x40054010) = 0U;
    *(volatile uint8_t *)(uintptr_t)UINT32_C(0x4004cc10) = 0x80U;
#else
    CM_PWC->FCG0PC = 0U;
    CM_CMU->PERICKSEL = 0x0080U;
#endif
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
#if defined(DAMIAO_DM4310)
    /* Factory 0x25078 relocks clock protection before AOS trigger write. */
    CM_PWC->FCG0PC = 0xA5A50000UL;
    CM_AOS->ADC1_TRGSEL0 = config.aos_trigger;
#else
    CM_AOS->ADC1_TRGSEL0 = config.aos_trigger;
    CM_PWC->FCG0PC = 0xA5A50000UL;
#endif
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
#if defined(DAMIAO_DM4310)
    /* main 0x253ee..25402 sets priority before clearing pending IRQ2. */
    NVIC_SetPriority(INT002_IRQn, 2U);
    NVIC_ClearPendingIRQ(INT002_IRQn);
#else
    NVIC_ClearPendingIRQ(INT002_IRQn);
    NVIC_SetPriority(INT002_IRQn, 2U);
#endif
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
#if defined(DAMIAO_DM4310)
        const uint32_t raw_u = CM_ADC1->DR0;
        const uint32_t raw_bus = CM_ADC1->DR1;
        wait_for_eoca(CM_ADC2);
        const uint32_t raw_v = CM_ADC2->DR0;
        (void)CM_ADC2->DR1;
        wait_for_eoca(CM_ADC3);
        const uint32_t raw_w = CM_ADC3->DR0;
        (void)CM_ADC3->DR1;
        float converted;
        /* Factory captures all six halfwords before accumulation, then
         * clears ADC1 between the phase sums and bus conversion/add. */
        __asm volatile (
            "vmov %0, %4\nvcvt.f32.u32 %0, %0\n"
            "vadd.f32 %1, %1, %0\n"
            "vmov %0, %5\nvcvt.f32.u32 %0, %0\n"
            "vadd.f32 %2, %2, %0\n"
            "vmov %0, %6\nvcvt.f32.u32 %0, %0\n"
            "vadd.f32 %3, %3, %0"
            : "=&t" (converted), "+t" (phase_u), "+t" (phase_v),
              "+t" (phase_w)
            : "r" (raw_u), "r" (raw_v), "r" (raw_w) : "memory");
        CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
        __asm volatile (
            "vmov %0, %2\nvcvt.f32.u32 %0, %0\n"
            "vadd.f32 %1, %1, %0"
            : "=&t" (converted), "+t" (bus_raw)
            : "r" (raw_bus) : "memory");
#else
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
#endif
        CM_ADC2->ISCLRR = ADC_ISR_EOCAF;
    }

    /* calibrate_adc_offsets@0x21bde..0x21bfe uses four VDIV operations.
     * Multiplication by the rounded reciprocal can differ by one ULP. */
    const float sample_count = 1000.0f;
#if defined(DAMIAO_DM4310)
    float mean_u;
    float mean_v;
    float mean_w;
    float mean_bus;
    /* Factory interleaves division and publication: divide U/V, store U,
     * divide W, store V, divide bus, store W/bus (0x21be2..21bfe). */
    __asm volatile (
        "vdiv.f32 %0, %5, %9\n"
        "vdiv.f32 %1, %6, %9\n"
        "vstr %0, [%4]\n"
        "vdiv.f32 %2, %7, %9\n"
        "vstr %1, [%4, #4]\n"
        "vdiv.f32 %3, %8, %9\n"
        "vstr %2, [%4, #8]\n"
        "vstr %3, [%4, #12]"
        : "=&t" (mean_u), "=&t" (mean_v), "=&t" (mean_w),
          "=&t" (mean_bus)
        : "r" ((uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff144), UINT32_C(0x1ffff0d0))),
          "t" (phase_u), "t" (phase_v),
          "t" (phase_w), "t" (bus_raw), "t" (sample_count)
        : "memory");
    calibration->phase_offset_u = mean_u;
    calibration->phase_offset_v = mean_v;
    calibration->phase_offset_w = mean_w;
    calibration->bus_voltage_raw = mean_bus;
#else
    calibration->phase_offset_u = phase_u / sample_count;
    calibration->phase_offset_v = phase_v / sample_count;
    calibration->phase_offset_w = phase_w / sample_count;
    calibration->bus_voltage_raw = bus_raw / sample_count;
#endif
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
#if defined(DAMIAO_DM4310)
    /* Factory 0x24eee clears output turns after the settling delay and
     * before the first conversion, not during later mean publication. */
    *(volatile uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(UINT32_C(0x1ffff1ec), UINT32_C(0x1ffff178)) = 0U;
#endif
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
#if defined(DAMIAO_DM4310)
        const uint32_t raw_v = CM_ADC2->DR2;
        CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
        /* Factory 0x24f2a..40 clears ADC1 after reading V, then converts
         * and accumulates V before clearing ADC2. Keep that ordering even
         * when the compiler schedules pure FP operations around MMIO. */
        float converted_v;
        __asm volatile (
            "vmov %0, %2\nvcvt.f32.u32 %0, %0\n"
            "vadd.f32 %1, %0, %1"
            : "=&t" (converted_v), "+t" (sensor_v)
            : "r" (raw_v) : "memory");
#else
        sensor_v += (float)CM_ADC2->DR2;
        CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
#endif
        CM_ADC2->ISCLRR = ADC_ISR_EOCAF;
        CM_ADC3->ISCLRR = ADC_ISR_EOCAF;
    }

#if defined(DAMIAO_DM4310)
    /* 0x24f50 uses VMUL with literal 0x3a83126f.  VDIV by 1000.0f is not
     * interchangeable at every accumulated value. */
    const float reciprocal_sample_count = 0x1.0624dep-10f;
    calibration->mean_u = sensor_u * reciprocal_sample_count;
    calibration->mean_v = sensor_v * reciprocal_sample_count;
#else
    const float sample_count = 1000.0f;
    calibration->mean_u = sensor_u / sample_count;
    calibration->mean_v = sensor_v / sample_count;
#endif
    return true;
}

static void read_adc_registers(BoardAdcRawSample *sample)
{
    sample->phase_u = CM_ADC1->DR0;
    sample->phase_v = CM_ADC2->DR0;
    sample->phase_w = CM_ADC3->DR0;
    sample->bus_voltage = CM_ADC1->DR1;
    sample->temperature = CM_ADC2->DR1;
    sample->auxiliary = CM_ADC3->DR1;
    sample->phase_voltage_u = CM_ADC1->DR2;
    sample->phase_voltage_v = CM_ADC2->DR2;
    sample->phase_voltage_w = CM_ADC3->DR2;
}

bool board_adc_read(BoardAdcRawSample *sample)
{
    if (!adc_runtime_sampling_enabled || (sample == NULL) ||
        ((CM_ADC1->ISR & ADC_ISR_EOCAF) == 0U)) {
        return false;
    }
    read_adc_registers(sample);
    return true;
}

#if defined(DAMIAO_DM4310)
static volatile uint16_t control_irq_raw[10]
    __attribute__((section(FACTORY_LAYOUT_SECTION(
        ".dm4310_adc_raw", ".dm8009_adc_raw")), used, aligned(2)));

const volatile uint16_t *board_adc_read_control_irq(void)
{
    /* Factory IRQ002 consumes the nine registers without polling ISR or
     * consulting software enable state. Blocking callers retain polling. */
    /* Original order/layout at 0x1fffc778; word 9 is not written. */
    const uintptr_t adc1 = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF840CUL, 0x1FFF9720UL);
    const uint16_t phase_u = *(const volatile uint16_t *)(adc1 + 0x50U);
    volatile uint16_t *const raw = (volatile uint16_t *)(uintptr_t)
        *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8410UL, 0x1FFF9724UL);
    raw[0] = phase_u;
    const uintptr_t adc2_dr0 = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8414UL, 0x1FFF9728UL);
    const uint16_t phase_v = *(const volatile uint16_t *)adc2_dr0;
    raw[1] = phase_v;
    const uintptr_t adc3_dr0 = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8418UL, 0x1FFF972CUL);
    const uint16_t phase_w = *(const volatile uint16_t *)adc3_dr0;
    raw[2] = phase_w;
    const uint16_t bus_voltage = *(const volatile uint16_t *)(adc1 + 0x52U);
    raw[3] = bus_voltage;
    const uintptr_t adc2_dr1 = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF841CUL, 0x1FFF9730UL);
    const uint16_t temperature = *(const volatile uint16_t *)adc2_dr1;
    raw[6] = temperature;
    const uintptr_t adc3_dr1 = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8420UL, 0x1FFF9734UL);
    const uint16_t auxiliary = *(const volatile uint16_t *)adc3_dr1;
    raw[7] = auxiliary;
    const uint16_t phase_voltage_u = *(const volatile uint16_t *)(adc1 + 0x54U);
    raw[4] = phase_voltage_u;
    const uintptr_t adc2_dr2 = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8424UL, 0x1FFF9738UL);
    const uint16_t phase_voltage_v = *(const volatile uint16_t *)adc2_dr2;
    raw[5] = phase_voltage_v;
    const uintptr_t adc3_dr2 = *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8428UL, 0x1FFF973CUL);
    const uint16_t phase_voltage_w = *(const volatile uint16_t *)adc3_dr2;
    raw[8] = phase_voltage_w;
    return raw;
}
#endif

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
#if defined(DAMIAO_DM4310)
    /* IRQ002@0x1fff8616 clears only ADC1, unconditionally, then ICPR0.
     * Polling/commissioning clears three ADCs through a separate API. */
    CM_ADC_TypeDef *const adc = (CM_ADC_TypeDef *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF863CUL, 0x1FFF9984UL);
    adc->ISCLRR = ADC_ISR_EOCAF;
    NVIC_ClearPendingIRQ(INT002_IRQn);
#else
    board_adc_ack_polling_sample();
    NVIC_ClearPendingIRQ(INT002_IRQn);
    __DSB();
#endif
}

#if defined(DAMIAO_DM4310)
void board_adc_clear_primary_conversion_flags(void)
{
    /* main@0x25502..0x2550c restores the two conversion sources after a
     * blocking flash write, then discards the queued ADC IRQ. */
    CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
    CM_ADC2->ISCLRR = ADC_ISR_EOCAF;
    NVIC_ClearPendingIRQ(INT002_IRQn);
}
#endif
