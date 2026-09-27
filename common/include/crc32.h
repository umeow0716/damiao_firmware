#ifndef DM4310_CRC32_H
#define DM4310_CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_ieee(const void *data, size_t length, uint32_t seed);

#endif

