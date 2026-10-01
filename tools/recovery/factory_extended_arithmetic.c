/* Analysis-only port of ORIGINAL flash helpers 21704/2172e/21628.
 * Source: dm4310_v3_v5017_app_flash_00020000_memory.bin, base 0x20000.
 * Not linked to firmware. No binary64 conversion, pow10 or formatting here.
 * triple = {sign/exponent, significand high, significand low}; exponent
 * arithmetic masks off the top byte and uses bias 0x3fff. Bit 30 is the
 * wrapper's special/unsupported marker, not an IEEE binary64 exponent.
 * Division uses restoring 64-bit significand division instead of 212f0's
 * reciprocal-estimate acceleration. Factory equivalence is tested at returned
 * words; raw division guard contents are NOT asserted instruction-identical.
 * Multiplication uses four 32-bit limbs and the factory's guard folding.
 * No libc floating arithmetic, allocation, tables or __int128.
 * Target runtime/stack compatibility remains unaudited.
 */
#include <stdint.h>

typedef struct { uint32_t word[3]; } factory_extended;
typedef struct {
    uint32_t sign, high, low;
    int32_t exponent;
    uint32_t guard;
} factory_extended_raw;

static uint32_t fold16(uint32_t value)
{
    return (value | (value << 16)) >> 16;
}

/* 21628: raw r0/r1/r2/r3/r6, rounding=fp, mode=sl.
 * mode influences discarded exception bookkeeping, not the returned words.
 * As in the factory, negative exponents shift into guard/sticky with exponent
 * zero; there is no saturation for a large positive exponent.
 */
factory_extended factory_extended_round(factory_extended_raw raw,
                                       uint32_t rounding, uint32_t mode)
{
    uint32_t high = raw.high, low = raw.low, guard = raw.guard;
    int32_t exponent = raw.exponent;
    factory_extended result;
    (void)mode;
    if (exponent < 0) {
        guard = fold16(guard);
        if (exponent <= -64) {
            guard = fold16(guard | low) | high;
            if (exponent < -64) guard = fold16(guard);
            high = low = 0;
        } else {
            unsigned shift;
            if (exponent <= -32) {
                guard |= low;
                low = high;
                high = 0;
                exponent += 32;
            }
            shift = (unsigned)-exponent;
            if (shift) {
                guard = fold16(guard) | (low << (32 - shift));
                low = (low >> shift) | (high << (32 - shift));
                high >>= shift;
            }
        }
        exponent = 0;
    }
    if (guard && rounding != UINT32_MAX &&
        (rounding != 0 || guard > UINT32_C(0x80000000) ||
         (guard == UINT32_C(0x80000000) && (low & 1)))) {
        if (++low == 0 && ++high == 0) {
            high = UINT32_C(0x80000000);
            exponent = (int32_t)((uint32_t)exponent + 1);
        }
    }
    result.word[0] = (raw.sign & UINT32_C(0x80000000)) | (uint32_t)exponent;
    result.word[1] = high;
    result.word[2] = low;
    return result;
}

/* Shift remainder by one and subtract divisor, retaining the 65th bit. */
static unsigned divide_bit(uint64_t *remainder, uint64_t divisor)
{
    unsigned carry = (unsigned)(*remainder >> 63);
    *remainder <<= 1;
    if (carry || *remainder >= divisor) {
        *remainder -= divisor;
        return 1;
    }
    return 0;
}

factory_extended factory_extended_divide(const factory_extended *left,
                                        const factory_extended *right,
                                        uint32_t rounding, uint32_t mode)
{
    uint64_t numerator, denominator, remainder, quotient;
    factory_extended_raw raw;
    unsigned i, count;
    /* 21714..2171e: BICS sign tests; early exit returns left unchanged. */
    if (!((left->word[1] & ~(left->word[0] << 1)) & UINT32_C(0x80000000)) ||
        !((right->word[1] & ~(right->word[0] << 1)) & UINT32_C(0x80000000)))
        return *left;
    numerator = ((uint64_t)left->word[1] << 32) | left->word[2];
    denominator = ((uint64_t)right->word[1] << 32) | right->word[2];
    raw.sign = (left->word[0] ^ right->word[0]) & UINT32_C(0x80000000);
    raw.exponent = (int32_t)((left->word[0] & UINT32_C(0xffffff)) -
                           (right->word[0] & UINT32_C(0xffffff)) + 0x3fff);
    quotient = 0;
    remainder = numerator;
    count = 64;
    if (numerator >= denominator) {
        remainder -= denominator;
        quotient = 1;
        count = 63;
    } else {
        --raw.exponent;
    }
    for (i = 0; i < count; ++i)
        quotient = (quotient << 1) | divide_bit(&remainder, denominator);
    raw.high = (uint32_t)(quotient >> 32);
    raw.low = (uint32_t)quotient;
    raw.guard = 0;
    for (i = 0; i < 32; ++i)
        raw.guard = (raw.guard << 1) | divide_bit(&remainder, denominator);
    if (remainder) raw.guard |= 1;
    return factory_extended_round(raw, rounding, mode);
}

factory_extended factory_extended_multiply(const factory_extended *left,
                                          const factory_extended *right,
                                          uint32_t rounding, uint32_t mode)
{
    uint32_t a[2] = {left->word[2], left->word[1]};
    uint32_t b[2] = {right->word[2], right->word[1]};
    uint32_t product[4] = {0, 0, 0, 0};
    factory_extended_raw raw;
    unsigned i, j;
    /* 2173e..21748: specials return the loaded left triple unchanged. */
    if ((left->word[0] | right->word[0]) & UINT32_C(0x40000000)) return *left;
    for (i = 0; i < 2; ++i) {
        uint64_t carry = 0;
        for (j = 0; j < 2; ++j) {
            uint64_t value = (uint64_t)a[i] * b[j] + product[i + j] + carry;
            product[i + j] = (uint32_t)value;
            carry = value >> 32;
        }
        product[i + 2] = (uint32_t)carry;
    }
    raw.sign = (left->word[0] ^ right->word[0]) & UINT32_C(0x80000000);
    raw.exponent = (int32_t)((left->word[0] & UINT32_C(0xffffff)) +
                           (right->word[0] & UINT32_C(0xffffff)) - 0x3ffe);
    raw.high = product[3];
    raw.low = product[2];
    /* 2181e/21858: lowest word folds into 30 low guard bits. */
    raw.guard = product[1] | ((product[0] | (product[0] << 2)) >> 2);
    if (!(raw.high & UINT32_C(0x80000000))) {
        raw.high = (raw.high << 1) | (raw.low >> 31);
        raw.low = (raw.low << 1) | (raw.guard >> 31);
        raw.guard <<= 1;
        --raw.exponent;
    }
    return factory_extended_round(raw, rounding, mode);
}
