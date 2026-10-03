#ifndef DAMIAO_BOARD_UART_H
#define DAMIAO_BOARD_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BOARD_UART_RX_DMA_CAPACITY 200U

typedef struct
{
    uint32_t usart_brr;
    uint32_t usart_cr1_dma;
    uint32_t usart_cr1_polled;
    uint32_t usart_cr2;
    uint32_t tmr0_bconr;
    uint32_t tmr0_compare;
    uint32_t dma_dtctl0;
    uint32_t dma_chctl0;
    uint32_t dma_trigger;
    uint32_t irq_source;
    uint16_t tx_pin_control;
    uint16_t rx_pin_control;
    uint16_t tx_pin_function;
    uint16_t rx_pin_function;
} BoardUartRegisterImage;

void board_uart_build_config(BoardUartRegisterImage *config);
void board_uart_initialize_state(void);
uint16_t board_uart_expected_payload_length(void);
uint16_t board_uart_received_length(void);
bool board_uart_begin_receive_irq(const uint8_t **data, int16_t *length);
void board_uart_rearm_receive_irq(void);
bool board_uart_init(void);
bool board_uart_init_polled(void);
bool board_uart_receive(uint8_t *byte);
size_t board_uart_write(const void *data, size_t length);
bool board_uart_flush(void);
void board_uart_ack_interrupt(void);

#endif
