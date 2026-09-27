/**
 *******************************************************************************
 * @file  stl_test_flash.h
 * @brief This file contains all the functions prototypes of the flash test.
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

#ifndef __STL_TEST_FLASH_H__
#define __STL_TEST_FLASH_H__

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "stl_conf.h"
#include "stl_common.h"
#include "stl_sw_crc32.h"
#include "stl_utility.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup STL_IEC60730
 * @{
 */

/**
 * @addtogroup STL_IEC60730_Flash
 * @{
 */

/*******************************************************************************
 * Global type definitions ('typedef')
*******************************************************************************/
/**
 * @defgroup STL_IEC60730_Flash_Global_Type STL IEC60730 Flash Global Type
 * @{
 */
typedef struct {
    uint32_t u32TestFlashCrc32Start;
    uint32_t u32TestFlashCrc32Size;
    uint32_t u32TestFlashCrc32End;
    uint32_t u32TestFlashBlockSize;
    uint32_t u32TestFlashBuildCrc32;
    uint32_t u32CheckAddr;
    uint32_t u32CheckEndAddr;
    uint32_t u32CalcLen;
    uint32_t u32CalcCrc32Value;
} stc_flash_test_para_t;
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
 * @addtogroup STL_IEC60730_Flash_Global_Functions
 * @{
 */
uint32_t STL_FlashStartupTest(stc_flash_test_para_t *stcFlashTestPara);
uint32_t STL_FlashRuntimeInit(void);
uint32_t STL_FlashRuntimeTest(stc_flash_test_para_t *stcFlashTestPara);
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

#endif /* __STL_TEST_FLASH_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
