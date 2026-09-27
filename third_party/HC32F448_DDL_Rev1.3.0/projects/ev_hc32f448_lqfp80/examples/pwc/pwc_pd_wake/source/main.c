/**
 *******************************************************************************
 * @file  pwc/pwc_pd_wake/source/main.c
 * @brief Main program of PWC for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
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
 * @addtogroup PWC_PowerDown_wake
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/


/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define DLY_MS          (500UL)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void PowerDownModeConfig(void);
static void ResetCausePrint(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Wakeup timer IRQ handler
 * @param  None
 * @retval None
 */
void PWC_WKTM_Handler(void)
{
    if (SET == PWC_WKT_GetStatus()) {
        DDL_Printf("Wake-up timer overflow.\r\n");
        PWC_WKT_ClearStatus();
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  Power down mode behavior config.
 * @param  None
 * @retval None
 */
static void PowerDownModeConfig(void)
{
    stc_pwc_pd_mode_config_t stcPDModeConfig;

    (void)PWC_PD_StructInit(&stcPDModeConfig);

    stcPDModeConfig.u8IOState = PWC_PD_IO_KEEP1;
    stcPDModeConfig.u8Mode = PWC_PD_MD1;

    (void)PWC_PD_Config(&stcPDModeConfig);
    PWC_PD_ClearWakeupStatus(PWC_PD_WKUP_FLAG_ALL);

    /* Wake up by WKTM */
    PWC_PD_WakeupCmd(PWC_PD_WKUP_WKTM, ENABLE);

    /* Disable WKTM inadvance */
    PWC_WKT_Cmd(DISABLE);
    /* LRC for WKTM */
    CLK_LrcCmd(ENABLE);
    /* WKTM init */
    PWC_WKT_Config(PWC_WKT_CLK_SRC_LRC, 0xFFFU);

    NVIC_ClearPendingIRQ(WKTM_IRQn);
    NVIC_SetPriority(WKTM_IRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(WKTM_IRQn);
}

/**
 * @brief  Reset cause info. print.
 * @param  None
 * @retval None
 */
static void ResetCausePrint(void)
{
    if (SET == RMU_GetStatus(RMU_FLAG_PIN)) {
        DDL_Printf("Pin reset occurs.\r\n");
    }
    if (SET == RMU_GetStatus(RMU_FLAG_PWR_DOWN)) {
        DDL_Printf("Power down mode reset occurs.\r\n");
    }
    RMU_ClearStatus();
}

/**
 * @brief  Main function of PWC power down wakeup project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint8_t u8Count;

    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_EFM | LL_PERIPH_LVD);
    /* System clock init */
    BSP_CLK_Init();
    /* Expand IO init */
    BSP_IO_Init();
    /* LED init */
    BSP_LED_Init();

    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);

    PowerDownModeConfig();
    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_EFM);

    ResetCausePrint();

    /* KEY5 */
    while (PIN_RESET != GPIO_ReadInputPins(BSP_KEY5_PORT, BSP_KEY5_PIN)) {
        ;
    }

    for (;;) {
        u8Count = 2U;
        do {
            BSP_LED_Toggle(LED_BLUE);
            DDL_DelayMS(DLY_MS);
        } while ((--u8Count) != 0U);
        DDL_Printf("MCU will enter power down mode...\r\n");
        DDL_DelayMS(DLY_MS);

        PWC_WKT_Cmd(ENABLE);
        PWC_PD_Enter();
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
