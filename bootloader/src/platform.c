#include "boot_platform.h"

#include <string.h>

#include "board_clock.h"
#include "board_flash.h"
#include "board_led.h"
#include "board_mcan.h"
#include "board_uart.h"
#include "app_image.h"
#include "boot_record.h"
#include "device_auth.h"
#include "hc32f448.h"
#include "system_hc32f448.h"

#define BOOT_CAN_NODE_ID 2U
#define BOOT_CAN_RATE_SELECTOR 9U
#define BOOT_REQUEST_TIMEOUT_MS 1000U
#define OTP_KEY_SLOT_BASE       0x03000C00UL
#define OTP_KEY_SLOT_STRIDE     0x40UL

static bool boot_mcan_ready;
static bool boot_request_active;
static volatile uint32_t boot_milliseconds;
static uint32_t boot_request_started_at;

static void apply_debug_access_policy(bool disabled)
{
    /* PSPCR bits 0 and 1 are SWCLK and SWDIO.  Unlike the original image,
     * applying the stored policy explicitly in both directions also repairs
     * a debug-port state inherited across a software-only application jump. */
    CM_GPIO->PWPR = 0xA501U;
    if (disabled) {
        CM_GPIO->PSPCR &= (uint16_t)~(GPIO_PSPCR_SPFE & 0x03U);
    } else {
        CM_GPIO->PSPCR |= (uint16_t)(GPIO_PSPCR_SPFE & 0x03U);
    }
    CM_GPIO->PWPR = 0xA500U;
}

void SysTick_Handler(void)
{
    ++boot_milliseconds;
}

static void load_boot_prefix(BootRecordSectorPrefix *prefix)
{
    memcpy(prefix, (const void *)DM4310_BOOT_RECORD_ADDRESS, sizeof(*prefix));
    if (!boot_update_journal_in_progress(&prefix->update)) {
        boot_update_journal_clear(&prefix->update);
    }
}

static bool store_boot_prefix(const BootRecordSectorPrefix *prefix)
{
    return board_flash_replace_sector_prefix(DM4310_BOOT_RECORD_ADDRESS,
                                              prefix, sizeof(*prefix));
}

void boot_platform_init(void)
{
    const bool clock_ok = board_clock_init();
    board_led_init();
    /* Recovered user-visible state: loader/waiting-for-update is green;
     * the application changes to red before it accepts an Enable command. */
    board_led_set(BOARD_LED_GREEN);
    boot_mcan_ready = clock_ok &&
        board_mcan_init(BOOT_CAN_NODE_ID, BOOT_CAN_RATE_SELECTOR);
    if (clock_ok) {
        board_uart_init_polled();
    }
    if (boot_mcan_ready) {
        /* The source loader polls FIFO0 in its main loop.  The application
         * owns IRQ3, but the loader vector deliberately has no IRQ3 handler. */
        NVIC_DisableIRQ(INT003_IRQn);
        NVIC_ClearPendingIRQ(INT003_IRQn);
    }
    boot_milliseconds = 0U;
    SysTick_Config(SystemCoreClock / 1000U);

    const BootPersistentRecord *const record =
        (const BootPersistentRecord *)DM4310_BOOT_RECORD_ADDRESS;
    apply_debug_access_policy(record->swd_disabled == 1U);
}

bool boot_platform_authenticate_device(void)
{
    const uint32_t uid_words[3] = {
        CM_EFM->UQID0,
        CM_EFM->UQID1,
        CM_EFM->UQID2,
    };
    uint8_t uid[BOOT_DEVICE_UID_SIZE];
    memcpy(uid, uid_words, sizeof(uid));

#if DM4310_ENFORCE_DEVICE_BINDING
    uint8_t slots[BOOT_DEVICE_KEY_SLOT_COUNT][BOOT_DEVICE_TOKEN_SIZE];
    for (uint32_t slot = 0U; slot < BOOT_DEVICE_KEY_SLOT_COUNT; ++slot) {
        const volatile uint8_t *const source =
            (const volatile uint8_t *)(OTP_KEY_SLOT_BASE +
                                       slot * OTP_KEY_SLOT_STRIDE);
        for (uint32_t byte = 0U; byte < BOOT_DEVICE_TOKEN_SIZE; ++byte) {
            slots[slot][byte] = source[byte];
        }
    }
    uint8_t matched_slot;
    if (!boot_device_authenticate(uid, slots, &matched_slot)) {
        static const char failure[] =
            "Device key verification failed; recovery only.\r\n";
        board_led_set(BOARD_LED_RED);
        boot_platform_debug_write(failure, sizeof(failure) - 1U);
        return false;
    }
    (void)matched_slot;
#endif

    BootRecordSectorPrefix prefix;
    load_boot_prefix(&prefix);
    if (prefix.record.device_id != uid_words[0]) {
        prefix.record.device_id = uid_words[0];
        if (!store_boot_prefix(&prefix)) {
            return false;
        }
    }
    return true;
}

uint16_t boot_platform_node_id(void)
{
    return BOOT_CAN_NODE_ID;
}

uint16_t boot_platform_protocol_version(void)
{
    const BootPersistentRecord *const record =
        (const BootPersistentRecord *)DM4310_BOOT_RECORD_ADDRESS;
    return (uint16_t)record->application_identity;
}

bool boot_platform_stay_in_loader(void)
{
    BootRecordSectorPrefix prefix;
    load_boot_prefix(&prefix);
    if (boot_update_journal_in_progress(&prefix.update)) {
        /* A reset during sector replacement must never fall through to a
         * structurally plausible but mixed-version application image. */
        boot_request_active = false;
        return true;
    }
    if (boot_record_is_normal(&prefix.record)) {
        return false;
    }
    boot_request_active = boot_record_is_update_requested(&prefix.record);
    boot_request_started_at = boot_milliseconds;
    return true;
}

bool boot_platform_loader_timeout_expired(void)
{
    return boot_request_active &&
           ((uint32_t)(boot_milliseconds - boot_request_started_at) >=
            BOOT_REQUEST_TIMEOUT_MS);
}

uint32_t boot_platform_milliseconds(void)
{
    return boot_milliseconds;
}

bool boot_platform_clear_boot_request(void)
{
    BootRecordSectorPrefix prefix;
    load_boot_prefix(&prefix);
    if (boot_update_journal_in_progress(&prefix.update)) {
        return false;
    }
    boot_record_clear_request(&prefix.record);
    if (!store_boot_prefix(&prefix)) {
        return false;
    }
    boot_request_active = false;
    return true;
}

bool boot_platform_mark_update_started(void)
{
    BootRecordSectorPrefix prefix;
    load_boot_prefix(&prefix);
    boot_record_request_update(&prefix.record);
    boot_update_journal_begin(&prefix.update);
    return store_boot_prefix(&prefix);
}

bool boot_platform_mark_upgrade_complete(void)
{
    BootRecordSectorPrefix prefix;
    load_boot_prefix(&prefix);
    boot_record_request_update(&prefix.record);
    boot_update_journal_clear(&prefix.update);
    return store_boot_prefix(&prefix);
}

bool boot_platform_set_swd_disabled(bool disabled)
{
    BootRecordSectorPrefix prefix;
    load_boot_prefix(&prefix);
    const uint32_t requested = disabled ? 1U : 0U;
    if (prefix.record.swd_disabled != requested) {
        prefix.record.swd_disabled = requested;
        boot_record_request_update(&prefix.record);
        if (!store_boot_prefix(&prefix)) {
            return false;
        }
    }
    apply_debug_access_policy(disabled);
    return true;
}

bool boot_platform_receive(BootCanFrame *frame)
{
    BoardMcanFrame received;
    if (!boot_mcan_ready || (frame == NULL) ||
        !board_mcan_receive(&received)) {
        return false;
    }
    frame->id = received.id;
    frame->length = received.length;
    memcpy(frame->data, received.data, received.length);
    return true;
}

void boot_platform_transmit(uint32_t id, const void *data, size_t length)
{
    if (!boot_mcan_ready || (data == NULL) || (length > 8U)) {
        return;
    }
    BoardMcanFrame frame = {
        .id = id,
        .length = (uint8_t)length,
    };
    memcpy(frame.data, data, length);
    board_mcan_send(&frame);
}

bool boot_platform_debug_receive(uint8_t *byte)
{
    return board_uart_receive(byte);
}

void boot_platform_debug_write(const void *data, size_t length)
{
    board_uart_write(data, length);
}

void boot_platform_delay_ms(uint32_t milliseconds)
{
    const uint32_t started = boot_milliseconds;
    while ((uint32_t)(boot_milliseconds - started) < milliseconds) {
        __NOP();
    }
}

bool boot_platform_flush_transport(uint32_t timeout_ms)
{
    const bool can_done = !boot_mcan_ready ||
        board_mcan_flush(timeout_ms * 1000U);
    const bool uart_done = board_uart_flush();
    return can_done && uart_done;
}

void boot_platform_idle(void) { __NOP(); }
bool boot_platform_flash_program(uint32_t address, const void *data, size_t length)
{
    if ((data == NULL) || (length == 0U) ||
        ((address & (BOARD_FLASH_SECTOR_SIZE - 1U)) != 0U) ||
        ((length & 3U) != 0U) || (length > BOARD_FLASH_SECTOR_SIZE) ||
        (address < DM4310_APP_BASE) ||
        (address >= DM4310_APP_END) ||
        (length > (size_t)(DM4310_APP_END - address))) {
        return false;
    }
    return board_flash_replace_sector_prefix(address, data, length);
}

void boot_platform_reset(void)
{
    boot_platform_flush_transport(10U);
    __DSB();
    NVIC_SystemReset();
}
