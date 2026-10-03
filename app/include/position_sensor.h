#ifndef DAMIAO_POSITION_SENSOR_H
#define DAMIAO_POSITION_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct OuterLoopContext OuterLoopContext;

/* Shared IRQ001/IRQ002 scratch layout; pointers are retained by each IRQ. */
typedef struct PositionSensorScratch
{
    uint16_t raw_position;
    uint16_t reserved_02;
    uint32_t sample_ready;
    float wrapped_angle;
    float previous_angle;
    float accumulated_delta;
    float delta_angle;
} PositionSensorScratch;

typedef struct
{
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
                          const float correction_table[256], float gear_ratio, float output_offset,
                          float velocity_bandwidth, float velocity_sample_frequency,
                          uint8_t velocity_decimation);
void position_sensor_update(PositionSensorState *state, uint16_t dma_word);
volatile uint32_t *position_sensor_update_dma(float wrap_minimum);
bool position_sensor_control_tick(PositionSensorState *state);
bool position_sensor_take_control_sample(volatile PositionSensorScratch *scratch);
void position_sensor_velocity_tick(PositionSensorState *state,
                                   volatile PositionSensorScratch *scratch, float cleared,
                                   const OuterLoopContext *references);
void position_sensor_align_to_output(PositionSensorState *state, float output_position);
void position_sensor_set_inverted(PositionSensorState *state, bool inverted);
void position_sensor_set_output_offset(PositionSensorState *state, float output_offset);
void position_sensor_reset_accumulated_delta(void);
float position_sensor_zero_current(PositionSensorState *state);

#endif
