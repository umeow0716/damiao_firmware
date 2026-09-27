#include "flash_writer.h"

#include "app_image.h"
#include "boot_platform.h"
#include "crc32.h"

void flash_writer_init(FlashWriter *writer)
{
    writer->next_address = DM4310_APP_BASE;
    writer->running_crc = 0U;
    writer->bytes_written = 0U;
    writer->erased = false;
    writer->failed = false;
}

bool flash_writer_begin(FlashWriter *writer)
{
    flash_writer_init(writer);
    /* The recovered transport maps every packet to one 8 KiB sector.  Delay
     * the erase until the complete packet has passed CRC, otherwise erasing
     * here would stall CAN reception for all 15 application sectors. */
    writer->erased = boot_platform_mark_update_started();
    writer->failed = !writer->erased;
    return writer->erased;
}

bool flash_writer_append(FlashWriter *writer, const void *data,
                         size_t data_length, size_t storage_length)
{
    if (!writer->erased || writer->failed || (data == NULL) ||
        (data_length == 0U) || (storage_length < data_length) ||
        ((writer->next_address + storage_length) > DM4310_APP_END) ||
        ((writer->next_address & (FLASH_WRITER_SECTOR_SIZE - 1U)) != 0U) ||
        (storage_length > FLASH_WRITER_SECTOR_SIZE) ||
        ((storage_length & 3U) != 0U)) {
        writer->failed = true;
        return false;
    }
    if (!boot_platform_flash_program(writer->next_address, data,
                                     storage_length)) {
        writer->failed = true;
        return false;
    }
    writer->running_crc = crc32_ieee(data, data_length, writer->running_crc);
    writer->next_address += (uint32_t)storage_length;
    writer->bytes_written += data_length;
    return true;
}

uint32_t flash_writer_crc(const FlashWriter *writer)
{
    return writer->running_crc;
}
