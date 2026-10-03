#include "board_mcan.h"

#include "memory_layout.h"

#include <stddef.h>
#include <string.h>

#include "hc32_ll_mcan.h"
#include "system_hc32f448.h"

#define MCAN_MSG_RAM_BASE_ADDR (0x4002B000UL)
#define MCAN_MSG_RAM_WORDS (0x100U)

float board_mcan_current_data_rate_kbps(void)
{
    const uint32_t cccr = CM_MCAN1->CCCR;
    uint32_t prescaler;
    uint32_t time_quanta;
    if (((cccr & (MCAN_CCCR_FDOE | MCAN_CCCR_BRSE)) != 0U) &&
        ((CM_MCAN1->CCCR & (MCAN_CCCR_FDOE | MCAN_CCCR_BRSE)) == (MCAN_CCCR_FDOE | MCAN_CCCR_BRSE)))
    {
        /* Firmware diagnostics reload each field independently. */
        const uint32_t dbtp_prescaler = CM_MCAN1->DBTP;
        const uint32_t dbtp_segment_1 = CM_MCAN1->DBTP;
        const uint32_t dbtp_segment_2 = CM_MCAN1->DBTP;
        prescaler = ((dbtp_prescaler >> 16U) & 0x1FU) + 1U;
        time_quanta = ((dbtp_segment_1 >> 8U) & 0x1FU) + ((dbtp_segment_2 >> 4U) & 0x0FU) + 3U;
    }
    else
    {
        if ((cccr & (MCAN_CCCR_FDOE | MCAN_CCCR_BRSE)) != 0U)
        {
            /* The diagnostic path reports zero when FD flags are only partly
             * enabled, without reading either timing register. */
            return 0.0f;
        }
        const uint32_t nbtp_prescaler = CM_MCAN1->NBTP;
        const uint32_t nbtp_segment_1 = CM_MCAN1->NBTP;
        const uint32_t nbtp_segment_2 = CM_MCAN1->NBTP;
        /* debug_print_device_info uses only eight NBRP bits even though the
         * register field is nine bits wide. */
        prescaler = ((nbtp_prescaler >> 16U) & 0xFFU) + 1U;
        time_quanta = ((nbtp_segment_1 >> 8U) & 0xFFU) + (nbtp_segment_2 & 0xFFU) + 3U;
    }
    return (80000.0f / (float)prescaler) / (float)time_quanta;
}

typedef void (*McanInitializeFunction)(uint16_t data_rate_selector, uint16_t node_id);
typedef uint32_t (*McanSendFunction)(const uint8_t *data, uint16_t id, uint8_t length);

typedef struct
{
    McanInitializeFunction volatile initialize;
    McanSendFunction volatile send;
    uint8_t transmit_and_protocol[0x54U];
    uint32_t received_header_0;
    uint32_t received_header_1;
    uint32_t received_data[2];
    uint8_t protocol_tail[0x38U];
} McanDispatch;

/* Initialization and the control IRQ publish the callbacks as two independent
 * word stores.  The control IRQ builds outgoing payloads at +0x08 and uses the
 * remaining fields as its persistent CAN protocol workspace. */
static McanDispatch mcan_dispatch __attribute__((section(".mcan_dispatch")));

SRAM_ABI_ASSERT_OFFSET(McanDispatch, transmit_and_protocol, 0x08U);
SRAM_ABI_ASSERT_OFFSET(McanDispatch, received_header_0, 0x5CU);
SRAM_ABI_ASSERT_OFFSET(McanDispatch, protocol_tail, 0x6CU);
SRAM_ABI_ASSERT_SIZE(McanDispatch, 0xA4U);

static void wait_for_cccr(uint32_t mask, uint32_t expected)
{
    while ((CM_MCAN1->CCCR & mask) != expected)
    {
        /* Both firmware configuration routines wait without a timeout. */
    }
}

static __attribute__((noipa, section(".mcan_transition_delay"))) void
mcan_transition_delay(uint32_t count)
{
    /* short_busy_wait is called immediately before clearing INIT. */
    /* Includes the final zero-to-UINT32_MAX subtraction. No stack counter
     * or NOP: the firmware helper is exactly SUBS / BCS / BX LR. */
    __asm__ volatile("1: subs %0, %0, #1\n\t"
                     "bcs 1b"
                     : "+r"(count)
                     :
                     : "cc", "memory");
}

static void configure_pins_and_clock(const BoardMcanRegisterImage *config)
{
    /* MCAN1 is active-low gated in FCG1. */
    CM_PWC->FCG1 &= ~PWC_FCG1_MCAN1;

    /* Board routing: PB6=MCAN1_RXD (function 0x33),
     * PB7=MCAN1_TXD (function 0x32), high drive strength. */
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PCRB6 = config->rx_pin_control;
    CM_GPIO->PCRB7 = config->tx_pin_control;
    CM_GPIO->PFSRB6 = config->rx_pin_function;
    CM_GPIO->PFSRB7 = config->tx_pin_function;
    CM_GPIO->PWPR = 0xA500U;

    /* Select PLLQ.  Its configured 80 MHz output is the MCAN clock used by
     * the fixed-layout nominal/data timing constants. */
    CM_PWC->FPRC = (uint16_t)(CM_PWC->FPRC | 0xA501U);
    CM_CMU->CANCKCFGR = (uint16_t)((CM_CMU->CANCKCFGR & 0x00F0U) | 0x0008U);
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
    /* The firmware keeps distinct classic and FD CCCR write sequences.  The
     * intermediate RMW states are observable and classic mode does not touch
     * TEST, DBTP or TDCR. */
    CM_MCAN1->CCCR &= ~MCAN_CCCR_DAR;
    CM_MCAN1->CCCR &= ~MCAN_CCCR_TXP;
    if (config->fd_enabled)
    {
        CM_MCAN1->CCCR &= ~MCAN_CCCR_PXHD;
        CM_MCAN1->CCCR |= MCAN_CCCR_FDOE | MCAN_CCCR_BRSE;
        CM_MCAN1->CCCR &= ~(MCAN_CCCR_ASM | MCAN_CCCR_MON | MCAN_CCCR_TEST);
        CM_MCAN1->TEST &= ~MCAN_TEST_LBCK;
    }
    else
    {
        CM_MCAN1->CCCR |= MCAN_CCCR_PXHD;
        CM_MCAN1->CCCR &= ~(MCAN_CCCR_FDOE | MCAN_CCCR_BRSE);
    }

    CM_MCAN1->NBTP = config->nbtp;
    if (config->fd_enabled)
    {
        /* Firmware enables TDC only after programming its offset. */
        CM_MCAN1->DBTP = config->dbtp & ~UINT32_C(0x00800000);
        CM_MCAN1->TDCR = config->tdcr;
        if ((config->dbtp & UINT32_C(0x00800000)) != 0U)
        {
            CM_MCAN1->DBTP |= UINT32_C(0x00800000);
        }
    }

    CM_MCAN1->RXESC = config->rxesc;
    CM_MCAN1->TXESC = config->txesc;
    CM_MCAN1->SIDFC = config->sidfc;
    CM_MCAN1->XIDFC = config->xidfc;
    CM_MCAN1->RXF0C = config->rxf0c;
    CM_MCAN1->RXF1C = config->rxf1c;
    CM_MCAN1->RXBC = config->rxbc;
    CM_MCAN1->TXEFC = config->txefc;
    CM_MCAN1->TXBC = config->txbc;

    volatile uint32_t *const message_ram = (volatile uint32_t *)MCAN_MSG_RAM_BASE_ADDR;
    for (uint32_t i = 0U; i < MCAN_MSG_RAM_WORDS; ++i)
    {
        message_ram[i] = 0U;
    }
    CM_MCAN1->GFC = (CM_MCAN1->GFC & ~UINT32_C(0x3F)) | config->gfc;
    message_ram[0] = config->standard_filter[0];
    message_ram[1] = config->standard_filter[1];

    CM_MCAN1->IE = config->ie;
    CM_MCAN1->ILS = config->ils;
    CM_MCAN1->ILE = config->ile;
}

static void mcan_init_common(uint16_t node_id, uint16_t data_rate_selector, bool fd_enabled)
{
    BoardMcanRegisterImage config;
    board_mcan_build_config(node_id, data_rate_selector, &config);
    /* The selected firmware entry point determines mode, not its argument.
     * FD always keeps the fixed arbitration timing, including selectors 0..3. */
    config.fd_enabled = fd_enabled;
    if (fd_enabled)
    {
        config.nbtp = UINT32_C(0x26003A13);
        /* FD mode uses a different transmit-event FIFO configuration. */
        config.txefc = UINT32_C(0x00040308);
    }

    configure_pins_and_clock(&config);
    enter_configuration_mode();
    apply_register_image(&config);

    mcan_transition_delay(15U);
    CM_MCAN1->CCCR &= ~MCAN_CCCR_INIT;
    wait_for_cccr(MCAN_CCCR_INIT, 0U);

    CM_MCAN1->IR = 0xFFFFFFFFUL;
    CM_INTC->INTSEL3 = (uint32_t)INT_SRC_MCAN1_INT1;
    NVIC_SetPriority(INT003_IRQn, 3U);
    NVIC_ClearPendingIRQ(INT003_IRQn);
    NVIC_EnableIRQ(INT003_IRQn);
}

void board_mcan_begin_irq(McanIrqContext *references)
{
    references->controller = (volatile uint32_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b80), UINT32_C(0x1fff8540));
    references->initial_ir = references->controller[0x50U / 4U];
    references->config = (const volatile uint16_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b84), UINT32_C(0x1fff8544));
    references->status = (volatile uint32_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b88), UINT32_C(0x1fff8548));
}

bool board_mcan_receive_payload(uint32_t *id, uint8_t *length, uint8_t *data,
                                McanIrqContext *references)
{
    if ((id == NULL) || (length == NULL) || (data == NULL))
    {
        return false;
    }

    /* Gate receive work on IR.RF0N rather than the FIFO fill level.  Error-only
     * interrupts must leave the persistent receive image intact. */
    if ((references->initial_ir & UINT32_C(1)) == 0U)
    {
        return false;
    }
    /* IRQ003 copies the selected 16-byte FIFO0 element prefix into its
     * fixed workspace before acknowledging F0AI.  Capture it before the DDL
     * advances the FIFO so the SRAM image has the same persistent words. */
    const uint32_t get_index =
        (references->controller[0xA4U / 4U] & MCAN_RXF0S_F0GI) >> MCAN_RXF0S_F0GI_POS;
    volatile const uint32_t *const element =
        (volatile const uint32_t *)(MCAN_MSG_RAM_BASE_ADDR + 0x80UL + get_index * 72UL);
    const uint32_t received_header_0 = element[0];
    volatile McanDispatch *const dispatch = (volatile McanDispatch *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b8c), UINT32_C(0x1fff854c));
    references->dispatch = dispatch;
    dispatch->received_header_0 = received_header_0;
    dispatch->received_header_1 = element[1];
    const uint32_t received_data_0 = element[2];
    dispatch->received_data[0] = received_data_0;
    const uint32_t received_data_1 = element[3];
    volatile uint16_t *const received_id = (volatile uint16_t *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b90), UINT32_C(0x1fff8550));
    references->received_id = received_id;

    /* Extract standard-ID bits unconditionally, then acknowledge this exact
     * index without a second FIFO read. */
    *id = (received_header_0 >> 18U) & 0x07FFU;
    dispatch->received_data[1] = received_data_1;
    *received_id = (uint16_t)*id;
    references->controller[0xA8U / 4U] = get_index;
    references->cleared =
        *(volatile const float *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b48), UINT32_C(0x1fff8508));
    references->two_pi =
        *(volatile const float *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b68), UINT32_C(0x1fff8528));
    references->node_id =
        *(const volatile uint32_t *)(uintptr_t)((uintptr_t)references->config + 0x20U);
    references->sample = (volatile void *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b94), UINT32_C(0x1fff8554));
    references->motor = (volatile void *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff8b50), UINT32_C(0x1fff8510));
    *length = 8U;
    memcpy(data, &received_data_0, sizeof(received_data_0));
    memcpy(data + sizeof(received_data_0), &received_data_1, sizeof(received_data_1));
    return true;
}

bool board_mcan_receive(BoardMcanFrame *frame)
{
    McanIrqContext references;
    board_mcan_begin_irq(&references);
    return frame != NULL &&
           board_mcan_receive_payload(&frame->id, &frame->length, frame->data, &references);
}

__attribute__((noinline, section(".mcan_init_classic"))) void
board_mcan_init_classic(uint16_t data_rate_selector, uint16_t node_id)
{
    const uint8_t selector = data_rate_selector > 4U ? 4U : (uint8_t)data_rate_selector;
    mcan_init_common(node_id, selector, false);
}

__attribute__((noinline, section(".mcan_init_fd"))) void
board_mcan_init_fd(uint16_t data_rate_selector, uint16_t node_id)
{
    const uint16_t selector = data_rate_selector < 4U ? 4U : data_rate_selector;
    mcan_init_common(node_id, selector, true);
}

static inline __attribute__((always_inline)) uint32_t mcan_payload_word(const uint8_t *data)
{
    /* Firmware loads byte 3 before its unguarded 32-bit load, then replaces
     * that byte. Preserve both reads even for unaligned payloads. */
    const uint32_t high = *(const volatile uint8_t *)(data + 3U);
    const uint32_t word = *(const volatile uint32_t *)(uintptr_t)data;
    return (word & 0x00FFFFFFUL) | (high << 24U);
}

uint32_t board_mcan_send_classic_payload(const uint8_t *data, uint16_t id, uint8_t length)
{
    CM_MCAN_TypeDef *const controller = (CM_MCAN_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9C34UL, 0x1FFF9C6CUL);
    const uint32_t put_index = (controller->TXFQS >> 16U) & 0x1FU;
    volatile uint32_t *const element =
        (volatile uint32_t *)(*(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9C38UL,
                                                                                0x1FFF9C70UL) +
                              0x328U + put_index * 72U);
    element[0] = (uint32_t)id << 18U;
    element[1] = (uint32_t)length << 16U;
    element[2] = mcan_payload_word(data);
    element[3] = mcan_payload_word(data + 4U);
    const uint32_t request = 1UL << put_index;
    controller->TXBAR = request;
    return request;
}

static inline __attribute__((always_inline)) uint32_t
mcan_send_fd_common(const uint8_t *data, uint16_t id, uint8_t length, uint32_t dlc, uint32_t flags)
{
    CM_MCAN_TypeDef *const controller = (CM_MCAN_TypeDef *)(uintptr_t)*(
        volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9C34UL, 0x1FFF9C6CUL);
    const uint32_t put_index = (controller->TXFQS >> 16U) & 0x1FU;
    if (length <= 8U)
    {
        dlc = length;
    }
    else if (length <= 12U)
    {
        dlc = 9U;
    }
    else if (length <= 16U)
    {
        dlc = 10U;
    }
    else if (length <= 20U)
    {
        dlc = 11U;
    }
    else if (length <= 24U)
    {
        dlc = 12U;
    }
    else if (length <= 32U)
    {
        dlc = 13U;
    }
    else if (length <= 48U)
    {
        dlc = 14U;
    }
    else if (length <= 64U)
    {
        dlc = 15U;
    }
    volatile uint32_t *const element =
        (volatile uint32_t *)(*(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9C38UL,
                                                                                0x1FFF9C70UL) +
                              0x328U + put_index * 72U);
    element[0] = (uint32_t)id << 18U;
    element[1] = flags | (dlc << 16U);
    /* FD payload base is a separate pool load after both header stores. */
    volatile uint32_t *destination =
        (volatile uint32_t *)(*(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9C3CUL,
                                                                                0x1FFF9C74UL) +
                              put_index * 72U);
    uint8_t offset = 0U;
    while (offset < length)
    {
        *destination++ = mcan_payload_word(data + offset);
        offset = (uint8_t)(offset + 4U);
    }
    const uint32_t request = 1UL << put_index;
    controller->TXBAR = request;
    return request;
}

uint32_t board_mcan_send_fd_payload(const uint8_t *data, uint16_t id, uint8_t length,
                                    uint32_t initial_dlc)
{
    return mcan_send_fd_common(data, id, length, initial_dlc, 0x00300000UL);
}

uint32_t board_mcan_send_variable_fd_payload(const uint8_t *data, uint16_t id, uint8_t length,
                                             uint32_t initial_dlc)
{
    return mcan_send_fd_common(data, id, length, initial_dlc, 0x00200000UL);
}

bool board_mcan_init(uint16_t node_id, uint16_t data_rate_selector)
{
    /* Firmware callers ignore callback r0; the public board API still
     * reports completion once its unbounded hardware waits finish. */
    mcan_dispatch.initialize(data_rate_selector, node_id);
    return true;
}

void board_mcan_send_prebuilt(uint16_t id, uint8_t length)
{
    /* IRQ003 calls the selected transport with the already
     * published fixed payload; there is no intervening SRAM copy. */
    (void)mcan_dispatch.send(mcan_dispatch.transmit_and_protocol, id, length);
}

void board_mcan_send_prebuilt_irq(uint16_t id, uint8_t length, const McanIrqContext *references)
{
    volatile McanDispatch *const dispatch = (volatile McanDispatch *)references->response_dispatch;
    McanSendFunction const send = dispatch->send;
    (void)send((const uint8_t *)(uintptr_t)dispatch->transmit_and_protocol, id, length);
}

void board_mcan_reinitialize_live(bool after_parameter_write, const McanIrqContext *references)
{
    const volatile uint16_t *const config =
        references != NULL ? references->config
                           : (const volatile uint16_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                                 UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558));
    const uint16_t node_id = config[0x20U / 2U];
    McanInitializeFunction initialize;
    uint16_t selector;
    if (after_parameter_write)
    {
        /* This path: node, callback, then selector. */
        volatile McanDispatch *const dispatch =
            references != NULL ? (volatile McanDispatch *)references->response_dispatch
                               : &mcan_dispatch;
        initialize = dispatch->initialize;
        selector = config[0x8CU / 2U];
    }
    else
    {
        /* This path: node, callback-owner literal, selector, then
         * callback. This tail has a separate pool from the RX dispatch. */
        const uintptr_t dispatch = *(volatile const uint32_t *)MEMORY_LAYOUT_ADDRESS(
            UINT32_C(0x1fff9878), UINT32_C(0x1fff923c));
        selector = config[0x8CU / 2U];
        initialize = *(McanInitializeFunction volatile *)dispatch;
    }
    (void)initialize(selector, node_id);
}

bool board_mcan_send(const BoardMcanFrame *frame)
{
    if ((frame == NULL) || (frame->length > 8U) || (mcan_dispatch.send == NULL))
    {
        return false;
    }
    memcpy(mcan_dispatch.transmit_and_protocol, frame->data, frame->length);
    return mcan_dispatch.send(mcan_dispatch.transmit_and_protocol, (uint16_t)frame->id,
                              frame->length) != 0U;
}

void board_mcan_update_node_filter(uint16_t node_id)
{
    /* A node-ID parameter write updates filter 0 directly without a CCCR
     * transition or controller restart. */
    volatile uint32_t *const message_ram = (volatile uint32_t *)MCAN_MSG_RAM_BASE_ADDR;
    message_ram[0] = 0x880000FFUL | ((uint32_t)(node_id & 0x07FFU) << 16U);
}

static void select_transport_format(volatile McanDispatch *dispatch, uint8_t data_rate_selector)
{
    /* FCB swaps the classic/FD callback pair immediately without putting MCAN
     * back into INIT or rewriting bit timing. */
    if (data_rate_selector > 4U)
    {
        dispatch->initialize = board_mcan_init_fd;
        dispatch->send = mcan_send_fd_helper;
    }
    else
    {
        dispatch->initialize = board_mcan_init_classic;
        dispatch->send = mcan_send_classic_helper;
    }
}

void board_mcan_select_transport_format(uint8_t data_rate_selector)
{
    select_transport_format(&mcan_dispatch, data_rate_selector);
}

void board_mcan_select_transport_format_irq(uint8_t data_rate_selector,
                                            const McanIrqContext *references)
{
    volatile McanDispatch *const dispatch =
        references != NULL ? (volatile McanDispatch *)references->response_dispatch
                           : &mcan_dispatch;
    select_transport_format(dispatch, data_rate_selector);
}

bool board_mcan_flush(uint32_t timeout_us)
{
    /* TXBRP remains set while any FIFO/queue request is waiting for bus
     * arbitration or acknowledgement.  Waiting on the hardware state avoids
     * dropping the last response when code immediately reconfigures MCAN or
     * resets the MCU.  The DWT deadline is bounded because an unplugged or
     * unacknowledged CAN bus can otherwise retry forever. */
    __DSB();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    const uint32_t cycles_per_us = 200U;
    const uint32_t requested = cycles_per_us * timeout_us;
    const uint32_t started = DWT->CYCCNT;
    uint32_t fallback = requested * 4U + 1U;
    while (CM_MCAN1->TXBRP != 0U)
    {
        if (((uint32_t)(DWT->CYCCNT - started) >= requested) || (fallback-- == 0U))
        {
            return false;
        }
    }
    return true;
}

bool board_mcan_reinitialization_requested(const McanIrqContext *references)
{
    return (references->controller[0x50U / 4U] & UINT32_C(0x00800000)) != 0U;
}

uint8_t board_mcan_recover_bus_off(const McanIrqContext *references)
{
    if ((references->controller[0x50U / 4U] & MCAN_IR_BO) != 0U)
    {
        references->controller[0x18U / 4U] &= ~MCAN_CCCR_INIT;
        return 2U;
    }
    return 0U;
}

uint8_t board_mcan_ack_interrupt(const McanIrqContext *references)
{
    const uint8_t error = 0U;
    /* IRQ003 writes all ones after handling both relevant branches. */
    references->controller[0x50U / 4U] = UINT32_MAX;
    NVIC_ClearPendingIRQ(INT003_IRQn);
    return error;
}
