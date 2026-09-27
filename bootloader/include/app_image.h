#ifndef DM4310_BOOT_APP_IMAGE_H
#define DM4310_BOOT_APP_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DM4310_APP_BASE       0x00020000UL
/* The recovered application stores its 37-word configuration at 0x30000 and
 * additional calibration records above it.  Source application updates must
 * stop before that first persistent sector. */
#define DM4310_APP_END        0x00030000UL
#define DM4310_SRAM_BASE      0x1FFF8000UL
#define DM4310_SRAM_END       0x20008000UL

typedef enum {
    APP_IMAGE_OK = 0,
    APP_IMAGE_ERASED,
    APP_IMAGE_BAD_STACK,
    APP_IMAGE_BAD_RESET,
    APP_IMAGE_BAD_CRC,
    APP_IMAGE_BAD_LENGTH,
} AppImageStatus;

AppImageStatus app_image_validate(const void *image, size_t image_length,
                                  uint32_t expected_crc);
bool app_image_installed(void);
bool app_image_installed_length(size_t image_length);
void app_image_jump(void) __attribute__((noreturn));

#endif
