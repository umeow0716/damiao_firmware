#ifndef DAMIAO_APP_CONFIG_H
#define DAMIAO_APP_CONFIG_H

#include "motor_control.h"

#define APP_CONFIG_WORD_COUNT 37U

/* Load the model's effective startup defaults.  Persistent configuration and
 * per-device calibration may override
 * their corresponding fields after this call. */
void app_config_load_defaults(MotorConfig *config);
void app_config_encode_persistent(const MotorConfig *config, uint32_t words[APP_CONFIG_WORD_COUNT]);
bool app_config_decode_persistent(MotorConfig *config, const uint32_t words[APP_CONFIG_WORD_COUNT]);
void app_config_decode_runtime(MotorConfig *config, const uint32_t words[APP_CONFIG_WORD_COUNT]);
uint32_t *app_config_staging_record(void);
void app_config_initialize_scatter_defaults(void);
bool app_config_record_present(const volatile uint32_t words[APP_CONFIG_WORD_COUNT]);
void app_config_stage_and_decode(MotorConfig *config,
                                 const volatile uint32_t words[APP_CONFIG_WORD_COUNT]);

#endif
