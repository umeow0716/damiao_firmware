#include "board_flash.h"

#include "factory_layout.h"

#include "hc32f448.h"

#define BOARD_FLASH_END         (0x00040000UL)
#define DM4310_BOOT_RECORD_WORDS   (5U)

#if defined(DAMIAO_DM4310)
#define FLASH_WRITER_SECTION ".dm4310_flash_writer"
#else
#define FLASH_WRITER_SECTION ".ramfunc"
#endif

__attribute__((section(FLASH_WRITER_SECTION), noinline, used))
#if defined(DAMIAO_DM4310)
__attribute__((optimize("Os")))
#endif
static void erase_and_program_from_sram(uint32_t sector_address,
                                        const uint32_t *source,
                                        uint32_t word_count)
{
    const uint32_t sector = sector_address / BOARD_FLASH_SECTOR_SIZE;
    const uint32_t cache_mask = EFM_FRMC_ICACHE | EFM_FRMC_DCACHE |
                                EFM_FRMC_PREFETE;
#if !defined(DAMIAO_DM4310)
    const uint32_t saved_cache = CM_EFM->FRMC & cache_mask;
#endif
#if defined(DAMIAO_DM4310)
    CM_EFM_TypeDef *const protection = (CM_EFM_TypeDef *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
#else
    CM_EFM_TypeDef *const protection = CM_EFM;
#endif
    protection->FAPRT = 0x0123U;
    protection->FAPRT = 0x3210U;
    /* FWMC is unlocked by writing both words to KEY1.  KEY2 is the separate
     * OTP unlock register.  The historical APP RAM writer at 0x1fff8640 and
     * HC32F448 DDL EFM_FWMC_Cmd() both use this exact KEY1/KEY1 sequence. */
#if defined(DAMIAO_DM4310)
    const uint32_t key = *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC4UL, 0x1FFF9AFCUL);
#else
    const uint32_t key = 0x01234567UL;
#endif
    protection->KEY1 = key;
    protection->KEY1 = ~key;
#if defined(DAMIAO_DM4310)
    CM_EFM_TypeDef *const cache = (CM_EFM_TypeDef *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    /* 0x1fff997e captures FRMC only after the four unlock writes. */
    const uint32_t saved_cache = cache->FRMC & cache_mask;
#else
    CM_EFM_TypeDef *const cache = CM_EFM;
#endif
    /* flash_replace_words clears FRMC[19:16], but saves/restores only the
     * cache/prefetch bits [18:16].  CRST therefore remains cleared. */
    cache->FRMC &= ~(cache_mask | EFM_FRMC_CRST);
#if defined(DAMIAO_DM4310)
    CM_EFM_TypeDef *const status = (CM_EFM_TypeDef *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
#else
    CM_EFM_TypeDef *const status = CM_EFM;
#endif
    status->FSCLR = EFM_FSCLR_OTPWERRCLR | EFM_FSCLR_PRTWERRCLR |
                    EFM_FSCLR_PGSZERRCLR | EFM_FSCLR_MISMTCHCLR |
                    EFM_FSCLR_OPTENDCLR | EFM_FSCLR_COLERRCLR;
    while ((status->FSR & EFM_FSR_RDY) == 0U) {}
#if defined(DAMIAO_DM4310)
    CM_EFM_TypeDef *const mode = (CM_EFM_TypeDef *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
#else
    CM_EFM_TypeDef *const mode = CM_EFM;
#endif
    mode->FWMC = (mode->FWMC & ~EFM_FWMC_PEMOD) |
                   (4UL << EFM_FWMC_PEMOD_POS);
    /* flash_replace_words@0x1fff9950 selects sector-erase mode before
     * opening the corresponding F0NWPRT bit.  Preserve that MMIO ordering;
     * the controller observes both writes even though no flash access occurs
     * between them. */
#if defined(DAMIAO_DM4310)
    volatile uint32_t *const sector_protection = (volatile uint32_t *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC8UL, 0x1FFF9B00UL);
#else
    volatile uint32_t *const sector_protection = &CM_EFM->F0NWPRT;
#endif
    *sector_protection = 1UL << sector;
    *(volatile uint32_t *)sector_address = 0U;
    while ((status->FSR & EFM_FSR_RDY) == 0U) {}
    status->FSCLR = EFM_FSCLR_OPTENDCLR;

    mode->FWMC = (mode->FWMC & ~EFM_FWMC_PEMOD) |
                   (3UL << EFM_FWMC_PEMOD_POS);
    volatile uint32_t *destination = (volatile uint32_t *)sector_address;
#if defined(DAMIAO_DM4310)
    const volatile uint32_t *read_source = source;
#endif
    for (uint32_t index = 0U; index < word_count; ++index) {
#if defined(DAMIAO_DM4310)
        *destination++ = *read_source++;
#else
        destination[index] = source[index];
#endif
        while ((status->FSR & EFM_FSR_OPTEND) == 0U) {}
        status->FSCLR = EFM_FSCLR_OPTENDCLR;
    }

    mode->FWMC &= ~EFM_FWMC_PEMOD;
    while ((status->FSR & EFM_FSR_RDY) == 0U) {
        status->FSCLR = EFM_FSCLR_OPTENDCLR;
    }
    *sector_protection = 0U;
#if defined(DAMIAO_DM4310)
    /* 0x1fff9a18 ORs saved bits into a fresh FRMC read; it does not clear
     * cache bits another context may have changed while flash was busy. */
    cache->FRMC |= saved_cache;
#else
    cache->FRMC = (cache->FRMC & ~cache_mask) | saved_cache;
#endif
    mode->FWMC |= EFM_FWMC_KEY1LOCK;
    /* The shipped RAM writer finishes by writing the second protection key
     * again; retain that instruction-level behavior instead of substituting
     * the DDL's generic lock helper. */
    protection->FAPRT = 0x3210U;
}

#if defined(DAMIAO_DM4310)
/* Unreferenced factory primitive at 0x1fff9a38. It erases only: no program
 * phase, protection-bit close or interrupt-mask changes belong to this body. */
__attribute__((section(".dm4310_flash_erase_only"), noinline, used))
void board_flash_erase_sector_from_sram(uint32_t sector_address)
{
    const uint32_t cache_mask = EFM_FRMC_ICACHE | EFM_FRMC_DCACHE |
                                EFM_FRMC_PREFETE;
    /* Shared pool references are distinct reads at 9a3a/54/5c/6e/86/98.
     * Retain each cached register pointer through the corresponding tail. */
    CM_EFM_TypeDef *const protection = (CM_EFM_TypeDef *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    protection->FAPRT = 0x0123U;
    protection->FAPRT = 0x3210U;
    const uint32_t key = *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC4UL, 0x1FFF9AFCUL);
    protection->KEY1 = key;
    protection->KEY1 = ~key;
    CM_EFM_TypeDef *const cache = (CM_EFM_TypeDef *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    const uint32_t saved_cache = cache->FRMC & cache_mask;
    cache->FRMC &= ~(cache_mask | EFM_FRMC_CRST);
    CM_EFM_TypeDef *const status = (CM_EFM_TypeDef *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    status->FSCLR = EFM_FSCLR_OTPWERRCLR | EFM_FSCLR_PRTWERRCLR |
                    EFM_FSCLR_PGSZERRCLR | EFM_FSCLR_MISMTCHCLR |
                    EFM_FSCLR_OPTENDCLR | EFM_FSCLR_COLERRCLR;
    while ((status->FSR & EFM_FSR_RDY) == 0U) {}
    CM_EFM_TypeDef *const mode = (CM_EFM_TypeDef *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    mode->FWMC = (mode->FWMC & ~EFM_FWMC_PEMOD) |
                   (4UL << EFM_FWMC_PEMOD_POS);
    volatile uint32_t *const sector_protection = (volatile uint32_t *)(uintptr_t)
        *(volatile const uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF9AC8UL, 0x1FFF9B00UL);
    *sector_protection = 1UL << (sector_address / BOARD_FLASH_SECTOR_SIZE);
    *(volatile uint32_t *)sector_address = 0U;
    while ((status->FSR & EFM_FSR_RDY) == 0U) {}
    status->FSCLR = EFM_FSCLR_OPTENDCLR;
    cache->FRMC |= saved_cache;
    mode->FWMC |= EFM_FWMC_KEY1LOCK;
    protection->FAPRT = 0x3210U;
}

__attribute__((section(".dm4310_boot_record_writer"), noinline, used, optimize("Os")))
static void write_boot_record_from_sram(void)
{
    const uint32_t cache_mask = EFM_FRMC_ICACHE | EFM_FRMC_DCACHE |
                                EFM_FRMC_PREFETE;
    uint32_t saved_cache;
    __disable_irq();
    const volatile uint32_t *literals =
        (const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8724UL, 0x1FFF80E4UL);
    /* Keep one pool base register; eight separately materialized addresses
     * would grow the fixed SRAM body into the adjacent shared pool. */
    __asm volatile ("" : "+r" (literals));
    CM_EFM_TypeDef *const protection = (CM_EFM_TypeDef *)(uintptr_t)
        literals[0];
    protection->FAPRT = 0x0123U;
    protection->FAPRT = 0x3210U;
    const uint32_t key = literals[1];
    protection->KEY1 = key;
    protection->KEY1 = ~key;
    volatile uint32_t *const cache = (volatile uint32_t *)(uintptr_t)
        literals[2];
    saved_cache = *cache & cache_mask;
    *cache &= ~(cache_mask | EFM_FRMC_CRST);
    volatile uint32_t *const status = (volatile uint32_t *)(uintptr_t)
        literals[3];
    volatile const uint32_t *const ready =
        (volatile const uint32_t *)((uintptr_t)status - 4U);
    *status = EFM_FSCLR_OTPWERRCLR | EFM_FSCLR_PRTWERRCLR |
                    EFM_FSCLR_PGSZERRCLR | EFM_FSCLR_MISMTCHCLR |
                    EFM_FSCLR_OPTENDCLR | EFM_FSCLR_COLERRCLR;
    while ((*ready & EFM_FSR_RDY) == 0U) {}
    volatile uint32_t *const mode = (volatile uint32_t *)(uintptr_t)
        literals[4];
    *mode = (*mode & ~EFM_FWMC_PEMOD) |
            (4UL << EFM_FWMC_PEMOD_POS);
    volatile uint32_t *const sector_protection = (volatile uint32_t *)(uintptr_t)
        literals[5];
    *sector_protection = 0x8000UL;
    volatile uint32_t *destination = (volatile uint32_t *)(uintptr_t)
        literals[6];
    *destination = 0U;
    while ((*ready & EFM_FSR_RDY) == 0U) {}
    *status = EFM_FSCLR_OPTENDCLR;
    *mode = (*mode & ~EFM_FWMC_PEMOD) |
                   (3UL << EFM_FWMC_PEMOD_POS);
    destination = (volatile uint32_t *)(uintptr_t)
        literals[6];
    const volatile uint32_t *const source = (const volatile uint32_t *)(uintptr_t)
        literals[7];
    for (uint32_t index = 0U; index < DM4310_BOOT_RECORD_WORDS; ++index) {
        destination[index] = source[index];
        while ((*ready & EFM_FSR_OPTEND) == 0U) {}
        *status = EFM_FSCLR_OPTENDCLR;
    }
    *mode &= ~EFM_FWMC_PEMOD;
    while ((*ready & EFM_FSR_RDY) == 0U) {
        *status = EFM_FSCLR_OPTENDCLR;
    }
    *sector_protection = 0U;
    /* 0x1fff870c..10 retains newly set bits in the current FRMC value. */
    *cache |= saved_cache;
    *mode |= EFM_FWMC_KEY1LOCK;
    protection->FAPRT = 0x3210U;
    __enable_irq();
}

void board_flash_write_boot_record(void)
{
    write_boot_record_from_sram();
}

void board_flash_write_zero_record(void)
{
    __disable_irq();
    erase_and_program_from_sram(0x00036000UL,
        (const uint32_t *)(uintptr_t)FACTORY_SRAM_ADDRESS(0x1FFFA5C0UL, 0x1FFFA550UL), 2U);
    __enable_irq();
}

void board_flash_write_zero_record_from(uint32_t sector_address,
                                        const uint32_t *source)
{
    erase_and_program_from_sram(sector_address, source, 2U);
}
#endif

bool board_flash_replace_sector_prefix(uint32_t sector_address,
                                       const void *data,
                                       size_t length)
{
    if ((data == NULL) || (length == 0U) || ((length & 3U) != 0U) ||
        ((sector_address & (BOARD_FLASH_SECTOR_SIZE - 1U)) != 0U) ||
        (sector_address >= BOARD_FLASH_END) ||
        (length > BOARD_FLASH_SECTOR_SIZE)) {
        return false;
    }

#if !defined(DAMIAO_DM4310)
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
#endif
    erase_and_program_from_sram(
        sector_address, (const uint32_t *)data,
        (uint32_t)(length / sizeof(uint32_t)));
#if !defined(DAMIAO_DM4310)
    __DSB();
    __ISB();
    if (primask == 0U) {
        __enable_irq();
    }
#endif
    return true;
}
