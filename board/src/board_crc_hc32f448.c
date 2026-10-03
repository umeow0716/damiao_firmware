#include "board_crc.h"

#include "hc32f448.h"

void board_crc_enable_clock(void)
{
    /* crc_clock_enable uses this exact protected FCG0 sequence. */
    CM_PWC->FCG0PC = 0xA5A50001UL;
    CM_PWC->FCG0 &= ~PWC_FCG0_CRC;
    CM_PWC->FCG0PC = 0xA5A50000UL;
}
