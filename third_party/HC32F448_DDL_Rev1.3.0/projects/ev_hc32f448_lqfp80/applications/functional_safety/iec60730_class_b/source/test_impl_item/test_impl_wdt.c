/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/test_impl_item/test_impl_wdt.c
 * @brief This file provides firmware functions to implement the watch test.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-08-01       CDT             Change watchdog to SWDT
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
#include "test_impl_wdt.h"

/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/**
 * @defgroup Test_Implement_WDT Test Implement Watchdog
 * @{
 */

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define WDT_TIMEOUT                 (1000UL)
#define WDT_STARTUP_PERIOD          (SWDT_CNT_PERIOD256)
#define WDT_RUNTIME_PERIOD          (SWDT_CNT_PERIOD16384)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
stc_wdt_test_para_t stcWdtTestPara;

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @defgroup Test_Implement_WDT_Global_Functions Test Implement Watchdog Global Functions
 * @{
 */

/**
 * @brief  Watchdog test Para initialize in startup.
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          Initialize successfully.
 *           - STL_ERR:         Initialize unsuccessfully.
 */
uint32_t STL_WdtStartupParaInit(void)
{
    stcWdtTestPara.u32RstFlagRge       = (uint32_t)(&CM_RMU->RSTF0);
    stcWdtTestPara.u32RstFlagRgeWdtMsk = (uint32_t)RMU_RSTF0_SWDRF;
    stcWdtTestPara.u32RstFlagRgeWdtClr = (uint32_t)RMU_RSTF0_CLRF;
    stcWdtTestPara.u32WdtTimeout       = 6000UL;

    if (SET == RMU_GetStatus(RMU_FLAG_PWR_ON | RMU_FLAG_PIN)) {
        RMU_ClearStatus();
    }

    return STL_OK;
}

/**
 * @brief  Watchdog test initialize in runtime.
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          Initialize successfully.
 */
uint32_t STL_WdtStartupTestInit(void)
{
    stc_swdt_init_t stcSwdtInit;

    /* Stop count when core halt */
    DBGC_PeriphCmd(DBGC_PERIPH_SWDT, DISABLE);

    stcSwdtInit.u32CountPeriod = WDT_STARTUP_PERIOD;
    stcSwdtInit.u32ClockDiv = SWDT_CLK_DIV1;
    stcSwdtInit.u32RefreshRange = SWDT_RANGE_0TO100PCT;
    stcSwdtInit.u32LPMCount = SWDT_LPM_CNT_STOP;
    stcSwdtInit.u32ExceptionType = SWDT_EXP_TYPE_RST;
    (void)SWDT_Init(&stcSwdtInit);
    return STL_OK;
}

/**
 * @brief  Watchdog test initialize in runtime.
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          Initialize successfully.
 */
uint32_t STL_WdtRuntimeInit(void)
{
    stc_swdt_init_t stcSwdtInit;

    stcSwdtInit.u32CountPeriod = WDT_RUNTIME_PERIOD;
    stcSwdtInit.u32ClockDiv = SWDT_CLK_DIV1;
    stcSwdtInit.u32RefreshRange = SWDT_RANGE_0TO100PCT;
    stcSwdtInit.u32LPMCount = SWDT_LPM_CNT_STOP;
    stcSwdtInit.u32ExceptionType = SWDT_EXP_TYPE_RST;
    (void)SWDT_Init(&stcSwdtInit);
    return STL_OK;
}

/**
 * @brief  Watchdog start
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          Test pass.
 */
uint32_t STL_WdtTestStart(void)
{
    /* First reload counter to start WDT */
    SWDT_FeedDog();
    return STL_OK;
}

/**
 * @brief  Watchdog feed in runtime.
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          Test pass.
 */
uint32_t STL_WdtRuntimeFeed(void)
{
    SWDT_FeedDog();
    return STL_OK;
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
