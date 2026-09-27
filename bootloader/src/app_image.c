#include "app_image.h"

#include "hc32f448.h"

typedef void (*ResetEntry)(void);

bool app_image_installed(void)
{
    return app_image_installed_length(DM4310_APP_END - DM4310_APP_BASE);
}

bool app_image_installed_length(size_t image_length)
{
    return app_image_validate((const void *)DM4310_APP_BASE,
                              image_length, 0U) == APP_IMAGE_OK;
}

void app_image_jump(void)
{
    const uint32_t *vectors = (const uint32_t *)DM4310_APP_BASE;
    const uint32_t initial_sp = vectors[0];
    const ResetEntry reset = (ResetEntry)vectors[1];

    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    for (uint32_t i = 0U; i < 8U; ++i) {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }
    SCB->VTOR = DM4310_APP_BASE;
    __DSB();
    __ISB();
    __set_MSP(initial_sp);
    reset();
    for (;;) {}
}
