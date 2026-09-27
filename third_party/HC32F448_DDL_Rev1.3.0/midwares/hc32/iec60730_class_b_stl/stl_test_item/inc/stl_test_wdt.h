/**
 *******************************************************************************
 * @file  stl_test_wdt.h
 * @brief This file contains all the functions prototypes of the wdt test.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2025-08-01       CDT             First version
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

#ifndef __STL_TEST_WDT_H__
#define __STL_TEST_WDT_H__

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "stl_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup STL_IEC60730
 * @{
 */

/**
 * @addtogroup STL_IEC60730_WDT
 * @{
 */

/*******************************************************************************
 * Global type definitions ('typedef')
*******************************************************************************/
/**
 * @defgroup STL_IEC60730_WDT_Global_Type STL IEC60730 WDT Global Type
 * @{
 */
typedef struct {
    uint32_t u32RstFlagRge;
    uint32_t u32RstFlagRgeWdtMsk;
    uint32_t u32RstFlagRgeWdtClr;
    uint32_t u32WdtTimeout;
} stc_wdt_test_para_t;
/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup STL_IEC60730_WDT_Global_Functions
 * @{
 */
uint32_t STL_WdtStartupTest(void);

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

#endif /* __STL_TEST_WDT_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
