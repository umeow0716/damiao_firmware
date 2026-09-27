#ifndef DM4310_POSITION_SENSOR_H
#define DM4310_POSITION_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const float *correction_table;
    uint16_t raw_position;
    int32_t revolutions;
    float wrapped_angle;
    float previous_angle;
    float delta_angle;
    float accumulated_delta;
    float continuous_angle;
    float output_position;
    float rotor_velocity;
    float output_velocity;
    float gear_ratio;
    float output_scale;
    float output_offset;
    float velocity_previous_weight;
    float velocity_new_weight;
    float velocity_sample_frequency;
    uint8_t velocity_divider;
    uint8_t velocity_decimation;
    bool inverted;
} PositionSensorState;

void position_sensor_init(PositionSensorState *state, bool inverted,
                          const float correction_table[256],
                          float gear_ratio, float output_offset,
                          float velocity_bandwidth,
                          float velocity_sample_frequency,
                          uint8_t velocity_decimation);
void position_sensor_update(PositionSensorState *state, uint16_t dma_word);
void position_sensor_control_tick(PositionSensorState *state);
void position_sensor_align_to_output(PositionSensorState *state,
                                     float output_position);
void position_sensor_set_inverted(PositionSensorState *state, bool inverted);
void position_sensor_set_output_offset(PositionSensorState *state,
                                       float output_offset);

#endif
