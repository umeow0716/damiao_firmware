/* Analysis-only composition of factory 0x20996 fixed-mode scaling path. */
#include <stdint.h>

typedef struct {
    uint32_t exponent_sign;
    uint32_t significand_high;
    uint32_t significand_low;
} FactoryExtended;

extern FactoryExtended factory_binary64_extended(uint32_t high, uint32_t low);
extern FactoryExtended factory_decimal_power(int exponent, int rounding);
extern FactoryExtended factory_extended_multiply(const FactoryExtended *a,
    const FactoryExtended *b, uint32_t rounding, uint32_t mode);

int factory_fixed_scaled(uint64_t bits, unsigned precision, uint64_t *scaled)
{
    if (scaled == 0 || precision > 17U) {
        return -1;
    }
    FactoryExtended value = factory_binary64_extended((uint32_t)(bits >> 32U),
                                                       (uint32_t)bits);
    FactoryExtended power = factory_decimal_power((int)precision, 0);
    value.exponent_sign -= UINT32_C(0x201f);
    power.exponent_sign -= UINT32_C(0x201f);
    const FactoryExtended result =
        factory_extended_multiply(&value, &power, 0U, 0U);
    if ((result.exponent_sign & UINT32_C(0xffff)) != 0U) {
        *scaled = UINT64_C(0x7fffffffffffffff);
        return 1;
    }
    *scaled = ((uint64_t)result.significand_high << 32U) |
              result.significand_low;
    return 0;
}
