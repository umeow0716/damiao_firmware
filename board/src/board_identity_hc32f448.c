#include "board_identity.h"

#include "hc32f448.h"

uint8_t board_identity_read_hardware_variant(void)
{
    /* read_hardware_variant@0x221f4 configures PC14 and PC15 with their
     * internal pull-ups, then returns PIDRC[15:14].  These are board straps,
     * not the unrelated 32-bit identity word stored in the boot record. */
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PCRC14 = GPIO_PCR_PUU;
    CM_GPIO->PCRC15 = GPIO_PCR_PUU;
    CM_GPIO->PWPR = 0xA500U;

    return board_identity_decode_hardware_variant(CM_GPIO->PIDRC);
}

#if defined(DAMIAO_DM4310)
uint8_t board_identity_initialize_status_and_read_variant(void)
{
    /* read_hardware_variant@0x221f4 is also the one-time LED setup.  Keep
     * this combined path so splitting the responsibilities does not add
     * protected-register cycles, output-reset writes or barriers. */
    CM_GPIO->PWPR = 0xA501U;
    CM_GPIO->PCRH2 = GPIO_PCR_POUTE;
    CM_GPIO->PCRC13 = GPIO_PCR_POUTE;
    CM_GPIO->PCRC14 = GPIO_PCR_PUU;
    CM_GPIO->PCRC15 = GPIO_PCR_PUU;
    CM_GPIO->PWPR = 0xA500U;
    CM_GPIO->POSRC |= (uint16_t)(1U << 13U);
    CM_GPIO->PORRH |= (uint16_t)(1U << 2U);
    return (uint8_t)(CM_GPIO->PIDRC >> 14U);
}
#endif
