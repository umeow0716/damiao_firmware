/* Analysis-only C recovery of factory helper 0x2120c.
 * Explicit three-word return: Ghidra's undefined8 omits the live r2 word.
 * Input high/low words correspond to r0/r1, not normal AAPCS double order.
 * Not linked into production until differential and ABI audits pass. */
#include <stdint.h>

typedef struct {
    uint32_t exponent_sign;
    uint32_t significand_high;
    uint32_t significand_low;
} FactoryExtended;

FactoryExtended factory_binary64_extended(uint32_t high, uint32_t low)
{
    const uint32_t sign = high & UINT32_C(0x80000000);
    uint32_t exponent = (high >> 20U) & 0x7FFU;
    uint64_t fraction = ((uint64_t)(high & 0xFFFFFU) << 32U) | low;
    if (exponent == 0U && fraction == 0U) {
        return (FactoryExtended) {sign, 0U, 0U};
    }
    if (exponent == 0U) {
        exponent = 15361U;
        while ((fraction & (UINT64_C(1) << 52U)) == 0U) {
            fraction <<= 1U;
            --exponent;
        }
    } else {
        fraction |= UINT64_C(1) << 52U;
        exponent += 15360U;
        if (((high >> 20U) & 0x7FFU) == 0x7FFU) {
            exponent |= UINT32_C(0x40000000);
        }
    }
    fraction <<= 11U;
    return (FactoryExtended) {sign | exponent,
                              (uint32_t)(fraction >> 32U),
                              (uint32_t)fraction};
}
