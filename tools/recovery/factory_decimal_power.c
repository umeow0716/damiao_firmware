/* Analysis-only reconstruction of 0x20f8c. Arithmetic helper integration and
 * differential verification are pending. Data literals are typed constants,
 * not linked factory machine code. */
#include <stdint.h>

typedef struct {
    uint32_t exponent_sign;
    uint32_t significand_high;
    uint32_t significand_low;
} FactoryExtended;

extern FactoryExtended factory_extended_multiply(const FactoryExtended *a,
    const FactoryExtended *b, uint32_t rounding, uint32_t mode);
extern FactoryExtended factory_extended_divide(const FactoryExtended *a,
    const FactoryExtended *b, uint32_t rounding, uint32_t mode);

FactoryExtended factory_decimal_power(int exponent, int rounding)
{
    /* 0x285a0: 10^(1,2,4,8,16), in factory three-word format. */
    static const FactoryExtended small[] = {
        {0x4002U, 0xA0000000U, 0U},
        {0x4005U, 0xC8000000U, 0U},
        {0x400CU, 0x9C400000U, 0U},
        {0x4019U, 0xBEBC2000U, 0U},
        {0x4034U, 0x8E1BC9BFU, 0x04000000U},
    };
    /* 0x285dc: 10^(55,110,220,440), plus rounding correction tags. */
    static const struct {
        FactoryExtended value;
        int correction;
    } large[] = {
        {{0x40B5U, 0xD0CF4B50U, 0xCFE20766U}, 1},
        {{0x416CU, 0xAA51823EU, 0x34A7EEDFU}, 1},
        {{0x42D9U, 0xE2A0B5DCU, 0x971F303AU}, -1},
        {{0x45B4U, 0xC8A025FDU, 0x4FC1A3E9U}, -1},
    };
    FactoryExtended a = {0x3FFFU, 0x80000000U, 0U};
    FactoryExtended b = a;
    const int biased = exponent + 7067;
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
            if (large[index].correction + rounding == 0) {
                factor.significand_low += (uint32_t)rounding;
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
