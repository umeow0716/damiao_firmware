#include "board_uart.h"

void board_uart_build_config(BoardUartRegisterImage *config)
{
    if (config == NULL) {
        return;
    }

    /* These values are independently present in the application code and in
     * the captured USART1/TMR0/AOS register images. */
#if defined(DAMIAO_DM4310)
    /* Factory 0x2360a arithmetic for baud=921600; live init recalculates it. */
    config->usart_brr = 0x00000562UL;
#else
    config->usart_brr = 0x000005E2UL;
#endif
    config->usart_cr1_dma = 0xA000000FUL;
    config->usart_cr1_polled = 0xA000000CUL;
    config->usart_cr2 = 0x00000800UL;
    config->tmr0_bconr = 0x00005530UL;
    config->tmr0_compare = 100UL;
    config->dma_dtctl0 = 0x00C80001UL;
    config->dma_chctl0 = 0x00000004UL;
    config->dma_trigger = 322UL;
    config->irq_source = 324UL;
    /* debug_usart1_dma_init changes only PFS.  The 0x0100 observed in PCR is
     * the live PIN level rather than a writable configuration bit. */
    config->tx_pin_control = 0x0000U;
    config->rx_pin_control = 0x0000U;
    config->tx_pin_function = 0x0020U;
    config->rx_pin_function = 0x0021U;
}
