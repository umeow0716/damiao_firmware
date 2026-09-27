#ifndef DM4310_BOARD_POSITION_H
#define DM4310_BOARD_POSITION_H

#include <stdbool.h>
#include <stdint.h>

#define BOARD_POSITION_SENSOR_REGISTER_COUNT 11U
#define BOARD_POSITION_SENSOR_WRITABLE_COUNT 10U

typedef struct {
    uint32_t spi_cr;
    uint32_t spi_cfg1;
    uint32_t spi_cfg2;
    uint32_t dma_dtctl0;
    uint32_t dma_chctl0;
    uint32_t dma_intmask0;
    uint32_t dma_intmask1;
    uint32_t dma_trigger;
    uint32_t dma_irq_source;
    uint16_t miso_pin_control;
    uint16_t ss_pin_control;
    uint16_t mosi_pin_control;
    uint16_t sck_pin_control;
    uint16_t miso_pin_function;
    uint16_t ss_pin_function;
    uint16_t mosi_pin_function;
    uint16_t sck_pin_function;
    uint8_t sensor_register[BOARD_POSITION_SENSOR_REGISTER_COUNT];
    uint8_t sensor_value[BOARD_POSITION_SENSOR_REGISTER_COUNT];
} BoardPositionRegisterImage;

void board_position_build_config(BoardPositionRegisterImage *config);
bool board_position_init(void);
bool board_position_take_sample(uint16_t *dma_word);
void board_position_handle_timer_interrupt(void);
void board_position_ack_dma_interrupt(void);

#endif
