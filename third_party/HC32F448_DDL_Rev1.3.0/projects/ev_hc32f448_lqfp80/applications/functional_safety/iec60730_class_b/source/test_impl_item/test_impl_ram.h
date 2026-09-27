/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/test_impl_item/test_impl_ram.h
 * @brief This file contains all the functions prototypes of the clock test.
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

#ifndef __TEST_IMPL_RAM_H__
#define __TEST_IMPL_RAM_H__

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "stl_test_ram.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/**
 * @addtogroup Test_Implement_Clock
 * @{
 */

/*******************************************************************************
 * Global type definitions ('typedef')
*******************************************************************************/

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
extern uint32_t m_au32MarchRAM[STL_MARCH_RAM_WORDS];
extern uint32_t m_au32MarchRAMBuf[STL_MARCH_RAM_BUF_WORDS];
extern uint32_t *m_pu32MarchRAM;
extern uint32_t m_au32StackBoundary[STL_STACK_BOUNDARY_WORDS];
extern stc_ram_test_para_t stcRamTestPara;

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup Test_Implement_Ram_Global_Functions
 * @{
 */
void STL_RamRuntimeParaInit(void);
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* __TEST_IMPL_RAM_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
