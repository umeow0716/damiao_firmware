#include "board_position.h"

#include <string.h>

#include "app_profile.h"

void board_position_build_config(BoardPositionRegisterImage *config)
{
    static const uint8_t registers[BOARD_POSITION_SENSOR_REGISTER_COUNT] = {
        0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U,
        0x06U, 0x09U, 0x0EU, 0x10U, 0x1BU,
    };
    static const uint8_t values[BOARD_POSITION_SENSOR_REGISTER_COUNT] = {
        0x00U, 0x00U, 0x00U, 0x00U, 0xC0U, 0xFFU,
        0x1CU, 0x00U, 0x77U, 0x9CU, 0x0EU,
    };

    memset(config, 0, sizeof(*config));
    config->spi_cr = 0x00000048UL;
    /* Bit 4 is present in the live read-back but the factory APP writes the
     * 0x50000000 configuration image. */
    config->spi_cfg1 = 0x50000000UL;
    config->spi_cfg2 = 0x0000EC08UL;
    config->dma_dtctl0 = 0x00010001UL;
    config->dma_chctl0 = 0x00001100UL;
    config->dma_intmask0 = 0x00010001UL;
    config->dma_intmask1 = 0x00010000UL;
    config->dma_trigger = 0x00000173UL;
    config->dma_irq_source = 65UL;
    config->miso_pin_control = 0x0000U;
    config->ss_pin_control = 0x0020U;
    config->mosi_pin_control = 0x0020U;
    config->sck_pin_control = 0x0020U;
    config->miso_pin_function = 0x0029U;
    config->ss_pin_function = 0x002AU;
    config->mosi_pin_function = 0x0028U;
    config->sck_pin_function = 0x002BU;
    memcpy(config->sensor_register, registers, sizeof(registers));
    memcpy(config->sensor_value, values, sizeof(values));
}
