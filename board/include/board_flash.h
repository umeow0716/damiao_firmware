#ifndef DM4310_BOARD_FLASH_H
#define DM4310_BOARD_FLASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BOARD_FLASH_SECTOR_SIZE (0x2000UL)

/* Erases one 8 KiB main-Flash sector and programs its prefix.  Like the
 * factory routine, bytes after length remain erased and power loss is not an
 * atomic transaction.  The primitive executes from SRAM while Flash is busy. */
bool board_flash_replace_sector_prefix(uint32_t sector_address,
                                       const void *data,
                                       size_t length);

#endif
