#include "firmware_control.h"

#include <string.h>

#include "app_config.h"
#include "app_state.h"
#include "commissioning.h"
#include "debug_console.h"
#include "hc32f448.h"
#include "platform.h"
#include "runtime_compat.h"

void firmware_control_export_block(
    const MotorConfig *config,
    uint8_t block[FIRMWARE_CONTROL_BLOCK_SIZE])
{
#if defined(DAMIAO_DM4310)
    (void)config;
    dm4310_runtime_copy_bytes(block, app_config_staging_record(),
                              FIRMWARE_CONTROL_BLOCK_SIZE);
#else
    uint32_t words[APP_CONFIG_WORD_COUNT];
    app_config_encode_persistent(config, words);
    memcpy(block, words, FIRMWARE_CONTROL_BLOCK_SIZE);
#endif
}

bool firmware_control_import_block(
    MotorConfig *config,
    const uint8_t block[FIRMWARE_CONTROL_BLOCK_SIZE])
{
    if ((config == NULL) || (block == NULL)) {
        return false;
    }

    /* UgU/UgQ replace the first 32 words of the original 37-word live
     * configuration staging record. Preserve words 32..36 exactly as the
     * original UART path does before validation. */
#if defined(DAMIAO_DM4310)
    uint32_t *const words = app_config_staging_record();
    memcpy(words, block, FIRMWARE_CONTROL_BLOCK_SIZE);
#else
    uint32_t words[APP_CONFIG_WORD_COUNT];
    app_config_encode_persistent(config, words);
    memcpy(words, block, FIRMWARE_CONTROL_BLOCK_SIZE);
#endif

    MotorConfig candidate = *config;
    /* handle_firmware_control_request@0x26d00 copies these 128 bytes directly
     * over the live record and immediately re-derives controller parameters;
     * startup-only erased-record/default normalization does not run here. */
    app_config_decode_runtime(&candidate, words);
    *config = candidate;
    return true;
}

#if defined(DAMIAO_DM4310)
void firmware_control_service(uint8_t request)
{
#else
void firmware_control_service(void)
{
    const uint8_t request =
        (uint8_t)APP_DEFERRED_EVENTS.firmware_control_request;
    if (request == 0U) {
        return;
    }
#endif

#if defined(DAMIAO_DM4310)
    uint8_t *const response = dm4310_runtime_alloc(
        FIRMWARE_CONTROL_BLOCK_SIZE + 2U);
#endif

    if (request == 1U) {
#if !defined(DAMIAO_DM4310)
        uint8_t response[FIRMWARE_CONTROL_BLOCK_SIZE + 2U];
#endif
        response[0] = 'g';
        firmware_control_export_block(&g_app.config, &response[1]);
        response[FIRMWARE_CONTROL_BLOCK_SIZE + 1U] = 0xAAU;
        platform_debug_write(response, FIRMWARE_CONTROL_BLOCK_SIZE + 2U);

#if defined(DAMIAO_DM4310)
        dm4310_runtime_free(response);
#endif

#if !defined(DAMIAO_DM4310)
        APP_DEFERRED_EVENTS.firmware_control_request = 0U;
        debug_console_return_to_menu();
#endif
        return;
    }

    if (request == 2U) {
#if !defined(DAMIAO_DM4310)
        if (!g_app.firmware_control_payload_valid) {
            return;
        }
        g_app.firmware_control_payload_valid = false;
#endif

        __disable_irq();
#if defined(DAMIAO_DM4310)
        platform_store_staged_parameters();
#else
        firmware_control_import_block(
            &g_app.config, g_app.firmware_control_payload);
        platform_store_parameters(&g_app.config);
#endif
        __enable_irq();

#if defined(DAMIAO_DM4310)
        volatile uint8_t *const acknowledgement = response;
        acknowledgement[0] = 'U';
        acknowledgement[1] = 'g';
        acknowledgement[2] = 'U';
        platform_debug_write(response, 3U);
#else
        static const uint8_t acknowledgement[3] = {'U', 'g', 'U'};
        platform_debug_write(acknowledgement, sizeof(acknowledgement));
#endif
        platform_system_reset();
        return;
    }

    if (request == 6U) {
#if !defined(DAMIAO_DM4310)
        const MotorControlMode previous_mode = g_app.config.control_mode;
#endif

#if defined(DAMIAO_DM4310)
        volatile uint8_t *const acknowledgement = response;
        acknowledgement[0] = 'U';
        acknowledgement[1] = 'g';
        acknowledgement[2] = 'Q';
        /* Vector_20 already published the live staging block. Factory
         * 0x26d88 reads it directly, without copying or decoding again. */
#else
        if (!g_app.firmware_control_payload_valid ||
            !firmware_control_import_block(
                &g_app.config, g_app.firmware_control_payload)) {
            g_app.firmware_control_payload_valid = false;
            return;
        }
#endif

#if !defined(DAMIAO_DM4310)
        g_app.firmware_control_payload_valid = false;

        if (g_app.config.control_mode != previous_mode) {
            memset(&g_app.motor.command, 0, sizeof(g_app.motor.command));
            g_app.motor.command.mode = g_app.config.control_mode;
        }
#endif

#if defined(DAMIAO_DM4310)
        commissioning_update_runtime_parameter_cache_ugq(&g_app.config);
#endif
#if defined(DAMIAO_DM4310)
        dm4310_derive_control_parameters_helper();
        commissioning_configure_runtime_loop_states(&g_app.config);
#else
        motor_control_configure(&g_app.motor, &g_app.config,
                                g_app.motor.feedback.bus_voltage);
        commissioning_configure_runtime_drive_states(
            &g_app.config, g_app.motor.feedback.bus_voltage);
        commissioning_configure_runtime_loop_states(&g_app.config);
#endif

#if defined(DAMIAO_DM4310)
        platform_debug_write(response, 3U);
        dm4310_runtime_free(response);
#else
        static const uint8_t acknowledgement[3] = {'U', 'g', 'Q'};
        platform_debug_write(acknowledgement, sizeof(acknowledgement));
#endif

#if !defined(DAMIAO_DM4310)
        APP_DEFERRED_EVENTS.firmware_control_request = 0U;
        debug_console_return_to_menu();
#endif
        return;
    }

    /* Request 4, UgZ: accepted but no reply. */
#if defined(DAMIAO_DM4310)
    dm4310_runtime_free(response);
#else
    g_app.firmware_control_payload_valid = false;
#endif
#if !defined(DAMIAO_DM4310)
    APP_DEFERRED_EVENTS.firmware_control_request = 0U;
    debug_console_return_to_menu();
#endif
}
