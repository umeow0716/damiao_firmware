#ifndef DAMIAO_BOARD_FLASH_H
#define DAMIAO_BOARD_FLASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BOARD_FLASH_SECTOR_SIZE (0x2000UL)

/* Erases one 8 KiB main-Flash sector and programs its prefix.  Like the
 * firmware routine, bytes after length remain erased and power loss is not an
 * atomic transaction.  The primitive executes from SRAM while Flash is busy. */
bool board_flash_replace_sector_prefix(uint32_t sector_address, const void *data, size_t length);
/* SRAM-resident writer for the five-word persistent boot record. */
void board_flash_write_boot_record(void);
void board_flash_write_zero_record(void);
void board_flash_write_zero_record_from(uint32_t sector_address, const uint32_t *source);
/* SRAM-resident erase-only primitive; the caller supplies the sector address. */
void board_flash_erase_sector_from_sram(uint32_t sector_address);

#endif
