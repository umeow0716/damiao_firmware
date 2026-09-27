#ifndef DAMIAO_OUTPUT_SENSOR_H
#define DAMIAO_OUTPUT_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const uint16_t *correction_table;
    float filtered_u;
    float filtered_v;
    float filter_previous_weight;
    float filter_new_weight;
    float center_u;
    float center_v;
    float gain_v;
    float phase_sine;
    float phase_cosine;
    float zero_offset;
    float wrapped_angle;
    float previous_angle;
    float continuous_angle;
    int32_t revolutions;
} OutputSensorState;

void output_sensor_init(OutputSensorState *state,
                        const float calibration[4],
                        const uint16_t correction_table[4096],
                        float zero_offset,
                        float initial_u, float initial_v);
void output_sensor_update(OutputSensorState *state,
                          uint16_t raw_u, uint16_t raw_v);
void output_sensor_normalize_startup(OutputSensorState *state);
void output_sensor_set_zero_offset(OutputSensorState *state,
                                   float zero_offset);
bool output_sensor_set_calibration(OutputSensorState *state,
                                   const float calibration[4],
                                   float initial_u, float initial_v);
float output_sensor_uncorrected_angle(const OutputSensorState *state);

#endif
