/* Fixed-point and significant-digit conversion for the debug formatter. */
#include <stdint.h>
#include "formatter_arithmetic.h"

/* Register ABI: input and quotient in r0:r1, remainder in r2:r3. */
__attribute__((naked, used, noipa)) static void divide_ten_registers(void)
{
    __asm__ volatile("push {r4, r5, lr}\n\t"
                     "subs.w r2, r0, #10\n\t"
                     "mov.w lr, r0, lsr #2\n\t"
                     "sbcs.w r3, r1, #0\n\t"
                     "orr.w lr, lr, r1, lsl #30\n\t"
                     "subs.w r0, r0, lr\n\t"
                     "sbc.w r1, r1, r1, lsr #2\n\t"
                     "mov.w lr, r0, lsr #4\n\t"
                     "orr.w lr, lr, r1, lsl #28\n\t"
                     "adds.w r0, r0, lr\n\t"
                     "adc.w r1, r1, r1, lsr #4\n\t"
                     "mov.w lr, r0, lsr #8\n\t"
                     "orr.w lr, lr, r1, lsl #24\n\t"
                     "adds.w r0, r0, lr\n\t"
                     "adc.w r1, r1, r1, lsr #8\n\t"
                     "mov.w lr, r0, lsr #16\n\t"
                     "orr.w lr, lr, r1, lsl #16\n\t"
                     "adds.w r0, r0, lr\n\t"
                     "adc.w r1, r1, r1, lsr #16\n\t"
                     "adds r0, r0, r1\n\t"
                     "adc.w r1, r1, #0\n\t"
                     "mov.w r0, r0, lsr #3\n\t"
                     "orr.w r0, r0, r1, lsl #29\n\t"
                     "mov.w r1, r1, lsr #3\n\t"
                     "adds.w r5, r0, r0, lsl #2\n\t"
                     "mov.w r4, r1, lsl #2\n\t"
                     "orr.w r4, r4, r0, lsr #30\n\t"
                     "adc.w r4, r4, r1\n\t"
                     "adds r5, r5, r5\n\t"
                     "adc.w r4, r4, r4\n\t"
                     "subs r2, r2, r5\n\t"
                     "sbcs r3, r3, r4\n\t"
                     "mov.w r3, #0\n\t"
                     "it mi\n\t"
                     "addmi r2, #10\n\t"
                     "bpl 1f\n\t"
                     "pop {r4, r5, pc}\n"
                     "1:\n\t"
                     "adds r0, r0, #1\n\t"
                     "adc.w r1, r1, #0\n\t"
                     "pop {r4, r5, pc}");
}

__attribute__((noipa)) static uint64_t divide_ten(uint64_t value, uint32_t *remainder)
{
    register uint32_t low __asm__("r0") = (uint32_t)value;
    register uint32_t high __asm__("r1") = (uint32_t)(value >> 32U);
    register uint32_t digit __asm__("r2");
    __asm__ volatile("bl divide_ten_registers"
                     : "+r"(low), "+r"(high), "=r"(digit)
                     :
                     : "r3", "lr", "cc", "memory");
    *remainder = digit;
    return ((uint64_t)high << 32U) | low;
}

int fixed_scaled(uint64_t bits, unsigned precision, uint64_t *scaled)
{
    if (scaled == 0 || precision > 17U)
    {
        return -1;
    }
    ExtendedFloat value = binary64_extended((uint32_t)(bits >> 32U), (uint32_t)bits);
    ExtendedFloat power = decimal_power((int)precision, 0);
    value.fields.exponent_sign -= UINT32_C(0x201f);
    power.fields.exponent_sign -= UINT32_C(0x201f);
    const ExtendedFloat result = extended_multiply(&value, &power, 0U, 0U);
    if ((result.fields.exponent_sign & UINT32_C(0xffff)) != 0U)
    {
        *scaled = UINT64_C(0x7fffffffffffffff);
        return 1;
    }
    *scaled = ((uint64_t)result.fields.significand_high << 32U) | result.fields.significand_low;
    return 0;
}

/* Try fixed conversion first, then fall back to 17 significant digits when
 * the scaled integer cannot fit. Do not cast to float or use libc rounding. */
__attribute__((noipa)) void decimal_digits(uint64_t bits, unsigned precision, uint32_t fixed_mode,
                                           DecimalDigits *decimal)
{
    volatile char *const digit_buffer = decimal->digit;
    volatile DecimalDigits *const metadata = decimal;
    unsigned result_count;
    int result_exponent;
    if ((bits << 1U) == 0U)
    {
        if (fixed_mode != 1U)
        {
            for (unsigned index = 0U; index < precision; ++index)
            {
                digit_buffer[index] = '0';
            }
            digit_buffer[precision] = '\0';
            result_count = precision;
            result_exponent = 0;
        }
        else
        {
            digit_buffer[0] = '\0';
            result_count = 0U;
            result_exponent = -(int)precision - 1;
        }
        metadata->fixed_mode = fixed_mode;
        metadata->exponent = result_exponent;
        metadata->count = result_count;
        return;
    }
    int binary_exponent = (int)((bits >> 52U) & UINT64_C(0x7ff));
    if (binary_exponent == 0)
    {
        binary_exponent = -1;
    }
    const int estimate_product = (binary_exponent - 1023) * 19728;
    /* Arithmetic right shift as performed by ASRS, including negatives. */
    int estimate =
        estimate_product >= 0 ? estimate_product / 65536 : -((-estimate_product + 65535) / 65536);
    uint32_t fixed = fixed_mode;
    for (;;)
    {
        const int scale = fixed ? -(int)precision : estimate - (int)precision + 1;
        ExtendedFloat power = decimal_power(scale < 0 ? -scale : scale, 0);
        ExtendedFloat value = binary64_extended((uint32_t)(bits >> 32U), (uint32_t)bits);
        value.fields.exponent_sign -= UINT32_C(0x201f);
        ExtendedFloat result;
        if (scale > 0)
        {
            power.fields.exponent_sign += UINT32_C(0x201f);
            result = extended_divide(&value, &power, 0U, 0U);
        }
        else
        {
            power.fields.exponent_sign -= UINT32_C(0x201f);
            result = extended_multiply(&value, &power, 0U, 0U);
        }
        uint64_t integer =
            (result.fields.exponent_sign & UINT32_C(0xffff)) != 0U
                ? UINT64_C(0x7fffffffffffffff)
                : ((uint64_t)result.fields.significand_high << 32U) | result.fields.significand_low;
        if (fixed)
        {
            unsigned count = 0U;
            while (integer != 0U && count < 17U)
            {
                uint32_t remainder;
                integer = divide_ten(integer, &remainder);
                digit_buffer[count++] = (char)('0' + remainder);
            }
            if (integer != 0U)
            {
                fixed = 0;
                precision = 17U;
                continue;
            }
            for (unsigned index = 0U; index < count / 2U; ++index)
            {
                /* Read the right byte first to preserve in-place reversal. */
                const char right = digit_buffer[count - index - 1U];
                const char left = digit_buffer[index];
                digit_buffer[index] = right;
                digit_buffer[count - index - 1U] = left;
            }
            result_count = count;
            result_exponent = (int)count - (int)precision - 1;
        }
        else
        {
            for (unsigned index = precision; index != 0U; --index)
            {
                uint32_t remainder;
                integer = divide_ten(integer, &remainder);
                digit_buffer[index - 1U] = (char)('0' + remainder);
            }
            if (integer != 0U)
            {
                ++estimate;
                continue;
            }
            if (digit_buffer[0] == '0')
            {
                --estimate;
                continue;
            }
            result_count = precision;
            result_exponent = estimate;
        }
        digit_buffer[result_count] = '\0';
        metadata->fixed_mode = fixed;
        metadata->exponent = result_exponent;
        metadata->count = result_count;
        return;
    }
}

void fixed_digits(uint64_t bits, unsigned precision, DecimalDigits *decimal)
{
    decimal_digits(bits, precision, 1U, decimal);
}
