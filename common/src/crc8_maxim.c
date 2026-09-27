#include "crc8_maxim.h"

uint8_t damiao_crc8_maxim(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint8_t crc = 0U;
    for (size_t index = 0U; index < length; ++index) {
        crc ^= bytes[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (uint8_t)((crc >> 1U) ^
                            ((crc & 1U) != 0U ? 0x8cU : 0U));
        }
    }
    return crc;
}
