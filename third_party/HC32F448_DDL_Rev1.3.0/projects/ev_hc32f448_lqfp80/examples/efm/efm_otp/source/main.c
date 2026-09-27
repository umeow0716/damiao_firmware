/**
 *******************************************************************************
 * @file  efm/efm_otp/source/main.c
 * @brief Main program of EFM for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Use LL_PERIPH_WE to lock/unlock OTP area write protect
                                    Update API: EFM_OTP_Lock
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
 * @addtogroup EFM_Otp
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define EFM_OTP_BLOCK_NUM       (13U)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Main function of EFM project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint32_t u32Data = 0x5A5A5A5AU;

    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_EFM  | LL_PERIPH_EFM_OTP);

    /* System clock init */
    BSP_CLK_Init();
    /* Expand IO init */
    BSP_IO_Init();
    /* LED init */
    BSP_LED_Init();
    /* KEY Init */
    BSP_KEY_Init();

    /* Wait flash ready. */
    while (SET != EFM_GetStatus(EFM_FLAG_RDY)) {
        ;
    }

    /* EFM_FWMC write enable */
    EFM_FWMC_Cmd(ENABLE);
    /* If otp enable, erase invalid except sector0. If locked, all otp area erase invalid. */
    (void)EFM_SectorErase(EFM_OTP_BLOCK13);
    /* Enable otp */
    EFM_OTP_Enable();
    /* Program to specified otp block */
    EFM_ProgramWord(EFM_OTP_BLOCK13, u32Data);
    if (u32Data == RW_MEM32(EFM_OTP_BLOCK13)) {
        BSP_LED_On(LED_BLUE);
    } else {
        BSP_LED_On(LED_YELLOW);
    }

    /* K1 */
    while (RESET == BSP_KEY_GetStatus(BSP_KEY_1)) {
        ;
    }

    /* Lock OTP block */
    (void)EFM_OTP_Lock(EFM_OTP_BLOCK_NUM, 1U);
    /* Program to lock otp block */
    EFM_ProgramWord(EFM_OTP_BLOCK13 + 4UL, u32Data);

    if (SET == EFM_GetStatus(EFM_FLAG_OTPWERR)) {
        BSP_LED_Off(LED_BLUE);
        BSP_LED_On(LED_RED);
        /* Clear Flag */
        EFM_ClearStatus(EFM_FLAG_OTPWERR);
    }

    EFM_FWMC_Cmd(DISABLE);
    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_EFM  | LL_PERIPH_EFM_OTP);

    for (;;) {
        ;
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
