/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/startup_test.c
 * @brief IEC60730 class B for the startup test.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-08-01       CDT             Apply certified code
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
#include "stl_bsp_conf.h"
#include "stl_utility.h"
#include "stl_test_adc.h"
#include "stl_test_cpu.h"
#include "stl_test_pc.h"
#include "stl_test_ram.h"
#include "stl_test_wdt.h"
#include "test_impl_clk.h"
#include "test_impl_flash.h"

/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/**
 * @addtogroup Startup_Test
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define FAIL_HANLDER               (STL_SafetyFailure)

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
 * @brief  Power on self-test.
 * @param  None
 * @retval None
 */
void STL_StartupTest(void)
{
    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_ALL);

    /* Initialize BSP clock */
    BSP_CLK_Init();

    /* Initialize LED */
    STL_RED_LED_INIT();

#if (STL_PRINT_ENABLE == STL_ON)
    (void)STL_PrintfInit();   /* startup debug print */
#endif

    STL_Printf("********    Self-test startup start            ********\r\n");

    if (STL_WdtStartupTest() != STL_OK) {
        STL_Printf("********    Test fail in startup: Watchdog     ********\r\n");
        STL_SafetyFailure();
    }

    if (STL_CpuTestStartup() != STL_OK) {
        STL_Printf("********    Test fail in startup: CPU          ********\r\n");
        STL_SafetyFailure();
    }

    if (STL_PcTest() != STL_OK) {
        STL_Printf("********    Test fail in startup: PC           ********\r\n");
        STL_SafetyFailure();
    }

    STL_WdtRuntimeFeed();

    STL_FlashStartupParaInit();
    if (STL_FlashStartupTest(&g_stcFlashTestPara0) != STL_OK) {
        STL_Printf("********    Test fail in startup: Flash        ********\r\n");
        STL_SafetyFailure();
    }

    STL_WdtRuntimeFeed();

    if (STL_ClockStartupTest() != STL_OK) {
        STL_Printf("********    Test fail in startup: Clock        ********\r\n");
        STL_SafetyFailure();
    }

    STL_WdtRuntimeFeed();

    /* Note: Full ram test MUST be independently tested,  because test destroy stack. */
    if (STL_FullRamTestStartup(STL_RAM1_START, STL_RAM1_END) != STL_OK) {
        STL_Printf("********    Test fail in startup: RAM1         ********\r\n");
        STL_SafetyFailure();
    }

    if (STL_FullRamTestStartup(STL_RAM2_START, STL_RAM2_END) != STL_OK) {
        STL_Printf("********    Test fail in startup: RAM2         ********\r\n");
        STL_SafetyFailure();
    }

    STL_WdtRuntimeFeed();

    STL_Printf("********    Self-test startup end              ********\r\n");

    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_MRC);
    CLK_PLLCmd(DISABLE);

    /* Jump to the application after power-onself-test complete successfully */
    CallApplicationStartUp();
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
