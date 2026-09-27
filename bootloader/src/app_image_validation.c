#include "app_image.h"

#include "crc32.h"

AppImageStatus app_image_validate(const void *image, size_t image_length,
                                  uint32_t expected_crc)
{
    if (image == NULL) {
        return APP_IMAGE_ERASED;
    }
    if ((image_length < 8U) ||
        (image_length > (DM4310_APP_END - DM4310_APP_BASE))) {
        return APP_IMAGE_BAD_LENGTH;
    }

    const uint32_t *vectors = (const uint32_t *)image;
    const uint32_t initial_sp = vectors[0];
    const uint32_t reset = vectors[1];
    if ((initial_sp == 0xFFFFFFFFUL) && (reset == 0xFFFFFFFFUL)) {
        return APP_IMAGE_ERASED;
    }
    if ((initial_sp < DM4310_SRAM_BASE) || (initial_sp > DM4310_SRAM_END) ||
        ((initial_sp & 7U) != 0U)) {
        return APP_IMAGE_BAD_STACK;
    }

    const uint32_t reset_address = reset & ~1UL;
    const uint32_t received_end = DM4310_APP_BASE + (uint32_t)image_length;
    if (((reset & 1U) == 0U) || (reset_address < DM4310_APP_BASE) ||
        (reset_address >= received_end)) {
        return APP_IMAGE_BAD_RESET;
    }
    if ((expected_crc != 0U) &&
        (crc32_ieee(image, image_length, 0U) != expected_crc)) {
        return APP_IMAGE_BAD_CRC;
    }
    return APP_IMAGE_OK;
}
