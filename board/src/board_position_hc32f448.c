#include "board_position.h"

#include "factory_layout.h"

#include <stddef.h>
#include <stdint.h>

#include "board_delay.h"
#include "hc32f448.h"

#define POSITION_DMA_CLEAR_ALL     (0x000F000FUL)
#define POSITION_DMA_CHANNEL       (DMA_CHEN_CHEN_0)
#define POSITION_DMA_TC_FLAG       (DMA_INTSTAT1_TC_0)
#define POSITION_SWD_PIN_MASK      ((uint16_t)0x0003U)

#if defined(DAMIAO_DM8009_V3)
static uint8_t position_sensor_expected[BOARD_POSITION_SENSOR_REGISTER_COUNT]
    __attribute__((used, section(".dm4310_position_sensor_expected"), aligned(1)));
static volatile uint16_t position_dma_storage
    __attribute__((used, section(".dm4310_position_dma_word"), aligned(2)));
#define position_dma_word position_dma_storage
__asm (".global position_dma_word\n"
       ".type position_dma_word, %object\n"
       ".set position_dma_word, position_dma_storage\n"
       ".size position_dma_word, 2\n");
#elif defined(DAMIAO_DM4310)
/* The last expected byte is also the low byte of the DMA halfword. */
static union {
    uint8_t expected[BOARD_POSITION_SENSOR_REGISTER_COUNT];
    struct {
        uint8_t reserved[10];
        volatile uint16_t word;
    } dma;
} position_sensor_storage
    __attribute__((used, section(".dm4310_position_sensor_expected"), aligned(2)));
#define position_dma_word position_sensor_storage.dma.word
#define position_sensor_expected position_sensor_storage.expected
__asm (".global position_sensor_expected\n"
       ".type position_sensor_expected, %object\n"
       ".set position_sensor_expected, position_sensor_storage\n"
       ".size position_sensor_expected, 11\n"
       ".global position_dma_word\n"
       ".type position_dma_word, %object\n"
       ".set position_dma_word, position_sensor_storage + 10\n"
       ".size position_dma_word, 2\n");
#else
static volatile uint16_t position_dma_word __attribute__((aligned(4)));
#endif
static bool position_initialized;
#if defined(DAMIAO_DM4310)
static uint8_t position_sensor_observed[BOARD_POSITION_SENSOR_REGISTER_COUNT]
    __attribute__((section(".dm4310_position_sensor_observed"), aligned(1)));
const uint8_t
    dm4310_position_sensor_expected_image[BOARD_POSITION_SENSOR_REGISTER_COUNT] = {
        0x00U, 0x00U, 0x00U, 0x00U, 0xC0U, 0xFFU,
        0x1CU, 0x00U, 0x77U, 0x9CU,
#if defined(DAMIAO_DM8009_V3)
        0x00U,
#else
        0x0EU,
#endif
    };
#endif

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
#if defined(DAMIAO_DM4310)
    volatile uint8_t *const observed = position_sensor_observed;
    volatile const uint8_t *const expected = position_sensor_expected;
#else
    uint8_t observed[BOARD_POSITION_SENSOR_REGISTER_COUNT];
    const uint8_t *const expected = config->sensor_value;
#endif

    for (uint32_t i = 0U; i < BOARD_POSITION_SENSOR_REGISTER_COUNT; ++i) {
        const uint16_t read_command = (uint16_t)(0x4000U |
            ((uint16_t)config->sensor_register[i] << 8U));
        spi_transfer(read_command);
        board_delay_ms(1U);
        observed[i] = (uint8_t)(spi_transfer(0U) >> 8U);
        board_delay_ms(25U);
    }

    for (uint32_t i = 0U; i < BOARD_POSITION_SENSOR_WRITABLE_COUNT; ++i) {
        /* Factory 0x22c5a..68 reads observed first, captures expected once
         * and retains it for the write command on mismatch. */
        const uint8_t observed_value = observed[i];
        const uint8_t expected_value = expected[i];
        if (observed_value == expected_value) {
            continue;
        }
        const uint16_t write_command = (uint16_t)(0x8000U |
            ((uint16_t)config->sensor_register[i] << 8U) |
            expected_value);
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
#if !defined(DAMIAO_DM4310)
    position_dma_word = 0U;
#endif

    configure_pins_and_spi(&config);
    configure_sensor_registers(&config);
    configure_dma(&config);

    CM_INTC->INTSEL1 = config.dma_irq_source;
#if defined(DAMIAO_DM4310)
    /* Factory 0x23138..4a sets IRQ1 priority before clearing pending. */
    NVIC_SetPriority(INT001_IRQn, 1U);
    NVIC_ClearPendingIRQ(INT001_IRQn);
#else
    NVIC_ClearPendingIRQ(INT001_IRQn);
    NVIC_SetPriority(INT001_IRQn, 1U);
#endif
    position_initialized = true;
    NVIC_EnableIRQ(INT001_IRQn);
    return true;
}

#if defined(DAMIAO_DM4310)
bool board_position_dma_sample_pending(void)
{
    const volatile uint32_t *const status = (const volatile uint32_t *)(uintptr_t)
        *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8B4CUL, 0x1FFF850CUL);
    return (*status & POSITION_DMA_TC_FLAG) != 0U;
}
#endif

bool board_position_take_sample(uint16_t *dma_word)
{
#if defined(DAMIAO_DM4310)
    if ((dma_word == NULL) || !board_position_dma_sample_pending()) {
        return false;
    }
#else
    if (
        !position_initialized ||
        (dma_word == NULL) ||
        ((CM_DMA2->INTSTAT1 & POSITION_DMA_TC_FLAG) == 0U)) {
        return false;
    }
#endif
    *dma_word = position_dma_word;
    return true;
}

void board_position_handle_timer_interrupt(void)
{
#if defined(DAMIAO_DM4310)
    CM_TMR4_TypeDef *const timer = (CM_TMR4_TypeDef *)(uintptr_t)
        *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8B40UL, 0x1FFF8500UL);
#else
    if (!position_initialized) {
        return;
    }
    CM_TMR4_TypeDef *const timer = CM_TMR4_1;
#endif

    if ((timer->CCSR & TMR4_CCSR_IRQZF) != 0U) {
        /* The recovered sensor protocol obtains a new angle with a zero-word
         * transfer at each TMR4 counter-zero event. */
#if defined(DAMIAO_DM4310)
        volatile uint32_t *const transmit = (volatile uint32_t *)(uintptr_t)
            *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8B44UL, 0x1FFF8504UL);
        *transmit = 0U;
#else
        CM_SPI3->DR = 0U;
#endif
    }
    timer->CCSR &= (uint16_t)~(TMR4_CCSR_IRQPF | TMR4_CCSR_IRQZF);
    NVIC_ClearPendingIRQ(INT000_IRQn);
#if !defined(DAMIAO_DM4310)
    __DSB();
#endif
}

void board_position_ack_dma_interrupt(bool sample_ready, volatile uint32_t *dma_count)
{
#if !defined(DAMIAO_DM4310)
    (void)sample_ready;
    (void)dma_count;
    if (!position_initialized) {
        return;
    }
#endif
    /* A one-word block completes with CNT=0. The original IRQ reloads CNT
     * and re-enables channel 0 before clearing TC, ready for the next TMR4
     * zero event. */
#if defined(DAMIAO_DM4310)
    if (sample_ready) {
#endif
#if defined(DAMIAO_DM4310)
        *dma_count = (*dma_count & 0x0000FFFFUL) | 0x00010000UL;
        volatile uint32_t *const enable = (volatile uint32_t *)(uintptr_t)
            *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8B78UL, 0x1FFF8538UL);
        *enable = POSITION_DMA_CHANNEL;
#else
        CM_DMA2->DTCTL0 = (CM_DMA2->DTCTL0 & 0x0000FFFFUL) | 0x00010000UL;
        CM_DMA2->CHEN = POSITION_DMA_CHANNEL;
#endif
#if defined(DAMIAO_DM4310)
    }
#endif
#if defined(DAMIAO_DM4310)
    volatile uint32_t *const clear = (volatile uint32_t *)(uintptr_t)
        *(const volatile uint32_t *)FACTORY_SRAM_ADDRESS(0x1FFF8B7CUL, 0x1FFF853CUL);
    *clear = POSITION_DMA_CLEAR_ALL;
#else
    CM_DMA2->INTCLR1 = POSITION_DMA_CLEAR_ALL;
#endif
    NVIC_ClearPendingIRQ(INT001_IRQn);
#if !defined(DAMIAO_DM4310)
    __DSB();
#endif
}

#if defined(DAMIAO_DM4310)
void board_position_clear_timer_event_after_flash(void)
{
    /* main@0x25510..0x25520 clears both TMR4 position-event flags and the
     * pending IRQ000 bit after flash programming resumes normal sampling. */
    CM_TMR4_1->CCSR &=
        (uint16_t)~(TMR4_CCSR_IRQPF | TMR4_CCSR_IRQZF);
    NVIC_ClearPendingIRQ(INT000_IRQn);
}
#endif
