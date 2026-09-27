/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/test_impl_item/test_impl_flash.h
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

#ifndef __TEST_IMPL_FLASH_H__
#define __TEST_IMPL_FLASH_H__

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "stl_test_flash.h"

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
extern const uint32_t __checksum;

extern stc_flash_test_para_t g_stcFlashTestPara0;

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup Test_Implement_Flash_Global_Functions
 * @{
 */
void STL_FlashStartupParaInit(void);
void STL_FlashRuntimeParaInit(void);
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


#endif /* __TEST_IMPL_FLASH_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
