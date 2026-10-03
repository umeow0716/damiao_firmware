#include "board_position.h"

#include "memory_layout.h"

#include <stddef.h>
#include <stdint.h>

#include "board_delay.h"
#include "hc32f448.h"

#define POSITION_DMA_CLEAR_ALL (0x000F000FUL)
#define POSITION_DMA_CHANNEL (DMA_CHEN_CHEN_0)
#define POSITION_DMA_TC_FLAG (DMA_INTSTAT1_TC_0)
#define POSITION_SWD_PIN_MASK ((uint16_t)0x0003U)

#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
static uint8_t position_sensor_expected[BOARD_POSITION_SENSOR_REGISTER_COUNT]
    __attribute__((used, section(".position_sensor_expected"), aligned(1)));
static volatile uint16_t position_dma_storage
    __attribute__((used, section(".position_dma_word"), aligned(2)));
#define position_dma_word position_dma_storage
__asm(".global position_dma_word\n"
      ".type position_dma_word, %object\n"
      ".set position_dma_word, position_dma_storage\n"
      ".size position_dma_word, 2\n");
#else
/* The last expected byte is also the low byte of the DMA halfword. */
static union
{
    uint8_t expected[BOARD_POSITION_SENSOR_REGISTER_COUNT];
    struct
    {
        uint8_t reserved[10];
        volatile uint16_t word;
    } dma;
} position_sensor_storage __attribute__((used, section(".position_sensor_expected"), aligned(2)));
#define position_dma_word position_sensor_storage.dma.word
#define position_sensor_expected position_sensor_storage.expected
__asm(".global position_sensor_expected\n"
      ".type position_sensor_expected, %object\n"
      ".set position_sensor_expected, position_sensor_storage\n"
      ".size position_sensor_expected, 11\n"
      ".global position_dma_word\n"
      ".type position_dma_word, %object\n"
      ".set position_dma_word, position_sensor_storage + 10\n"
      ".size position_dma_word, 2\n");
#endif
static bool position_initialized;
static uint8_t position_sensor_observed[BOARD_POSITION_SENSOR_REGISTER_COUNT]
    __attribute__((section(".position_sensor_observed"), aligned(1)));
const uint8_t position_sensor_expected_image[BOARD_POSITION_SENSOR_REGISTER_COUNT] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0xC0U, 0xFFU, 0x1CU, 0x00U, 0x77U, 0x9CU,
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
    0x00U,
#else
    0x0EU,
#endif
};

static uint16_t spi_transfer(uint16_t transmit)
{
    while ((CM_SPI3->SR & SPI_SR_TDEF) == 0U)
    {
    }
    CM_SPI3->DR = transmit;

    while ((CM_SPI3->SR & SPI_SR_RDFF) == 0U)
    {
    }
    return (uint16_t)CM_SPI3->DR;
}

static void configure_pins_and_spi(const BoardPositionRegisterImage *config)
{
    CM_PWC->FCG1 &= ~PWC_FCG1_SPI3;

    /* Board routing: PB4=MISO, PB5=SS, PB8=MOSI, PB9=SCK. Masking PSPCR to
     * SWCLK/SWDIO preserves the loader's state while disabling unused JTAG/SWO
     * pins. */
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

static void configure_sensor_registers(const BoardPositionRegisterImage *config)
{
    volatile uint8_t *const observed = position_sensor_observed;
    volatile const uint8_t *const expected = position_sensor_expected;

    for (uint32_t i = 0U; i < BOARD_POSITION_SENSOR_REGISTER_COUNT; ++i)
    {
        const uint16_t read_command =
            (uint16_t)(0x4000U | ((uint16_t)config->sensor_register[i] << 8U));
        spi_transfer(read_command);
        board_delay_ms(1U);
        observed[i] = (uint8_t)(spi_transfer(0U) >> 8U);
        board_delay_ms(25U);
    }

    for (uint32_t i = 0U; i < BOARD_POSITION_SENSOR_WRITABLE_COUNT; ++i)
    {
        /* This path reads observed first, captures expected once
         * and retains it for the write command on mismatch. */
        const uint8_t observed_value = observed[i];
        const uint8_t expected_value = expected[i];
        if (observed_value == expected_value)
        {
            continue;
        }
        const uint16_t write_command =
            (uint16_t)(0x8000U | ((uint16_t)config->sensor_register[i] << 8U) | expected_value);
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

    configure_pins_and_spi(&config);
    configure_sensor_registers(&config);
    configure_dma(&config);

    CM_INTC->INTSEL1 = config.dma_irq_source;
    /* This path sets IRQ1 priority before clearing pending. */
    NVIC_SetPriority(INT001_IRQn, 1U);
    NVIC_ClearPendingIRQ(INT001_IRQn);
    position_initialized = true;
    NVIC_EnableIRQ(INT001_IRQn);
    return true;
}

bool board_position_dma_sample_pending(void)
{
    const volatile uint32_t *const status = (const volatile uint32_t *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B4CUL, 0x1FFF850CUL);
    return (*status & POSITION_DMA_TC_FLAG) != 0U;
}

bool board_position_take_sample(uint16_t *dma_word)
{
    if ((dma_word == NULL) || !board_position_dma_sample_pending())
    {
        return false;
    }
    *dma_word = position_dma_word;
    return true;
}

void board_position_handle_timer_interrupt(void)
{
    CM_TMR4_TypeDef *const timer = (CM_TMR4_TypeDef *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B40UL, 0x1FFF8500UL);

    if ((timer->CCSR & TMR4_CCSR_IRQZF) != 0U)
    {
        /* The fixed-layout sensor protocol obtains a new angle with a zero-word
         * transfer at each TMR4 counter-zero event. */
        volatile uint32_t *const transmit = (volatile uint32_t *)(uintptr_t)*(
            const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B44UL, 0x1FFF8504UL);
        *transmit = 0U;
    }
    timer->CCSR &= (uint16_t)~(TMR4_CCSR_IRQPF | TMR4_CCSR_IRQZF);
    NVIC_ClearPendingIRQ(INT000_IRQn);
}

void board_position_ack_dma_interrupt(bool sample_ready, volatile uint32_t *dma_count)
{
    /* A one-word block completes with CNT=0.  Reload CNT
     * and re-enables channel 0 before clearing TC, ready for the next TMR4
     * zero event. */
    if (sample_ready)
    {
        *dma_count = (*dma_count & 0x0000FFFFUL) | 0x00010000UL;
        volatile uint32_t *const enable = (volatile uint32_t *)(uintptr_t)*(
            const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B78UL, 0x1FFF8538UL);
        *enable = POSITION_DMA_CHANNEL;
    }
    volatile uint32_t *const clear = (volatile uint32_t *)(uintptr_t)*(
        const volatile uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF8B7CUL, 0x1FFF853CUL);
    *clear = POSITION_DMA_CLEAR_ALL;
    NVIC_ClearPendingIRQ(INT001_IRQn);
}

void board_position_clear_timer_event_after_flash(void)
{
    /* main clears both TMR4 position-event flags and the
     * pending IRQ000 bit after flash programming resumes normal sampling. */
    CM_TMR4_1->CCSR &= (uint16_t)~(TMR4_CCSR_IRQPF | TMR4_CCSR_IRQZF);
    NVIC_ClearPendingIRQ(INT000_IRQn);
}
