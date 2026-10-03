#ifndef DAMIAO_AES256_CTR_H
#define DAMIAO_AES256_CTR_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t round_keys[240];
    uint8_t counter[16];
    uint8_t stream[16];
    uint8_t stream_used;
} Aes256Ctr;

void aes256_ctr_init(Aes256Ctr *context, const uint8_t key[32], const uint8_t initial_counter[16]);
void aes256_ctr_transform(Aes256Ctr *context, void *data, size_t length);
void aes256_encrypt_block(const uint8_t key[32], const uint8_t input[16], uint8_t output[16]);

#endif
