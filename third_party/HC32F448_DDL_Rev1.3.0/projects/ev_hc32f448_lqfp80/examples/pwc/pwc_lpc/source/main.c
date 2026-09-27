/**
 *******************************************************************************
 * @file  pwc/pwc_lpc/source/main.c
 * @brief Main program of PWC for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Disable HRC when enter sleep mode
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2025, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "main.h"

/**
 * @addtogroup HC32F448_DDL_Examples
 * @{
 */

/**
 * @addtogroup PWC_Lpc
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define KEY5_PORT               (GPIO_PORT_B)
#define KEY5_PIN                (GPIO_PIN_06)
#define KEY5_EXTINT_CH          (EXTINT_CH06)
#define KEY5_INT_SRC            (INT_SRC_PORT_EIRQ6)
#define KEY5_INT_IRQn           (INT001_IRQn)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
__IO uint8_t u8Count = 0U;
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
static void KEY5_IrqHandle()
{
    if (SET == EXTINT_GetExtIntStatus(KEY5_EXTINT_CH)) {
        u8Count++;
        if (u8Count >= 6U) {
            u8Count = 0U;
        }
    }
    DDL_DelayMS(1000U);
}

/**
 * @brief  Key5 init function
 * @param  None
 * @retval None
 */
static void Key5_Init(void)
{
    stc_extint_init_t stcExtIntInit;
    stc_irq_signin_config_t stcIrqSignConfig;
    stc_gpio_init_t stcGpioInit;

    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
    stcGpioInit.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(KEY5_PORT, KEY5_PIN, &stcGpioInit);

    /* ExtInt config */
    (void)EXTINT_StructInit(&stcExtIntInit);
    stcExtIntInit.u32Edge        = EXTINT_TRIG_FALLING;
    (void)EXTINT_Init(KEY5_EXTINT_CH, &stcExtIntInit);

    /* IRQ sign-in */
    stcIrqSignConfig.enIntSrc    = KEY5_INT_SRC;
    stcIrqSignConfig.enIRQn      = KEY5_INT_IRQn;
    stcIrqSignConfig.pfnCallback = &KEY5_IrqHandle;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);
}
/**
 * @brief  Stop mode init function
 * @param  None
 * @retval None
 */
static void StopMode_Init(void)
{
    stc_pwc_stop_mode_config_t stcStopConfig;

    (void)PWC_STOP_StructInit(&stcStopConfig);

    stcStopConfig.u16ExBusHold = PWC_STOP_EXBUS_HIZ;
    stcStopConfig.u16Clock = PWC_STOP_CLK_KEEP;
    stcStopConfig.u8StopDrv = PWC_STOP_DRV_LOW;
    stcStopConfig.u16FlashWait = PWC_STOP_FLASH_WAIT_ON;

    (void)PWC_STOP_Config(&stcStopConfig);

    /* Wake-up source config (EXTINT Ch.6 here) */
    INTC_WakeupSrcCmd(INTC_STOP_WKUP_EXTINT_CH6, ENABLE);
}

/**
 * @brief  Power down mode init function
 * @param  None
 * @retval None
 */
static void PdMode_Init(void)
{
    stc_pwc_pd_mode_config_t stcPDModeConfig;

    (void)PWC_PD_StructInit(&stcPDModeConfig);
    stcPDModeConfig.u8IOState = PWC_PD_IO_KEEP1;
    stcPDModeConfig.u8Mode = PWC_PD_MD3;
    (void)PWC_PD_Config(&stcPDModeConfig);
    PWC_PD_ClearWakeupStatus(PWC_PD_WKUP_FLAG_ALL);

    /* wake_up 1_1 event */
    PWC_PD_SetWakeupTriggerEdge(PWC_PD_WKUP_TRIG_WKUP1, PWC_PD_WKUP_TRIG_FALLING);
    PWC_PD_WakeupCmd(PWC_PD_WKUP_WKUP12, ENABLE);
    PWC_PD_ClearWakeupStatus(PWC_PD_WKUP_FLAG_WKUP1);
}

/**
 * @brief  Main function of low power consumption
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_EFM | LL_PERIPH_GPIO | LL_PERIPH_LVD);
    /* Key init */
    Key5_Init();

    CLK_HrcCmd(DISABLE);
    CLK_LrcCmd(DISABLE);
    CLK_XtalCmd(DISABLE);
    CLK_Xtal32Cmd(DISABLE);
    PWC_LDO_Cmd(PWC_LDO_PLL, DISABLE);

    for (;;) {
        if (1U == u8Count) {
            PWC_LDO_Cmd(PWC_LDO_HRC, DISABLE);
            PWC_SLEEP_Enter(PWC_SLEEP_WFI);
        } else if (3U == u8Count) {
            /* stop mode init */
            StopMode_Init();
            PWC_STOP_Enter(PWC_STOP_WFI);
        } else if (5U == u8Count) {
            /* Power down mode init */
            PdMode_Init();
            PWC_XTAL32_PowerCmd(DISABLE);
            PWC_PD_Enter();
        }
    }
}

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
