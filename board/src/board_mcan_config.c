#include "board_mcan.h"

#include <stddef.h>

typedef struct {
    uint8_t prescaler;
    uint8_t time_seg1;
    uint8_t time_seg2;
    uint8_t sync_jump_width;
} DataBitTiming;

static DataBitTiming data_timing_for_selector(uint8_t selector)
{
    if (selector < 4U) {
        selector = 4U;
    }

    switch (selector) {
    case 5U:
        return (DataBitTiming) {1U, 31U, 8U, 8U};
    case 6U:
        return (DataBitTiming) {1U, 25U, 6U, 6U};
    case 7U:
        return (DataBitTiming) {1U, 19U, 5U, 5U};
    case 8U:
        return (DataBitTiming) {1U, 15U, 4U, 4U};
    case 9U:
        return (DataBitTiming) {1U, 12U, 3U, 3U};
    case 10U:
        return (DataBitTiming) {1U, 7U, 2U, 2U};
    case 11U:
        return (DataBitTiming) {1U, 5U, 2U, 2U};
    case 4U:
    default:
        return (DataBitTiming) {2U, 31U, 8U, 8U};
    }
}

static uint32_t nominal_timing_for_selector(uint8_t selector)
{
    /* The captured 1 Mbps image uses 80 time quanta at an 80 MHz MCAN
     * source clock. Selectors 0..3 preserve its sample point and scale the
     * prescaler; CAN-FD selectors keep 1 Mbps arbitration. */
    static const uint8_t prescaler_by_selector[5] = {8U, 5U, 4U, 2U, 1U};
    const uint8_t prescaler = selector < 5U ? prescaler_by_selector[selector] : 1U;
    return 0x26003A13UL | ((uint32_t)(prescaler - 1U) << 16U);
}

void board_mcan_build_config(uint16_t node_id,
                                       uint8_t data_rate_selector,
                                       BoardMcanRegisterImage *config)
{
    if (config == NULL) {
        return;
    }

    const DataBitTiming timing = data_timing_for_selector(data_rate_selector);
    uint32_t dbtp = ((uint32_t)(timing.time_seg1 - 1U) << 8U) |
                    ((uint32_t)(timing.prescaler - 1U) << 16U) |
                    ((uint32_t)(timing.time_seg2 - 1U) << 4U) |
                    (uint32_t)(timing.sync_jump_width - 1U);
    if (data_rate_selector > 5U) {
        dbtp |= 0x00800000UL; /* DBTP.TDC */
    }

    *config = (BoardMcanRegisterImage) {
        .nbtp = nominal_timing_for_selector(data_rate_selector),
        .dbtp = dbtp,
        .tdcr = (uint32_t)timing.prescaler * (uint32_t)timing.time_seg1 << 8U,
        .gfc = 0x0000002BUL,
        .sidfc = 0x00020000UL,
        .xidfc = 0x00000040UL,
        .rxf0c = 0x80030080UL,
        .rxbc = 0x00000230UL,
        .rxf1c = 0x80030158UL,
        .rxesc = 0x00000777UL,
        .txbc = 0x03000328UL,
        .txesc = 0x00000007UL,
        .txefc = 0x00040308UL,
        .ie = 0x02800011UL,
        .ils = 0x02800001UL,
        .ile = 0x00000003UL,
        /* The captured 0x0120 includes GPIO_PCR_PIN because both wires were
         * high.  mcan1_configure_* writes only high-drive configuration. */
        .rx_pin_control = 0x0020U,
        .tx_pin_control = 0x0020U,
        .rx_pin_function = 0x0033U,
        .tx_pin_function = 0x0032U,
        .fd_enabled = data_rate_selector > 4U,
        .standard_filter = {
            0x880000FFUL | ((uint32_t)(node_id & 0x07FFU) << 16U),
            0x8FFF07FFUL,
        },
    };
}
