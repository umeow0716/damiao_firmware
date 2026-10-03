#ifndef DAMIAO_BOARD_IDENTITY_H
#define DAMIAO_BOARD_IDENTITY_H

#include <stdint.h>

/* PC14/PC15 are pull-up strap inputs that select the visible hardware suffix
 * (V2.0, V3.0, V4.0 or V1.0). */
uint8_t board_identity_decode_hardware_variant(uint16_t port_c_input);
uint8_t board_identity_read_hardware_variant(void);
uint8_t board_identity_initialize_status_and_read_variant(void);

#endif
