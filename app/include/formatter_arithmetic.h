#ifndef DAMIAO_FORMATTER_ARITHMETIC_H
#define DAMIAO_FORMATTER_ARITHMETIC_H

#include <stdint.h>

typedef union
{
    struct
    {
        uint32_t exponent_sign;
        uint32_t significand_high;
        uint32_t significand_low;
    } fields;
    uint32_t word[3];
} ExtendedFloat;

ExtendedFloat binary64_extended(uint32_t high, uint32_t low);
uint32_t binary64_classify(uint64_t bits);
ExtendedFloat extended_multiply(const ExtendedFloat *left, const ExtendedFloat *right,
                                uint32_t rounding, uint32_t mode);
ExtendedFloat extended_divide(const ExtendedFloat *left, const ExtendedFloat *right,
                              uint32_t rounding, uint32_t mode);
ExtendedFloat decimal_power(int exponent, int rounding);
int fixed_scaled(uint64_t bits, unsigned precision, uint64_t *scaled);
typedef struct
{
    char digit[18];
    int exponent;
    unsigned count;
    uint32_t fixed_mode;
} DecimalDigits;

void fixed_digits(uint64_t bits, unsigned precision, DecimalDigits *decimal);
void decimal_digits(uint64_t bits, unsigned precision, uint32_t fixed_mode, DecimalDigits *decimal);

#endif
