/* Typed adapter for the non-AAPCS three-word arithmetic entry points. */
#include <stdint.h>
#include "formatter_arithmetic.h"

/* Non-AAPCS triple-return entries; only call through the register adapter. */
extern void extended_divide_entry(void);
extern void extended_multiply_entry(void);

static ExtendedFloat extended_entry(const ExtendedFloat *left, const ExtendedFloat *right,
                                    uint32_t rounding, uint32_t mode, void (*entry)(void))
{
    register uint32_t first __asm__("r0") = (uint32_t)(uintptr_t)left;
    register uint32_t second __asm__("r1") = (uint32_t)(uintptr_t)right;
    register uint32_t third __asm__("r2") = rounding;
    register uint32_t fourth __asm__("r3") = mode;
    __asm__ volatile("blx %4"
                     : "+r"(first), "+r"(second), "+r"(third), "+r"(fourth)
                     : "r"(entry)
                     : "r12", "lr", "cc", "memory");
    const ExtendedFloat result = {.word = {first, second, third}};
    return result;
}

ExtendedFloat extended_divide(const ExtendedFloat *left, const ExtendedFloat *right,
                              uint32_t rounding, uint32_t mode)
{
    return extended_entry(left, right, rounding, mode, extended_divide_entry);
}

ExtendedFloat extended_multiply(const ExtendedFloat *left, const ExtendedFloat *right,
                                uint32_t rounding, uint32_t mode)
{
    return extended_entry(left, right, rounding, mode, extended_multiply_entry);
}
