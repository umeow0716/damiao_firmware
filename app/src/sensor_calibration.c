#include "sensor_calibration.h"

#include <math.h>
#include <string.h>

#include "motor_encoder_calibration.h"
#include "motor_math.h"

/* Literal constants used by load_and_validate_calibration at 0x22658. */
#define POSITION_COUNT_TO_RAD 0x1.921fb6p-10f
#define PI_F                  0x1.921fb6p+1f
#define TWO_PI_F              0x1.921fb6p+2f
#define MAX_POSITION_STEP     0x1.657184p-5f

/* validate_current_sensors at 0x24ec0 implements this interval through an
 * unsigned float-bit range comparison.  Naming the resulting limits is much
 * clearer and produces the same decision for finite positive ADC means. */
#define CURRENT_SENSOR_MINIMUM 186.18182373046875f
#define CURRENT_SENSOR_MAXIMUM 3909.818359375f

MotorFault sensor_calibration_validate_position(
    const uint16_t *samples, size_t sample_count, float *maximum_step)
{
    float largest = 0.0f;
    if ((samples == NULL) ||
        (sample_count != POSITION_CALIBRATION_SAMPLE_COUNT)) {
        if (maximum_step != NULL) {
            *maximum_step = INFINITY;
        }
        return MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING;
    }

    for (size_t index = 1U; index < sample_count; ++index) {
        const uint16_t previous = samples[index - 1U];
        const uint16_t current = samples[index];
        if ((previous == UINT16_MAX) || (current == UINT16_MAX)) {
            if (maximum_step != NULL) {
                *maximum_step = INFINITY;
            }
            return MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING;
        }

        float step = fabsf(((float)current - (float)previous) *
                           POSITION_COUNT_TO_RAD);
        if (step > PI_F) {
            step = fabsf(step - TWO_PI_F);
        }
        if (step > largest) {
            largest = step;
        }
    }

    if (maximum_step != NULL) {
        *maximum_step = largest;
    }
    return (largest <= MAX_POSITION_STEP) ? MOTOR_FAULT_NONE :
                                            MOTOR_FAULT_OUTPUT_CALIBRATION;
}

bool sensor_calibration_decode_motor_record(
    const uint32_t record[MOTOR_ENCODER_RECORD_WORD_COUNT],
    float correction[MOTOR_ENCODER_CORRECTION_COUNT],
    float *electrical_offset, float *direction)
{
    if ((record == NULL) || (correction == NULL) ||
        (electrical_offset == NULL) || (direction == NULL)) {
        return false;
    }

    /* load_and_validate_calibration@0x22658 does not reject an erased or
     * partially invalid motor record.  Its integer predicate is abs(bits) >
     * 0x7f800000: each NaN is normalized independently, while infinities are
     * deliberately preserved. */
    for (size_t index = 0U; index < MOTOR_ENCODER_CORRECTION_COUNT; ++index) {
        float value;
        memcpy(&value, &record[index], sizeof(value));
        correction[index] = isnan(value) ? 0.0f : value;
    }
    float offset;
    float stored_direction;
    memcpy(&offset, &record[MOTOR_ENCODER_ELECTRICAL_OFFSET_WORD],
           sizeof(offset));
    memcpy(&stored_direction, &record[MOTOR_ENCODER_DIRECTION_WORD],
           sizeof(stored_direction));
    *electrical_offset = isnan(offset) ? 0.0f : offset;
    *direction = isnan(stored_direction) ? 1.0f : stored_direction;
    return true;
}

bool sensor_calibration_validate_output_parameters(
    const float calibration[4])
{
    return (calibration != NULL) && isfinite(calibration[0]) &&
           isfinite(calibration[1]) && isfinite(calibration[2]) &&
           isfinite(calibration[3]) &&
           (calibration[0] >= 0.0f) && (calibration[0] <= 4095.0f) &&
           (calibration[1] >= 0.0f) && (calibration[1] <= 4095.0f) &&
           (calibration[2] >= 0.25f) && (calibration[2] <= 4.0f);
}

static float wrap_signed(float angle)
{
    while (angle > PI_F) {
        angle -= TWO_PI_F;
    }
    while (angle < -PI_F) {
        angle += TWO_PI_F;
    }
    return angle;
}

bool sensor_calibration_analyze_output_extrema(
    const OutputSensorExtrema *extrema, float calibration[4])
{
    if ((extrema == NULL) || (calibration == NULL)) {
        return false;
    }
    const float span_u = (float)(extrema->maximum_u - extrema->minimum_u);
    const float span_v = (float)(extrema->maximum_v - extrema->minimum_v);

    calibration[0] = 0.5f *
        (float)((uint32_t)extrema->maximum_u + extrema->minimum_u);
    calibration[1] = 0.5f *
        (float)((uint32_t)extrema->maximum_v + extrema->minimum_v);
    calibration[2] = span_u / span_v;
    const float maximum_phase = wrap_signed(
        extrema->angle_at_maximum_v - extrema->angle_at_maximum_u);
    const float minimum_phase = wrap_signed(
        extrema->angle_at_minimum_v - extrema->angle_at_minimum_u);
    calibration[3] = 0.5f * (maximum_phase + minimum_phase);
    /* calibrate_position_sensor@0x246f8 writes the four derived values
     * directly.  It has no additional span, finite-value or phase gate. */
    return true;
}

bool sensor_calibration_current_means_valid(float sensor_u, float sensor_v)
{
    return sensor_calibration_current_mean_valid(sensor_u) &&
           sensor_calibration_current_mean_valid(sensor_v);
}

bool sensor_calibration_current_mean_valid(float sensor_mean)
{
    return isfinite(sensor_mean) &&
           (sensor_mean >= CURRENT_SENSOR_MINIMUM) &&
           (sensor_mean < CURRENT_SENSOR_MAXIMUM);
}

bool sensor_calibration_startup_bus_valid(float raw_average,
                                          float volts_per_count,
                                          float maximum_voltage,
                                          float *bus_voltage)
{
    if ((bus_voltage == NULL) || !isfinite(raw_average) ||
        !isfinite(volts_per_count) || !isfinite(maximum_voltage) ||
        (raw_average < 0.0f) || (volts_per_count <= 0.0f) ||
        (maximum_voltage <= 0.0f)) {
        return false;
    }
    const float voltage = raw_average * volts_per_count;
    if (!isfinite(voltage)) {
        return false;
    }
    *bus_voltage = voltage;
    return voltage <= maximum_voltage;
}
