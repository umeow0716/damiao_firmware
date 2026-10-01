#ifndef DAMIAO_FACTORY_LAYOUT_H
#define DAMIAO_FACTORY_LAYOUT_H

/*
 * Select an address from the factory fixed-SRAM ABI.  DM4310 and DM4340
 * share the V5017 layout; DM8009 V6417 reorders both helper pools and runtime
 * objects.  Keep both values at each low-level use site so disassembly audits
 * remain local and future family targets do not inherit an implicit offset.
 */
#if defined(DAMIAO_DM8009_V3)
#define FACTORY_SRAM_ADDRESS(dm43xx_address, dm8009_address) (dm8009_address)
#define FACTORY_LAYOUT_SECTION(dm43xx_section, dm8009_section) dm8009_section
#else
#define FACTORY_SRAM_ADDRESS(dm43xx_address, dm8009_address) (dm43xx_address)
#define FACTORY_LAYOUT_SECTION(dm43xx_section, dm8009_section) dm43xx_section
#endif

#endif
