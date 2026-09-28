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
#include "calibration_store.h"
#include "device_auth.h"
#include "debug_console.h"
#include "app_config.h"
#include "app_profile.h"
#include "app_state.h"
#include "motor_encoder_calibration.h"
#include "output_sensor.h"
#include "position_sensor.h"
#include "sensor_calibration.h"
#include "temperature_table.h"
#include "hc32f448.h"
#include "system_hc32f448.h"

#define APP_CONFIG_FLASH_ADDRESS (0x0003E000UL)
#if defined(DAMIAO_DM4310)
#define ZERO_POSITION_FLASH_ADDRESS (0x00036000UL)
#define OUTPUT_SENSOR_PARAMETERS_FLASH_ADDRESS (0x00038000UL)
#elif defined(DAMIAO_DM8009)
#define ZERO_POSITION_FLASH_ADDRESS (0x00038000UL)
#define OUTPUT_SENSOR_PARAMETERS_FLASH_ADDRESS (0x0003A000UL)
#endif
#define OUTPUT_SENSOR_TABLE_FLASH_ADDRESS (0x0003A000UL)
#define MOTOR_ENCODER_CALIBRATION_FLASH_ADDRESS (0x0003C000UL)
#define POSITION_VELOCITY_SAMPLE_FREQUENCY (1000.0f)
#define POSITION_VELOCITY_DECIMATION (20U)
#define BUS_VOLTS_PER_COUNT (0x1.226666p-7f)
#define STARTUP_MAX_BUS_VOLTAGE (32.0f)
#define OTP_KEY_SLOT_BASE       (0x03000C00UL)
#define OTP_KEY_SLOT_STRIDE     (0x40UL)

static PositionSensorState position_sensor;
static OutputSensorState output_sensor;
static bool recovery_transport_ready;
static uint8_t cached_hardware_variant;
static float motor_encoder_correction[256];
static CorrectionTableEntry output_sensor_correction[CORRECTION_TABLE_COUNT];
static float output_sensor_calibration[4];
static bool output_sensor_table_valid;
static MotorFault output_sensor_table_fault;

typedef struct {
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
static __attribute__((noinline)) void service_blocking_fault_recovery(void)
{
    /* The installed loader jumps to the APP with PRIMASK set.  Normal startup
     * enables interrupts after board initialization, which is unreachable in
     * the fault loops below.  UART/MCAN are initialized before those loops. */
    __enable_irq();
}

static bool authenticate_device_key_slots(void)
{
    const uint32_t uid_words[3] = {
        CM_EFM->UQID0,
        CM_EFM->UQID1,
        CM_EFM->UQID2,
    };
    uint8_t uid[BOOT_DEVICE_UID_SIZE];
    uint8_t slots[BOOT_DEVICE_KEY_SLOT_COUNT][BOOT_DEVICE_TOKEN_SIZE];
    memcpy(uid, uid_words, sizeof(uid));
    for (uint32_t slot = 0U; slot < BOOT_DEVICE_KEY_SLOT_COUNT; ++slot) {
        const volatile uint8_t *const source =
            (const volatile uint8_t *)(OTP_KEY_SLOT_BASE +
                                       slot * OTP_KEY_SLOT_STRIDE);
        for (uint32_t byte = 0U; byte < BOOT_DEVICE_TOKEN_SIZE; ++byte) {
            slots[slot][byte] = source[byte];
        }
    }
    return boot_device_authenticate(uid, slots, NULL);
}

static void require_device_authentication(void)
{
    if (authenticate_device_key_slots()) {
        return;
    }

    /* enforce_device_key_binding_or_halt@0x26eb8 is called by the original
     * control-parameter derivation path, including ordinary startup. */
    for (;;) {
        service_blocking_fault_recovery();
        board_delay_ms(500U);
        debug_console_printf(
            "The key verification failed. This is a duplicate!\n");
    }
}

static void halt_on_power_stage_failure(uint8_t failure_mask)
{
    char bitmap[] = "W  | W  | V  | V  | U  | U ";
    for (size_t index = 0U; index < 6U; ++index) {
        const uint8_t bit = (uint8_t)(1U << (5U - index));
        bitmap[index * 5U] = (failure_mask & bit) != 0U ? '1' : '0';
    }

    for (;;) {
        service_blocking_fault_recovery();
        debug_console_printf(
            "MOSFET ERROR,  WH | WL | VH | VL | UH | UL\r\n"
            "               %s\r\n"
            "\r\n",
            bitmap);
        board_led_set(BOARD_LED_RED);
        board_delay_ms(500U);
    }
}

static void halt_on_bus_overvoltage(float bus_voltage)
{
    for (;;) {
        service_blocking_fault_recovery();
        board_led_set(BOARD_LED_RED);
        board_delay_ms(1000U);
        debug_console_printf("V_BUS= %.4f Over Voltage!!!\r\n",
                             (double)bus_voltage);
    }
}

/* The MCU pin mux, timer routing, ADC triggers, DMA and MCAN message RAM are
 * recovered in docs/DUMP_ANALYSIS.md. */

bool platform_early_init(void)
{
    return board_clock_init();
}

void platform_prepare_board_startup(void)
{
    memset(&startup, 0, sizeof(startup));
    recovery_transport_ready = false;
    output_sensor_table_valid = false;
    output_sensor_table_fault = MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING;
    /* The recovered APP confirms the boot record before this 500 ms settling
     * delay.  Keeping the delay in this post-confirm phase is essential: a
     * reset or peripheral fault here must not make the installed loader
     * report an otherwise accepted UART image as an upgrade failure. */
    board_delay_ms(500U);
    /* read_hardware_variant@0x221f4 configures the two LED pins and both
     * PC14/PC15 straps after the settling delay.  The source keeps the LED
     * and strap drivers separate but preserves that observable timing. */
    board_led_init();
    board_led_set(BOARD_LED_RED);
    cached_hardware_variant = board_identity_read_hardware_variant();
}

bool platform_initialize_peripherals(void)
{
    if (!board_clock_is_ready()) {
        return false;
    }

    /* main@0x252f4 performs these calls in this order.  UART is deliberately
     * available before either blocking diagnostic loop starts. */
    startup.uart_ready = board_uart_init();
    recovery_transport_ready = startup.uart_ready;
    startup.adc_ready = board_adc_init();

    uint8_t power_stage_failure_mask;
    if (!board_power_stage_self_test(&power_stage_failure_mask)) {
        halt_on_power_stage_failure(power_stage_failure_mask);
    }

    board_crc_enable_clock();
    startup.position_ready = board_position_init();
    startup.calibration_ready =
        platform_load_motor_calibration(&g_app.motor);
    startup.adc_calibration_ready = startup.adc_ready &&
        board_adc_calibrate_startup(&startup.adc_calibration);
    startup.bus_voltage_valid = startup.adc_calibration_ready &&
        sensor_calibration_startup_bus_valid(
            startup.adc_calibration.bus_voltage_raw, BUS_VOLTS_PER_COUNT,
            STARTUP_MAX_BUS_VOLTAGE, &startup.bus_voltage);
    g_app.motor.feedback.bus_voltage = startup.bus_voltage;

    return startup.uart_ready && startup.adc_ready &&
           startup.position_ready && startup.calibration_ready &&
           startup.adc_calibration_ready && startup.bus_voltage_valid;
}

bool platform_initialize_runtime(void)
{
    position_sensor_init(&position_sensor, g_app.config.direction == 1.0f,
                         motor_encoder_correction,
                         g_app.config.gear_ratio,
                         g_app.motor.motor_output_position_offset,
                         g_app.config.velocity_filter_bandwidth,
                         POSITION_VELOCITY_SAMPLE_FREQUENCY,
                         POSITION_VELOCITY_DECIMATION);
    /* The original SPI/DMA state is live during the delayed 1,000-sample
     * U/V calibration.  Those zero-word SPI transfers establish the latest
     * rotor angle before the two encoder turn counts are aligned. */
    startup.output_sensor_calibration_ready =
        board_adc_calibrate_output_sensor(
            &startup.output_sensor_calibration);

    MotorFault startup_fault = MOTOR_FAULT_NONE;
    if (!output_sensor_table_valid) {
        startup_fault = output_sensor_table_fault;
    }

    output_sensor_init(&output_sensor,
                       output_sensor_calibration,
                       output_sensor_correction,
                       g_app.motor.output_position_offset,
                       startup.output_sensor_calibration.mean_u,
                       startup.output_sensor_calibration.mean_v);
    output_sensor_normalize_startup(&output_sensor);
    position_sensor_align_to_output(&position_sensor,
                                    output_sensor.continuous_angle);
    const bool current_sensor_ok =
        startup.output_sensor_calibration_ready &&
        sensor_calibration_current_means_valid(
            startup.output_sensor_calibration.mean_u,
            startup.output_sensor_calibration.mean_v);
    if (startup.output_sensor_calibration_ready &&
        !sensor_calibration_current_mean_valid(
            startup.output_sensor_calibration.mean_u)) {
#if defined(DAMIAO_DM4310)
        debug_console_printf("Sensor U broken!U=%.4f\r\n",
                             (double)startup.output_sensor_calibration.mean_u);
#elif defined(DAMIAO_DM8009)
        debug_console_printf("Sensor U broken!\r\n");
#endif
    }
    if (startup.output_sensor_calibration_ready &&
        !sensor_calibration_current_mean_valid(
            startup.output_sensor_calibration.mean_v)) {
#if defined(DAMIAO_DM4310)
        debug_console_printf("Sensor V broken!V=%.4f\r\n",
                             (double)startup.output_sensor_calibration.mean_v);
#elif defined(DAMIAO_DM8009)
        debug_console_printf("Sensor V broken!\r\n");
#endif
    }
    if (!current_sensor_ok) {
        startup_fault = MOTOR_FAULT_OUTPUT_SENSOR;
    }
    /* The U/V check belongs to the analogue output encoder.  The original
     * never discards the separately measured three-phase current offsets. */
    motor_control_set_current_calibration(
        &g_app.motor, startup.adc_calibration.phase_offset_u,
        startup.adc_calibration.phase_offset_v,
        startup.adc_calibration.phase_offset_w);
    safety_set_startup_fault(&g_app.safety, startup_fault);
    g_app.motor.feedback.fault = startup_fault;
    require_device_authentication();
    motor_control_configure(&g_app.motor, &g_app.config,
                            g_app.motor.feedback.bus_voltage);

    /* validate_current_sensors configures the ADC trigger here, but main
     * does not route or enable IRQ002 until after pwm_timer_init. */
    const bool adc_runtime_configured =
        board_adc_configure_runtime_sampling();
    startup.adc_runtime_ready = startup.output_sensor_calibration_ready &&
                                adc_runtime_configured;
    startup.mcan_ready = board_mcan_init(
        g_app.config.can_id, g_app.config.can_data_rate_selector);
    recovery_transport_ready = startup.uart_ready || startup.mcan_ready;

    return startup.position_ready && startup.calibration_ready &&
           current_sensor_ok && startup.bus_voltage_valid &&
           startup.adc_runtime_ready && startup.mcan_ready &&
           (startup_fault == MOTOR_FAULT_NONE);
}

void platform_check_startup_bus_voltage(void)
{
    /* main@0x252f4 prints device information first, then remains in this
     * one-second diagnostic loop when the averaged reading exceeds 32 V. */
    if (startup.adc_calibration_ready &&
        (startup.bus_voltage > STARTUP_MAX_BUS_VOLTAGE)) {
        halt_on_bus_overvoltage(startup.bus_voltage);
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
    BoardAdcRawSample raw;
    if ((sample == NULL) || !board_adc_read(&raw)) {
        return false;
    }

    sample->phase_u = (float)raw.phase_u;
    sample->phase_v = (float)raw.phase_v;
    sample->phase_w = (float)raw.phase_w;
    sample->bus_voltage = (float)raw.bus_voltage * BUS_VOLTS_PER_COUNT;
    sample->mos_temperature = temperature_celsius_table[
        (raw.temperature & 0x0FFFU) >> 4U];
    sample->motor_temperature = temperature_celsius_table[
        (raw.auxiliary & 0x0FFFU) >> 4U];
    output_sensor_update(&output_sensor, raw.phase_voltage_u,
                         raw.phase_voltage_v);
    position_sensor_control_tick(&position_sensor);
    sample->rotor_angle = position_sensor.wrapped_angle;
    sample->rotor_velocity = position_sensor.rotor_velocity;
    sample->analog_output_position = output_sensor.continuous_angle;
    sample->output_position = position_sensor.output_position;
    sample->output_velocity = position_sensor.output_velocity;
    return true;
}

bool platform_read_position(float *rotor_position, float *rotor_angle,
                            float *output_position, uint16_t *raw_position)
{
    uint16_t dma_word;
    if ((rotor_position == NULL) || (rotor_angle == NULL) ||
        (output_position == NULL) || (raw_position == NULL) ||
        !board_position_take_sample(&dma_word)) {
        return false;
    }

    position_sensor_update(&position_sensor, dma_word);
    *rotor_position = position_sensor.continuous_angle;
    *rotor_angle = position_sensor.wrapped_angle;
    *output_position = position_sensor.output_position;
    *raw_position = position_sensor.raw_position;
    return true;
}

void platform_write_pwm(PhaseDuty duty)
{
    board_sampling_timer_write_pwm(duty.a, duty.b, duty.c);
}

void platform_set_status_led(PlatformLedColor color)
{
    switch (color) {
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

void platform_set_position_zero_offsets(float output_offset,
                                        float motor_output_offset)
{
    output_sensor_set_zero_offset(&output_sensor, output_offset);
    position_sensor_set_output_offset(&position_sensor, motor_output_offset);
}

void platform_set_motor_encoder_direction(bool inverted)
{
    position_sensor_set_inverted(&position_sensor, inverted);
}

bool platform_commissioning_begin(void)
{
    require_device_authentication();
    NVIC_DisableIRQ(INT002_IRQn);
    return true;
}

void platform_commissioning_drive(float voltage_d, float voltage_q,
                                  float electrical_angle)
{
    float sine;
    float cosine;
    motor_fast_sincos(motor_wrapf(electrical_angle,
                                  -3.1415927410125732f,
                                  3.1415927410125732f),
                      &sine, &cosine);
    const DirectQuadrature rotating = {
        .d = voltage_d,
        .q = voltage_q,
    };
    platform_write_pwm(motor_svpwm(
        motor_inverse_park(rotating, sine, cosine)));
}

void platform_commissioning_delay_us(uint32_t microseconds)
{
    board_delay_us(microseconds);
}

bool platform_commissioning_read_sample(PlatformCommissioningSample *sample)
{
    if (sample == NULL) {
        return false;
    }
    BoardAdcRawSample raw;
    while (!board_adc_read(&raw)) {
        /* The factory commissioning routines wait for the next ADC result.
         * They do not turn a temporarily late conversion into a setup
         * failure or advance the energized sequence without a sample. */
    }
    sample->current_u = (g_app.motor.current_offset_u - (float)raw.phase_u) *
                        g_app.motor.current_scale;
    sample->current_v = (g_app.motor.current_offset_v - (float)raw.phase_v) *
                        g_app.motor.current_scale;
    sample->current_w = (g_app.motor.current_offset_w - (float)raw.phase_w) *
                        g_app.motor.current_scale;
    sample->bus_voltage = (float)raw.bus_voltage * BUS_VOLTS_PER_COUNT;
    output_sensor_update(&output_sensor, raw.phase_voltage_u,
                         raw.phase_voltage_v);
    /* The factory commissioning frames and extrema use the filtered ADC
     * values produced by the analogue sensor state, not the just-read raw
     * conversion words.  VCVT.U32.F32 truncates them before packing. */
    sample->output_raw_u = (uint16_t)output_sensor.filtered_u;
    sample->output_raw_v = (uint16_t)output_sensor.filtered_v;
    sample->output_position = output_sensor.continuous_angle;
    sample->output_uncorrected_angle =
        output_sensor_uncorrected_angle(&output_sensor);
    return true;
}

void platform_commissioning_finish_sample(void)
{
    /* Energized factory loops update their estimator and PWM before they
     * clear the three ADC completion flags.  INT002 stays disabled until
     * platform_commissioning_end(). */
    board_adc_ack_polling_sample();
}

bool platform_commissioning_get_output_calibration(float calibration[4])
{
    if (calibration == NULL) {
        return false;
    }
    memcpy(calibration, output_sensor_calibration,
           sizeof(output_sensor_calibration));
    return true;
}

void platform_commissioning_reset_motor_encoder(void)
{
    memset(motor_encoder_correction, 0, sizeof(motor_encoder_correction));
    position_sensor.correction_table = motor_encoder_correction;
    position_sensor_set_inverted(&position_sensor, true);
}

bool platform_commissioning_apply_output_calibration(
    const float calibration[4], uint16_t initial_u, uint16_t initial_v)
{
    if (!output_sensor_set_calibration(&output_sensor, calibration,
                                       (float)initial_u,
                                       (float)initial_v)) {
        return false;
    }
    memcpy(output_sensor_calibration, calibration,
           sizeof(output_sensor_calibration));
    return true;
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
    BoardMcanFrame received;
    if ((frame == NULL) || !board_mcan_receive(&received)) {
        return false;
    }
    frame->id = received.id;
    frame->length = received.length;
    memcpy(frame->data, received.data, received.length);
    return true;
}

void platform_mcan_send(const CanFrame *frame)
{
    if ((frame != NULL) && (frame->length <= 8U)) {
        BoardMcanFrame outgoing;
        outgoing.id = frame->id;
        outgoing.length = frame->length;
        memcpy(outgoing.data, frame->data, frame->length);
        board_mcan_send(&outgoing);
    }
}
void platform_update_mcan_node_filter(uint16_t node_id)
{
    board_mcan_update_node_filter(node_id);
}
bool platform_reconfigure_mcan(void)
{
    /* The reference sends the write/legacy-ID acknowledgement at the old
     * bit rate, waits 5 ms for it to leave the Tx FIFO, and only then applies
     * the new filter/timing image. */
    platform_commissioning_delay_us(5000U);
    return board_mcan_init(g_app.config.can_id,
                           g_app.config.can_data_rate_selector);
}
bool platform_load_parameters(MotorConfig *config)
{
    return (config != NULL) && app_config_decode_persistent(
        config, (const uint32_t *)APP_CONFIG_FLASH_ADDRESS);
}

bool platform_store_parameters(const MotorConfig *config)
{
    if (config == NULL) {
        return false;
    }
    uint32_t record[APP_CONFIG_WORD_COUNT];
    app_config_encode_persistent(config, record);
    return board_flash_replace_sector_prefix(APP_CONFIG_FLASH_ADDRESS,
                                              record, sizeof(record));
}
bool platform_store_factory_parameters(void)
{
    uint32_t record[APP_CONFIG_WORD_COUNT];
    app_config_encode_factory_persistent(record);
    return board_flash_replace_sector_prefix(APP_CONFIG_FLASH_ADDRESS,
                                              record, sizeof(record));
}
bool platform_load_motor_calibration(MotorController *controller)
{
    if (controller == NULL) {
        return false;
    }
    const uint32_t *const stored_motor =
        (const uint32_t *)MOTOR_ENCODER_CALIBRATION_FLASH_ADDRESS;
    float stored_direction;
    sensor_calibration_decode_motor_record(
        stored_motor, motor_encoder_correction,
        &controller->electrical_offset, &stored_direction);
    g_app.config.direction = stored_direction;
    g_app.config.sensor_inverted = stored_direction == 1.0f;

#if defined(DAMIAO_DM8009)
    float i_sensor_max;
    float i_sensor_min;
    const MotorFault i_sensor_fault =
        sensor_calibration_validate_current_record(
            motor_encoder_correction,
            MOTOR_ENCODER_CORRECTION_COUNT,
            &i_sensor_max, &i_sensor_min);
    if (i_sensor_fault == MOTOR_FAULT_OUTPUT_SENSOR) {
        debug_console_printf(
            "I-sensor fail!Max=%.4f Min=%.4f\r\n",
            (double)i_sensor_max, (double)i_sensor_min);
    } else if (i_sensor_fault == MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING) {
        debug_console_printf("Error,O-sensor need calibration!\r\n");
    }

    /* DM8009 0x3a000 layout: [4 float params][256 float corrections],
     * 1040 bytes total.  O-sensor validation and lookup both skip the
     * leading four parameters and operate on the table at byte 16. */
    const float *const stored_8009_table =
        (const float *)OUTPUT_SENSOR_TABLE_FLASH_ADDRESS;
    memcpy(output_sensor_correction, stored_8009_table + 4U,
           CORRECTION_TABLE_COUNT * sizeof(CorrectionTableEntry));
    float o_sensor_max;
    float o_sensor_min;
    output_sensor_table_fault = sensor_calibration_validate_output_record(
        (const float *)output_sensor_correction,
        CORRECTION_TABLE_COUNT, &o_sensor_max, &o_sensor_min);
    output_sensor_table_valid = output_sensor_table_fault == MOTOR_FAULT_NONE;
    if (output_sensor_table_fault == MOTOR_FAULT_OUTPUT_CALIBRATION) {
        debug_console_printf("O-sensor fail!Max=%.4f Min=%.4f\r\n",
                             (double)o_sensor_max, (double)o_sensor_min);
    } else if (!output_sensor_table_valid) {
        debug_console_printf("Error,O-sensor need calibration!\r\n");
    }
#elif defined(DAMIAO_DM4310)
    const uint16_t *const stored_output_table =
        (const uint16_t *)OUTPUT_SENSOR_TABLE_FLASH_ADDRESS;
    memcpy(output_sensor_correction, stored_output_table,
           CORRECTION_TABLE_COUNT * sizeof(CorrectionTableEntry));
    float maximum_step;
    output_sensor_table_fault = sensor_calibration_validate_position(
        stored_output_table, POSITION_CALIBRATION_SAMPLE_COUNT,
        &maximum_step);
    output_sensor_table_valid = output_sensor_table_fault == MOTOR_FAULT_NONE;
    if (output_sensor_table_fault == MOTOR_FAULT_OUTPUT_CALIBRATION) {
        debug_console_printf("O-sensor fail!Max=%.4f\r\n",
                             (double)maximum_step);
    } else if (!output_sensor_table_valid) {
        debug_console_printf("Error,O-sensor need calibration!\r\n");
    }
#endif
    const float *const stored_output_parameters =
        (const float *)OUTPUT_SENSOR_PARAMETERS_FLASH_ADDRESS;
    memcpy(output_sensor_calibration, stored_output_parameters,
           sizeof(output_sensor_calibration));
    calibration_store_decode_zero(
        controller, (const uint32_t *)ZERO_POSITION_FLASH_ADDRESS);
    return true;
}

bool platform_store_output_sensor_calibration(
    const uint16_t correction_table[4096], const float calibration[4])
{
    if ((correction_table == NULL) || (calibration == NULL)) {
        return false;
    }
#if defined(DAMIAO_DM8009)
    /* DM8009 stores [4 float params][256 float corrections] at 0x3a000
     * in one 1040-byte block.  The uint16[4096] upload buffer is first
     * converted to a float table, then packed with the parameters. */
    static float packed_8009[4 + 256];
    memcpy(packed_8009, calibration, 4U * sizeof(float));
    for (size_t index = 0U; index < CORRECTION_TABLE_COUNT; ++index) {
        packed_8009[4U + index] = (float)correction_table[index];
    }
    if (!board_flash_replace_sector_prefix(
            OUTPUT_SENSOR_TABLE_FLASH_ADDRESS, packed_8009,
            sizeof(packed_8009))) {
        return false;
    }
    memcpy(output_sensor_correction, packed_8009 + 4,
           CORRECTION_TABLE_COUNT * sizeof(CorrectionTableEntry));
    memcpy(output_sensor_calibration, calibration,
           sizeof(output_sensor_calibration));
    output_sensor.correction_table = output_sensor_correction;
    output_sensor_table_valid = true;
    output_sensor_table_fault = MOTOR_FAULT_NONE;
    return true;
#else
    if (!board_flash_replace_sector_prefix(
            OUTPUT_SENSOR_TABLE_FLASH_ADDRESS, correction_table,
            4096U * sizeof(uint16_t))) {
        return false;
    }
    if (!board_flash_replace_sector_prefix(
            OUTPUT_SENSOR_PARAMETERS_FLASH_ADDRESS, calibration,
            4U * sizeof(float))) {
        return false;
    }
    memcpy(output_sensor_correction, correction_table,
           CORRECTION_TABLE_COUNT * sizeof(CorrectionTableEntry));
    memcpy(output_sensor_calibration, calibration,
           sizeof(output_sensor_calibration));
    output_sensor.correction_table = output_sensor_correction;
    output_sensor_table_valid = true;
    output_sensor_table_fault = MOTOR_FAULT_NONE;
    return true;
#endif
}

bool platform_store_motor_encoder_calibration(
    const uint32_t record[259])
{
    if ((record == NULL) || !board_flash_replace_sector_prefix(
            MOTOR_ENCODER_CALIBRATION_FLASH_ADDRESS, record,
            259U * sizeof(uint32_t))) {
        return false;
    }
    float electrical_offset;
    float direction;
    sensor_calibration_decode_motor_record(
        record, motor_encoder_correction, &electrical_offset, &direction);
    g_app.motor.electrical_offset = electrical_offset;
    g_app.config.direction = direction;
    g_app.config.sensor_inverted = direction == 1.0f;
    position_sensor.correction_table = motor_encoder_correction;
    position_sensor_set_inverted(&position_sensor,
                                 g_app.config.sensor_inverted);
    return true;
}

bool platform_store_zero_position(const MotorController *controller)
{
    if (controller == NULL) {
        return false;
    }
    uint32_t record[ZERO_POSITION_RECORD_WORD_COUNT];
    calibration_store_encode_zero(controller, record);
    return board_flash_replace_sector_prefix(ZERO_POSITION_FLASH_ADDRESS,
                                              record, sizeof(record));
}
bool platform_recovery_transport_ready(void)
{
    return recovery_transport_ready;
}
bool platform_confirm_application_boot(void)
{
    const BootPersistentRecord *const stored =
        (const BootPersistentRecord *)APP_BOOT_RECORD_ADDRESS;
    /* select_configuration_bank_a@0x26de8 is called solely when word 1 is
     * not one.  Match that predicate rather than adding a policy check on
     * word 0; the write below still normalizes both words to (0, 1). */
    if (stored->application_confirmed == 1U) {
        return true;
    }
    BootPersistentRecord record;
    memcpy(&record, stored, sizeof(record));
    boot_record_confirm_application(&record);
    return board_flash_replace_sector_prefix(APP_BOOT_RECORD_ADDRESS,
                                              &record, sizeof(record));
}

bool platform_update_application_identity(void)
{
    const BootPersistentRecord *const stored =
        (const BootPersistentRecord *)APP_BOOT_RECORD_ADDRESS;
    if (stored->application_identity == APP_APPLICATION_IDENTITY) {
        return true;
    }
    BootPersistentRecord record;
    memcpy(&record, stored, sizeof(record));
    boot_record_set_application_identity(&record);
    return board_flash_replace_sector_prefix(APP_BOOT_RECORD_ADDRESS,
                                              &record, sizeof(record));
}

bool platform_read_device_identity(uint32_t *device_id,
                                   uint32_t *application_identity)
{
    if ((device_id == NULL) || (application_identity == NULL)) {
        return false;
    }
    const BootPersistentRecord *const record =
        (const BootPersistentRecord *)APP_BOOT_RECORD_ADDRESS;
    *device_id = record->device_id;
    *application_identity = record->application_identity;
    return true;
}
uint8_t platform_read_hardware_variant(void) { return cached_hardware_variant; }
bool platform_debug_receive(uint8_t *byte) { return board_uart_receive(byte); }
void platform_debug_write(const void *data, size_t length)
{
    board_uart_write(data, length);
}
void platform_ack_position_timer_irq(void)
{
    board_position_handle_timer_interrupt();
}
void platform_ack_position_dma_irq(void)
{
    board_position_ack_dma_interrupt();
}
void platform_ack_adc_irq(void) { board_adc_ack_interrupt(); }
uint8_t platform_ack_mcan_irq(void) { return board_mcan_ack_interrupt(); }
void platform_ack_debug_uart_irq(void) { board_uart_ack_interrupt(); }

static __attribute__((noinline)) bool platform_request_bootloader_record(void)
{
    BootPersistentRecord record;
    memcpy(&record, (const void *)APP_BOOT_RECORD_ADDRESS, sizeof(record));
    boot_record_request_update(&record);
    return board_flash_replace_sector_prefix(APP_BOOT_RECORD_ADDRESS,
                                             &record, sizeof(record));
}

void platform_enter_bootloader(void)
{
    __disable_irq();

    platform_request_bootloader_record();

    board_delay_us(100U);
    __DSB();
    NVIC_SystemReset();
}

void platform_enter_bootloader_from_can(void)
{
    platform_request_bootloader_record();

    board_delay_ms(10U);
    __DSB();
    NVIC_SystemReset();
}

void platform_system_reset(void)
{
    /* UgU uses the original post-TX 100 us reset sequence. */
    __disable_irq();
    board_delay_us(100U);
    __DSB();
    NVIC_SystemReset();
}
