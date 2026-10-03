/* Extended-precision powers of ten used by decimal formatting. */
#include <stdint.h>
#include "formatter_arithmetic.h"

ExtendedFloat decimal_power(int exponent, int rounding)
{
    /* 10^(1, 2, 4, 8, 16) in the formatter's three-word representation. */
    static const ExtendedFloat small[] = {
        {.word = {0x4002U, 0xA0000000U, 0U}},          {.word = {0x4005U, 0xC8000000U, 0U}},
        {.word = {0x400CU, 0x9C400000U, 0U}},          {.word = {0x4019U, 0xBEBC2000U, 0U}},
        {.word = {0x4034U, 0x8E1BC9BFU, 0x04000000U}},
    };
    /* 10^(55, 110, 220, 440), with rounding-correction tags. */
    static const struct
    {
        ExtendedFloat value;
        int correction;
    } large[] = {
        {{.word = {0x40B5U, 0xD0CF4B50U, 0xCFE20766U}}, 1},
        {{.word = {0x416CU, 0xAA51823EU, 0x34A7EEDFU}}, 1},
        {{.word = {0x42D9U, 0xE2A0B5DCU, 0x971F303AU}}, -1},
        {{.word = {0x45B4U, 0xC8A025FDU, 0x4FC1A3E9U}}, -1},
    };
    ExtendedFloat a = {.word = {0x3FFFU, 0x80000000U, 0U}};
    ExtendedFloat b = a;
    /* The exponent bias intentionally wraps at 32 bits for extreme inputs. */
    const int32_t biased = (int32_t)((uint32_t)exponent + UINT32_C(7067));
    int quotient = biased / 55 - 128;
    int remainder = biased % 55 - 27;
    const int reciprocal = remainder < 0;
    if (reciprocal)
    {
        remainder = -remainder;
    }
    for (unsigned index = 0U; remainder != 0; ++index, remainder >>= 1)
    {
        if ((remainder & 1) != 0)
        {
            a = extended_multiply(&a, &small[index], (uint32_t)rounding, 1U);
        }
    }
    for (unsigned index = 0U; quotient != 0; ++index, quotient >>= 1)
    {
        if ((quotient & 1) != 0)
        {
            ExtendedFloat factor = large[index].value;
            if ((uint32_t)large[index].correction + (uint32_t)rounding == 0U)
            {
                factor.fields.significand_low += (uint32_t)rounding;
            }
            b = extended_multiply(&b, &factor, (uint32_t)rounding, 1U);
        }
    }
    return reciprocal ? extended_divide(&b, &a, (uint32_t)rounding, 1U)
                      : extended_multiply(&b, &a, (uint32_t)rounding, 1U);
}
