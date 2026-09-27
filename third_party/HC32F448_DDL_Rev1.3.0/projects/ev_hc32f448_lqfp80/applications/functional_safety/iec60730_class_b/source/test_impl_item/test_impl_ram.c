/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/test_impl_item/test_impl_ram.c
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
#include "test_impl_ram.h"

/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/**
 * @defgroup Test_Implement_Ram Test Implement Ram
 * @{
 */

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/**
 * @defgroup STL_IEC60730_RAM_Local_Variables STL IEC60730 RAM Local Variables
 * @{
 */
stc_ram_test_para_t stcRamTestPara STL_SECTION(".march_ram_para");

STL_USED uint32_t  m_au32MarchRAM[STL_MARCH_RAM_WORDS] STL_SECTION(".march_ram");
STL_USED uint32_t  m_au32MarchRAMBuf[STL_MARCH_RAM_BUF_WORDS] STL_SECTION(".march_ram_buf");
STL_USED uint32_t *m_pu32MarchRAM STL_SECTION(".march_ram_pointer");
STL_USED uint32_t  m_au32StackBoundary[STL_STACK_BOUNDARY_WORDS] STL_SECTION(".stack_boundary");
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
 * @brief  Initialize para initial at runtime
 * @param  None
 * @retval None
 */
void STL_RamRuntimeParaInit(void)
{
    stcRamTestPara.u32TestRamBufWords =  STL_MARCH_RAM_BUF_WORDS;
    stcRamTestPara.u32TestRamBckGrnd  = STL_MARCH_RAM_BCKGRND;
    stcRamTestPara.u32TestRamInvBckGrnd =  STL_MARCH_RAM_INVBCKGRND;
    stcRamTestPara.u32TestRamMarchRamStart =  STL_MARCH_RAM_START;
    stcRamTestPara.u32TestRamMarchRamEnd =  STL_MARCH_RAM_END;
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
