#include "board_position.h"

#include <stddef.h>
#include <stdint.h>

#include "board_delay.h"
#include "hc32f448.h"

#define POSITION_DMA_CLEAR_ALL     (0x000F000FUL)
#define POSITION_DMA_CHANNEL       (DMA_CHEN_CHEN_0)
#define POSITION_DMA_TC_FLAG       (DMA_INTSTAT1_TC_0)
#define POSITION_SWD_PIN_MASK      ((uint16_t)0x0003U)

static volatile uint16_t position_dma_word __attribute__((aligned(4)));
static bool position_initialized;

static uint16_t spi_transfer(uint16_t transmit)
{
    while ((CM_SPI3->SR & SPI_SR_TDEF) == 0U) {
    }
    CM_SPI3->DR = transmit;

    while ((CM_SPI3->SR & SPI_SR_RDFF) == 0U) {
    }
    return (uint16_t)CM_SPI3->DR;
}

static void configure_pins_and_spi(
    const BoardPositionRegisterImage *config)
{
    CM_PWC->FCG1 &= ~PWC_FCG1_SPI3;

    /* Original PCB routing: PB4=MISO, PB5=SS, PB8=MOSI, PB9=SCK. Masking
     * PSPCR to SWCLK/SWDIO preserves the loader's enabled/disabled state
     * while removing the unused JTAG/SWO pins as done at 0x23170. */
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PCRB9 = config->sck_pin_control;
    CM_GPIO->PCRB8 = config->mosi_pin_control;
    CM_GPIO->PCRB5 = config->ss_pin_control;
    CM_GPIO->PSPCR &= POSITION_SWD_PIN_MASK;
    CM_GPIO->PFSRB9 = config->sck_pin_function;
    CM_GPIO->PFSRB8 = config->mosi_pin_function;
    CM_GPIO->PFSRB5 = config->ss_pin_function;
    CM_GPIO->PFSRB4 = config->miso_pin_function;
    CM_GPIO->PWPR = 0xA500U;

    CM_SPI3->CFG2 = config->spi_cfg2;
    CM_SPI3->CFG1 = config->spi_cfg1;
    CM_SPI3->CR = config->spi_cr & ~SPI_CR_SPE;
    CM_SPI3->CR |= SPI_CR_SPE;
}

static void configure_sensor_registers(
    const BoardPositionRegisterImage *config)
{
    uint8_t observed[BOARD_POSITION_SENSOR_REGISTER_COUNT];

    for (uint32_t i = 0U; i < BOARD_POSITION_SENSOR_REGISTER_COUNT; ++i) {
        const uint16_t read_command = (uint16_t)(0x4000U |
            ((uint16_t)config->sensor_register[i] << 8U));
        spi_transfer(read_command);
        board_delay_ms(1U);
        observed[i] = (uint8_t)(spi_transfer(0U) >> 8U);
        board_delay_ms(25U);
    }

    for (uint32_t i = 0U; i < BOARD_POSITION_SENSOR_WRITABLE_COUNT; ++i) {
        if (observed[i] == config->sensor_value[i]) {
            continue;
        }
        const uint16_t write_command = (uint16_t)(0x8000U |
            ((uint16_t)config->sensor_register[i] << 8U) |
            config->sensor_value[i]);
        spi_transfer(write_command);
        board_delay_ms(25U);
        observed[i] = (uint8_t)(spi_transfer(0U) >> 8U);
        board_delay_ms(5U);
    }
}

static void configure_dma(const BoardPositionRegisterImage *config)
{
    CM_PWC->FCG0PC = 0xA5A50001UL;
    CM_PWC->FCG0 &= ~PWC_FCG0_DMA2;
    CM_PWC->FCG0PC = 0xA5A50000UL;

    CM_DMA2->EN = DMA_EN_EN;
    CM_DMA2->SAR0 = (uint32_t)(uintptr_t)&CM_SPI3->DR;
    CM_DMA2->DAR0 = (uint32_t)(uintptr_t)&position_dma_word;
    CM_DMA2->DTCTL0 = config->dma_dtctl0;
    CM_DMA2->CHCTL0 = config->dma_chctl0;
    CM_DMA2->CHEN = POSITION_DMA_CHANNEL;
    CM_DMA2->INTCLR0 = POSITION_DMA_CLEAR_ALL;
    CM_DMA2->INTCLR1 = POSITION_DMA_CLEAR_ALL;
    CM_DMA2->INTMASK0 = config->dma_intmask0;
    CM_DMA2->INTMASK1 = config->dma_intmask1;

    CM_PWC->FCG0PC = 0xA5A50001UL;
    CM_PWC->FCG0 &= ~PWC_FCG0_AOS;
    CM_AOS->DMA2_TRGSEL0 = config->dma_trigger;
    CM_PWC->FCG0PC = 0xA5A50000UL;
}

bool board_position_init(void)
{
    BoardPositionRegisterImage config;
    board_position_build_config(&config);
    position_initialized = false;
    position_dma_word = 0U;

    configure_pins_and_spi(&config);
    configure_sensor_registers(&config);
    configure_dma(&config);

    CM_INTC->INTSEL1 = config.dma_irq_source;
    NVIC_ClearPendingIRQ(INT001_IRQn);
    NVIC_SetPriority(INT001_IRQn, 1U);
    position_initialized = true;
    NVIC_EnableIRQ(INT001_IRQn);
    return true;
}

bool board_position_take_sample(uint16_t *dma_word)
{
    if (!position_initialized || (dma_word == NULL) ||
        ((CM_DMA2->INTSTAT1 & POSITION_DMA_TC_FLAG) == 0U)) {
        return false;
    }
    *dma_word = position_dma_word;
    return true;
}

#if defined(DAMIAO_DM8009)
bool board_position_sample_now(uint16_t *spi_word)
{
    if (!position_initialized || (spi_word == NULL)) {
        return false;
    }

    const uint16_t word = spi_transfer(0U);
    position_dma_word = word;
    *spi_word = word;
    return true;
}
#endif

void board_position_handle_timer_interrupt(void)
{
    if (!position_initialized) {
        return;
    }

    if ((CM_TMR4_1->CCSR & TMR4_CCSR_IRQZF) != 0U) {
        /* The recovered sensor protocol obtains a new angle with a zero-word
         * transfer at each TMR4 counter-zero event. */
        CM_SPI3->DR = 0U;
    }
    CM_TMR4_1->CCSR &= (uint16_t)~(TMR4_CCSR_IRQPF | TMR4_CCSR_IRQZF);
    NVIC_ClearPendingIRQ(INT000_IRQn);
    __DSB();
}

void board_position_ack_dma_interrupt(void)
{
    if (!position_initialized) {
        return;
    }
    /* A one-word block completes with CNT=0. The original IRQ reloads CNT
     * and re-enables channel 0 before clearing TC, ready for the next TMR4
     * zero event. */
    CM_DMA2->DTCTL0 = (CM_DMA2->DTCTL0 & 0x0000FFFFUL) | 0x00010000UL;
    CM_DMA2->CHEN = POSITION_DMA_CHANNEL;
    CM_DMA2->INTCLR1 = POSITION_DMA_CLEAR_ALL;
    NVIC_ClearPendingIRQ(INT001_IRQn);
    __DSB();
}
