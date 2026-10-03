#include "firmware_control.h"

#include <string.h>

#include "app_config.h"
#include "app_state.h"
#include "commissioning.h"
#include "debug_console.h"
#include "hc32f448.h"
#include "platform.h"
#include "runtime_compat.h"

void firmware_control_export_block(const MotorConfig *config,
                                   uint8_t block[FIRMWARE_CONTROL_BLOCK_SIZE])
{
    (void)config;
    runtime_copy_bytes(block, app_config_staging_record(), FIRMWARE_CONTROL_BLOCK_SIZE);
}

bool firmware_control_import_block(MotorConfig *config,
                                   const uint8_t block[FIRMWARE_CONTROL_BLOCK_SIZE])
{
    if ((config == NULL) || (block == NULL))
    {
        return false;
    }

    /* UgU and UgQ replace the first 32 words of the 37-word live configuration
     * staging record.  Preserve words 32..36 before validation. */
    uint32_t *const words = app_config_staging_record();
    memcpy(words, block, FIRMWARE_CONTROL_BLOCK_SIZE);

    MotorConfig candidate = *config;
    /* handle_firmware_control_request copies these 128 bytes directly
     * over the live record and immediately re-derives controller parameters;
     * startup-only erased-record/default normalization does not run here. */
    app_config_decode_runtime(&candidate, words);
    *config = candidate;
    return true;
}

void firmware_control_service(uint8_t request)
{
    uint8_t *const response = runtime_alloc(FIRMWARE_CONTROL_BLOCK_SIZE + 2U);

    if (request == 1U)
    {
        response[0] = 'g';
        firmware_control_export_block(&g_app.config, &response[1]);
        response[FIRMWARE_CONTROL_BLOCK_SIZE + 1U] = 0xAAU;
        platform_debug_write(response, FIRMWARE_CONTROL_BLOCK_SIZE + 2U);

        runtime_free(response);

        return;
    }

    if (request == 2U)
    {
        __disable_irq();
        platform_store_staged_parameters();
        __enable_irq();

        volatile uint8_t *const acknowledgement = response;
        acknowledgement[0] = 'U';
        acknowledgement[1] = 'g';
        acknowledgement[2] = 'U';
        platform_debug_write(response, 3U);
        platform_system_reset();
        return;
    }

    if (request == 6U)
    {
        volatile uint8_t *const acknowledgement = response;
        acknowledgement[0] = 'U';
        acknowledgement[1] = 'g';
        acknowledgement[2] = 'Q';
        /* The IRQ has already published the live staging block.  Consume it
         * directly without another copy or decode pass. */

        commissioning_update_runtime_parameter_cache_ugq(&g_app.config);
        derive_control_parameters_helper();
        commissioning_configure_runtime_loop_states(&g_app.config);

        platform_debug_write(response, 3U);
        runtime_free(response);

        return;
    }

    /* Request 4, UgZ: accepted but no reply. */
    runtime_free(response);
}
