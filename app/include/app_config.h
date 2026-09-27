#ifndef DM4310_APP_CONFIG_H
#define DM4310_APP_CONFIG_H

#include "motor_control.h"

#define APP_CONFIG_WORD_COUNT 37U

/* Load the effective startup defaults extracted from the official V5017.04
 * image. Persistent configuration and per-device calibration may override
 * their corresponding fields after this call. */
void app_config_load_defaults(MotorConfig *config);
void app_config_encode_persistent(const MotorConfig *config,
                                  uint32_t words[APP_CONFIG_WORD_COUNT]);
void app_config_encode_factory_persistent(
    uint32_t words[APP_CONFIG_WORD_COUNT]);
bool app_config_decode_persistent(MotorConfig *config,
                                  const uint32_t words[APP_CONFIG_WORD_COUNT]);
void app_config_decode_runtime(MotorConfig *config,
                               const uint32_t words[APP_CONFIG_WORD_COUNT]);

#endif
