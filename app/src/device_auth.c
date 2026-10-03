#include "device_auth.h"

#include <stddef.h>
#include <string.h>

#include "aes256_ctr.h"

/* Device-binding credentials are independent of the configurable firmware
 * update profile. */
static const uint8_t device_binding_key[32] = {
    0x62, 0x11, 0x66, 0x2f, 0x52, 0x18, 0x73, 0x89, 0x6d, 0x2a, 0x62, 0x11, 0x72, 0x31, 0x77, 0x40,
    0x73, 0xca, 0x73, 0xca, 0x59, 0x79, 0x66, 0x2f, 0x62, 0x11, 0x76, 0x84, 0x59, 0x73, 0x79, 0x5e,
};

static const uint8_t device_binding_initial_counter[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
};

void boot_device_derive_token(const uint8_t uid[BOOT_DEVICE_UID_SIZE],
                              uint8_t token[BOOT_DEVICE_TOKEN_SIZE])
{
    memcpy(token, uid, BOOT_DEVICE_UID_SIZE);

    token[12] = 1U;
    token[13] = 2U;
    token[14] = 3U;
    token[15] = 4U;

    Aes256Ctr cipher;

    aes256_ctr_init(&cipher, device_binding_key, device_binding_initial_counter);

    aes256_ctr_transform(&cipher, token, BOOT_DEVICE_TOKEN_SIZE);
}

int boot_device_find_matching_slot(
    const uint8_t token[BOOT_DEVICE_TOKEN_SIZE],
    const uint8_t slots[BOOT_DEVICE_KEY_SLOT_COUNT][BOOT_DEVICE_TOKEN_SIZE])
{
    if ((token == NULL) || (slots == NULL))
    {
        return -1;
    }
    for (uint8_t slot = 0U; slot < BOOT_DEVICE_KEY_SLOT_COUNT; ++slot)
    {
        /* Accept the first complete 16-byte match. */
        if (memcmp(token, slots[slot], BOOT_DEVICE_TOKEN_SIZE) == 0)
        {
            return (int)slot;
        }
    }
    return -1;
}

bool boot_device_authenticate(
    const uint8_t uid[BOOT_DEVICE_UID_SIZE],
    const uint8_t slots[BOOT_DEVICE_KEY_SLOT_COUNT][BOOT_DEVICE_TOKEN_SIZE], uint8_t *matched_slot)
{
    if ((uid == NULL) || (slots == NULL))
    {
        return false;
    }
    uint8_t token[BOOT_DEVICE_TOKEN_SIZE];
    boot_device_derive_token(uid, token);
    const int slot = boot_device_find_matching_slot(token, slots);
    if (slot < 0)
    {
        return false;
    }
    if (matched_slot != NULL)
    {
        *matched_slot = (uint8_t)slot;
    }
    return true;
}
