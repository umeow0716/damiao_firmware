#include "position_sensor.h"

#include "memory_layout.h"

#include <stddef.h>
#include <string.h>

#include "motor_control.h"
#include "motor_math.h"

/* Exact conversion constants used by the position-sensor DMA interrupt. */
#define COUNTS_PER_BIN 64U
#define COUNT_TO_RAD 0x1.921fb6p-12f
#define TWO_PI 0x1.921fb6p+2f
#define WRAP_THRESHOLD 5.5f
#define VELOCITY_BANDWIDTH_LIMIT_BITS UINT32_C(0x43fa0000)

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* IRQ001 owns this exact 24-byte scratch object.  The ADC/control
 * IRQ consumes the accumulated angle delta at +0x10 every twentieth tick. */
#define position_sensor_scratch                                                                    \
    (*(volatile PositionSensorScratch *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff190),     \
                                                                         UINT32_C(0x1ffff11c)))

SRAM_ABI_ASSERT_SIZE(PositionSensorScratch, 0x18U);
SRAM_ABI_ASSERT_OFFSET(PositionSensorScratch, accumulated_delta, 0x10U);

static float vfp_multiply_add(float accumulator, float lhs, float rhs)
{
    __asm volatile("vmla.f32 %0, %1, %2" : "+t"(accumulator) : "t"(lhs), "t"(rhs));
    return accumulator;
}

static float vfp_negative_multiply_subtract(float accumulator, float lhs, float rhs)
{
    __asm volatile("vnmls.f32 %0, %1, %2" : "+t"(accumulator) : "t"(lhs), "t"(rhs));
    return accumulator;
}

void position_sensor_init(PositionSensorState *state, bool inverted,
                          const float correction_table[256], float gear_ratio, float output_offset,
                          float velocity_bandwidth, float velocity_sample_frequency,
                          uint8_t velocity_decimation)
{
    memset(state, 0, sizeof(*state));
    /* The firmware SPI scratch is initialized by Reset and remains live
     * through configuration; only explicit consumers clear ready/delta. */
    state->correction_table = correction_table;
    state->inverted = inverted;
    state->gear_ratio = gear_ratio;
    state->output_scale = 1.0f / gear_ratio;
    state->output_offset = output_offset;
    state->velocity_sample_frequency = velocity_sample_frequency;
    state->velocity_decimation = velocity_decimation;
    /* load_motor_configuration performs this as a signed comparison
     * of the IEEE-754 word against 500.0f.  This deliberately accepts zero
     * and negative finite values; the persistent-record decoder is where
     * NaN/default and upper-bound normalization happens. */
    if ((int32_t)float_bits(velocity_bandwidth) < (int32_t)VELOCITY_BANDWIDTH_LIMIT_BITS)
    {
        state->velocity_previous_weight =
            velocity_sample_frequency / (velocity_sample_frequency + velocity_bandwidth * TWO_PI);
        state->velocity_new_weight = 1.0f - state->velocity_previous_weight;
    }
    else
    {
        state->velocity_new_weight = 1.0f;
    }
}

/* Keep the fixed-layout DMA pointer/threshold branches outside Thumb IT blocks.
 * Besides matching IRQ001, this prevents conditional literal loads from
 * carrying a stale base register into the fixed 0x1fff8bxx pool. */
__attribute__((optimize("no-if-conversion", "no-if-conversion2"))) static volatile uint32_t *
update_position(PositionSensorState *state, uint16_t dma_word, bool dma_irq, float wrap_minimum)
{
    volatile uint32_t *dma_count = NULL;
    const uintptr_t motor =
        dma_irq ? *(const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B50UL, 0x1FFF8510UL)
                : MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014));
    const volatile uint16_t *const dma_source =
        dma_irq ? (const volatile uint16_t *)(uintptr_t)*(
                      const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B54UL, 0x1FFF8514UL)
                : NULL;
    volatile PositionSensorScratch *const scratch =
        dma_irq ? (volatile PositionSensorScratch *)(uintptr_t)*(
                      const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B58UL, 0x1FFF8518UL)
                : &position_sensor_scratch;
    const uint32_t direction = *(const volatile uint32_t *)(motor + 0x34U);
    if (dma_irq)
    {
        dma_word = *dma_source;
    }
    uint16_t raw = (uint16_t)((dma_word >> 2U) & 0x3fffU);
    if (direction == UINT32_C(0x3f800000))
    {
        raw = (uint16_t)(0x3fffU - raw);
    }
    const uint16_t index = raw >> 6U;
    const uint16_t next = (uint16_t)((index + 1U) & 0xffU);
    scratch->raw_position = raw;
    const volatile float *const correction_table =
        (const volatile float *)(uintptr_t)(dma_irq
                                                ? *(const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(
                                                      0x1FFF8B5CUL, 0x1FFF851CUL)
                                                : MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffc84c),
                                                                        UINT32_C(0x1fffc7d8)));
    const float current_correction = correction_table[index];
    const float next_correction = correction_table[next];
    const float bin_scale =
        dma_irq ? *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B60UL, 0x1FFF8520UL)
                : 1.0f / (float)COUNTS_PER_BIN;
    const float scaled_raw = (float)raw * bin_scale;
    const float bin = (float)(uint16_t)scaled_raw;
    /* This path computes raw/64 - bin with VNMLS. */
    const float fraction = vfp_negative_multiply_subtract(bin, (float)raw, bin_scale);
    const float correction =
        vfp_multiply_add(current_correction, next_correction - current_correction, fraction);
    float corrected_raw;
    __asm volatile("vadd.f32 %0, %1, %2"
                   : "=t"(corrected_raw)
                   : "t"((float)raw), "t"(correction)
                   : "memory");
    const float count_to_rad =
        dma_irq ? *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B64UL, 0x1FFF8524UL)
                : COUNT_TO_RAD;
    scratch->wrapped_angle = corrected_raw * count_to_rad;
    const float two_pi =
        dma_irq ? *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B68UL, 0x1FFF8528UL)
                : TWO_PI;
    wrap_helper(&scratch->wrapped_angle, wrap_minimum, two_pi);
    const float wrapped = scratch->wrapped_angle;

    float delta = wrapped - scratch->previous_angle;
    volatile uint32_t *const turns = (volatile uint32_t *)(motor + 0x08U);
    const int32_t positive_wrap =
        dma_irq ? *(const volatile int32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B6CUL, 0x1FFF852CUL)
                : (int32_t)UINT32_C(0x40b00000);
    uint32_t delta_bits = float_bits(delta);
    if ((int32_t)delta_bits > positive_wrap)
    {
        delta -= two_pi;
        *turns = *turns - 1U;
    }
    delta_bits = float_bits(delta);
    const uint32_t negative_wrap =
        dma_irq ? *(const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B70UL, 0x1FFF8530UL)
                : UINT32_C(0xc0b00000);
    if (delta_bits > negative_wrap)
    {
        delta += two_pi;
        *turns = *turns + 1U;
    }

    const int32_t revolutions = (int32_t)*turns;
    const float continuous = vfp_multiply_add(wrapped, (float)revolutions, two_pi);
    *(volatile float *)motor = continuous;
    const float output_offset = *(const volatile float *)(motor + 0x24U);
    const float output_scale = *(const volatile float *)(motor + 0x5cU);
    const float output_position =
        vfp_negative_multiply_subtract(output_offset, continuous, output_scale);
    *(volatile float *)(motor + 0x18U) = output_position;
    scratch->previous_angle = wrapped;
    scratch->delta_angle = delta;
    const float accumulated = scratch->accumulated_delta + delta;
    if (dma_irq)
    {
        /* IRQ001@888a retains this before the accumulated/ready stores. */
        dma_count = (volatile uint32_t *)(uintptr_t)*(
            const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B74UL, 0x1FFF8534UL);
    }
    scratch->accumulated_delta = accumulated;
    scratch->sample_ready = 1U;
    if (!dma_irq)
    {
        state->continuous_angle = continuous;
        state->output_position = output_position;
        state->raw_position = raw;
        state->wrapped_angle = wrapped;
    }
    return dma_count;
}

void position_sensor_update(PositionSensorState *state, uint16_t dma_word)
{
    (void)update_position(state, dma_word, false, 0.0f);
}

volatile uint32_t *position_sensor_update_dma(float wrap_minimum)
{
    return update_position(NULL, 0U, true, wrap_minimum);
}

void position_sensor_velocity_tick(PositionSensorState *state,
                                   volatile PositionSensorScratch *scratch, float cleared,
                                   const OuterLoopContext *references)
{
    /* ADC IRQ uses the live motor words, not configuration
     * mirrors, and publishes the unfiltered velocity before filtering. */
    volatile float *const motor = (volatile float *)references->motor;
    volatile float *const sample = (volatile float *)references->sample;
    const float delta = scratch->accumulated_delta;
    const float sample_frequency =
        *(const volatile float *)MEMORY_LAYOUT_ADDRESS(0x1FFF845CUL, 0x1FFF9774UL);
    float raw_velocity;
    __asm volatile("vmul.f32 %0, %1, %2"
                   : "=t"(raw_velocity)
                   : "t"(delta), "t"(sample_frequency)
                   : "memory");
    motor[8] = raw_velocity;
    float filtered = motor[1];
    const float previous_weight = motor[26];
    __asm volatile("vmul.f32 %0, %0, %1" : "+t"(filtered) : "t"(previous_weight) : "memory");
    const float new_weight = motor[27];
    __asm volatile("vmla.f32 %0, %1, %2"
                   : "+t"(filtered)
                   : "t"(raw_velocity), "t"(new_weight)
                   : "memory");
    motor[1] = filtered;
    const float scale = motor[23];
    float output_velocity;
    __asm volatile("vmul.f32 %0, %1, %2"
                   : "=t"(output_velocity)
                   : "t"(filtered), "t"(scale)
                   : "memory");
    motor[7] = output_velocity;
    ((volatile uint32_t *)sample)[14] = 0U;
    /* This path publishes the zero through the VFP unit. */
    __asm volatile("vstr %0, [%1]" : : "t"(cleared), "r"(&scratch->accumulated_delta) : "memory");
    state->rotor_velocity = filtered;
    state->output_velocity = output_velocity;
}

bool position_sensor_take_control_sample(volatile PositionSensorScratch *scratch)
{
    if (scratch->sample_ready == 0U)
    {
        return false;
    }
    scratch->sample_ready = 0U;
    return true;
}

bool position_sensor_control_tick(PositionSensorState *state)
{
    const bool sample_ready = position_sensor_take_control_sample(&position_sensor_scratch);
    const OuterLoopContext references = {
        .sample = (volatile void *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff104),
                                                                    UINT32_C(0x1ffff090)),
        .motor = (volatile void *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff088),
                                                                   UINT32_C(0x1ffff014)),
    };
    position_sensor_velocity_tick(state, &position_sensor_scratch, 0.0f, &references);
    return sample_ready;
}

void position_sensor_align_to_output(PositionSensorState *state, float output_position)
{
    float offset;
    float angle;
    float ratio;
    float turns;
    float scale;
    int32_t revolutions;
    /* This path uses fixed live words, publishes integer turns
     * through VSTR before conversion back to float, then loads scale and
     * publishes output. Mirror updates must follow this entire sequence. */
    __asm volatile(
        "vldr %1, [%7, #36]\nvldr %2, [%8, #8]\n"
        "vldr %3, [%7, #76]\nvadd.f32 %0, %0, %1\n"
        "vmov.f32 %4, %2\nvnmls.f32 %4, %3, %0\n"
        "vdiv.f32 %4, %4, %9\n"
        "vcmpe.f32 %4, #0.0\nvmrs APSR_nzcv, FPSCR\n"
        "ite gt\nvaddgt.f32 %4, %4, %10\n"
        "vsuble.f32 %4, %4, %10\n"
        "vcvt.s32.f32 %4, %4\nvstr %4, [%7, #8]\n"
        "vmov %6, %4\nvcvt.f32.s32 %4, %4\n"
        "vmla.f32 %2, %4, %9\nvldr %5, [%7, #92]\n"
        "vnmls.f32 %1, %2, %5\nvstr %1, [%7, #24]"
        : "+&t"(output_position), "=&t"(offset), "=&t"(angle), "=&t"(ratio), "=&t"(turns),
          "=&t"(scale), "=&r"(revolutions)
        : "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014))),
          "r"((uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff190), UINT32_C(0x1ffff11c))),
          "t"(TWO_PI), "t"(0.5f)
        : "cc", "memory");
    state->revolutions = revolutions;
    state->continuous_angle = angle;
    state->output_position = offset;
}

void position_sensor_set_inverted(PositionSensorState *state, bool inverted)
{
    if (state->inverted != inverted)
    {
        state->inverted = inverted;
        state->revolutions = 0;
        state->accumulated_delta = 0.0f;
    }
}

void position_sensor_set_output_offset(PositionSensorState *state, float output_offset)
{
    state->output_offset = output_offset;
}

void position_sensor_reset_accumulated_delta(void)
{
    /* Firmware main publishes this float through VSTR. */
    __asm volatile("vstr %1, [%0]"
                   :
                   : "r"(&position_sensor_scratch.accumulated_delta), "t"(0.0f)
                   : "memory");
}

float position_sensor_zero_current(PositionSensorState *state)
{
    /* FE folds complete output-shaft turns out of the motor-side
     * revolution counter before forming the new output offset.  Preserve
     * the VFP operation order because the persisted float is observable. */
    volatile int32_t *const turns = (volatile int32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
        UINT32_C(0x1ffff090), UINT32_C(0x1ffff01c));
    const float live_turn_bits = *(volatile const float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
        UINT32_C(0x1ffff090), UINT32_C(0x1ffff01c));
    const float scale = *(const volatile float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
        UINT32_C(0x1ffff0e4), UINT32_C(0x1ffff070));
    const float ratio = *(const volatile float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
        UINT32_C(0x1ffff0d4), UINT32_C(0x1ffff060));
    float folded_turns = live_turn_bits;
    float output_turns;
    int32_t folded_bits;
    __asm volatile("vcvt.f32.s32 %1, %0\nvcvt.f32.s32 %0, %0\n"
                   "vmul.f32 %1, %1, %3\n"
                   "vcvt.s32.f32 %1, %1\nvcvt.f32.s32 %1, %1\n"
                   "vmls.f32 %0, %1, %4\nvcvt.s32.f32 %0, %0\n"
                   "vstr %0, [%5]\nvmov %2, %0"
                   : "+&t"(folded_turns), "=&t"(output_turns), "=r"(folded_bits)
                   : "t"(scale), "t"(ratio), "r"(turns)
                   : "memory");
    *(volatile uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff1ec),
                                                           UINT32_C(0x1ffff178)) = 0U;
    /* This path converts the retained integer VFP bits before
     * its angle load. Keep this ordered across the volatile accesses. */
    __asm volatile("vcvt.f32.s32 %0, %0" : "+t"(folded_turns) : : "memory");
    const float angle = position_sensor_scratch.wrapped_angle;
    const float continuous = vfp_multiply_add(angle, folded_turns, TWO_PI);
    float offset;
    __asm volatile("vmul.f32 %0, %1, %2" : "=t"(offset) : "t"(continuous), "t"(scale) : "memory");
    *(volatile float *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff0ac),
                                                        UINT32_C(0x1ffff038)) = offset;
    state->revolutions = folded_bits;
    state->output_offset = offset;
    return state->output_offset;
}
