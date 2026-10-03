#ifndef DAMIAO_MEMORY_LAYOUT_H
#define DAMIAO_MEMORY_LAYOUT_H

#include <stddef.h>
#include <stdint.h>

/*
 * These checks emit no code or data. Use them only for objects whose layout
 * is consumed by fixed-address SRAM code, an interrupt ABI, or persistent
 * storage. Ordinary C-only types must remain free to evolve without one.
 */
#define SRAM_ABI_ASSERT_SIZE(type, expected_size)                                                  \
    _Static_assert(sizeof(type) == (expected_size),                                                \
                   #type " size changed; this breaks the fixed SRAM ABI")
#define SRAM_ABI_ASSERT_OFFSET(type, member, expected_offset)                                      \
    _Static_assert(offsetof(type, member) == (expected_offset),                                    \
                   #type "." #member " moved; this breaks the fixed SRAM ABI")

/*
 * Select an address from the fixed-SRAM ABI. The standard and relocated
 * layouts place both RAM code and runtime objects differently. Keep both
 * values at each low-level use site so the address contract remains explicit.
 */
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
#define MEMORY_LAYOUT_ADDRESS(standard_address, relocated_address) (relocated_address)
#define MEMORY_LAYOUT_SECTION(standard_section, relocated_section) relocated_section
#elif defined(DAMIAO_LAYOUT_SHIFTED_SRAM)
/* This layout inserts one word before the callback owner, shifting that word
 * and all later runtime objects by four bytes. */
#define MEMORY_LAYOUT_ADDRESS(standard_address, relocated_address)                                 \
    ((standard_address) +                                                                          \
     (((standard_address) >= UINT32_C(0x1fff9878)) ? UINT32_C(4) : UINT32_C(0)))
#define MEMORY_LAYOUT_SECTION(standard_section, relocated_section) standard_section
#else
#define MEMORY_LAYOUT_ADDRESS(standard_address, relocated_address) (standard_address)
#define MEMORY_LAYOUT_SECTION(standard_section, relocated_section) standard_section
#endif

#endif
