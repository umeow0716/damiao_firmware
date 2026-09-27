#ifndef DM4310_BOARD_MCAN_H
#define DM4310_BOARD_MCAN_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t id;
    uint8_t length;
    uint8_t data[64];
} BoardMcanFrame;

typedef struct {
    uint32_t nbtp;
    uint32_t dbtp;
    uint32_t tdcr;
    uint32_t gfc;
    uint32_t sidfc;
    uint32_t xidfc;
    uint32_t rxf0c;
    uint32_t rxbc;
    uint32_t rxf1c;
    uint32_t rxesc;
    uint32_t txbc;
    uint32_t txesc;
    uint32_t txefc;
    uint32_t ie;
    uint32_t ils;
    uint32_t ile;
    uint16_t rx_pin_control;
    uint16_t tx_pin_control;
    uint16_t rx_pin_function;
    uint16_t tx_pin_function;
    bool fd_enabled;
    uint32_t standard_filter[2];
} BoardMcanRegisterImage;

void board_mcan_build_config(uint16_t node_id,
                                       uint8_t data_rate_selector,
                                       BoardMcanRegisterImage *config);
bool board_mcan_init(uint16_t node_id, uint8_t data_rate_selector);
bool board_mcan_receive(BoardMcanFrame *frame);
bool board_mcan_send(const BoardMcanFrame *frame);
void board_mcan_update_node_filter(uint16_t node_id);
bool board_mcan_flush(uint32_t timeout_us);
uint8_t board_mcan_ack_interrupt(void);

#endif
