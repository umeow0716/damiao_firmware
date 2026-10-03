#include "board_adc.h"

#include <string.h>

#include "app_profile.h"

void board_adc_build_config(BoardAdcRegisterImage *config)
{
    memset(config, 0, sizeof(*config));
    config->channel_select = 0x00000007UL;
#if APP_PROFILE_ADC_PA1_PA3_LAYOUT
    config->adc1_channel_mux = 0x0213U;
    config->adc2_channel_mux = 0x3015U;
    config->adc3_channel_mux = 0x3B76U;
#else
    config->adc1_channel_mux = 0x3210U;
    config->adc2_channel_mux = 0x3510U;
    config->adc3_channel_mux = 0x3B76U;
#endif
    config->trigger_select = 0x0081U;
    /* validate_current_sensors writes SYNCMD=3, then sets SYNCEN.  SYNCDLY
     * bit 8 may read back as one on the target, but is not part of the
     * register image written by the firmware APP. */
    config->sync_control = 0x0031U;
    config->sample_time = 0x14U;
    /* GPIO_PCR_PIN (0x0100) is the live input level, not configuration.
     * adc_sampling_init writes only DDIS to the eight analogue pins
     * and deliberately leaves ADC3 channel 2 / PB10 in its reset state. */
    config->analog_pin_control = 0x8000U;
    config->auxiliary_pin_control = 0x0000U;
    config->aos_trigger = 0x000000CEUL;
    config->irq_source = 480UL;
}
