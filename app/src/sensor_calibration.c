#include "sensor_calibration.h"

#include "app_profile.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "motor_encoder_calibration.h"
#include "motor_math.h"

/* Conversion constants used while loading and validating calibration data. */
#define POSITION_COUNT_TO_RAD 0x1.921fb6p-10f
#define TWO_PI_F 0x1.921fb6p+2f

/* Current-sensor validation implements this interval through an unsigned
 * float-bit comparison, which has the same decision for finite positive ADC
 * means. */
#define CURRENT_SENSOR_MINIMUM 186.18182373046875f
#define CURRENT_SENSOR_MAXIMUM 3909.818359375f

MotorFault sensor_calibration_validate_position(const uint16_t *samples, size_t sample_count,
                                                float *maximum_step)
{
    float largest = 0.0f;
    if ((samples == NULL) || (sample_count != POSITION_CALIBRATION_SAMPLE_COUNT))
    {
        if (maximum_step != NULL)
        {
            *maximum_step = INFINITY;
        }
        return MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING;
    }

    for (size_t index = 1U; index < sample_count; ++index)
    {
        volatile const uint16_t *const live_samples = samples;
        const uint16_t current = live_samples[index];
        const uint16_t previous = live_samples[index - 1U];
        float previous_angle;
        float current_angle;
        /* Firmware reads current then previous, and scales both even when
         * either is erased, before branching on the 0xffff sentinel. */
        __asm volatile("vmov %0, %2\nvcvt.f32.u32 %0, %0\n"
                       "vmul.f32 %0, %0, %4\n"
                       "vmov %1, %3\nvcvt.f32.u32 %1, %1\n"
                       "vmul.f32 %1, %1, %4"
                       : "=&t"(previous_angle), "=&t"(current_angle)
                       : "r"((uint32_t)previous), "r"((uint32_t)current), "t"(POSITION_COUNT_TO_RAD)
                       : "memory");
        if ((previous == UINT16_MAX) || (current == UINT16_MAX))
        {
            if (maximum_step != NULL)
            {
                /* Missing samples report the finite sentinel 4096, not infinity. */
                *maximum_step = 4096.0f;
            }
            return MOTOR_FAULT_OUTPUT_CALIBRATION_MISSING;
        }

        /* Firmware converts and scales each unsigned count separately before
         * subtracting.  Combining the subtraction ahead of the multiply can
         * differ by several float ULPs and changes the reported maximum. */
        float step = fabsf(current_angle - previous_angle);
        int32_t step_bits;
        memcpy(&step_bits, &step, sizeof(step_bits));
        if (step_bits > INT32_C(0x40c90fdb))
        {
            step -= TWO_PI_F;
        }
        else if (step_bits > INT32_C(0x40490fdb))
        {
            step = TWO_PI_F - step;
        }
        __asm volatile("vcmpe.f32 %1, %0\nvmrs APSR_nzcv, FPSCR\n"
                       "it gt\nvmovgt.f32 %0, %1"
                       : "+t"(largest)
                       : "t"(step)
                       : "cc");
    }

    if (maximum_step != NULL)
    {
        *maximum_step = largest;
    }
    int32_t largest_bits;
    memcpy(&largest_bits, &largest, sizeof(largest_bits));
    return largest_bits <= INT32_C(0x3d32b8c2) ? MOTOR_FAULT_NONE : MOTOR_FAULT_OUTPUT_CALIBRATION;
}

bool sensor_calibration_decode_motor_record(const uint32_t record[MOTOR_ENCODER_RECORD_WORD_COUNT],
                                            float correction[MOTOR_ENCODER_CORRECTION_COUNT],
                                            float *electrical_offset, float *direction)
{
    if ((record == NULL) || (correction == NULL) || (electrical_offset == NULL) ||
        (direction == NULL))
    {
        return false;
    }

    /* load_and_validate_calibration does not reject an erased or
     * partially invalid motor record.  Its integer predicate is abs(bits) >
     * 0x7f800000: each NaN is normalized independently, while infinities are
     * deliberately preserved. */
    for (size_t index = 0U; index < MOTOR_ENCODER_CORRECTION_COUNT; ++index)
    {
        const uint32_t bits = record[index];
        volatile uint32_t *const destination = (volatile uint32_t *)(void *)&correction[index];
        /* This path publishes raw bits before normalization,
         * without a floating comparison or signaling-NaN exception. */
        *destination = bits;
        if ((bits & UINT32_C(0x7fffffff)) > UINT32_C(0x7f800000))
        {
            __asm volatile("vstr %1, [%0]" : : "r"(destination), "t"(0.0f) : "memory");
        }
    }
    uint32_t offset_bits = record[MOTOR_ENCODER_ELECTRICAL_OFFSET_WORD];
    volatile uint32_t *const fixed_offset = (volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
        UINT32_C(0x1ffff09c), UINT32_C(0x1ffff028));
    *fixed_offset = offset_bits;
    if ((offset_bits & UINT32_C(0x7fffffff)) > UINT32_C(0x7f800000))
    {
        __asm volatile("vstr %1, [%0]" : : "r"(fixed_offset), "t"(0.0f) : "memory");
        offset_bits = 0U;
    }
    uint32_t direction_bits = record[MOTOR_ENCODER_DIRECTION_WORD];
    volatile uint32_t *const fixed_direction =
        (volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff0bc),
                                                              UINT32_C(0x1ffff048));
    *fixed_direction = direction_bits;
    if ((direction_bits & UINT32_C(0x7fffffff)) > UINT32_C(0x7f800000))
    {
        __asm volatile("vstr %1, [%0]" : : "r"(fixed_direction), "t"(1.0f) : "memory");
        direction_bits = UINT32_C(0x3f800000);
    }
    memcpy(electrical_offset, &offset_bits, sizeof(offset_bits));
    memcpy(direction, &direction_bits, sizeof(direction_bits));
    return true;
}

bool sensor_calibration_validate_output_parameters(const float calibration[4])
{
    return (calibration != NULL) && isfinite(calibration[0]) && isfinite(calibration[1]) &&
           isfinite(calibration[2]) && isfinite(calibration[3]) && (calibration[0] >= 0.0f) &&
           (calibration[0] <= 4095.0f) && (calibration[1] >= 0.0f) && (calibration[1] <= 4095.0f) &&
           (calibration[2] >= 0.25f) && (calibration[2] <= 4.0f);
}

static float wrap_signed(float angle)
{
    uint32_t bits;
    memcpy(&bits, &angle, sizeof(bits));
    /* extrema: one signed upper comparison, then
     * one unsigned lower comparison, not floating comparisons or loops. */
    if ((int32_t)bits > (int32_t)UINT32_C(0x40490fdb))
    {
        angle -= TWO_PI_F;
    }
    else if (bits > UINT32_C(0xc0490fdb))
    {
        angle += TWO_PI_F;
    }
    return angle;
}

void sensor_calibration_publish_output_extrema(const OutputSensorExtrema *extrema,
                                               OutputSensorState *sensor,
                                               volatile float calibration[4])
{
    volatile OutputSensorState *const state = sensor;
    float maximum_phase = extrema->angle_at_maximum_v - extrema->angle_at_maximum_u;
    __asm volatile("" : "+t"(maximum_phase) : : "memory");
    state->center_u = 0.5f * (float)((uint32_t)extrema->maximum_u + extrema->minimum_u);
    state->center_v = 0.5f * (float)((uint32_t)extrema->maximum_v + extrema->minimum_v);
    float span_u = (float)((int32_t)extrema->maximum_u - extrema->minimum_u);
    float span_v = (float)((int32_t)extrema->maximum_v - extrema->minimum_v);
    __asm volatile("" : "+t"(span_u), "+t"(span_v) : : "memory");
    maximum_phase = wrap_signed(maximum_phase);
    const float minimum_phase =
        wrap_signed(extrema->angle_at_minimum_v - extrema->angle_at_minimum_u);
    float phase;
    /* Preserve operand order for NaN payload selection as well as finite
     * values: this path adds maximum phase to minimum phase. */
    __asm volatile("vadd.f32 %0, %1, %2\nvmul.f32 %0, %0, %3"
                   : "=&t"(phase)
                   : "t"(maximum_phase), "t"(minimum_phase), "t"(0.5f)
                   : "memory");
    /* Helper publishes sine then cosine directly to the live fields. */
    motor_target_sincos(phase, &sensor->phase_sine, &sensor->phase_cosine);
    float gain = span_u / span_v;
    __asm volatile("" : "+t"(gain) : : "memory");
    state->gain_v = gain;
    calibration[0] = state->center_u;
    calibration[1] = state->center_v;
    calibration[2] = gain;
    calibration[3] = phase;
}

bool sensor_calibration_analyze_output_extrema(const OutputSensorExtrema *extrema,
                                               float calibration[4])
{
    if ((extrema == NULL) || (calibration == NULL))
    {
        return false;
    }
    const float span_u = (float)(extrema->maximum_u - extrema->minimum_u);
    const float span_v = (float)(extrema->maximum_v - extrema->minimum_v);

    calibration[0] = 0.5f * (float)((uint32_t)extrema->maximum_u + extrema->minimum_u);
    calibration[1] = 0.5f * (float)((uint32_t)extrema->maximum_v + extrema->minimum_v);
    calibration[2] = span_u / span_v;
    const float maximum_phase =
        wrap_signed(extrema->angle_at_maximum_v - extrema->angle_at_maximum_u);
    const float minimum_phase =
        wrap_signed(extrema->angle_at_minimum_v - extrema->angle_at_minimum_u);
    calibration[3] = 0.5f * (maximum_phase + minimum_phase);
    /* calibrate_position_sensor writes the four derived values
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
    uint32_t bits;
    memcpy(&bits, &sensor_mean, sizeof(bits));
    /* validate_current_sensors uses ADD/CMP/BCC on the raw float
     * word.  Preserve that unsigned interval transform, including
     * its behavior for non-canonical NaNs and signed values. */
    return (bits + APP_PROFILE_CURRENT_SENSOR_RANGE_OFFSET) <= APP_PROFILE_CURRENT_SENSOR_RANGE_MAX;
}

bool sensor_calibration_startup_bus_valid(float raw_average, float volts_per_count,
                                          float maximum_voltage, float *bus_voltage)
{
    if ((bus_voltage == NULL) || !isfinite(raw_average) || !isfinite(volts_per_count) ||
        !isfinite(maximum_voltage) || (raw_average < 0.0f) || (volts_per_count <= 0.0f) ||
        (maximum_voltage <= 0.0f))
    {
        return false;
    }
    const float voltage = raw_average * volts_per_count;
    if (!isfinite(voltage))
    {
        return false;
    }
    *bus_voltage = voltage;
    return voltage <= maximum_voltage;
}
