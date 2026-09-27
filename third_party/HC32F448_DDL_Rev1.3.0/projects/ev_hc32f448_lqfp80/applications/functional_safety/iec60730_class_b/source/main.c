/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/main.c
 * @brief main project.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-08-01       CDT             Reimplement functionality using certified code
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
 * @addtogroup HC32F448_DDL_Applications
 * @{
 */

/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

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
 * @brief  Main function
 * @param  None
 * @retval None
 */
int32_t main(void)
{

    /* BSP initialize */
    BSP_CLK_Init();

    /* Initiliaze LED */
    STL_RED_LED_INIT();

    /* STL initialize */
    STL_RuntimeTestInit();

    /* Configure systick for period test */
    SysTick_Config(STL_SYSTICK_TICK_VALUE);

    for (;;) {
        /* Non interrupt test cnt */
        stcClockTestPara.u32NoneIntTestCnt++;

        /* Runtime test: loop test */
        if (g_u32SysTickCount >= 1UL)  {
            g_u32SysTickCount = 0UL;
            STL_RuntimeTest();
        } else {
            TEST_FeedWatchdog();

            if (STL_NonIntTestRuntime() != STL_OK) {
                STL_Printf("********    Test fail in runtime: NonInterrupt    ********\r\n");
                STL_SafetyFailure();
            }
        }

        /* Add user code */
        {
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
