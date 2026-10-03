#include "board_flash.h"

#include "memory_layout.h"

#include "hc32f448.h"

#define BOARD_FLASH_END (0x00040000UL)
#define BOOT_RECORD_WORDS (5U)

#define FLASH_WRITER_SECTION ".flash_writer"

__attribute__((section(FLASH_WRITER_SECTION), noinline, used))
__attribute__((optimize("Os"))) static void
erase_and_program_from_sram(uint32_t sector_address, const uint32_t *source, uint32_t word_count)
{
    const uint32_t sector = sector_address / BOARD_FLASH_SECTOR_SIZE;
    const uint32_t cache_mask = EFM_FRMC_ICACHE | EFM_FRMC_DCACHE | EFM_FRMC_PREFETE;
    CM_EFM_TypeDef *const protection = (CM_EFM_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    protection->FAPRT = 0x0123U;
    protection->FAPRT = 0x3210U;
    /* FWMC is unlocked by writing both words to KEY1. KEY2 is the separate
     * OTP unlock register. The HC32F448 DDL EFM_FWMC_Cmd() uses this same
     * KEY1/KEY1 sequence. */
    const uint32_t key =
        *(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC4UL, 0x1FFF9AFCUL);
    protection->KEY1 = key;
    protection->KEY1 = ~key;
    CM_EFM_TypeDef *const cache = (CM_EFM_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    /* Capture FRMC only after completing the protection-key sequence. */
    const uint32_t saved_cache = cache->FRMC & cache_mask;
    /* flash_replace_words clears FRMC[19:16], but saves/restores only the
     * cache/prefetch bits [18:16].  CRST therefore remains cleared. */
    cache->FRMC &= ~(cache_mask | EFM_FRMC_CRST);
    CM_EFM_TypeDef *const status = (CM_EFM_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    status->FSCLR = EFM_FSCLR_OTPWERRCLR | EFM_FSCLR_PRTWERRCLR | EFM_FSCLR_PGSZERRCLR |
                    EFM_FSCLR_MISMTCHCLR | EFM_FSCLR_OPTENDCLR | EFM_FSCLR_COLERRCLR;
    while ((status->FSR & EFM_FSR_RDY) == 0U)
    {
    }
    CM_EFM_TypeDef *const mode = (CM_EFM_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    mode->FWMC = (mode->FWMC & ~EFM_FWMC_PEMOD) | (4UL << EFM_FWMC_PEMOD_POS);
    /* flash_replace_words selects sector-erase mode before
     * opening the corresponding F0NWPRT bit.  Preserve that MMIO ordering;
     * the controller observes both writes even though no flash access occurs
     * between them. */
    volatile uint32_t *const sector_protection = (volatile uint32_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC8UL, 0x1FFF9B00UL);
    *sector_protection = 1UL << sector;
    *(volatile uint32_t *)sector_address = 0U;
    while ((status->FSR & EFM_FSR_RDY) == 0U)
    {
    }
    status->FSCLR = EFM_FSCLR_OPTENDCLR;

    mode->FWMC = (mode->FWMC & ~EFM_FWMC_PEMOD) | (3UL << EFM_FWMC_PEMOD_POS);
    volatile uint32_t *destination = (volatile uint32_t *)sector_address;
    const volatile uint32_t *read_source = source;
    for (uint32_t index = 0U; index < word_count; ++index)
    {
        *destination++ = *read_source++;
        while ((status->FSR & EFM_FSR_OPTEND) == 0U)
        {
        }
        status->FSCLR = EFM_FSCLR_OPTENDCLR;
    }

    mode->FWMC &= ~EFM_FWMC_PEMOD;
    while ((status->FSR & EFM_FSR_RDY) == 0U)
    {
        status->FSCLR = EFM_FSCLR_OPTENDCLR;
    }
    *sector_protection = 0U;
    /* Merge saved bits into a fresh FRMC read so cache bits changed by another
     * context while flash was busy are not cleared. */
    cache->FRMC |= saved_cache;
    mode->FWMC |= EFM_FWMC_KEY1LOCK;
    /* Finish by writing the second protection key again; the DDL's generic
     * lock helper does not provide the required register sequence. */
    protection->FAPRT = 0x3210U;
}

/* Erase-only SRAM primitive.  Programming, protection close, and interrupt
 * masking remain the caller's responsibility. */
__attribute__((section(".flash_erase_only"), noinline, used)) void
board_flash_erase_sector_from_sram(uint32_t sector_address)
{
    const uint32_t cache_mask = EFM_FRMC_ICACHE | EFM_FRMC_DCACHE | EFM_FRMC_PREFETE;
    /* Keep each register pointer live through its corresponding operation;
     * these accesses intentionally remain distinct reads. */
    CM_EFM_TypeDef *const protection = (CM_EFM_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    protection->FAPRT = 0x0123U;
    protection->FAPRT = 0x3210U;
    const uint32_t key =
        *(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC4UL, 0x1FFF9AFCUL);
    protection->KEY1 = key;
    protection->KEY1 = ~key;
    CM_EFM_TypeDef *const cache = (CM_EFM_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    const uint32_t saved_cache = cache->FRMC & cache_mask;
    cache->FRMC &= ~(cache_mask | EFM_FRMC_CRST);
    CM_EFM_TypeDef *const status = (CM_EFM_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    status->FSCLR = EFM_FSCLR_OTPWERRCLR | EFM_FSCLR_PRTWERRCLR | EFM_FSCLR_PGSZERRCLR |
                    EFM_FSCLR_MISMTCHCLR | EFM_FSCLR_OPTENDCLR | EFM_FSCLR_COLERRCLR;
    while ((status->FSR & EFM_FSR_RDY) == 0U)
    {
    }
    CM_EFM_TypeDef *const mode = (CM_EFM_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC0UL, 0x1FFF9AF8UL);
    mode->FWMC = (mode->FWMC & ~EFM_FWMC_PEMOD) | (4UL << EFM_FWMC_PEMOD_POS);
    volatile uint32_t *const sector_protection = (volatile uint32_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9AC8UL, 0x1FFF9B00UL);
    *sector_protection = 1UL << (sector_address / BOARD_FLASH_SECTOR_SIZE);
    *(volatile uint32_t *)sector_address = 0U;
    while ((status->FSR & EFM_FSR_RDY) == 0U)
    {
    }
    status->FSCLR = EFM_FSCLR_OPTENDCLR;
    cache->FRMC |= saved_cache;
    mode->FWMC |= EFM_FWMC_KEY1LOCK;
    protection->FAPRT = 0x3210U;
}

__attribute__((section(".boot_record_writer"), noinline, used, optimize("Os"))) static void
write_boot_record_from_sram(void)
{
    const uint32_t cache_mask = EFM_FRMC_ICACHE | EFM_FRMC_DCACHE | EFM_FRMC_PREFETE;
    uint32_t saved_cache;
    __disable_irq();
    const volatile uint32_t *literals =
        (const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8724UL, 0x1FFF80E4UL);
    /* Keep one pool base register; eight separately materialized addresses
     * would grow the fixed SRAM body into the adjacent shared pool. */
    __asm volatile("" : "+r"(literals));
    CM_EFM_TypeDef *const protection = (CM_EFM_TypeDef *)(uintptr_t)literals[0];
    protection->FAPRT = 0x0123U;
    protection->FAPRT = 0x3210U;
    const uint32_t key = literals[1];
    protection->KEY1 = key;
    protection->KEY1 = ~key;
    volatile uint32_t *const cache = (volatile uint32_t *)(uintptr_t)literals[2];
    saved_cache = *cache & cache_mask;
    *cache &= ~(cache_mask | EFM_FRMC_CRST);
    volatile uint32_t *const status = (volatile uint32_t *)(uintptr_t)literals[3];
    volatile const uint32_t *const ready = (volatile const uint32_t *)((uintptr_t)status - 4U);
    *status = EFM_FSCLR_OTPWERRCLR | EFM_FSCLR_PRTWERRCLR | EFM_FSCLR_PGSZERRCLR |
              EFM_FSCLR_MISMTCHCLR | EFM_FSCLR_OPTENDCLR | EFM_FSCLR_COLERRCLR;
    while ((*ready & EFM_FSR_RDY) == 0U)
    {
    }
    volatile uint32_t *const mode = (volatile uint32_t *)(uintptr_t)literals[4];
    *mode = (*mode & ~EFM_FWMC_PEMOD) | (4UL << EFM_FWMC_PEMOD_POS);
    volatile uint32_t *const sector_protection = (volatile uint32_t *)(uintptr_t)literals[5];
    *sector_protection = 0x8000UL;
    volatile uint32_t *destination = (volatile uint32_t *)(uintptr_t)literals[6];
    *destination = 0U;
    while ((*ready & EFM_FSR_RDY) == 0U)
    {
    }
    *status = EFM_FSCLR_OPTENDCLR;
    *mode = (*mode & ~EFM_FWMC_PEMOD) | (3UL << EFM_FWMC_PEMOD_POS);
    destination = (volatile uint32_t *)(uintptr_t)literals[6];
    const volatile uint32_t *const source = (const volatile uint32_t *)(uintptr_t)literals[7];
    for (uint32_t index = 0U; index < BOOT_RECORD_WORDS; ++index)
    {
        destination[index] = source[index];
        while ((*ready & EFM_FSR_OPTEND) == 0U)
        {
        }
        *status = EFM_FSCLR_OPTENDCLR;
    }
    *mode &= ~EFM_FWMC_PEMOD;
    while ((*ready & EFM_FSR_RDY) == 0U)
    {
        *status = EFM_FSCLR_OPTENDCLR;
    }
    *sector_protection = 0U;
    /* Merge the saved cache bits without clearing newly set FRMC bits. */
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
    erase_and_program_from_sram(
        0x00036000UL,
        (const uint32_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(0x1FFFA5C0UL, 0x1FFFA550UL), 2U);
    __enable_irq();
}

void board_flash_write_zero_record_from(uint32_t sector_address, const uint32_t *source)
{
    erase_and_program_from_sram(sector_address, source, 2U);
}

bool board_flash_replace_sector_prefix(uint32_t sector_address, const void *data, size_t length)
{
    if ((data == NULL) || (length == 0U) || ((length & 3U) != 0U) ||
        ((sector_address & (BOARD_FLASH_SECTOR_SIZE - 1U)) != 0U) ||
        (sector_address >= BOARD_FLASH_END) || (length > BOARD_FLASH_SECTOR_SIZE))
    {
        return false;
    }

    erase_and_program_from_sram(sector_address, (const uint32_t *)data,
                                (uint32_t)(length / sizeof(uint32_t)));
    return true;
}
