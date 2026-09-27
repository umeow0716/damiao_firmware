#ifndef DAMIAO_BOARD_CLOCK_H
#define DAMIAO_BOARD_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t system_clock_config;
    uint32_t pll_config;
    uint32_t flash_read_config;
    uint32_t sram_wait_config;
    uint32_t system_hz;
    uint32_t hclk_hz;
    uint32_t pclk0_hz;
    uint32_t pclk1_hz;
    uint32_t pclk2_hz;
    uint32_t pclk3_hz;
    uint32_t pclk4_hz;
    uint32_t pll_q_hz;
} BoardClockConfig;

void board_clock_build_config(BoardClockConfig *config);
bool board_clock_validate_config(const BoardClockConfig *config);
bool board_clock_init(void);
bool board_clock_is_ready(void);

#endif
