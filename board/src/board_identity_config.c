#include "board_identity.h"

#define HARDWARE_VARIANT_SHIFT 14U
#define HARDWARE_VARIANT_MASK 0x03U

uint8_t board_identity_decode_hardware_variant(uint16_t port_c_input)
{
    return (uint8_t)((port_c_input >> HARDWARE_VARIANT_SHIFT) & HARDWARE_VARIANT_MASK);
}
