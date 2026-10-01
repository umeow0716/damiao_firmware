#include "board_uart.h"

#include "hc32f448.h"
#if defined(DAMIAO_DM4310)
#include <string.h>
#include "runtime_compat.h"
#endif

#define UART_DMA_CHANNEL          (DMA_CHEN_CHEN_0)
#define UART_DMA_CLEAR_ALL        (0x000F000FUL)

static bool uart_initialized;
static bool uart_dma_receive;
#if defined(DAMIAO_DM4310)
static volatile uint8_t uart_rx_dma_buffer[BOARD_UART_RX_DMA_CAPACITY]
    __attribute__((section(".dm4310_uart_rx_buffer"), aligned(1)));
#else
static volatile uint8_t uart_rx_dma_buffer[BOARD_UART_RX_DMA_CAPACITY]
    __attribute__((aligned(4)));
#endif
#if defined(DAMIAO_DM4310)
typedef struct {
    uint16_t expected_payload_length;
    uint16_t received_length;
} Dm4310UartLengthState;

static volatile Dm4310UartLengthState uart_length_state
    __attribute__((section(".dm4310_uart_length_state")));
#endif
static uint16_t uart_rx_length;
static uint16_t uart_rx_cursor;

#if defined(DAMIAO_DM4310)
void board_uart_initialize_state(void)
{
    /* Factory scatter initialization restores this pair before main. UART
     * peripheral reinitialization must preserve both live halfwords. */
    uart_length_state.expected_payload_length = 63U;
    uart_length_state.received_length = 0U;
}

uint16_t board_uart_expected_payload_length(void)
{
    return uart_length_state.expected_payload_length;
}

uint16_t board_uart_received_length(void)
{
    return uart_length_state.received_length;
}

__attribute__((noipa))
static void configure_baud(uint32_t baud)
{
    /* Factory 0x2360a..0x236d4. Keep each float operation separate, including
     * the VCMPE whose status is visible in FPSCR even on rejected divisors. */
    const uint32_t mode = (CM_USART1->CR1 >> 15U) & 1U;
    const float denominator = ((float)baud * 8.0f) * (2.0f - (float)mode);
    const float divisor = 100000000.0f / denominator - 1.0f;
    uint32_t integer;
    uint32_t status;
    __asm volatile ("vcvt.u32.f32 s1, %2\n"
                    "vmov %0, s1\n"
                    "vcmpe.f32 %2, #0.0\n"
                    "vmrs %1, fpscr"
                    : "=r" (integer), "=r" (status) : "t" (divisor) : "s1");
    if ((((status >> 31U) ^ (status >> 28U)) & 1U) != 0U || integer > 255U) {
        return;
    }
    const float fraction = divisor - (float)integer;
    uint32_t fraction_bits;
    memcpy(&fraction_bits, &fraction, sizeof(fraction_bits));
    if ((int32_t)fraction_bits <= INT32_C(0x3727c5ac)) {
        return;
    }
    const uint64_t product = (uint64_t)(2U - mode) * (integer + 1U) * baud;
    const float scaled = dm4310_runtime_uint64_to_float(product << 11U);
    const uint32_t control = CM_USART1->CR1;
    const float correction = scaled / 100000000.0f - 128.0f;
    uint32_t fractional;
    __asm volatile ("vcvt.u32.f32 s0, %1\n"
                    "vmov %0, s0"
                    : "=r" (fractional) : "t" (correction) : "s0");
    CM_USART1->CR1 = control | (fractional <= 127U ? USART_CR1_FBME : 0U);
    CM_USART1->BRR = fractional | (integer << 8U);
}
#endif

static void configure_receive_timeout(
    const BoardUartRegisterImage *config)
{
    CM_PWC->FCG2 &= ~PWC_FCG2_TMR0_1;
#if defined(DAMIAO_DM4310)
    /* debug_uart_timeout_init@0x23574 writes exactly these four words. */
    CM_TMR0_1->CNTAR = 0U;
    CM_TMR0_1->BCONR = config->tmr0_bconr;
    CM_TMR0_1->CMPAR = config->tmr0_compare;
    CM_TMR0_1->STFLR = 0U;
#else
    CM_TMR0_1->BCONR = 0U;
    CM_TMR0_1->CNTAR = 0U;
    CM_TMR0_1->CMPAR = config->tmr0_compare;
    CM_TMR0_1->STFLR = 0U;
    CM_TMR0_1->BCONR = config->tmr0_bconr;
#endif
}

static void configure_receive_dma(const BoardUartRegisterImage *config)
{
    CM_PWC->FCG0PC = 0xA5A50001UL;
#if defined(DAMIAO_DM4310)
    CM_PWC->FCG0 &= ~PWC_FCG0_DMA1;
#else
    CM_PWC->FCG0 &= ~(PWC_FCG0_DMA1 | PWC_FCG0_AOS);
#endif
    CM_PWC->FCG0PC = 0xA5A50000UL;

    CM_DMA1->EN = DMA_EN_EN;
#if defined(DAMIAO_DM4310)
    CM_DMA1->CHCTL0 &= ~0x00001000UL;
    CM_DMA1->SAR0 = (uint32_t)(uintptr_t)&CM_USART1->RDR;
    CM_DMA1->DAR0 = (uint32_t)(uintptr_t)uart_rx_dma_buffer;
    CM_DMA1->DTCTL0 = config->dma_dtctl0;
    CM_DMA1->CHCTL0 = config->dma_chctl0;
    CM_DMA1->CHEN = UART_DMA_CHANNEL;
    CM_DMA1->INTCLR0 = UART_DMA_CLEAR_ALL;
    CM_DMA1->INTCLR1 = UART_DMA_CLEAR_ALL;

    CM_PWC->FCG0PC = 0xA5A50001UL;
    CM_PWC->FCG0 &= ~PWC_FCG0_AOS;
    CM_AOS->DMA1_TRGSEL0 = config->dma_trigger;
    CM_PWC->FCG0PC = 0xA5A50000UL;
#else
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
#endif
}

#if !defined(DAMIAO_DM4310)
static void rearm_receive_dma(const BoardUartRegisterImage *config)
{
    uart_rx_length = 0U;
    uart_rx_cursor = 0U;
#if !defined(DAMIAO_DM4310)
    CM_DMA1->CHENCLR = UART_DMA_CHANNEL;
    CM_DMA1->DAR0 = (uint32_t)(uintptr_t)uart_rx_dma_buffer;
    CM_DMA1->DTCTL0 = config->dma_dtctl0;
    CM_DMA1->INTCLR0 = UART_DMA_CLEAR_ALL;
    CM_DMA1->INTCLR1 = UART_DMA_CLEAR_ALL;
#else
    (void)config;
#endif
    CM_DMA1->CHEN = UART_DMA_CHANNEL;
}
#endif

#if defined(DAMIAO_DM4310)
bool board_uart_begin_receive_irq(const uint8_t **data, int16_t *length)
{
    if ((CM_USART1->SR & USART_SR_RTOF) == 0U) {
        return false;
    }

    /* Vector_20@0x2225c snapshots the live DMA count without consulting any
     * source-only cursor state.  Keep the signed halfword result: malformed
     * hardware counts are observable to the command gates in the factory. */
    CM_DMA1->CHENCLR |= UART_DMA_CHANNEL;
    const uint32_t remaining =
        CM_DMA1->MONDTCTL0 >> DMA_MONDTCTL_CNT_POS;
    const int16_t received = (int16_t)(BOARD_UART_RX_DMA_CAPACITY - remaining);
    uart_length_state.received_length = (uint16_t)received;

    /* Reload count and destination before parsing, preserving the low
     * control halfword exactly as the factory UXTH/ORR sequence does. */
    CM_DMA1->DTCTL0 = (CM_DMA1->DTCTL0 & UINT32_C(0x0000FFFF)) |
                      (BOARD_UART_RX_DMA_CAPACITY << DMA_MONDTCTL_CNT_POS);
    CM_DMA1->DAR0 = (uint32_t)(uintptr_t)uart_rx_dma_buffer;

    *data = (const uint8_t *)(uintptr_t)uart_rx_dma_buffer;
    *length = received;
    return true;
}

void board_uart_rearm_receive_irq(void)
{
    /* Vector_20 common parser epilogue, before timer/USART/NVIC ack. */
    CM_DMA1->CHEN = UART_DMA_CHANNEL;
}
#endif

static void snapshot_receive_dma(void)
{
    if ((uart_rx_length != 0U) ||
        ((CM_USART1->SR & USART_SR_RTOF) == 0U)) {
        return;
    }

    CM_DMA1->CHENCLR = UART_DMA_CHANNEL;
    uint32_t remaining =
        (CM_DMA1->MONDTCTL0 & DMA_MONDTCTL_CNT) >> DMA_MONDTCTL_CNT_POS;
#if defined(DAMIAO_DM4310)
    uart_length_state.received_length =
        (uint16_t)(BOARD_UART_RX_DMA_CAPACITY - remaining);
#endif
    if (remaining > BOARD_UART_RX_DMA_CAPACITY) {
        remaining = BOARD_UART_RX_DMA_CAPACITY;
    }
    uart_rx_cursor = 0U;
    uart_rx_length = (uint16_t)(BOARD_UART_RX_DMA_CAPACITY - remaining);
#if defined(DAMIAO_DM4310)
    /* Vector_20 reloads the 200-byte count and destination before entering
     * the command parser, then reenables the channel in its common epilogue. */
    BoardUartRegisterImage config;
    board_uart_build_config(&config);
    CM_DMA1->DTCTL0 = config.dma_dtctl0;
    CM_DMA1->DAR0 = (uint32_t)(uintptr_t)uart_rx_dma_buffer;
#endif
}

static bool init_uart(bool receive_interrupts)
{
    BoardUartRegisterImage config;
    board_uart_build_config(&config);
#if !defined(DAMIAO_DM4310)
    /* The generic byte-stream facade owns these fields.  Factory DM4310
     * initializes its DMA transport without touching source-only cursor or
     * readiness state. */
    uart_initialized = false;
    uart_dma_receive = false;
    uart_rx_length = 0U;
    uart_rx_cursor = 0U;
#endif
    /* debug_usart1_dma_init@0x2359c configures timeout channel A before it
     * enables USART1 or touches the UART pin mux. */
    if (receive_interrupts) {
#if defined(DAMIAO_DM4310)
        configure_receive_timeout(&config);
#endif
    }

    /* Recovered PCB routing: PA11=USART1_TX (function 0x20),
     * PA12=USART1_RX (function 0x21). */
    CM_PWC->FCG3 &= ~PWC_FCG3_USART1;
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PFSRA11 = config.tx_pin_function;
    CM_GPIO->PFSRA12 = config.rx_pin_function;
    CM_GPIO->PWPR = 0xA500U;

    /* 921600 8N1; DM4310 calculates the divisor as in the factory code. */
#if defined(DAMIAO_DM4310)
    /* Preserve the vendor initializer's observable reset/deinit writes before
     * applying the recovered 921600-baud register image. */
    CM_USART1->CR1 = 0x801B0000UL;
    CM_USART1->CR2 = 0U;
    CM_USART1->CR3 = 0U;
    CM_USART1->BRR = 0x0000FFFFUL;
    CM_USART1->PR = 0U;
    CM_USART1->CR3 &= ~USART_CR3_SCEN;
    CM_USART1->PR = 0U;
    CM_USART1->CR1 = 0x80000000UL;
    CM_USART1->CR2 = config.usart_cr2;
    CM_USART1->CR3 = 0U;
    configure_baud(921600U);
#else
    CM_USART1->CR1 = config.usart_cr1_polled &
        ~(USART_CR1_RE | USART_CR1_TE);
    CM_USART1->CR2 = config.usart_cr2;
    CM_USART1->CR3 = 0U;
    CM_USART1->PR = 0U;
    CM_USART1->BRR = config.usart_brr;
#endif

    if (receive_interrupts) {
        /* The application receives a variable-length burst through DMA1 and
         * handles it at the recovered USART receiver-timeout boundary. */
#if !defined(DAMIAO_DM4310)
        configure_receive_timeout(&config);
#endif
        configure_receive_dma(&config);
        /* The factory routine first enables receiver-timeout detection, then
         * installs/enables IRQ004, and only afterwards starts RX and TX. */
        CM_USART1->CR1 |= USART_CR1_RTOE | USART_CR1_RTOIE;
        CM_INTC->INTSEL4 = config.irq_source;
#if defined(DAMIAO_DM4310)
        NVIC_SetPriority(INT004_IRQn, 4U);
        NVIC_ClearPendingIRQ(INT004_IRQn);
#else
        NVIC_ClearPendingIRQ(INT004_IRQn);
        NVIC_SetPriority(INT004_IRQn, 4U);
#endif
#if !defined(DAMIAO_DM4310)
        uart_dma_receive = true;
#endif
    } else {
        NVIC_DisableIRQ(INT004_IRQn);
        NVIC_ClearPendingIRQ(INT004_IRQn);
        CM_USART1->CR1 = config.usart_cr1_polled;
    }
#if !defined(DAMIAO_DM4310)
    uart_initialized = true;
#endif
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
#if !defined(DAMIAO_DM4310)
    if (!uart_initialized || ((data == NULL) && (length != 0U))) {
        return 0U;
    }
#endif

    const uint8_t *bytes = (const uint8_t *)data;
#if defined(DAMIAO_DM4310)
    uint16_t written = 0U;
#else
    size_t written = 0U;
#endif
    while (written < length) {
        while ((CM_USART1->SR & USART_SR_TXE) == 0U) {
            /* debug_uart_write@0x2379c waits without a timeout. */
        }
        /* Factory advances the pointer even when its 16-bit count wraps. */
        CM_USART1->TDR = *bytes++;
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
#if !defined(DAMIAO_DM4310)
    if (!uart_initialized) {
        return;
    }
#endif

    /* IRQ004 is sourced only by receiver timeout.  The original epilogue
     * stops TMR0 and clears PE/FE/ORE/RTOF together with 0x001b0000. */
    CM_TMR0_1->BCONR &= ~TMR0_BCONR_CSTA;
    CM_USART1->CR1 |= USART_CR1_CPE | USART_CR1_CFE |
                      USART_CR1_CORE | USART_CR1_CRTOF;
#if !defined(DAMIAO_DM4310)
    if (uart_dma_receive) {
        BoardUartRegisterImage config;
        board_uart_build_config(&config);
        rearm_receive_dma(&config);
    }
#endif
    NVIC_ClearPendingIRQ(INT004_IRQn);
#if !defined(DAMIAO_DM4310)
    __DSB();
#endif
}
