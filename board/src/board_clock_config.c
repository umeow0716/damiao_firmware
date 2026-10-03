#include "board_clock.h"

#include <string.h>

void board_clock_build_config(BoardClockConfig *config)
{
    memset(config, 0, sizeof(*config));

    /* PLLH is fed by the board's 8 MHz crystal:
     * VCO=8/1*100=800 MHz, P=200 MHz, Q=80 MHz and R=200 MHz. */
    config->system_clock_config = 0x00112210UL;
    config->pll_config = 0x39306300UL;
    config->flash_read_config = 0x00070003UL;
    config->sram_wait_config = 0x11000000UL;

    config->system_hz = 200000000UL;
    config->hclk_hz = 200000000UL;
    config->pclk0_hz = 200000000UL;
    config->pclk1_hz = 100000000UL;
    config->pclk2_hz = 50000000UL;
    config->pclk3_hz = 50000000UL;
    config->pclk4_hz = 200000000UL;
    config->pll_q_hz = 80000000UL;
}

bool board_clock_validate_config(const BoardClockConfig *config)
{
    if (config == NULL)
    {
        return false;
    }

    const uint32_t pll_m = (config->pll_config & 0x3UL) + 1UL;
    const uint32_t pll_n = ((config->pll_config >> 8U) & 0xFFUL) + 1UL;
    const uint32_t pll_r = ((config->pll_config >> 20U) & 0xFUL) + 1UL;
    const uint32_t pll_q = ((config->pll_config >> 24U) & 0xFUL) + 1UL;
    const uint32_t pll_p = ((config->pll_config >> 28U) & 0xFUL) + 1UL;
    const uint32_t vco_hz = (8000000UL / pll_m) * pll_n;

    return ((config->pll_config & 0x80UL) == 0UL) &&
           (config->system_clock_config == 0x00112210UL) &&
           (config->flash_read_config == 0x00070003UL) &&
           (config->sram_wait_config == 0x11000000UL) && ((vco_hz / pll_p) == config->system_hz) &&
           ((vco_hz / pll_q) == config->pll_q_hz) && ((vco_hz / pll_r) == 200000000UL) &&
           (config->system_hz == config->hclk_hz) && (config->pclk0_hz == config->system_hz) &&
           ((config->system_hz >> 1U) == config->pclk1_hz) &&
           ((config->system_hz >> 2U) == config->pclk2_hz) &&
           ((config->system_hz >> 2U) == config->pclk3_hz) &&
           (config->pclk4_hz == config->system_hz);
}
