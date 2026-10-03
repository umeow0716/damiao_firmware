#include "platform.h"

#include <math.h>
#include <string.h>

#include "board_adc.h"
#include "board_clock.h"
#include "board_crc.h"
#include "board_delay.h"
#include "board_flash.h"
#include "board_identity.h"
#include "board_led.h"
#include "board_mcan.h"
#include "board_position.h"
#include "board_power_stage.h"
#include "board_sampling_timer.h"
#include "board_uart.h"
#include "boot_record.h"
#include "calibration_upload.h"
#include "calibration_store.h"
#include "commissioning.h"
#include "device_auth.h"
#include "debug_console.h"
#include "app_config.h"
#include "app_profile.h"
#include "app_state.h"
#include "motor_encoder_calibration.h"
#include "output_sensor.h"
#include "position_sensor.h"
#include "runtime_compat.h"
#include "sensor_calibration.h"
#include "temperature_table.h"
#include "hc32f448.h"
#include "system_hc32f448.h"

#define APP_CONFIG_FLASH_ADDRESS (0x0003E000UL)
#define ZERO_POSITION_FLASH_ADDRESS (0x00036000UL)
#define OUTPUT_SENSOR_PARAMETERS_FLASH_ADDRESS (0x00038000UL)
#define OUTPUT_SENSOR_TABLE_FLASH_ADDRESS (0x0003A000UL)
#define MOTOR_ENCODER_CALIBRATION_FLASH_ADDRESS (0x0003C000UL)
#define POSITION_VELOCITY_SAMPLE_FREQUENCY (1000.0f)
#define POSITION_VELOCITY_DECIMATION (20U)
#define BUS_VOLTS_PER_COUNT (0x1.226666p-7f)
#define STARTUP_MAX_BUS_VOLTAGE (32.0f)
#define OTP_KEY_SLOT_BASE (0x03000C00UL)
#define OTP_KEY_SLOT_STRIDE (0x40UL)

static PositionSensorState position_sensor;
static OutputSensorState output_sensor __attribute__((section(".output_sensor_state")));
static uint8_t cached_hardware_variant __attribute__((section(".hardware_variant")));
static float motor_encoder_correction[256] __attribute__((section(".motor_correction")));
/* Runtime lookup and calibration-upload packets share the same correction
 * table so an uploaded calibration becomes visible without another copy. */
#define output_sensor_correction calibration_upload_output_table_storage()
static float output_sensor_calibration[4] __attribute__((section(".output_calibration")));
static bool output_sensor_table_valid;
static MotorFault output_sensor_table_fault;

/* copy_persistent_configuration stages all five boot-record words at
 * this address before either bank selector rewrites the first two words. */
static BootPersistentRecord boot_record_staging __attribute__((section(".boot_record_staging")));
static volatile float zero_position_staging[2] __attribute__((section(".zero_position_staging")));

static void stage_boot_record(void)
{
    const volatile BootPersistentRecord *const stored =
        (const volatile BootPersistentRecord *)APP_BOOT_RECORD_ADDRESS;
    volatile BootPersistentRecord *const staging = &boot_record_staging;
    /* Capture all five persistent words before publishing any staging word.
     * A generic 20-byte copy may interleave the last read with earlier writes. */
    const uint32_t boot_request = stored->boot_request;
    const uint32_t application_confirmed = stored->application_confirmed;
    const uint32_t device_id = stored->device_id;
    const uint32_t application_identity = stored->application_identity;
    const uint32_t swd_disabled = stored->swd_disabled;
    staging->boot_request = boot_request;
    staging->application_confirmed = application_confirmed;
    staging->device_id = device_id;
    staging->application_identity = application_identity;
    staging->swd_disabled = swd_disabled;
}

typedef struct
{
    bool uart_ready;
    bool adc_ready;
    bool position_ready;
    bool calibration_ready;
    bool adc_calibration_ready;
    bool output_sensor_calibration_ready;
    bool bus_voltage_valid;
    bool adc_runtime_ready;
    bool mcan_ready;
    BoardAdcStartupCalibration adc_calibration;
    BoardAdcOutputSensorCalibration output_sensor_calibration;
    float bus_voltage;
} PlatformStartupState;

static PlatformStartupState startup;

/* Startup diagnostics intentionally stop normal motor initialization, but
 * they must never stop the maintenance transport.  The installed loader has
 * only a short pre-APP UART window, so the APP must service 'X' itself. */

static bool authenticate_device_key_slots(void)
{
    uint8_t uid[BOOT_DEVICE_UID_SIZE];
    uint8_t token[BOOT_DEVICE_TOKEN_SIZE];
    const volatile uint8_t *const uid_source = (const volatile uint8_t *)UINT32_C(0x40010450);
    for (uint32_t byte = 0U; byte < BOOT_DEVICE_UID_SIZE; ++byte)
    {
        uid[byte] = uid_source[byte];
    }
    boot_device_derive_token(uid, token);
    uint64_t expected[2];
    memcpy(expected, token, sizeof(expected));
    for (uint32_t slot = 0U; slot < BOOT_DEVICE_KEY_SLOT_COUNT; ++slot)
    {
        const volatile uint64_t *const source =
            (const volatile uint64_t *)(OTP_KEY_SLOT_BASE + slot * OTP_KEY_SLOT_STRIDE);
        /* Firmware uses LDRD for each pair, skipping the second pair
         * unless both words of the first pair match. */
        const uint64_t first = source[0];
        if ((first ^ expected[0]) != 0U)
        {
            continue;
        }
        const uint64_t second = source[1];
        if ((second ^ expected[1]) == 0U)
        {
            return true;
        }
    }
    return false;
}

void platform_require_device_authentication(void)
{
    if (authenticate_device_key_slots())
    {
        return;
    }

    /* Device-key binding is enforced by control-parameter derivation,
     * including ordinary startup. */
    for (;;)
    {
        board_delay_ms(500U);
        debug_console_printf("The key verification failed. This is a duplicate!\n");
    }
}

static void halt_on_power_stage_failure(uint8_t failure_mask)
{
    char bitmap[] = "W  | W  | V  | V  | U  | U ";
    /* power_stage_self_test_or_halt initializes the complete
     * USART/DMA path again before formatting the six failure bits. */
    board_uart_init();
    for (size_t index = 0U; index < 6U; ++index)
    {
        const uint8_t bit = (uint8_t)(1U << (5U - index));
        bitmap[index * 5U] = (failure_mask & bit) != 0U ? '1' : '0';
    }

    for (;;)
    {
        debug_console_printf("MOSFET ERROR,  WH | WL | VH | VL | UH | UL\r\n");
        debug_console_printf("               %s\r\n", bitmap);
        debug_console_printf("\r\n");
        board_led_toggle_power_stage_fault_indicator();
        board_delay_ms(500U);
    }
}

static void halt_on_bus_overvoltage(float bus_voltage)
{
    for (;;)
    {
        /* Startup resets PC13 and sets PH2, delays, then reloads
         * fixed volts each iteration. It never enables interrupts here. */
        board_led_set(BOARD_LED_GREEN);
        board_delay_ms(1000U);
        const uint32_t bus_bits = *(volatile const uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
            UINT32_C(0x1ffff16c), UINT32_C(0x1ffff0f8));
        memcpy(&bus_voltage, &bus_bits, sizeof(bus_voltage));
        debug_console_printf("V_BUS= %.4f Over Voltage!!!\r\n", (double)bus_voltage);
    }
}

/* The MCU pin mux, timer routing, ADC triggers, DMA and MCAN message RAM are
 * fixed-layout in docs/DUMP_ANALYSIS.md. */

bool platform_early_init(void)
{
    return board_clock_init();
}

void platform_prepare_board_startup(void)
{
    memset(&startup, 0, sizeof(startup));
    output_sensor_table_valid = false;
    output_sensor_table_fault = MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING;
    /* The fixed-layout APP confirms the boot record before this 500 ms settling
     * delay.  Keeping the delay in this post-confirm phase is essential: a
     * reset or peripheral fault here must not make the installed loader
     * report an otherwise accepted UART image as an upgrade failure. */
    board_delay_ms(500U);
    /* read_hardware_variant configures both LED pins and PC14/PC15
     * straps in one protected-register window, selects red, then samples the
     * straps. */
    cached_hardware_variant = board_identity_initialize_status_and_read_variant();
}

bool platform_initialize_peripherals(void)
{
    if (!board_clock_is_ready())
    {
        return false;
    }

    /* main performs these calls in this order.  UART is deliberately
     * available before either blocking diagnostic loop starts. */
    startup.uart_ready = board_uart_init();
    startup.adc_ready = board_adc_init();

    uint8_t power_stage_failure_mask;
    if (!board_power_stage_self_test(&power_stage_failure_mask))
    {
        halt_on_power_stage_failure(power_stage_failure_mask);
    }

    board_crc_enable_clock();
    startup.position_ready = board_position_init();
    startup.calibration_ready = platform_load_motor_calibration(&g_app.motor);
    startup.adc_calibration_ready =
        startup.adc_ready && board_adc_calibrate_startup(&startup.adc_calibration);
    float startup_bus_voltage;
    /* Startup reloads the fixed ADC mean and publishes volts
     * before runtime parameter derivation; no finite/range filtering here. */
    __asm volatile(
        "vldr %0, [%1]\n"
        "vmul.f32 %0, %0, %3\n"
        "vstr %0, [%2]"
        : "=&t"(startup_bus_voltage)
        : "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff150), UINT32_C(0x1ffff0dc))),
          "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff16c), UINT32_C(0x1ffff0f8))),
          "t"(BUS_VOLTS_PER_COUNT)
        : "memory");
    startup.bus_voltage = startup_bus_voltage;
    int32_t startup_bus_bits;
    memcpy(&startup_bus_bits, &startup_bus_voltage, sizeof(startup_bus_bits));
    startup.bus_voltage_valid =
        startup_bus_bits <= (int32_t)APP_PROFILE_STARTUP_BUS_OVERVOLTAGE_BITS;
    g_app.motor.feedback.bus_voltage = startup.bus_voltage;

    return startup.uart_ready && startup.adc_ready && startup.position_ready &&
           startup.calibration_ready && startup.adc_calibration_ready && startup.bus_voltage_valid;
}

void platform_prepare_runtime_configuration(void)
{
    position_sensor_init(&position_sensor, g_app.config.direction == 1.0f, motor_encoder_correction,
                         g_app.config.gear_ratio, g_app.motor.motor_output_position_offset,
                         g_app.config.velocity_filter_bandwidth, POSITION_VELOCITY_SAMPLE_FREQUENCY,
                         POSITION_VELOCITY_DECIMATION);
    /* Select both MCAN callbacks before deriving the runtime parameter cache. */
    /* This path tests the full staging word and leaves the
     * existing callback pair untouched for selectors above eleven. */
    const uint32_t transport_selector =
        *(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA654UL, 0x1FFFA5E4UL);
    if (transport_selector <= 11U)
    {
        platform_select_mcan_transport_format((uint8_t)transport_selector);
    }
    commissioning_initialize_runtime_parameter_cache(&g_app.config);
    motor_control_configure_velocity_filter(&g_app.motor, g_app.config.velocity_filter_bandwidth);
    derive_control_parameters_helper();
    motor_control_configure_runtime_motor(&g_app.motor, &g_app.config);
}

bool platform_initialize_runtime(void)
{
    /* The SPI/DMA state remains live during the delayed 1,000-sample
     * U/V calibration.  Those zero-word SPI transfers establish the latest
     * rotor angle before the two encoder turn counts are aligned. */
    startup.output_sensor_calibration_ready =
        board_adc_calibrate_output_sensor(&startup.output_sensor_calibration);

    MotorFault startup_fault = MOTOR_FAULT_NONE;
    if (!output_sensor_table_valid)
    {
        startup_fault = output_sensor_table_fault;
    }

    /* Firmware retains both means in FP registers across diagnostic calls
     * and checks each once, U first and V only after the U diagnostic. */
    const float mean_u = startup.output_sensor_calibration.mean_u;
    const float mean_v = startup.output_sensor_calibration.mean_v;
    bool current_sensor_ok = startup.output_sensor_calibration_ready;
    if (!sensor_calibration_current_mean_valid(mean_u))
    {
        current_sensor_ok = false;
        motor_control_set_runtime_fault(MOTOR_FAULT_OUTPUT_SENSOR);
        debug_console_printf("Sensor U broken!U=%.4f\r\n", (double)mean_u);
    }
    if (!sensor_calibration_current_mean_valid(mean_v))
    {
        current_sensor_ok = false;
        motor_control_set_runtime_fault(MOTOR_FAULT_OUTPUT_SENSOR);
        debug_console_printf("Sensor V broken!V=%.4f\r\n", (double)mean_v);
    }
    if (!current_sensor_ok)
    {
        startup_fault = MOTOR_FAULT_OUTPUT_SENSOR;
    }
    /* This path publishes means and decodes/aligns the encoders only
     * after both U/V diagnostics, retaining any fault already published. */
    output_sensor_init(&output_sensor, output_sensor_calibration, output_sensor_correction,
                       g_app.motor.output_position_offset, mean_u, mean_v);
    const float aligned_output_angle = output_sensor_normalize_startup(&output_sensor);
    position_sensor_align_to_output(&position_sensor, aligned_output_angle);
    /* The U/V check belongs to the analogue output encoder.  Preserve the
     * separately measured three-phase current offsets. */
    /* ADC calibration has already published the offsets; update the runtime
     * mirrors without a second fixed-layout publication. */
    g_app.motor.current_offset_u = startup.adc_calibration.phase_offset_u;
    g_app.motor.current_offset_v = startup.adc_calibration.phase_offset_v;
    g_app.motor.current_offset_w = startup.adc_calibration.phase_offset_w;
    safety_set_startup_fault(&g_app.safety, startup_fault);
    g_app.motor.feedback.fault = startup_fault;
    /* validate_current_sensors configures the ADC trigger here, but main
     * does not route or enable IRQ002 until after pwm_timer_init. */
    const bool adc_runtime_configured = board_adc_configure_runtime_sampling();
    startup.adc_runtime_ready = startup.output_sensor_calibration_ready && adc_runtime_configured;
    /* Startup clears position readiness and accumulated
     * delta before publishing menu mode/event and initializing MCAN. */
    __asm volatile(
        "str %0, [%3, #4]\n\t"
        "vstr %4, [%3, #16]\n\t"
        "strd %0, %1, [%2]"
        :
        : "r"(0U), "r"(1U),
          "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff0c0), UINT32_C(0x1ffff04c))),
          "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff190), UINT32_C(0x1ffff11c))),
          "t"(0.0f)
        : "memory");
    const uint16_t node_id = *(volatile const uint16_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
        UINT32_C(0x1fffa5e8), UINT32_C(0x1fffa578));
    const uint16_t data_rate_selector =
        *(volatile const uint16_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa654),
                                                                     UINT32_C(0x1fffa5e4));
    startup.mcan_ready = board_mcan_init(node_id, data_rate_selector);
    return startup.position_ready && startup.calibration_ready && current_sensor_ok &&
           startup.bus_voltage_valid && startup.adc_runtime_ready && startup.mcan_ready &&
           (startup_fault == MOTOR_FAULT_NONE);
}

void platform_check_startup_bus_voltage(void)
{
    /* main prints device information first, then remains in this
     * one-second diagnostic loop when the averaged reading exceeds the
     * model's configured firmware limit. */
    const int32_t bus_bits = *(volatile const int32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
        UINT32_C(0x1ffff16c), UINT32_C(0x1ffff0f8));
    if (bus_bits > (int32_t)APP_PROFILE_STARTUP_BUS_OVERVOLTAGE_BITS)
    {
        float bus_voltage;
        memcpy(&bus_voltage, &bus_bits, sizeof(bus_voltage));
        halt_on_bus_overvoltage(bus_voltage);
    }
}

bool platform_start_control_loop(void)
{
    const bool timer_ready = board_sampling_timer_init();
    const bool adc_irq_enabled = board_adc_enable_runtime_irq();
    return timer_ready && startup.adc_runtime_ready && adc_irq_enabled;
}

void platform_idle(void)
{
    __NOP();
}

bool platform_read_adc(AdcSample *sample)
{
    if (sample == NULL)
    {
        return false;
    }
    (void)platform_read_adc_control();
    /* Conversion/publication belongs to the ordered control prefix. */
    return true;
}

const volatile uint16_t *platform_read_adc_control(void)
{
    return board_adc_read_control_irq();
}

volatile PositionSensorScratch *platform_finish_adc_sensor_sample(AdcSample *sample,
                                                                  const volatile uint16_t *raw)
{
    OutputSensorState *const sensor = (OutputSensorState *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8440UL, 0x1FFF9754UL);
    sample->analog_output_position = output_sensor_update_control_irq(sensor, raw + 4);
    volatile PositionSensorScratch *const scratch = (volatile PositionSensorScratch *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8444UL, 0x1FFF9758UL);
    sample->position_sample_ready = position_sensor_take_control_sample(scratch);
    sample->output_position = position_sensor.output_position;
    return scratch;
}

void platform_finish_adc_velocity_sample(AdcSample *sample, volatile PositionSensorScratch *scratch,
                                         float cleared, const OuterLoopContext *references)
{
    if (sample->outer_loop_due)
    {
        position_sensor_velocity_tick(&position_sensor, scratch, cleared, references);
        /* IRQ002 publishes this before position/control work. */
        references->status[11] = 1U;
    }
    sample->rotor_velocity = position_sensor.rotor_velocity;
    sample->output_velocity = position_sensor.output_velocity;
}

static bool read_position(float *rotor_position, float *rotor_angle, float *output_position,
                          uint16_t *raw_position)
{
    uint16_t dma_word = 0U;
    if ((rotor_position == NULL) || (rotor_angle == NULL) || (output_position == NULL) ||
        (raw_position == NULL))
    {
        return false;
    }
    if (!board_position_take_sample(&dma_word))
    {
        return false;
    }

    position_sensor_update(&position_sensor, dma_word);
    *rotor_position = position_sensor.continuous_angle;
    *rotor_angle = position_sensor.wrapped_angle;
    *output_position = position_sensor.output_position;
    *raw_position = position_sensor.raw_position;
    return true;
}

bool platform_read_position(float *rotor_position, float *rotor_angle, float *output_position,
                            uint16_t *raw_position)
{
    return read_position(rotor_position, rotor_angle, output_position, raw_position);
}

volatile uint32_t *platform_read_position_dma(void)
{
    const float wrap_minimum =
        *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B48UL, 0x1FFF8508UL);
    if (!board_position_dma_sample_pending())
    {
        return NULL;
    }
    return position_sensor_update_dma(wrap_minimum);
}

void platform_write_pwm(PhaseDuty duty)
{
    board_sampling_timer_write_modulation(duty.modulation_a, duty.modulation_b, duty.modulation_c);
}

void platform_set_status_led(PlatformLedColor color)
{
    switch (color)
    {
    case PLATFORM_LED_GREEN:
        board_led_set(BOARD_LED_GREEN);
        break;
    case PLATFORM_LED_RED:
        board_led_set(BOARD_LED_RED);
        break;
    case PLATFORM_LED_OFF:
    default:
        board_led_set(BOARD_LED_OFF);
        break;
    }
}

void platform_toggle_fault_indicator(void)
{
    board_led_toggle_fault_indicator();
}

void platform_set_position_zero_offsets(float output_offset, float motor_output_offset)
{
    output_sensor_set_zero_offset(&output_sensor, output_offset);
    position_sensor_set_output_offset(&position_sensor, motor_output_offset);
}

void platform_zero_current_position(MotorController *controller)
{
    if (controller == NULL)
    {
        return;
    }
    /* FE masks ADC before reading either encoder state,
     * including the CMSIS DSB/ISB sequence. Reenabled after persistence. */
    NVIC_DisableIRQ(INT002_IRQn);
    controller->motor_output_position_offset = position_sensor_zero_current(&position_sensor);
    volatile float *const zero_record = zero_position_staging;
    zero_record[1] = controller->motor_output_position_offset;
    controller->output_position_offset = output_sensor_zero_current(&output_sensor);
    zero_record[0] = controller->output_position_offset;
    *(volatile float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff0a0),
                                                        UINT32_C(0x1ffff02c)) = 0.0f;
    *(volatile float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff104),
                                                        UINT32_C(0x1ffff090)) = 0.0f;
}

void platform_zero_current_position_irq(const McanIrqContext *references)
{
    volatile uint32_t *const motor = (volatile uint32_t *)references->motor;
    volatile uint32_t *const sample = (volatile uint32_t *)references->sample;
    uintptr_t output_owner;
    uintptr_t position_scratch;
    uintptr_t zero_staging;
    NVIC_DisableIRQ(INT002_IRQn);
    __asm__ volatile("vldr s0, [%[motor], #8]\n\t"
                     "vldr s1, [%[motor], #92]\n\t"
                     "vldr s3, [%[motor], #76]\n\t"
                     "vcvt.f32.s32 s2, s0\n\t"
                     "vcvt.f32.s32 s0, s0\n\t"
                     "ldr %[output], [%[output_pool]]\n\t"
                     "ldr %[scratch], [%[scratch_pool]]\n\t"
                     "vmul.f32 s2, s2, s1\n\t"
                     "vcvt.s32.f32 s2, s2\n\t"
                     "vcvt.f32.s32 s2, s2\n\t"
                     "vmls.f32 s0, s2, s3\n\t"
                     "vcvt.s32.f32 s0, s0\n\t"
                     "vstr s0, [%[motor], #8]\n\t"
                     "str %[zero_word], [%[output], #68]\n\t"
                     "vcvt.f32.s32 s0, s0\n\t"
                     "vldr s2, [%[scratch], #8]\n\t"
                     "ldr %[staging], [%[staging_pool]]\n\t"
                     "vmla.f32 s2, s0, %[two_pi]\n\t"
                     "vmul.f32 s0, s2, s1\n\t"
                     "vstr s0, [%[motor], #36]\n\t"
                     "vstr s0, [%[staging], #4]\n\t"
                     "vldr s0, [%[output], #52]\n\t"
                     "vstr s0, [%[output], #56]\n\t"
                     "vstr s0, [%[staging]]\n\t"
                     "vstr %[zero_float], [%[motor], #24]\n\t"
                     "vstr %[zero_float], [%[sample]]"
                     : [output] "=&r"(output_owner), [scratch] "=&r"(position_scratch),
                       [staging] "=&r"(zero_staging)
                     : [motor] "r"(motor), [sample] "r"(sample),
                       [output_pool] "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b98),
                                                                          UINT32_C(0x1fff8558))),
                       [scratch_pool] "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b58),
                                                                           UINT32_C(0x1fff8518))),
                       [staging_pool] "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b9c),
                                                                           UINT32_C(0x1fff855c))),
                       [zero_word] "r"(UINT32_C(0)), [zero_float] "t"(references->cleared),
                       [two_pi] "t"(references->two_pi)
                     : "s0", "s1", "s2", "s3", "memory");
    (void)output_owner;
    (void)position_scratch;
    (void)zero_staging;

    __disable_irq();
    const uint32_t *const source = (const uint32_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b9c), UINT32_C(0x1fff855c));
    const uint32_t destination = *(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(
        UINT32_C(0x1fff8ba0), UINT32_C(0x1fff8560));
    board_flash_write_zero_record_from(destination, source);
    __enable_irq();

    volatile uint8_t *const primary_flags = (volatile uint8_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8ba4), UINT32_C(0x1fff8564));
    primary_flags[0x46U] = 1U;
    volatile uint8_t *const secondary_flag = (volatile uint8_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8ba8), UINT32_C(0x1fff8568));
    *secondary_flag = 1U;
    NVIC_ClearPendingIRQ(INT002_IRQn);
    NVIC_EnableIRQ(INT002_IRQn);
}

void platform_set_motor_encoder_direction(bool inverted)
{
    position_sensor_set_inverted(&position_sensor, inverted);
}

bool platform_commissioning_begin(void)
{
    /* motor-ID disables INT002 (including DSB/ISB) before entering
     * the authentication routine. */
    NVIC_DisableIRQ(INT002_IRQn);
    platform_require_device_authentication();
    return true;
}

bool platform_commissioning_begin_unauthenticated(void)
{
    /* output-calibration disables INT002 directly and deliberately
     * skips the device-authentication path used by the other workers. */
    NVIC_DisableIRQ(INT002_IRQn);
    return true;
}

void platform_commissioning_drive(float voltage_d, float voltage_q, float electrical_angle)
{
    float sine;
    float cosine;
    /* Firmware commissioning calls the pointer-based SRAM wrap helper;
     * retain its VCMPE flags and intermediate conditional stores. */
    wrap_helper(&electrical_angle, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
    motor_target_sincos(electrical_angle, &sine, &cosine);
    const DirectQuadrature rotating = {
        .d = voltage_d,
        .q = voltage_q,
    };
    AlphaBeta stationary;
    __asm volatile("vmul.f32 %0, %2, %4\n"
                   "vmls.f32 %0, %3, %5\n"
                   "vmul.f32 %1, %3, %4\n"
                   "vmla.f32 %1, %2, %5"
                   : "=&t"(stationary.alpha), "=&t"(stationary.beta)
                   : "t"(cosine), "t"(sine), "t"(rotating.d), "t"(rotating.q));
    svpwm_helper(stationary.alpha, stationary.beta);
}

void platform_commissioning_delay_us(uint32_t microseconds)
{
    board_delay_us(microseconds);
}

void platform_commissioning_wait_identification_sample(void)
{
    while ((*((const volatile uint8_t *)0x40040044UL) & 1U) == 0U)
    {
    }
}

bool platform_commissioning_read_identification_sample(PlatformCommissioningSample *sample,
                                                       bool publish_projected_bus)
{
    if (sample == NULL)
    {
        return false;
    }
    /* Motor-ID reads precisely these four halfwords;
     * the third phase conversion is read, then replaced by the bus word. */
    const uint16_t phase_u = *((const volatile uint16_t *)0x40040050UL);
    const uint16_t phase_v = *((const volatile uint16_t *)0x40040450UL);
    (void)*((const volatile uint16_t *)0x40040850UL);
    const uint16_t bus = *((const volatile uint16_t *)0x40040052UL);
    const float bus_scale =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF23CUL, 0x1FFFF1C8UL));
    const float bus_voltage = bus_scale * (float)bus;
    *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF16CUL, 0x1FFFF0F8UL)) = bus_voltage;
    __asm volatile("" : : : "memory");
    if (publish_projected_bus)
    {
        const float projected_bus = bus_voltage * 0x1.279a74p-1f;
        *((volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF170UL, 0x1FFFF0FCUL)) = projected_bus;
    }
    const float offset_u =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF144UL, 0x1FFFF0D0UL));
    float current_u = (offset_u - (float)phase_u) * 0x1p-11f;
    __asm volatile("" : "+t"(current_u) : : "memory");
    const float offset_v =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF148UL, 0x1FFFF0D4UL));
    const float current_v = (offset_v - (float)phase_v) * 0x1p-11f;
    sample->current_u = current_u;
    sample->current_v = current_v;
    sample->current_w = 0.0f;
    sample->bus_voltage = bus_voltage;
    return true;
}

bool platform_commissioning_read_sample(PlatformCommissioningSample *sample)
{
    if (sample == NULL)
    {
        return false;
    }
    BoardAdcRawSample raw;
    while (!board_adc_read(&raw))
    {
        /* The firmware commissioning routines wait for the next ADC result.
         * They do not turn a temporarily late conversion into a setup
         * failure or advance the energized sequence without a sample. */
    }
    sample->current_u =
        (g_app.motor.current_offset_u - (float)raw.phase_u) * g_app.motor.current_scale;
    sample->current_v =
        (g_app.motor.current_offset_v - (float)raw.phase_v) * g_app.motor.current_scale;
    sample->current_w =
        (g_app.motor.current_offset_w - (float)raw.phase_w) * g_app.motor.current_scale;
    sample->bus_voltage = (float)raw.bus_voltage * BUS_VOLTS_PER_COUNT;
    float rotor_position;
    float rotor_angle;
    float motor_output_position;
    uint16_t raw_position;
    if (platform_read_position(&rotor_position, &rotor_angle, &motor_output_position,
                               &raw_position))
    {
        g_app.rotor_position = rotor_position;
        g_app.rotor_angle = rotor_angle;
        g_app.motor_output_position = motor_output_position;
        g_app.raw_position = raw_position;
    }
    output_sensor_update(&output_sensor, raw.phase_voltage_u, raw.phase_voltage_v);
    /* The firmware commissioning frames and extrema use the filtered ADC
     * values produced by the analogue sensor state, not the just-read raw
     * conversion words.  VCVT.U32.F32 truncates them before packing. */
    sample->output_raw_u = (uint16_t)output_sensor.filtered_u;
    sample->output_raw_v = (uint16_t)output_sensor.filtered_v;
    sample->output_position = output_sensor.continuous_angle;
    sample->output_uncorrected_angle = output_sensor_uncorrected_angle(&output_sensor);
    return true;
}

static void commissioning_clear_adc_flags(void)
{
    /* Electrical-fit and commissioning epilogues
     * clear each ADC with STRB, independently of board sampling state. */
    *((volatile uint8_t *)0x40040046UL) = 1U;
    *((volatile uint8_t *)0x40040446UL) = 1U;
    *((volatile uint8_t *)0x40040846UL) = 1U;
}

void platform_commissioning_finish_sample(void)
{
    /* Energized firmware loops update their estimator and PWM before they
     * clear the three ADC completion flags.  INT002 stays disabled until
     * platform_commissioning_end(). */
    commissioning_clear_adc_flags();
}

static void commissioning_filter_output(PlatformCommissioningSample *sample)
{
    /* extrema: no truncation, position update or ADC ack. */
    while ((CM_ADC1->ISR & ADC_ISR_EOCAF) == 0U)
    {
    }
    const uint16_t raw_u = CM_ADC1->DR2;
    const uint16_t raw_v = CM_ADC2->DR2;
    volatile OutputSensorState *const live = &output_sensor;
    const float u = live->filtered_u;
    const float old_weight = live->filter_previous_weight;
    float input_u;
    __asm volatile("vmov %0, %1\nvcvt.f32.u32 %0, %0"
                   : "=t"(input_u)
                   : "r"((uint32_t)raw_u)
                   : "memory");
    float filtered_u;
    __asm volatile("vmul.f32 %0, %1, %2" : "=t"(filtered_u) : "t"(u), "t"(old_weight) : "memory");
    const float new_weight = live->filter_new_weight;
    __asm volatile("vmla.f32 %0, %1, %2"
                   : "+t"(filtered_u)
                   : "t"(input_u), "t"(new_weight)
                   : "memory");
    live->filtered_u = filtered_u;
    float filtered_v = live->filtered_v;
    __asm volatile("vmul.f32 %0, %0, %1" : "+t"(filtered_v) : "t"(old_weight) : "memory");
    const float input_v = (float)raw_v;
    __asm volatile("vmla.f32 %0, %1, %2"
                   : "+t"(filtered_v)
                   : "t"(input_v), "t"(new_weight)
                   : "memory");
    uint32_t truncated_u = 0U;
    if (sample != NULL)
    {
        float converted_u;
        __asm volatile("vcvt.u32.f32 %0, %2\nvmov %1, %0"
                       : "=&t"(converted_u), "=r"(truncated_u)
                       : "t"(filtered_u)
                       : "memory");
    }
    live->filtered_v = filtered_v;
    if (sample != NULL)
    {
        uint32_t truncated_v;
        float converted_v;
        __asm volatile("vcvt.u32.f32 %0, %2\nvmov %1, %0"
                       : "=&t"(converted_v), "=r"(truncated_v)
                       : "t"(filtered_v)
                       : "memory");
        sample->output_raw_u = (uint16_t)truncated_u;
        sample->output_raw_v = (uint16_t)truncated_v;
    }
}

void platform_commissioning_prime_output_filter(void)
{
    commissioning_filter_output(NULL);
}

void platform_commissioning_read_extrema_sample(PlatformCommissioningSample *sample)
{
    commissioning_filter_output(sample);
}

void platform_commissioning_read_offset_sample(PlatformCommissioningSample *sample, float *atan_y,
                                               float *atan_x)
{
    /* offset uses only the two analogue DR2 channels.
     * It does not publish centered/lookup/position state. */
    while ((CM_ADC1->ISR & ADC_ISR_EOCAF) == 0U)
    {
    }
    const uint16_t raw_u = CM_ADC1->DR2;
    const uint16_t raw_v = CM_ADC2->DR2;
    volatile OutputSensorState *const live = &output_sensor;
    float u = live->filtered_u;
    const float old_weight = live->filter_previous_weight;
    const float new_weight = live->filter_new_weight;
    const float input_u = (float)raw_u;
    __asm volatile("vmul.f32 %0, %0, %1\n"
                   "vmla.f32 %0, %2, %3"
                   : "+t"(u)
                   : "t"(old_weight), "t"(input_u), "t"(new_weight)
                   : "memory");
    live->filtered_u = u;
    float v = live->filtered_v;
    /* Keep V live before converting U so the IRQ preserves its FP data flow. */
    __asm volatile("" : "+t"(v) : : "memory");
    uint32_t truncated_u;
    float converted_u;
    __asm volatile("vcvt.u32.f32 %0, %2\nvmov %1, %0"
                   : "=&t"(converted_u), "=r"(truncated_u)
                   : "t"(u)
                   : "memory");
    const uint16_t filtered_u = (uint16_t)truncated_u;
    const float input_v = (float)raw_v;
    __asm volatile("vmul.f32 %0, %0, %1\n"
                   "vmla.f32 %0, %2, %3"
                   : "+t"(v)
                   : "t"(old_weight), "t"(input_v), "t"(new_weight)
                   : "memory");
    live->filtered_v = v;
    const uint16_t filtered_v = (uint16_t)(uint32_t)v;
    const float center_u = live->center_u;
    const float center_v = live->center_v;
    float centered_u;
    float centered_v;
    __asm volatile("vsub.f32 %0, %2, %3\n"
                   "vsub.f32 %1, %4, %5"
                   : "=&t"(centered_u), "=&t"(centered_v)
                   : "t"((float)filtered_u), "t"(center_u), "t"((float)filtered_v), "t"(center_v)
                   : "memory");
    const float gain = live->gain_v;
    float y;
    __asm volatile("vmul.f32 %0, %1, %2" : "=t"(y) : "t"(centered_v), "t"(gain) : "memory");
    const float cosine = live->phase_cosine;
    __asm volatile("vmls.f32 %0, %1, %2" : "+t"(y) : "t"(centered_u), "t"(cosine) : "memory");
    const float sine = live->phase_sine;
    *atan_y = y;
    *atan_x = centered_u * sine;
    sample->output_raw_u = filtered_u;
    sample->output_raw_v = filtered_v;
}

void platform_commissioning_finish_output_sample(void)
{
    /* offset clears only ADC1 after publishing its PWM update. */
    CM_ADC1->ISCLRR = ADC_ISR_EOCAF;
}

void platform_commissioning_restore_control_irq(void)
{
    /* Successful motor-ID has already written neutral PWM.  Its epilogue
     * only clears the three ADC completion flags and restores INT002. */
    /* output-calibration and motor-ID use byte clears,
     * with no source sampling-enabled predicate. Polling ACK elsewhere
     * retains its separate board API and access widths. */
    commissioning_clear_adc_flags();
    NVIC_ClearPendingIRQ(INT002_IRQn);
    NVIC_EnableIRQ(INT002_IRQn);
}

void platform_commissioning_finish_alignment(void)
{
    /* alignment: STRB twice, not the shared three-ADC word clear.
     * Preserve write widths/order and do not add a sampling-enabled guard. */
    *((volatile uint8_t *)0x40040046UL) = 1U;
    *((volatile uint8_t *)0x40040446UL) = 1U;
    NVIC_ClearPendingIRQ(INT002_IRQn);
    NVIC_EnableIRQ(INT002_IRQn);
}

bool platform_commissioning_get_output_calibration(float calibration[4])
{
    if (calibration == NULL)
    {
        return false;
    }
    memcpy(calibration, output_sensor_calibration, sizeof(output_sensor_calibration));
    return true;
}

void platform_commissioning_reset_motor_encoder(void)
{
    /* direction publishes 256 individual zero words. */
    volatile float *const correction = motor_encoder_correction;
    for (uint32_t index = 0U; index < 256U; ++index)
    {
        correction[index] = 0.0f;
    }
    position_sensor.correction_table = motor_encoder_correction;
    position_sensor_set_inverted(&position_sensor, true);
}

void platform_commissioning_publish_output_extrema(const OutputSensorExtrema *extrema)
{
    sensor_calibration_publish_output_extrema(extrema, &output_sensor, output_sensor_calibration);
}

void platform_commissioning_end(void)
{
    platform_commissioning_drive(0.0f, 0.0f, 0.0f);
    board_adc_ack_interrupt();
    NVIC_ClearPendingIRQ(INT002_IRQn);
    NVIC_EnableIRQ(INT002_IRQn);
}
bool platform_mcan_receive(CanFrame *frame)
{
    McanIrqContext references;
    board_mcan_begin_irq(&references);
    return frame != NULL &&
           board_mcan_receive_payload(&frame->id, &frame->length, frame->data, &references);
}

void platform_begin_mcan_irq(McanIrqContext *references)
{
    board_mcan_begin_irq(references);
}

bool platform_mcan_receive_irq(CanFrame *frame, McanIrqContext *references)
{
    return frame != NULL &&
           board_mcan_receive_payload(&frame->id, &frame->length, frame->data, references);
}

void platform_mcan_send(const CanFrame *frame)
{
    if ((frame != NULL) && (frame->length <= 8U))
    {
        BoardMcanFrame outgoing;
        outgoing.id = frame->id;
        outgoing.length = frame->length;
        memcpy(outgoing.data, frame->data, frame->length);
        board_mcan_send(&outgoing);
    }
}
void platform_mcan_send_prebuilt(uint16_t id, uint8_t length)
{
    board_mcan_send_prebuilt(id, length);
}

void platform_mcan_send_prebuilt_irq(uint16_t id, uint8_t length, const McanIrqContext *references)
{
    board_mcan_send_prebuilt_irq(id, length, references);
}
void platform_update_mcan_node_filter(uint16_t node_id)
{
    board_mcan_update_node_filter(node_id);
}
void platform_select_mcan_transport_format(uint8_t data_rate_selector)
{
    board_mcan_select_transport_format(data_rate_selector);
}
void platform_select_mcan_transport_format_irq(uint8_t data_rate_selector,
                                               const McanIrqContext *references)
{
    board_mcan_select_transport_format_irq(data_rate_selector, references);
}
void platform_reconfigure_mcan_irq(const McanIrqContext *references)
{
    platform_delay_ms(5U);
    board_mcan_reinitialize_live(true, references);
}

bool platform_reconfigure_mcan(void)
{
    /* The reference sends the write/legacy-ID acknowledgement at the old
     * bit rate, waits 5 ms for it to leave the Tx FIFO, and only then applies
     * the new filter/timing image. */
    /* This path calls the millisecond helper with 5. The us API
     * adds a final zero-tick timer transaction for exact 500-us multiples. */
    platform_reconfigure_mcan_irq(NULL);
    return true;
}
bool platform_load_parameters(MotorConfig *config)
{
    if (config == NULL)
    {
        return false;
    }
    const volatile uint32_t *const stored = (const volatile uint32_t *)APP_CONFIG_FLASH_ADDRESS;
    /* load_motor_configuration tests Flash before copying it over
     * the scatter-loaded staging defaults.  An erased identity therefore
     * writes the existing staging image, not an all-ones decoded record. */
    if (!app_config_record_present(stored))
    {
        __disable_irq();
        board_flash_replace_sector_prefix(APP_CONFIG_FLASH_ADDRESS, app_config_staging_record(),
                                          APP_CONFIG_WORD_COUNT * sizeof(uint32_t));
        __enable_irq();
    }
    app_config_stage_and_decode(config, stored);
    return true;
}

bool platform_store_parameters(const MotorConfig *config)
{
    if (config == NULL)
    {
        return false;
    }
    uint32_t *const record = app_config_staging_record();
    app_config_encode_persistent(config, record);
    return board_flash_replace_sector_prefix(APP_CONFIG_FLASH_ADDRESS, record,
                                             APP_CONFIG_WORD_COUNT * sizeof(uint32_t));
}

bool platform_store_staged_parameters(void)
{
    const uint32_t *const record = app_config_staging_record();
    return board_flash_replace_sector_prefix(APP_CONFIG_FLASH_ADDRESS, record,
                                             APP_CONFIG_WORD_COUNT * sizeof(uint32_t));
}

void platform_finish_flash_commit(void)
{
    board_adc_clear_primary_conversion_flags();
    board_position_clear_timer_event_after_flash();
}

bool platform_load_motor_calibration(MotorController *controller)
{
    if (controller == NULL)
    {
        return false;
    }
    const uint32_t *const stored_motor = (const uint32_t *)MOTOR_ENCODER_CALIBRATION_FLASH_ADDRESS;
    float stored_direction;
    sensor_calibration_decode_motor_record(stored_motor, motor_encoder_correction,
                                           &controller->electrical_offset, &stored_direction);
    /* The firmware decoder publishes raw fixed words and conditional NaN
     * normalization in firmware order; do not publish them again here. */
    g_app.config.direction = stored_direction;
    g_app.config.sensor_inverted = stored_direction == 1.0f;

    volatile uint32_t *const zero_words = (volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
        UINT32_C(0x1fffa5c0), UINT32_C(0x1fffa550));
    const uint32_t *const stored_zero = (const uint32_t *)ZERO_POSITION_FLASH_ADDRESS;
    uint32_t zero_first;
    uint32_t zero_second;
    /* This path captures both Flash words before publishing
     * either SRAM word, with LDRD/STRD rather than interleaved copies. */
    __asm volatile("ldrd %0, %1, [%2]\nstrd %0, %1, [%3]"
                   : "=&r"(zero_first), "=&r"(zero_second)
                   : "r"(stored_zero), "r"(zero_words)
                   : "memory");
    if ((zero_words[0] & UINT32_C(0x7fffffff)) > UINT32_C(0x7f800000))
    {
        __asm volatile("vstr %1, [%0]" : : "r"(zero_words), "t"(0.0f) : "memory");
    }
    float motor_zero = 0.0f;
    if ((zero_words[1] & UINT32_C(0x7fffffff)) > UINT32_C(0x7f800000))
    {
        __asm volatile("vstr %1, [%0, #4]" : : "r"(zero_words), "t"(motor_zero) : "memory");
    }
    else
    {
        __asm volatile("vldr %0, [%1, #4]" : "=t"(motor_zero) : "r"(zero_words) : "memory");
    }
    /* This path retains zero on NaN, only VLDRs valid motor
     * staging, then publishes motor/output before decoded mirrors. */
    float output_zero;
    __asm volatile(
        "vstr %4, [%2]\n\t"
        "vldr %0, [%1]\n\t"
        "vstr %0, [%3]"
        : "=&t"(output_zero)
        : "r"(zero_words),
          "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff0ac), UINT32_C(0x1ffff038))),
          "r"(&output_sensor.zero_offset), "t"(motor_zero)
        : "memory");
    /* Staging has already been normalized; decoded mirrors must not
     * repeat the NaN predicates or reread the motor zero. */
    controller->output_position_offset = output_zero;
    controller->motor_output_position_offset = motor_zero;

    const float *const stored_output_parameters =
        (const float *)OUTPUT_SENSOR_PARAMETERS_FLASH_ADDRESS;
    /* Read all four words before publishing the staging copy. */
    runtime_copy_bytes(output_sensor_calibration, stored_output_parameters,
                       sizeof(output_sensor_calibration));
    output_sensor_load_persistent_calibration(&output_sensor, stored_output_parameters);

    const uint16_t *const stored_output_table = (const uint16_t *)OUTPUT_SENSOR_TABLE_FLASH_ADDRESS;
    /* Use the aligned grouped copy for the complete 8 KiB table. */
    runtime_copy_bytes(output_sensor_correction, stored_output_table,
                       CORRECTION_TABLE_COUNT * sizeof(CorrectionTableEntry));
    float maximum_step;
    output_sensor_table_fault =
        sensor_calibration_validate_position(calibration_upload_output_table_storage(),
                                             POSITION_CALIBRATION_SAMPLE_COUNT, &maximum_step);
    output_sensor_table_valid = output_sensor_table_fault == MOTOR_FAULT_NONE;
    if (output_sensor_table_fault == MOTOR_FAULT_OUTPUT_CALIBRATION)
    {
        debug_console_printf("O-sensor fail!Max=%.4f\r\n", (double)maximum_step);
        /* Firmware publishes literal 3 after printf, not a mirror reload. */
        motor_control_set_runtime_fault(MOTOR_FAULT_OUTPUT_CALIBRATION);
    }
    else if (!output_sensor_table_valid)
    {
        debug_console_printf("Error,O-sensor need calibration!\r\n");
        /* Erased-table branch publishes literal 2 after printf. */
        motor_control_set_runtime_fault(MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING);
    }
    else
    {
        motor_control_set_runtime_fault(MOTOR_FAULT_NONE);
        board_led_set_ready_state();
    }
    return true;
}

bool platform_store_output_sensor_calibration(const uint16_t correction_table[4096],
                                              const float calibration[4])
{
    if ((correction_table == NULL) || (calibration == NULL))
    {
        return false;
    }
    if (!board_flash_replace_sector_prefix(OUTPUT_SENSOR_TABLE_FLASH_ADDRESS, correction_table,
                                           4096U * sizeof(uint16_t)))
    {
        return false;
    }
    if (!board_flash_replace_sector_prefix(OUTPUT_SENSOR_PARAMETERS_FLASH_ADDRESS, calibration,
                                           4U * sizeof(float)))
    {
        return false;
    }
    /* DM main clears the request then reloads both records from Flash;
     * no intermediate SRAM copy or calibration-valid publication here. */
    return true;
}

bool platform_store_motor_encoder_calibration(const uint32_t record[259])
{
    if ((record == NULL) ||
        !board_flash_replace_sector_prefix(MOTOR_ENCODER_CALIBRATION_FLASH_ADDRESS, record,
                                           259U * sizeof(uint32_t)))
    {
        return false;
    }
    return true;
}

bool platform_store_zero_position(const MotorController *controller)
{
    if (controller == NULL)
    {
        return false;
    }
    board_flash_write_zero_record();
    board_adc_clear_primary_conversion_flags();
    /* This path clears pending then enables INT002. */
    NVIC_EnableIRQ(INT002_IRQn);
    return true;
}
bool platform_confirm_application_boot(void)
{
    const BootPersistentRecord *const stored =
        (const BootPersistentRecord *)APP_BOOT_RECORD_ADDRESS;
    /* select_configuration_bank_a is called solely when word 1 is
     * not one.  Match that predicate rather than adding a policy check on
     * word 0; the write below still normalizes both words to (0, 1). */
    if (stored->application_confirmed == 1U)
    {
        return true;
    }
    stage_boot_record();
    /* select_configuration_bank_a writes word 1 before word 0. */
    volatile BootPersistentRecord *const staging = &boot_record_staging;
    staging->application_confirmed = 1U;
    staging->boot_request = 0U;
    board_flash_write_boot_record();
    return true;
}

void platform_derive_control_parameters(void)
{
    /* derive_control_parameters is a no-argument global-state
     * routine.  Authentication is its first observable operation. */
    platform_require_device_authentication();
    /* Firmware retains s1 from this single sample read across both stages. */
    const float bus_voltage =
        *((const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFFF16CUL, 0x1FFFF0F8UL));
    commissioning_configure_runtime_drive_states(&g_app.config, bus_voltage);
    motor_control_configure(&g_app.motor, &g_app.config, bus_voltage);
}

void platform_select_configuration_bank_b(void)
{
    stage_boot_record();
    boot_record_request_update(&boot_record_staging);
    board_flash_write_boot_record();
}

bool platform_update_application_identity(void)
{
    const BootPersistentRecord *const stored =
        (const BootPersistentRecord *)APP_BOOT_RECORD_ADDRESS;
    if (stored->application_identity == APP_PROFILE_APPLICATION_IDENTITY)
    {
        return true;
    }
    stage_boot_record();
    boot_record_set_application_identity(&boot_record_staging, APP_PROFILE_APPLICATION_IDENTITY);
    board_flash_write_boot_record();
    return true;
}

bool platform_read_device_identity(uint32_t *device_id, uint32_t *application_identity)
{
    if ((device_id == NULL) || (application_identity == NULL))
    {
        return false;
    }
    const BootPersistentRecord *const record =
        (const BootPersistentRecord *)APP_BOOT_RECORD_ADDRESS;
    *device_id = record->device_id;
    *application_identity = record->application_identity;
    return true;
}
uint8_t platform_read_hardware_variant(void)
{
    return cached_hardware_variant;
}
float platform_read_mcan_data_rate_kbps(void)
{
    return board_mcan_current_data_rate_kbps();
}
bool platform_debug_receive(uint8_t *byte)
{
    return board_uart_receive(byte);
}
bool platform_begin_debug_uart_irq(const uint8_t **data, int16_t *length)
{
    return board_uart_begin_receive_irq(data, length);
}
void platform_rearm_debug_uart_irq(void)
{
    board_uart_rearm_receive_irq();
}
void platform_delay_ms(uint32_t milliseconds)
{
    board_delay_ms(milliseconds);
}
void platform_debug_write(const void *data, size_t length)
{
    board_uart_write(data, length);
}
void platform_ack_position_timer_irq(void)
{
    board_position_handle_timer_interrupt();
}
void platform_ack_position_dma_irq(bool sample_ready, volatile uint32_t *dma_count)
{
    board_position_ack_dma_interrupt(sample_ready, dma_count);
}
void platform_ack_adc_irq(void)
{
    board_adc_ack_interrupt();
}
uint8_t platform_ack_mcan_irq(const McanIrqContext *references)
{
    /* Handle the reinitialization request before rereading IR for bus-off.
     * This IRQ path intentionally omits the parameter-change settling delay. */
    if (board_mcan_reinitialization_requested(references))
    {
        /* IRQ003 passes the full two halfwords to the selected callback. */
        board_mcan_reinitialize_live(false, references);
        references->status[0x30U / 4U] = 1U;
    }
    const uint8_t error = board_mcan_recover_bus_off(references);
    if (error != 0U)
    {
        references->status[0x30U / 4U] = error;
    }
    return board_mcan_ack_interrupt(references);
}
void platform_ack_debug_uart_irq(void)
{
    board_uart_ack_interrupt();
}

void platform_enter_bootloader(void)
{
    __disable_irq();

    select_configuration_bank_b_helper();

    board_delay_us(100U);
    NVIC_SystemReset();
}

static __attribute__((noreturn)) void reset_after_barrier(void)
{
    SCB->AIRCR = (SCB->AIRCR & UINT32_C(0x00000700)) | UINT32_C(0x05fa0004);
    __DSB();
    for (;;)
    {
        __NOP();
    }
}

void platform_enter_bootloader_from_can(void)
{
    select_configuration_bank_b_helper();

    board_delay_ms(10U);
    __DSB();
    /* This path has one barrier before AIRCR. */
    reset_after_barrier();
}

void platform_system_reset(void)
{
    /* UgU uses the post-transmit 100 us reset sequence. */
    __disable_irq();
    board_delay_us(100U);
    __DSB();
    /* This path has only one pre-AIRCR barrier. */
    reset_after_barrier();
}
