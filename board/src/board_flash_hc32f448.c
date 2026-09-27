#include "board_flash.h"

#include "hc32f448.h"

#define BOARD_FLASH_END         (0x00040000UL)

__attribute__((section(".ramfunc"), noinline))
static void erase_and_program_from_sram(uint32_t sector_address,
                                        const uint32_t *source,
                                        uint32_t word_count)
{
    const uint32_t sector = sector_address / BOARD_FLASH_SECTOR_SIZE;
    const uint32_t cache_mask = EFM_FRMC_ICACHE | EFM_FRMC_DCACHE |
                                EFM_FRMC_PREFETE;
    const uint32_t saved_cache = CM_EFM->FRMC & cache_mask;
    CM_EFM->FAPRT = 0x0123U;
    CM_EFM->FAPRT = 0x3210U;
    /* FWMC is unlocked by writing both words to KEY1.  KEY2 is the separate
     * OTP unlock register.  The historical APP RAM writer at 0x1fff8640 and
     * HC32F448 DDL EFM_FWMC_Cmd() both use this exact KEY1/KEY1 sequence. */
    CM_EFM->KEY1 = 0x01234567UL;
    CM_EFM->KEY1 = 0xFEDCBA98UL;
    CM_EFM->FRMC &= ~cache_mask;
    CM_EFM->FSCLR = EFM_FSCLR_OTPWERRCLR | EFM_FSCLR_PRTWERRCLR |
                    EFM_FSCLR_PGSZERRCLR | EFM_FSCLR_MISMTCHCLR |
                    EFM_FSCLR_OPTENDCLR | EFM_FSCLR_COLERRCLR;
    while ((CM_EFM->FSR & EFM_FSR_RDY) == 0U) {}
    CM_EFM->F0NWPRT = 1UL << sector;

    CM_EFM->FWMC = (CM_EFM->FWMC & ~EFM_FWMC_PEMOD) |
                   (4UL << EFM_FWMC_PEMOD_POS);
    *(volatile uint32_t *)sector_address = 0U;
    while ((CM_EFM->FSR & EFM_FSR_RDY) == 0U) {}
    CM_EFM->FSCLR = EFM_FSCLR_OPTENDCLR;

    CM_EFM->FWMC = (CM_EFM->FWMC & ~EFM_FWMC_PEMOD) |
                   (3UL << EFM_FWMC_PEMOD_POS);
    volatile uint32_t *destination = (volatile uint32_t *)sector_address;
    for (uint32_t index = 0U; index < word_count; ++index) {
        destination[index] = source[index];
        while ((CM_EFM->FSR & EFM_FSR_OPTEND) == 0U) {}
        CM_EFM->FSCLR = EFM_FSCLR_OPTENDCLR;
    }

    CM_EFM->FWMC &= ~EFM_FWMC_PEMOD;
    while ((CM_EFM->FSR & EFM_FSR_RDY) == 0U) {
        CM_EFM->FSCLR = EFM_FSCLR_OPTENDCLR;
    }
    CM_EFM->F0NWPRT = 0U;
    CM_EFM->FRMC = (CM_EFM->FRMC & ~cache_mask) | saved_cache;
    CM_EFM->FWMC |= EFM_FWMC_KEY1LOCK;
    /* The shipped RAM writer finishes by writing the second protection key
     * again; retain that instruction-level behavior instead of substituting
     * the DDL's generic lock helper. */
    CM_EFM->FAPRT = 0x3210U;
}

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

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    erase_and_program_from_sram(
        sector_address, (const uint32_t *)data,
        (uint32_t)(length / sizeof(uint32_t)));
    __DSB();
    __ISB();
    if (primask == 0U) {
        __enable_irq();
    }
    return true;
}
