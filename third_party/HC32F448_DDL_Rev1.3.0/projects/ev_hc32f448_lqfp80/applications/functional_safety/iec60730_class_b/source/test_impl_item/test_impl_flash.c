/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/test_impl_item/test_impl_flash.c
 * @brief This file provides firmware functions to implement the clock test.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2025-11-03       CDT             First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2024, Xiaohua Semiconductor Co., Ltd. All rights reserved.
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
#include "test_impl_flash.h"


/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/**
 * @defgroup Test_Implement_Flas Test Implement Flash
 * @{
 */

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/**
 * @defgroup STL_IEC60730_Flash_Local_Variables STL IEC60730 Flash Local Variables
 * @{
 */
stc_flash_test_para_t g_stcFlashTestPara0;
STL_USED const uint32_t __checksum STL_SECTION(".checksum") = 0UL;
/**
 * @}
 */

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
 * @defgroup Test_Implement_Clock_Global_Functions Test Implement Clock Global Functions
 * @{
 */

/**
 * @brief  Initialize startup para of flash initial at runtime
 * @param  None
 * @retval None
 */
void STL_FlashStartupParaInit(void)
{
    volatile uint32_t u32CcBulidCrc32Addr = (uint32_t)(&STL_ROM_CRC32_CC_CHECKSUM);

    g_stcFlashTestPara0.u32TestFlashCrc32Start = STL_ROM_CRC32_START;
    g_stcFlashTestPara0.u32TestFlashCrc32Size  = (STL_ROM_CRC32_END - STL_ROM_CRC32_START);
    g_stcFlashTestPara0.u32TestFlashBuildCrc32 = *(volatile uint32_t *)(u32CcBulidCrc32Addr);
}

/**
 * @brief  Initialize runtime para of flash initial at runtime
 * @param  None
 * @retval None
 */
void STL_FlashRuntimeParaInit(void)
{
    volatile uint32_t u32CcBulidCrc32Addr = (uint32_t)(&STL_ROM_CRC32_CC_CHECKSUM);

    g_stcFlashTestPara0.u32TestFlashCrc32Start = STL_ROM_CRC32_START;
    g_stcFlashTestPara0.u32TestFlashCrc32End   = STL_ROM_CRC32_END;
    g_stcFlashTestPara0.u32TestFlashCrc32Size  = (STL_ROM_CRC32_END - STL_ROM_CRC32_START);
    g_stcFlashTestPara0.u32TestFlashBlockSize  = STL_ROM_CRC32_BLOCK_SIZE;
    g_stcFlashTestPara0.u32TestFlashBuildCrc32 = *(volatile uint32_t *)(u32CcBulidCrc32Addr);
    g_stcFlashTestPara0.u32CheckAddr = g_stcFlashTestPara0.u32TestFlashCrc32Start;
    g_stcFlashTestPara0.u32CheckEndAddr = g_stcFlashTestPara0.u32TestFlashCrc32End + 3UL - g_stcFlashTestPara0.u32TestFlashBlockSize;
    g_stcFlashTestPara0.u32CalcLen = 0UL;
    g_stcFlashTestPara0.u32CalcCrc32Value = 0UL;
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
