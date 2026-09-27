/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/runtime_test.h
 * @brief This file contains IEC60730 runtime test.
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

#ifndef __RUNTIME_TEST_H__
#define __RUNTIME_TEST_H__

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/**
 * @addtogroup Runtime_Test
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
extern volatile uint32_t g_u32SysTickCount;

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
void TEST_FeedWatchdog(void);
void STL_RuntimeTestInit(void);
void STL_RuntimeTest(void);
/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RUNTIME_TEST_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
