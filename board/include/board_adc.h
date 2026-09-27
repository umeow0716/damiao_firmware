#ifndef DAMIAO_BOARD_ADC_H
#define DAMIAO_BOARD_ADC_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t phase_u;
    uint16_t phase_v;
    uint16_t phase_w;
    uint16_t bus_voltage;
    uint16_t temperature;
    uint16_t phase_voltage_u;
    uint16_t phase_voltage_v;
    uint16_t phase_voltage_w;
    uint16_t auxiliary;
} BoardAdcRawSample;

typedef struct {
    uint32_t channel_select;
    uint16_t adc1_channel_mux;
    uint16_t adc2_channel_mux;
    uint16_t adc3_channel_mux;
    uint16_t trigger_select;
    uint16_t sync_control;
    uint8_t sample_time;
    uint16_t analog_pin_control;
    uint16_t auxiliary_pin_control;
    uint32_t aos_trigger;
    uint32_t irq_source;
} BoardAdcRegisterImage;

typedef struct {
    float phase_offset_u;
    float phase_offset_v;
    float phase_offset_w;
    float bus_voltage_raw;
} BoardAdcStartupCalibration;

typedef struct {
    float mean_u;
    float mean_v;
} BoardAdcOutputSensorCalibration;

void board_adc_build_config(BoardAdcRegisterImage *config);
bool board_adc_init(void);
bool board_adc_calibrate_startup(BoardAdcStartupCalibration *calibration);
bool board_adc_calibrate_output_sensor(
    BoardAdcOutputSensorCalibration *calibration);
bool board_adc_configure_runtime_sampling(void);
bool board_adc_enable_runtime_irq(void);
bool board_adc_read(BoardAdcRawSample *sample);
void board_adc_ack_polling_sample(void);
void board_adc_ack_interrupt(void);

#endif
