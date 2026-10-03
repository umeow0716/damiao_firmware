#include "crc32.h"

uint32_t crc32_ieee(const void *data, size_t length, uint32_t seed)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = ~seed;
    for (size_t i = 0; i < length; ++i)
    {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8U; ++bit)
        {
            const uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}
