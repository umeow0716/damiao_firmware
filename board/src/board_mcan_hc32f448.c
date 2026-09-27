#include "board_mcan.h"

#include <string.h>

#include "hc32_ll_mcan.h"
#include "system_hc32f448.h"

#define MCAN_MSG_RAM_BASE_ADDR (0x4002B000UL)
#define MCAN_MSG_RAM_WORDS     (0x100U)

static bool mcan_initialized;
static bool mcan_fd_enabled;

static void wait_for_cccr(uint32_t mask, uint32_t expected)
{
    while ((CM_MCAN1->CCCR & mask) != expected) {
        /* Both factory configuration routines wait without a timeout. */
    }
}

static void mcan_transition_delay(void)
{
    /* short_busy_wait@0x21f20 is called immediately before clearing INIT. */
    for (volatile uint32_t count = 0U; count <= 15U; ++count) {
        __NOP();
    }
}

static void configure_pins_and_clock(
    const BoardMcanRegisterImage *config)
{
    /* MCAN1 is active-low gated in FCG1. */
    CM_PWC->FCG1 &= ~PWC_FCG1_MCAN1;

    /* Original PCB routing: PB6=MCAN1_RXD (function 0x33),
     * PB7=MCAN1_TXD (function 0x32), high drive strength. */
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PCRB6 = config->rx_pin_control;
    CM_GPIO->PFSRB6 = config->rx_pin_function;
    CM_GPIO->PCRB7 = config->tx_pin_control;
    CM_GPIO->PFSRB7 = config->tx_pin_function;
    CM_GPIO->PWPR = 0xA500U;

    /* Select PLLQ. The captured PLLQ output is the 80 MHz MCAN clock used by
     * the recovered nominal/data timing constants. */
    CM_PWC->FPRC = (uint16_t)(CM_PWC->FPRC | 0xA501U);
    CM_CMU->CANCKCFGR = (uint16_t)((CM_CMU->CANCKCFGR & 0xFFF0U) | 0x0008U);
    CM_PWC->FPRC = (uint16_t)((CM_PWC->FPRC & 0xFFFDU) | 0xA500U);
}

static void enter_configuration_mode(void)
{
    CM_MCAN1->CCCR &= ~MCAN_CCCR_CSR;
    wait_for_cccr(MCAN_CCCR_CSA, 0U);

    CM_MCAN1->CCCR |= MCAN_CCCR_INIT;
    wait_for_cccr(MCAN_CCCR_INIT, MCAN_CCCR_INIT);
    CM_MCAN1->CCCR |= MCAN_CCCR_CCE;
}

static void apply_register_image(const BoardMcanRegisterImage *config)
{
    CM_MCAN1->CCCR &= ~(MCAN_CCCR_ASM | MCAN_CCCR_MON | MCAN_CCCR_DAR |
                        MCAN_CCCR_TEST | MCAN_CCCR_PXHD | MCAN_CCCR_NISO |
                        MCAN_CCCR_FDOE | MCAN_CCCR_BRSE);
    if (config->fd_enabled) {
        CM_MCAN1->CCCR |= MCAN_CCCR_FDOE | MCAN_CCCR_BRSE;
    }
    CM_MCAN1->TEST &= ~MCAN_TEST_LBCK;

    CM_MCAN1->NBTP = config->nbtp;
    CM_MCAN1->DBTP = config->dbtp;
    CM_MCAN1->TDCR = config->tdcr;
    CM_MCAN1->GFC = config->gfc;
    CM_MCAN1->SIDFC = config->sidfc;
    CM_MCAN1->XIDFC = config->xidfc;
    CM_MCAN1->RXF0C = config->rxf0c;
    CM_MCAN1->RXBC = config->rxbc;
    CM_MCAN1->RXF1C = config->rxf1c;
    CM_MCAN1->RXESC = config->rxesc;
    CM_MCAN1->TXBC = config->txbc;
    CM_MCAN1->TXESC = config->txesc;
    CM_MCAN1->TXEFC = config->txefc;

    volatile uint32_t *const message_ram =
        (volatile uint32_t *)MCAN_MSG_RAM_BASE_ADDR;
    for (uint32_t i = 0U; i < MCAN_MSG_RAM_WORDS; ++i) {
        message_ram[i] = 0U;
    }
    message_ram[0] = config->standard_filter[0];
    message_ram[1] = config->standard_filter[1];

    CM_MCAN1->IE = config->ie;
    CM_MCAN1->ILS = config->ils;
    CM_MCAN1->ILE = config->ile;
}

bool board_mcan_init(uint16_t node_id, uint8_t data_rate_selector)
{
    BoardMcanRegisterImage config;
    board_mcan_build_config(node_id, data_rate_selector, &config);
    mcan_initialized = false;
    mcan_fd_enabled = config.fd_enabled;

    configure_pins_and_clock(&config);
    enter_configuration_mode();
    apply_register_image(&config);

    mcan_transition_delay();
    CM_MCAN1->CCCR &= ~MCAN_CCCR_INIT;
    wait_for_cccr(MCAN_CCCR_INIT, 0U);

    CM_MCAN1->IR = 0xFFFFFFFFUL;
    CM_INTC->INTSEL3 = (uint32_t)INT_SRC_MCAN1_INT1;
    NVIC_ClearPendingIRQ(INT003_IRQn);
    NVIC_SetPriority(INT003_IRQn, 3U);
    mcan_initialized = true;
    NVIC_EnableIRQ(INT003_IRQn);
    return true;
}

bool board_mcan_receive(BoardMcanFrame *frame)
{
    if (!mcan_initialized || (frame == NULL)) {
        return false;
    }

    stc_mcan_rx_msg_t message;
    memset(&message, 0, sizeof(message));
    if (MCAN_GetRxMsg(CM_MCAN1, MCAN_RX_FIFO0, &message) != LL_OK) {
        return false;
    }
    if ((message.IDE != MCAN_STD_ID) || (message.RTR != 0U)) {
        return false;
    }

    frame->id = message.ID;
    frame->length = (uint8_t)message.u32DataSize;
    memcpy(frame->data, message.au8Data, frame->length);
    return true;
}

bool board_mcan_send(const BoardMcanFrame *frame)
{
    if (!mcan_initialized || (frame == NULL) || (frame->id > 0x7FFU) ||
        (frame->length > 8U)) {
        return false;
    }

    stc_mcan_tx_msg_t message;
    memset(&message, 0, sizeof(message));
    message.ID = frame->id;
    message.IDE = MCAN_STD_ID;
    message.DLC = frame->length;
    message.FDF = mcan_fd_enabled ? 1U : 0U;
    message.BRS = mcan_fd_enabled ? 1U : 0U;
    memcpy(message.au8Data, frame->data, frame->length);
    return MCAN_AddMsgToTxFifoQueue(CM_MCAN1, &message) == LL_OK;
}

void board_mcan_update_node_filter(uint16_t node_id)
{
    /* The 0x7ff parameter write changes filter 0 directly in the original
     * receive IRQ; no CCCR transition or controller restart occurs. */
    volatile uint32_t *const message_ram =
        (volatile uint32_t *)MCAN_MSG_RAM_BASE_ADDR;
    message_ram[0] = 0x880000FFUL |
                     ((uint32_t)(node_id & 0x07FFU) << 16U);
    __DSB();
}

bool board_mcan_flush(uint32_t timeout_us)
{
    if (!mcan_initialized) {
        return false;
    }

    /* TXBRP remains set while any FIFO/queue request is waiting for bus
     * arbitration or acknowledgement.  Waiting on the hardware state avoids
     * dropping the last response when code immediately reconfigures MCAN or
     * resets the MCU.  The DWT deadline is bounded because an unplugged or
     * unacknowledged CAN bus can otherwise retry forever. */
    __DSB();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    const uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    const uint32_t requested = cycles_per_us * timeout_us;
    const uint32_t started = DWT->CYCCNT;
    uint32_t fallback = requested * 4U + 1U;
    while (CM_MCAN1->TXBRP != 0U) {
        if (((uint32_t)(DWT->CYCCNT - started) >= requested) ||
            (fallback-- == 0U)) {
            return false;
        }
    }
    return true;
}

uint8_t board_mcan_ack_interrupt(void)
{
    if (!mcan_initialized) {
        return 0U;
    }

    const uint32_t status = CM_MCAN1->IR;
    uint8_t error = (status & 0x00800000UL) != 0U ? 1U : 0U;
    if ((status & MCAN_IR_BO) != 0U) {
        /* Clearing INIT starts the Bosch M_CAN bus-off recovery sequence. */
        CM_MCAN1->CCCR &= ~MCAN_CCCR_INIT;
        error = 2U;
    }
    CM_MCAN1->IR = status;
    NVIC_ClearPendingIRQ(INT003_IRQn);
    __DSB();
    return error;
}
