/* Analysis-only positive finite IEEE binary64 -> fixed decimal prototype.
 * No production linkage; parser, sign and padding belong to the caller.
 * Precision is bounded to 0..17. Exact x*10^p = M*5^p*2^(e+p),
 * rounded once to nearest, ties to even. No floating scaling or printf.
 * 36 little-endian limbs cover 1152 bits (maximum required: 1081).
 * Caller-owned workspace: 472 bytes on usual 32/64-bit ABIs; output at
 * most 328 bytes including NUL. No allocation, recursion or large locals.
 * Runtime/target stack and uint64 division helper compatibility UNAUDITED.
 * NOT factory-equivalent across the domain: observed factory output loses
 * trailing significant digits for large values and some precision-17 cases.
 * This sidecar deliberately exposes exact rounding for differential analysis.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <float.h>

#define FACTORY_DECIMAL_LIMBS 36
#define FACTORY_DECIMAL_MAX_PRECISION 17
#define FACTORY_DECIMAL_OUTPUT_CAPACITY 328

typedef struct {
    uint32_t limbs[FACTORY_DECIMAL_LIMBS];
    char digits[327];
} factory_decimal_workspace;

_Static_assert(sizeof(double) == 8 && DBL_MANT_DIG == 53 &&
               DBL_MAX_EXP == 1024 && FLT_RADIX == 2, "binary64 required");

static unsigned decimal_bit(const uint32_t *a, unsigned bit)
{
    return bit < 1152 ? (a[bit / 32] >> (bit % 32)) & 1u : 0;
}

/* Returns 0 on success, -1 for domain/argument errors, -2 for capacity.
 * Accepts +0; rejects all sign-bit-set values (including -0), NaN and inf.
 * On error output is untouched; workspace is scratch and may be modified.
 * Workspace and output must not overlap. */
int factory_decimal_fixed(double value, unsigned precision, char *output,
                          size_t capacity, factory_decimal_workspace *work)
{
    uint64_t bits, mantissa, carry;
    unsigned exponent, i, j, count = 0, length, position = 0;
    int shift;
    uint32_t *a;
    if (!output || !work || precision > FACTORY_DECIMAL_MAX_PRECISION)
        return -1;
    memcpy(&bits, &value, sizeof bits);
    exponent = (unsigned)((bits >> 52) & 2047u);
    if ((bits >> 63) || exponent == 2047) return -1;
    mantissa = bits & UINT64_C(0x000fffffffffffff);
    shift = -1074;
    if (exponent) {
        mantissa |= UINT64_C(1) << 52;
        shift = (int)exponent - 1023 - 52;
    }
    a = work->limbs;
    memset(a, 0, sizeof work->limbs);
    a[0] = (uint32_t)mantissa;
    a[1] = (uint32_t)(mantissa >> 32);
    for (j = 0; j < precision; ++j) {
        carry = 0;
        for (i = 0; i < FACTORY_DECIMAL_LIMBS; ++i) {
            carry += (uint64_t)a[i] * 5;
            a[i] = (uint32_t)carry;
            carry >>= 32;
        }
    }
    shift += (int)precision;
    if (shift >= 0) {
        unsigned words = (unsigned)shift / 32, rem = (unsigned)shift % 32;
        for (i = FACTORY_DECIMAL_LIMBS; i-- > 0;) {
            uint32_t v = i >= words ? a[i - words] << rem : 0;
            if (rem && i > words) v |= a[i - words - 1] >> (32 - rem);
            a[i] = v;
        }
    } else {
        unsigned right = (unsigned)-shift, sticky = 0;
        unsigned half = decimal_bit(a, right - 1);
        unsigned odd = decimal_bit(a, right);
        unsigned words = right / 32, rem = right % 32;
        for (i = 0; i + 1 < right; ++i) sticky |= decimal_bit(a, i);
        for (i = 0; i < FACTORY_DECIMAL_LIMBS; ++i) {
            j = i + words;
            a[i] = j < FACTORY_DECIMAL_LIMBS ? a[j] >> rem : 0;
            if (rem && j + 1 < FACTORY_DECIMAL_LIMBS)
                a[i] |= a[j + 1] << (32 - rem);
        }
        if (half && (sticky || odd)) {
            for (i = 0; i < FACTORY_DECIMAL_LIMBS; ++i)
                if (++a[i]) break;
        }
    }
    /* Division remainders produce reversed base-10 digits, including carry. */
    do {
        uint32_t nonzero = 0;
        carry = 0;
        for (i = FACTORY_DECIMAL_LIMBS; i-- > 0;) {
            uint64_t v = (carry << 32) | a[i];
            a[i] = (uint32_t)(v / 10);
            carry = v % 10;
            nonzero |= a[i];
        }
        work->digits[count++] = (char)('0' + carry);
        if (!nonzero) break;
    } while (count < sizeof work->digits);
    while (count <= precision) work->digits[count++] = '0';
    length = count + (precision != 0);
    if (capacity <= length) return -2;
    for (i = count; i-- > 0;) {
        output[position++] = work->digits[i];
        if (precision && i == precision) output[position++] = '.';
    }
    output[position] = '\0';
    return 0;
}
