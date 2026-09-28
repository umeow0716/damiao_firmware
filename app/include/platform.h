#ifndef DAMIAO_APP_PLATFORM_H
#define DAMIAO_APP_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "can_protocol.h"
#include "motor_control.h"
#include "motor_math.h"
#include "motor_types.h"

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
bool platform_initialize_runtime(void);
void platform_check_startup_bus_voltage(void);
bool platform_start_control_loop(void);
void platform_idle(void);
bool platform_read_adc(AdcSample *sample);
bool platform_read_position(float *rotor_position, float *rotor_angle,
                            float *output_position, uint16_t *raw_position);
void platform_write_pwm(PhaseDuty duty);
void platform_set_status_led(PlatformLedColor color);
void platform_toggle_fault_indicator(void);
void platform_set_position_zero_offsets(float output_offset,
                                        float motor_output_offset);
void platform_set_motor_encoder_direction(bool inverted);
bool platform_commissioning_begin(void);
void platform_commissioning_drive(float voltage_d, float voltage_q,
                                  float electrical_angle);
void platform_commissioning_delay_us(uint32_t microseconds);
bool platform_commissioning_read_sample(PlatformCommissioningSample *sample);
void platform_commissioning_finish_sample(void);
bool platform_commissioning_get_output_calibration(float calibration[4]);
void platform_commissioning_reset_motor_encoder(void);
bool platform_commissioning_apply_output_calibration(
    const float calibration[4], uint16_t initial_u, uint16_t initial_v);
void platform_commissioning_end(void);
#if defined(DAMIAO_DM8009)
bool platform_store_output_sensor_calibration(
    const float correction_table[256], const float calibration[4]);
#else
bool platform_store_output_sensor_calibration(
    const uint16_t correction_table[4096], const float calibration[4]);
#endif
bool platform_store_motor_encoder_calibration(
    const uint32_t record[259]);
bool platform_mcan_receive(CanFrame *frame);
void platform_mcan_send(const CanFrame *frame);
void platform_update_mcan_node_filter(uint16_t node_id);
bool platform_reconfigure_mcan(void);
bool platform_load_parameters(MotorConfig *config);
bool platform_store_parameters(const MotorConfig *config);
bool platform_store_factory_parameters(void);
bool platform_load_motor_calibration(MotorController *controller);
bool platform_store_zero_position(const MotorController *controller);
bool platform_recovery_transport_ready(void);
bool platform_confirm_application_boot(void);
bool platform_update_application_identity(void);
bool platform_read_device_identity(uint32_t *device_id,
                                   uint32_t *application_identity);
uint8_t platform_read_hardware_variant(void);
bool platform_debug_receive(uint8_t *byte);
void platform_debug_write(const void *data, size_t length);
void platform_ack_position_timer_irq(void);
void platform_ack_position_dma_irq(void);
void platform_ack_adc_irq(void);
uint8_t platform_ack_mcan_irq(void);
void platform_ack_debug_uart_irq(void);
void platform_enter_bootloader(void);
void platform_enter_bootloader_from_can(void);
void platform_system_reset(void);

#endif
