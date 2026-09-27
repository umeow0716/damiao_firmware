#include "board_adc.h"

#include <string.h>

void board_adc_build_config(BoardAdcRegisterImage *config)
{
    memset(config, 0, sizeof(*config));
    config->channel_select = 0x00000007UL;
    config->adc1_channel_mux = 0x3210U;
    config->adc2_channel_mux = 0x3510U;
    config->adc3_channel_mux = 0x3B76U;
    config->trigger_select = 0x0081U;
    /* validate_current_sensors writes SYNCMD=3, then sets SYNCEN.  SYNCDLY
     * bit 8 reads back as one on the captured target, but is not part of the
     * register image written by the factory APP. */
    config->sync_control = 0x0031U;
    config->sample_time = 0x14U;
    /* GPIO_PCR_PIN (0x0100) is the live input level, not configuration.
     * adc_sampling_init@0x21a00 writes only DDIS to the eight analogue pins
     * and deliberately leaves ADC3 channel 2 / PB10 in its reset state. */
    config->analog_pin_control = 0x8000U;
    config->auxiliary_pin_control = 0x0000U;
    config->aos_trigger = 0x000000CEUL;
    config->irq_source = 480UL;
}
