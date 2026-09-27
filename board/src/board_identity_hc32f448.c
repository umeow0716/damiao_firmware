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
