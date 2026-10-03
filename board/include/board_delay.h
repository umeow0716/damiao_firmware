#ifndef DAMIAO_BOARD_DELAY_H
#define DAMIAO_BOARD_DELAY_H

#include <stdint.h>

/* Blocking delays backed by the TMRA1 time base configured by board_clock.
 * These reproduce delay_ms and delay_us from the firmware APP. */
void board_delay_ms(uint32_t milliseconds);
void board_delay_us(uint32_t microseconds);

#endif
