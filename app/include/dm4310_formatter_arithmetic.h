#ifndef DAMIAO_DM4310_FORMATTER_ARITHMETIC_H
#define DAMIAO_DM4310_FORMATTER_ARITHMETIC_H

#include <stdint.h>

#if defined(DAMIAO_DM4310)
typedef union {
    struct {
        uint32_t exponent_sign;
        uint32_t significand_high;
        uint32_t significand_low;
    } fields;
    uint32_t word[3];
} Dm4310FactoryExtended;

Dm4310FactoryExtended factory_binary64_extended(uint32_t high, uint32_t low);
uint32_t factory_binary64_classify(uint64_t bits);
Dm4310FactoryExtended factory_extended_multiply(
    const Dm4310FactoryExtended *left,
    const Dm4310FactoryExtended *right,
    uint32_t rounding, uint32_t mode);
Dm4310FactoryExtended factory_extended_divide(
    const Dm4310FactoryExtended *left,
    const Dm4310FactoryExtended *right,
    uint32_t rounding, uint32_t mode);
Dm4310FactoryExtended factory_decimal_power(int exponent, int rounding);
int dm4310_factory_fixed_scaled(uint64_t bits, unsigned precision,
                                uint64_t *scaled);
typedef struct {
    char digit[18];
    int exponent;
    unsigned count;
    uint32_t fixed_mode;
} Dm4310FactoryDecimal;

void dm4310_factory_fixed_digits(uint64_t bits, unsigned precision,
                                 Dm4310FactoryDecimal *decimal);
void dm4310_factory_decimal_digits(uint64_t bits, unsigned precision,
                                   uint32_t fixed_mode,
                                   Dm4310FactoryDecimal *decimal);
#endif

#endif
