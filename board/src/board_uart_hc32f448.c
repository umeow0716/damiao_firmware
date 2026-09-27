#include "board_uart.h"

#include "hc32f448.h"

#define UART_DMA_CHANNEL          (DMA_CHEN_CHEN_0)
#define UART_DMA_CLEAR_ALL        (0x000F000FUL)

static bool uart_initialized;
static bool uart_dma_receive;
static volatile uint8_t uart_rx_dma_buffer[BOARD_UART_RX_DMA_CAPACITY]
    __attribute__((aligned(4)));
static uint16_t uart_rx_length;
static uint16_t uart_rx_cursor;

static void configure_receive_timeout(
    const BoardUartRegisterImage *config)
{
    CM_PWC->FCG2 &= ~PWC_FCG2_TMR0_1;
    CM_TMR0_1->BCONR = 0U;
    CM_TMR0_1->CNTAR = 0U;
    CM_TMR0_1->CMPAR = config->tmr0_compare;
    CM_TMR0_1->STFLR = 0U;
    CM_TMR0_1->BCONR = config->tmr0_bconr;
}

static void configure_receive_dma(const BoardUartRegisterImage *config)
{
    CM_PWC->FCG0PC = 0xA5A50001UL;
    CM_PWC->FCG0 &= ~(PWC_FCG0_DMA1 | PWC_FCG0_AOS);
    CM_PWC->FCG0PC = 0xA5A50000UL;

    CM_DMA1->EN = DMA_EN_EN;
    CM_DMA1->CHENCLR = UART_DMA_CHANNEL;
    CM_DMA1->INTCLR0 = UART_DMA_CLEAR_ALL;
    CM_DMA1->INTCLR1 = UART_DMA_CLEAR_ALL;
    CM_DMA1->SAR0 = (uint32_t)(uintptr_t)&CM_USART1->RDR;
    CM_DMA1->DAR0 = (uint32_t)(uintptr_t)uart_rx_dma_buffer;
    CM_DMA1->DTCTL0 = config->dma_dtctl0;
    CM_DMA1->RPT0 = 0U;
    CM_DMA1->SNSEQCTL0 = 0U;
    CM_DMA1->DNSEQCTL0 = 0U;
    CM_DMA1->LLP0 = 0U;
    CM_DMA1->CHCTL0 = config->dma_chctl0;
    /* USART1 receiver timeout owns IRQ4; DMA completion/error interrupts are
     * deliberately masked, as in the recovered design. */
    CM_DMA1->INTMASK0 |= DMA_INTMASK0_MSKTRNERR_0 |
        DMA_INTMASK0_MSKREQERR_0;
    CM_DMA1->INTMASK1 |= DMA_INTMASK1_MSKTC_0 |
        DMA_INTMASK1_MSKBTC_0;
    CM_AOS->DMA1_TRGSEL0 = config->dma_trigger;
    CM_DMA1->CHEN = UART_DMA_CHANNEL;
}

static void rearm_receive_dma(const BoardUartRegisterImage *config)
{
    uart_rx_length = 0U;
    uart_rx_cursor = 0U;
    CM_DMA1->CHENCLR = UART_DMA_CHANNEL;
    CM_DMA1->DAR0 = (uint32_t)(uintptr_t)uart_rx_dma_buffer;
    CM_DMA1->DTCTL0 = config->dma_dtctl0;
    CM_DMA1->INTCLR0 = UART_DMA_CLEAR_ALL;
    CM_DMA1->INTCLR1 = UART_DMA_CLEAR_ALL;
    CM_DMA1->CHEN = UART_DMA_CHANNEL;
}

static void snapshot_receive_dma(void)
{
    if ((uart_rx_length != 0U) ||
        ((CM_USART1->SR & USART_SR_RTOF) == 0U)) {
        return;
    }

    CM_DMA1->CHENCLR = UART_DMA_CHANNEL;
    uint32_t remaining =
        (CM_DMA1->MONDTCTL0 & DMA_MONDTCTL_CNT) >> DMA_MONDTCTL_CNT_POS;
    if (remaining > BOARD_UART_RX_DMA_CAPACITY) {
        remaining = BOARD_UART_RX_DMA_CAPACITY;
    }
    uart_rx_cursor = 0U;
    uart_rx_length = (uint16_t)(BOARD_UART_RX_DMA_CAPACITY - remaining);
}

static bool init_uart(bool receive_interrupts)
{
    BoardUartRegisterImage config;
    board_uart_build_config(&config);
    uart_initialized = false;
    uart_dma_receive = false;
    uart_rx_length = 0U;
    uart_rx_cursor = 0U;

    /* Recovered PCB routing: PA11=USART1_TX (function 0x20),
     * PA12=USART1_RX (function 0x21). */
    CM_PWC->FCG3 &= ~PWC_FCG3_USART1;
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PFSRA11 = config.tx_pin_function;
    CM_GPIO->PFSRA12 = config.rx_pin_function;
    CM_GPIO->PWPR = 0xA500U;

    /* 921600 8N1 at the recovered 50 MHz USART peripheral clock. FBME is
     * needed for the fractional part encoded in BRR=0x05E2. */
    CM_USART1->CR1 = config.usart_cr1_polled &
        ~(USART_CR1_RE | USART_CR1_TE);
    CM_USART1->CR2 = config.usart_cr2;
    CM_USART1->CR3 = 0U;
    CM_USART1->PR = 0U;
    CM_USART1->BRR = config.usart_brr;

    if (receive_interrupts) {
        /* The application receives a variable-length burst through DMA1 and
         * handles it at the recovered USART receiver-timeout boundary. */
        configure_receive_timeout(&config);
        configure_receive_dma(&config);
        /* The factory routine first enables receiver-timeout detection, then
         * installs/enables IRQ004, and only afterwards starts RX and TX. */
        CM_USART1->CR1 |= USART_CR1_RTOE | USART_CR1_RTOIE;
        CM_INTC->INTSEL4 = config.irq_source;
        NVIC_ClearPendingIRQ(INT004_IRQn);
        NVIC_SetPriority(INT004_IRQn, 4U);
        uart_dma_receive = true;
    } else {
        NVIC_DisableIRQ(INT004_IRQn);
        NVIC_ClearPendingIRQ(INT004_IRQn);
        CM_USART1->CR1 = config.usart_cr1_polled;
    }
    uart_initialized = true;
    if (receive_interrupts) {
        NVIC_EnableIRQ(INT004_IRQn);
        CM_USART1->CR1 |= USART_CR1_RE | USART_CR1_TE;
    }
    return true;
}

bool board_uart_init(void)
{
    return init_uart(true);
}

bool board_uart_init_polled(void)
{
    return init_uart(false);
}

bool board_uart_receive(uint8_t *byte)
{
    if (!uart_initialized || (byte == NULL)) {
        return false;
    }

    if (uart_dma_receive) {
        snapshot_receive_dma();
        if (uart_rx_cursor >= uart_rx_length) {
            return false;
        }
        *byte = uart_rx_dma_buffer[uart_rx_cursor];
        ++uart_rx_cursor;
        return true;
    }

    if ((CM_USART1->SR & USART_SR_RXNE) == 0U) {
        return false;
    }
    *byte = (uint8_t)CM_USART1->RDR;
    return true;
}

size_t board_uart_write(const void *data, size_t length)
{
    if (!uart_initialized || ((data == NULL) && (length != 0U))) {
        return 0U;
    }

    const uint8_t *bytes = (const uint8_t *)data;
    size_t written = 0U;
    while (written < length) {
        while ((CM_USART1->SR & USART_SR_TXE) == 0U) {
            /* debug_uart_write@0x2379c waits without a timeout. */
        }
        CM_USART1->TDR = bytes[written];
        ++written;
    }
    return written;
}

bool board_uart_flush(void)
{
    if (!uart_initialized) {
        return false;
    }
    while ((CM_USART1->SR & USART_SR_TC) == 0U) {
        /* Keep the same blocking completion semantics as factory TX. */
    }
    return true;
}

void board_uart_ack_interrupt(void)
{
    if (!uart_initialized) {
        return;
    }

    /* IRQ004 is sourced only by receiver timeout.  The original epilogue
     * stops TMR0 and clears PE/FE/ORE/RTOF together with 0x001b0000. */
    CM_TMR0_1->BCONR &= ~TMR0_BCONR_CSTA;
    CM_USART1->CR1 |= USART_CR1_CPE | USART_CR1_CFE |
                      USART_CR1_CORE | USART_CR1_CRTOF;
    if (uart_dma_receive) {
        BoardUartRegisterImage config;
        board_uart_build_config(&config);
        rearm_receive_dma(&config);
    }
    NVIC_ClearPendingIRQ(INT004_IRQn);
    __DSB();
}
