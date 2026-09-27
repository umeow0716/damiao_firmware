/**
 *******************************************************************************
 * @file  stl_test_ram.h
 * @brief This file contains all the functions prototypes of the RAM test.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2022-03-31       CDT             First version
   2025-08-01       CDT             Add lib files instead of IEC60730 class B source files
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

#ifndef __STL_TEST_RAM_H__
#define __STL_TEST_RAM_H__

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "stl_conf.h"
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
 * @addtogroup STL_IEC60730_RAM
 * @{
 */

/*******************************************************************************
 * Global type definitions ('typedef')
*******************************************************************************/
/**
 * @defgroup STL_IEC60730_RAM_Global_Type STL IEC60730 RAM Global Type
 * @{
 */
typedef struct {
    uint32_t u32TestRamBufWords;
    uint32_t u32TestRamBckGrnd;
    uint32_t u32TestRamInvBckGrnd;
    uint32_t u32TestRamMarchRamStart;
    uint32_t u32TestRamMarchRamEnd;
} stc_ram_test_para_t;
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
 * @addtogroup STL_IEC60730_RAM_Global_Functions
 * @{
 */
uint32_t STL_StackRuntimeInit(void);
uint32_t STL_StackRuntimeTest(void);
uint32_t STL_RamRuntimeInit(void);
uint32_t STL_RamRuntimeTest(void);
uint32_t STL_FullRamTestStartup(uint32_t u32StartAddr, uint32_t u32EndAddr);
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

#endif /* __STL_TEST_RAM_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
