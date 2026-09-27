#ifndef DAMIAO_BOARD_IDENTITY_H
#define DAMIAO_BOARD_IDENTITY_H

#include <stdint.h>

/* PC14/PC15 are two pull-up strap inputs used by the original APP to select
 * the visible hardware suffix (V2.0, V3.0, V4.0 or V1.0). */
uint8_t board_identity_decode_hardware_variant(uint16_t port_c_input);
uint8_t board_identity_read_hardware_variant(void);

#endif
