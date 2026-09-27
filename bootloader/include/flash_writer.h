#ifndef DM4310_FLASH_WRITER_H
#define DM4310_FLASH_WRITER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FLASH_WRITER_SECTOR_SIZE 0x2000U

typedef struct {
    uint32_t next_address;
    uint32_t running_crc;
    size_t bytes_written;
    bool erased;
    bool failed;
} FlashWriter;

void flash_writer_init(FlashWriter *writer);
bool flash_writer_begin(FlashWriter *writer);
bool flash_writer_append(FlashWriter *writer, const void *data,
                         size_t data_length, size_t storage_length);
uint32_t flash_writer_crc(const FlashWriter *writer);

#endif
