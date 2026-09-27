#include "device_auth.h"

#include <stddef.h>
#include <string.h>

#include "aes256_ctr.h"
#include "update_profile.h"

void boot_device_derive_token(
    const uint8_t uid[BOOT_DEVICE_UID_SIZE],
    uint8_t token[BOOT_DEVICE_TOKEN_SIZE])
{
    memcpy(token, uid, BOOT_DEVICE_UID_SIZE);

    token[12] = 1U;
    token[13] = 2U;
    token[14] = 3U;
    token[15] = 4U;

    Aes256Ctr cipher;

    aes256_ctr_init(
        &cipher,
        dm4310_update_key,
        dm4310_update_initial_counter);

    aes256_ctr_transform(
        &cipher,
        token,
        BOOT_DEVICE_TOKEN_SIZE);
}

int boot_device_find_matching_slot(
    const uint8_t token[BOOT_DEVICE_TOKEN_SIZE],
    const uint8_t slots[BOOT_DEVICE_KEY_SLOT_COUNT][BOOT_DEVICE_TOKEN_SIZE])
{
    if ((token == NULL) || (slots == NULL)) {
        return -1;
    }
    for (uint8_t slot = 0U; slot < BOOT_DEVICE_KEY_SLOT_COUNT; ++slot) {
        if (memcmp(token, slots[slot], BOOT_DEVICE_TOKEN_SIZE) == 0) {
            return (int)slot;
        }
    }
    return -1;
}

bool boot_device_authenticate(
    const uint8_t uid[BOOT_DEVICE_UID_SIZE],
    const uint8_t slots[BOOT_DEVICE_KEY_SLOT_COUNT][BOOT_DEVICE_TOKEN_SIZE],
    uint8_t *matched_slot)
{
    if ((uid == NULL) || (slots == NULL)) {
        return false;
    }
    uint8_t token[BOOT_DEVICE_TOKEN_SIZE];
    boot_device_derive_token(uid, token);
    const int slot = boot_device_find_matching_slot(token, slots);
    if (slot < 0) {
        return false;
    }
    if (matched_slot != NULL) {
        *matched_slot = (uint8_t)slot;
    }
    return true;
}
