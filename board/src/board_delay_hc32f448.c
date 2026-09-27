#include "board_delay.h"

#include "hc32f448.h"

#define TMRA_TICKS_PER_MICROSECOND 100U
#define TMRA_MAX_DELAY_US          500U

static void delay_ticks(uint32_t ticks)
{
    CM_TMRA_1->CNTER = ticks;
    CM_TMRA_1->BCSTRL |= TMRA_BCSTRL_START;
    while ((CM_TMRA_1->BCSTRH & TMRA_BCSTRH_UDFF) == 0U) {
    }
    CM_TMRA_1->BCSTRH &= (uint8_t)~TMRA_BCSTRH_UDFF;
}

void board_delay_ms(uint32_t milliseconds)
{
    /* The original uses two 50,000-tick periods per millisecond. */
    for (uint32_t half_millisecond = 0U;
         half_millisecond < milliseconds * 2U;
         ++half_millisecond) {
        delay_ticks(TMRA_MAX_DELAY_US * TMRA_TICKS_PER_MICROSECOND);
    }
}

void board_delay_us(uint32_t microseconds)
{
    while (microseconds > TMRA_MAX_DELAY_US) {
        delay_ticks(TMRA_MAX_DELAY_US * TMRA_TICKS_PER_MICROSECOND);
        microseconds -= TMRA_MAX_DELAY_US;
    }
    delay_ticks(microseconds * TMRA_TICKS_PER_MICROSECOND);
}
