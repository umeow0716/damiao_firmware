#include "system_identification.h"
#include "board_mcan.h"
#include "hc32f448.h"

uint32_t identification_lock(void)
{
    const uint32_t mask = __get_PRIMASK();
    __disable_irq();
    return mask;
}

void identification_unlock(uint32_t mask)
{
    __set_PRIMASK(mask);
}

bool identification_transport_ready(void)
{
    return board_mcan_identification_ready();
}

bool identification_send(uint16_t id, const uint8_t *data, uint8_t length)
{
    return board_mcan_try_send_identification(id, data, length);
}
