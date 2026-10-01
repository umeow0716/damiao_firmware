#ifndef DAMIAO_APP_PLATFORM_H
#define DAMIAO_APP_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "can_protocol.h"
#include "motor_control.h"
#include "motor_math.h"
#include "motor_types.h"
#include "sensor_calibration.h"

#if defined(DAMIAO_DM4310)
typedef struct Dm4310McanIrqReferences Dm4310McanIrqReferences;
#endif

typedef enum {
    PLATFORM_LED_OFF = 0,
    PLATFORM_LED_RED,
    PLATFORM_LED_GREEN,
} PlatformLedColor;

typedef struct {
    float current_u;
    float current_v;
    float current_w;
    float bus_voltage;
    float output_position;
    float output_uncorrected_angle;
    uint16_t output_raw_u;
    uint16_t output_raw_v;
} PlatformCommissioningSample;

bool platform_early_init(void);
void platform_prepare_board_startup(void);
bool platform_initialize_peripherals(void);
#if defined(DAMIAO_DM4310)
void platform_prepare_runtime_configuration(void);
#endif
bool platform_initialize_runtime(void);
void platform_require_device_authentication(void);
void platform_check_startup_bus_voltage(void);
bool platform_start_control_loop(void);
void platform_idle(void);
bool platform_read_adc(AdcSample *sample);
#if defined(DAMIAO_DM4310)
const volatile uint16_t *platform_read_adc_control(void);
volatile struct Dm4310PositionSensorScratch *platform_finish_adc_sensor_sample(AdcSample *sample,
                                       const volatile uint16_t *raw);
void platform_finish_adc_velocity_sample(AdcSample *sample,
    volatile struct Dm4310PositionSensorScratch *scratch, float cleared,
    const Dm4310OuterLoopReferences *references);
#endif
bool platform_read_position(float *rotor_position, float *rotor_angle,
                            float *output_position, uint16_t *raw_position);
#if defined(DAMIAO_DM4310)
volatile uint32_t *platform_read_position_dma(void);
#endif
void platform_write_pwm(PhaseDuty duty);
void platform_set_status_led(PlatformLedColor color);
void platform_toggle_fault_indicator(void);
void platform_set_position_zero_offsets(float output_offset,
                                        float motor_output_offset);
void platform_set_motor_encoder_direction(bool inverted);
bool platform_commissioning_begin(void);
#if defined(DAMIAO_DM4310)
bool platform_commissioning_begin_unauthenticated(void);
#endif
void platform_commissioning_drive(float voltage_d, float voltage_q,
                                  float electrical_angle);
void platform_commissioning_delay_us(uint32_t microseconds);
bool platform_commissioning_read_sample(PlatformCommissioningSample *sample);
#if defined(DAMIAO_DM4310)
void platform_commissioning_wait_identification_sample(void);
/* Read the completed conversion after wait; do not poll a second time. */
bool platform_commissioning_read_identification_sample(
    PlatformCommissioningSample *sample, bool publish_projected_bus);
#else
#define platform_commissioning_read_identification_sample(sample, projected) \
    platform_commissioning_read_sample(sample)
#endif
void platform_commissioning_finish_sample(void);
#if defined(DAMIAO_DM4310)
void platform_commissioning_finish_output_sample(void);
void platform_commissioning_prime_output_filter(void);
void platform_commissioning_publish_output_extrema(const OutputSensorExtrema *extrema);
void platform_commissioning_read_extrema_sample(PlatformCommissioningSample *sample);
void platform_commissioning_read_offset_sample(
    PlatformCommissioningSample *sample, float *atan_y, float *atan_x);
#endif
void platform_commissioning_restore_control_irq(void);
#if defined(DAMIAO_DM4310)
void platform_commissioning_finish_alignment(void);
#endif
bool platform_commissioning_get_output_calibration(float calibration[4]);
void platform_commissioning_reset_motor_encoder(void);
void platform_commissioning_end(void);
bool platform_store_output_sensor_calibration(
    const uint16_t correction_table[4096], const float calibration[4]);
bool platform_store_motor_encoder_calibration(
    const uint32_t record[259]);
bool platform_mcan_receive(CanFrame *frame);
void platform_mcan_send(const CanFrame *frame);
#if defined(DAMIAO_DM4310)
void platform_mcan_send_prebuilt(uint16_t id, uint8_t length);
#endif
void platform_update_mcan_node_filter(uint16_t node_id);
void platform_select_mcan_transport_format(uint8_t data_rate_selector);
#if defined(DAMIAO_DM4310)
void platform_select_mcan_transport_format_irq(uint8_t data_rate_selector,
    const Dm4310McanIrqReferences *references);
#endif
bool platform_reconfigure_mcan(void);
#if defined(DAMIAO_DM4310)
void platform_reconfigure_mcan_irq(
    const Dm4310McanIrqReferences *references);
#endif
bool platform_load_parameters(MotorConfig *config);
bool platform_store_parameters(const MotorConfig *config);
#if defined(DAMIAO_DM4310)
bool platform_store_staged_parameters(void);
void platform_finish_flash_commit(void);
#endif
bool platform_store_factory_parameters(void);
bool platform_load_motor_calibration(MotorController *controller);
bool platform_store_zero_position(const MotorController *controller);
void platform_zero_current_position(MotorController *controller);
#if defined(DAMIAO_DM4310)
void platform_zero_current_position_irq(
    const Dm4310McanIrqReferences *references);
#endif
bool platform_recovery_transport_ready(void);
bool platform_confirm_application_boot(void);
#if defined(DAMIAO_DM4310)
void platform_derive_control_parameters(void);
void dm4310_derive_control_parameters_helper(void);
void platform_select_configuration_bank_b(void);
void dm4310_select_configuration_bank_b_helper(void);
#endif
bool platform_update_application_identity(void);
bool platform_read_device_identity(uint32_t *device_id,
                                   uint32_t *application_identity);
uint8_t platform_read_hardware_variant(void);
#if defined(DAMIAO_DM4310)
float platform_read_mcan_data_rate_kbps(void);
#endif
bool platform_debug_receive(uint8_t *byte);
#if defined(DAMIAO_DM4310)
bool platform_begin_debug_uart_irq(const uint8_t **data, int16_t *length);
void platform_rearm_debug_uart_irq(void);
#endif
void platform_debug_write(const void *data, size_t length);
void platform_delay_ms(uint32_t milliseconds);
void platform_ack_position_timer_irq(void);
void platform_ack_position_dma_irq(bool sample_ready, volatile uint32_t *dma_count);
void platform_ack_adc_irq(void);
#if defined(DAMIAO_DM4310)
void platform_begin_mcan_irq(Dm4310McanIrqReferences *references);
bool platform_mcan_receive_irq(CanFrame *frame,
    Dm4310McanIrqReferences *references);
void platform_mcan_send_prebuilt_irq(uint16_t id, uint8_t length,
    const Dm4310McanIrqReferences *references);
uint8_t platform_ack_mcan_irq(const Dm4310McanIrqReferences *references);
#else
uint8_t platform_ack_mcan_irq(void);
#endif
void platform_ack_debug_uart_irq(void);
void platform_enter_bootloader(void);
void platform_enter_bootloader_from_can(void);
void platform_system_reset(void);

#endif
