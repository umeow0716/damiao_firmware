/* Convert an IEEE-754 binary64 value into the formatter's three-word
 * extended representation. The word arguments follow the r0/r1 call order,
 * which is intentionally different from the in-memory double-word order. */
#include <stdint.h>
#include "formatter_arithmetic.h"

/* Runtime classification codes: 0 zero, 4 subnormal, 5 normal,
 * 3 infinity, and 7 NaN. */
__attribute__((noipa)) uint32_t binary64_classify(uint64_t bits)
{
    const uint32_t high = (uint32_t)(bits >> 32U);
    const uint32_t exponent = (high >> 20U) & UINT32_C(0x7ff);
    uint32_t classification = ((uint32_t)bits | (high << 12U)) != 0U ? 4U : 0U;
    if (exponent != 0U)
    {
        classification |= 1U;
    }
    if (exponent == UINT32_C(0x7ff))
    {
        classification |= 2U;
    }
    return classification == 1U ? 5U : classification;
}
ExtendedFloat binary64_extended(uint32_t high, uint32_t low)
{
    const uint32_t sign = high & UINT32_C(0x80000000);
    uint32_t exponent = (high >> 20U) & 0x7FFU;
    uint64_t fraction = ((uint64_t)(high & 0xFFFFFU) << 32U) | low;
    if (exponent == 0U && fraction == 0U)
    {
        return (ExtendedFloat){.word = {sign, 0U, 0U}};
    }
    if (exponent == 0U)
    {
        exponent = 15361U;
        while ((fraction & (UINT64_C(1) << 52U)) == 0U)
        {
            fraction <<= 1U;
            --exponent;
        }
    }
    else
    {
        fraction |= UINT64_C(1) << 52U;
        exponent += 15360U;
        if (((high >> 20U) & 0x7FFU) == 0x7FFU)
        {
            exponent |= UINT32_C(0x40000000);
        }
    }
    fraction <<= 11U;
    return (ExtendedFloat){
        .word = {sign | exponent, (uint32_t)(fraction >> 32U), (uint32_t)fraction}};
}
