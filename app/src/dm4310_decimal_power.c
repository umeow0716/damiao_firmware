#if defined(DAMIAO_DM4310)
/* Source reconstruction of 0x20f8c. Data literals are typed constants,
 * not linked factory machine code. */
#include <stdint.h>
#include "dm4310_formatter_arithmetic.h"

typedef Dm4310FactoryExtended FactoryExtended;

FactoryExtended factory_decimal_power(int exponent, int rounding)
{
    /* 0x285a0: 10^(1,2,4,8,16), in factory three-word format. */
    static const FactoryExtended small[] = {
        {.word = {0x4002U, 0xA0000000U, 0U}},
        {.word = {0x4005U, 0xC8000000U, 0U}},
        {.word = {0x400CU, 0x9C400000U, 0U}},
        {.word = {0x4019U, 0xBEBC2000U, 0U}},
        {.word = {0x4034U, 0x8E1BC9BFU, 0x04000000U}},
    };
    /* 0x285dc: 10^(55,110,220,440), plus rounding correction tags. */
    static const struct {
        FactoryExtended value;
        int correction;
    } large[] = {
        {{.word = {0x40B5U, 0xD0CF4B50U, 0xCFE20766U}}, 1},
        {{.word = {0x416CU, 0xAA51823EU, 0x34A7EEDFU}}, 1},
        {{.word = {0x42D9U, 0xE2A0B5DCU, 0x971F303AU}}, -1},
        {{.word = {0x45B4U, 0xC8A025FDU, 0x4FC1A3E9U}}, -1},
    };
    FactoryExtended a = {.word = {0x3FFFU, 0x80000000U, 0U}};
    FactoryExtended b = a;
    /* ADD/CMN wrap at 32 bits in the factory, including extreme inputs. */
    const int32_t biased = (int32_t)((uint32_t)exponent + UINT32_C(7067));
    int quotient = biased / 55 - 128;
    int remainder = biased % 55 - 27;
    const int reciprocal = remainder < 0;
    if (reciprocal) {
        remainder = -remainder;
    }
    for (unsigned index = 0U; remainder != 0; ++index, remainder >>= 1) {
        if ((remainder & 1) != 0) {
            a = factory_extended_multiply(&a, &small[index],
                                          (uint32_t)rounding, 1U);
        }
    }
    for (unsigned index = 0U; quotient != 0; ++index, quotient >>= 1) {
        if ((quotient & 1) != 0) {
            FactoryExtended factor = large[index].value;
            if ((uint32_t)large[index].correction + (uint32_t)rounding == 0U) {
                factor.fields.significand_low += (uint32_t)rounding;
            }
            b = factory_extended_multiply(&b, &factor,
                                          (uint32_t)rounding, 1U);
        }
    }
    return reciprocal ? factory_extended_divide(&b, &a,
                                                 (uint32_t)rounding, 1U) :
                        factory_extended_multiply(&b, &a,
                                                  (uint32_t)rounding, 1U);
}

#endif
