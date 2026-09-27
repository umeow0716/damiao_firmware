#include "firmware_control.h"

#include <string.h>

#include "app_config.h"
#include "app_state.h"
#include "debug_console.h"
#include "hc32f448.h"
#include "platform.h"

void firmware_control_export_block(
    const MotorConfig *config,
    uint8_t block[FIRMWARE_CONTROL_BLOCK_SIZE])
{
    uint32_t words[APP_CONFIG_WORD_COUNT];
    app_config_encode_persistent(config, words);
    memcpy(block, words, FIRMWARE_CONTROL_BLOCK_SIZE);
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
    uint32_t words[APP_CONFIG_WORD_COUNT];
    app_config_encode_persistent(config, words);
    memcpy(words, block, FIRMWARE_CONTROL_BLOCK_SIZE);

    MotorConfig candidate = *config;
    /* handle_firmware_control_request@0x26d00 copies these 128 bytes directly
     * over the live record and immediately re-derives controller parameters;
     * startup-only erased-record/default normalization does not run here. */
    app_config_decode_runtime(&candidate, words);
    *config = candidate;
    return true;
}

void firmware_control_service(void)
{
    const uint8_t request = g_app.events.firmware_control_request;
    if (request == 0U) {
        return;
    }

    if (request == 1U) {
        uint8_t response[FIRMWARE_CONTROL_BLOCK_SIZE + 2U];
        response[0] = 'g';
        firmware_control_export_block(&g_app.config, &response[1]);
        response[sizeof(response) - 1U] = 0xAAU;
        platform_debug_write(response, sizeof(response));

        g_app.events.firmware_control_request = 0U;
        debug_console_return_to_menu();
        return;
    }

    if ((request == 2U) || (request == 6U)) {
        const MotorControlMode previous_mode = g_app.config.control_mode;

        if (!g_app.firmware_control_payload_valid ||
            !firmware_control_import_block(
                &g_app.config, g_app.firmware_control_payload)) {
            g_app.firmware_control_payload_valid = false;
            return;
        }

        g_app.firmware_control_payload_valid = false;

        if (g_app.config.control_mode != previous_mode) {
            memset(&g_app.motor.command, 0, sizeof(g_app.motor.command));
            g_app.motor.command.mode = g_app.config.control_mode;
        }

        motor_control_configure(&g_app.motor, &g_app.config,
                                g_app.motor.feedback.bus_voltage);

        if (request == 2U) {
            __disable_irq();
            platform_store_parameters(&g_app.config);
            __enable_irq();

            static const uint8_t acknowledgement[3] = {'U', 'g', 'U'};
            platform_debug_write(acknowledgement,
                                 sizeof(acknowledgement));
            platform_system_reset();
            return;
        }

        static const uint8_t acknowledgement[3] = {'U', 'g', 'Q'};
        platform_debug_write(acknowledgement, sizeof(acknowledgement));

        g_app.events.firmware_control_request = 0U;
        debug_console_return_to_menu();
        return;
    }

    /* Request 4, UgZ: accepted but no reply. */
    g_app.firmware_control_payload_valid = false;
    g_app.events.firmware_control_request = 0U;
    debug_console_return_to_menu();
}
