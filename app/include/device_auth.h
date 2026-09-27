#ifndef DAMIAO_DEVICE_AUTH_H
#define DAMIAO_DEVICE_AUTH_H

#include <stdbool.h>
#include <stdint.h>

enum {
    BOOT_DEVICE_UID_SIZE = 12,
    BOOT_DEVICE_TOKEN_SIZE = 16,
    BOOT_DEVICE_KEY_SLOT_COUNT = 3,
};

void boot_device_derive_token(
    const uint8_t uid[BOOT_DEVICE_UID_SIZE],
    uint8_t token[BOOT_DEVICE_TOKEN_SIZE]);
int boot_device_find_matching_slot(
    const uint8_t token[BOOT_DEVICE_TOKEN_SIZE],
    const uint8_t slots[BOOT_DEVICE_KEY_SLOT_COUNT][BOOT_DEVICE_TOKEN_SIZE]);
bool boot_device_authenticate(
    const uint8_t uid[BOOT_DEVICE_UID_SIZE],
    const uint8_t slots[BOOT_DEVICE_KEY_SLOT_COUNT][BOOT_DEVICE_TOKEN_SIZE],
    uint8_t *matched_slot);

#endif
