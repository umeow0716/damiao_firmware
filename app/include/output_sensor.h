#ifndef DAMIAO_OUTPUT_SENSOR_H
#define DAMIAO_OUTPUT_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#include "app_profile.h"

typedef uint16_t CorrectionTableEntry;
#define CORRECTION_TABLE_COUNT 4096U

typedef struct {
    float filtered_u;
    float filtered_v;
    float filter_previous_weight;
    float filter_new_weight;
    float centered_u;
    float center_u;
    float centered_v;
    float center_v;
    uint32_t reserved_20;
    uint32_t reserved_24;
    float gain_v;
    float phase_sine;
    float phase_cosine;
    float wrapped_angle;
    float zero_offset;
    float continuous_angle;
    float previous_angle;
    int32_t revolutions;
} OutputSensorState;

void output_sensor_init(OutputSensorState *state,
                        const float calibration[4],
                        const CorrectionTableEntry correction_table[CORRECTION_TABLE_COUNT],
                        float zero_offset,
                        float initial_u, float initial_v);
void output_sensor_update(OutputSensorState *state,
                          uint16_t raw_u, uint16_t raw_v);
#if defined(DAMIAO_DM4310)
float output_sensor_update_control_irq(OutputSensorState *state,
                                       const volatile uint16_t *raw);
#endif
#if defined(DAMIAO_DM4310)
void output_sensor_load_persistent_calibration(
    OutputSensorState *state, const float calibration[4]);
float output_sensor_lookup_and_unwrap(OutputSensorState *state);
float dm4310_output_sensor_helper(OutputSensorState *state);
float dm4310_output_atan2f(float y, float x);
float dm4310_atan2_helper(float y, float x);
#endif
float output_sensor_normalize_startup(OutputSensorState *state);
void output_sensor_set_zero_offset(OutputSensorState *state,
                                   float zero_offset);
float output_sensor_zero_current(OutputSensorState *state);
bool output_sensor_set_calibration(OutputSensorState *state,
                                   const float calibration[4],
                                   float initial_u, float initial_v);
float output_sensor_uncorrected_angle(const OutputSensorState *state);

#endif
