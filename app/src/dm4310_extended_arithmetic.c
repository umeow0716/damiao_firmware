#if defined(DAMIAO_DM4310)
/* Typed interface to factory pointer-input entries 21704/2172e.
 * Raw entries retain the original reads and register contract; the C result
 * adapter does not establish identical outer stack layout.
 */
#include <stdint.h>
#include "dm4310_formatter_arithmetic.h"

typedef Dm4310FactoryExtended factory_extended;

/* Non-AAPCS triple-return entries; only call through the register adapter. */
extern void dm4310_extended_divide_entry(void);
extern void dm4310_extended_multiply_entry(void);

static factory_extended factory_extended_entry(
    const factory_extended *left, const factory_extended *right,
    uint32_t rounding, uint32_t mode, void (*entry)(void))
{
    register uint32_t first __asm__("r0") = (uint32_t)(uintptr_t)left;
    register uint32_t second __asm__("r1") = (uint32_t)(uintptr_t)right;
    register uint32_t third __asm__("r2") = rounding;
    register uint32_t fourth __asm__("r3") = mode;
    __asm__ volatile ("blx %4"
                      : "+r" (first), "+r" (second), "+r" (third),
                        "+r" (fourth)
                      : "r" (entry) : "r12", "lr", "cc", "memory");
    const factory_extended result = {.word = {first, second, third}};
    return result;
}

factory_extended factory_extended_divide(const factory_extended *left,
                                        const factory_extended *right,
                                        uint32_t rounding, uint32_t mode)
{
    return factory_extended_entry(left, right, rounding, mode,
                                  dm4310_extended_divide_entry);
}

factory_extended factory_extended_multiply(const factory_extended *left,
                                          const factory_extended *right,
                                          uint32_t rounding, uint32_t mode)
{
    return factory_extended_entry(left, right, rounding, mode,
                                  dm4310_extended_multiply_entry);
}
#endif
