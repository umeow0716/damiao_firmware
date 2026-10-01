#ifndef DAMIAO_BOARD_MCAN_H
#define DAMIAO_BOARD_MCAN_H

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

#if defined(DAMIAO_DM4310)
typedef struct Dm4310McanIrqReferences {
    volatile uint32_t *controller;
    const volatile uint16_t *config;
    volatile uint32_t *status;
    uint32_t initial_ir;
    volatile void *dispatch;
    volatile void *response_dispatch;
    volatile void *parameter_scratch;
    volatile uint16_t *received_id;
    volatile void *sample;
    volatile void *motor;
    float cleared;
    float two_pi;
    uint32_t node_id;
} Dm4310McanIrqReferences;
#endif

void board_mcan_build_config(uint16_t node_id,
                                       uint16_t data_rate_selector,
                                       BoardMcanRegisterImage *config);
bool board_mcan_init(uint16_t node_id, uint16_t data_rate_selector);
bool board_mcan_receive(BoardMcanFrame *frame);
bool board_mcan_send(const BoardMcanFrame *frame);
#if defined(DAMIAO_DM4310)
void board_mcan_begin_irq(Dm4310McanIrqReferences *references);
bool board_mcan_receive_payload(uint32_t *id, uint8_t *length, uint8_t *data,
                                Dm4310McanIrqReferences *references);
void board_mcan_send_prebuilt(uint16_t id, uint8_t length);
void board_mcan_send_prebuilt_irq(uint16_t id, uint8_t length,
    const Dm4310McanIrqReferences *references);
void board_mcan_reinitialize_live(bool after_parameter_write,
    const Dm4310McanIrqReferences *references);
#endif
#if defined(DAMIAO_DM4310)
float board_mcan_current_data_rate_kbps(void);
bool board_mcan_reinitialization_requested(
    const Dm4310McanIrqReferences *references);
uint8_t board_mcan_recover_bus_off(
    const Dm4310McanIrqReferences *references);
uint32_t board_mcan_send_classic_payload(const uint8_t *data, uint16_t id,
                                         uint8_t length);
/* R3 is the retained DLC seed when length exceeds the factory DLC table. */
uint32_t board_mcan_send_fd_payload(const uint8_t *data, uint16_t id,
                                    uint8_t length, uint32_t initial_dlc);
uint32_t board_mcan_send_variable_fd_payload(const uint8_t *data, uint16_t id,
                                             uint8_t length, uint32_t initial_dlc);
uint32_t dm4310_mcan_send_classic_helper(const uint8_t *data, uint16_t id,
                                         uint8_t length);
uint32_t dm4310_mcan_send_fd_helper(const uint8_t *data, uint16_t id,
                                    uint8_t length);
uint32_t dm4310_mcan_send_variable_fd_helper(const uint8_t *data, uint16_t id,
                                             uint8_t length, uint32_t initial_dlc);
#endif
void board_mcan_update_node_filter(uint16_t node_id);
void board_mcan_select_transport_format(uint8_t data_rate_selector);
#if defined(DAMIAO_DM4310)
void board_mcan_select_transport_format_irq(uint8_t data_rate_selector,
    const Dm4310McanIrqReferences *references);
#endif
bool board_mcan_flush(uint32_t timeout_us);
#if defined(DAMIAO_DM4310)
uint8_t board_mcan_ack_interrupt(
    const Dm4310McanIrqReferences *references);
#else
uint8_t board_mcan_ack_interrupt(void);
#endif

#endif
