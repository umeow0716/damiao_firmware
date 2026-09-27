#ifndef DAMIAO_SENSOR_CALIBRATION_H
#define DAMIAO_SENSOR_CALIBRATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "motor_types.h"

#define POSITION_CALIBRATION_SAMPLE_COUNT 4096U

typedef struct {
    uint16_t minimum_u;
    uint16_t maximum_u;
    uint16_t minimum_v;
    uint16_t maximum_v;
    float angle_at_minimum_u;
    float angle_at_maximum_u;
    float angle_at_minimum_v;
    float angle_at_maximum_v;
} OutputSensorExtrema;

MotorFault sensor_calibration_validate_position(
    const uint16_t *samples, size_t sample_count, float *maximum_step);
bool sensor_calibration_validate_output_parameters(
    const float calibration[4]);
bool sensor_calibration_analyze_output_extrema(
    const OutputSensorExtrema *extrema, float calibration[4]);
bool sensor_calibration_current_means_valid(float sensor_u, float sensor_v);
bool sensor_calibration_current_mean_valid(float sensor_mean);
bool sensor_calibration_startup_bus_valid(float raw_average,
                                          float volts_per_count,
                                          float maximum_voltage,
                                          float *bus_voltage);

#endif
