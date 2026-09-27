/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/test_impl_item/test_impl_clk.h
 * @brief This file contains all the functions prototypes of the clock test.
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

#ifndef __TEST_IMPL_CLK_H__
#define __TEST_IMPL_CLK_H__

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "stl_test_clock.h"

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
extern __IO uint32_t m_u32ClockErrCount;
extern __IO uint8_t m_u32FmcActived;
extern stc_clock_test_para_t stcClockTestPara;

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup Test_Implement_Clock_Global_Functions
 * @{
 */
uint32_t STL_ClkFcmRuntimeInit(void);
uint32_t STL_ClkFcmRuntimeTest(void);
uint32_t STL_ClockTestInit(void);
uint32_t STL_ClockTestDeInit(void);
uint32_t STL_ClockStartupTest(void);
uint32_t STL_ClockRuntimeTest(void);
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


#endif /* __TEST_IMPL_CLK_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
