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

#ifndef __STL_SW_CRC32_H__
#define __STL_SW_CRC32_H__

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "stl_common.h"
#include "test_impl_wdt.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup STL_IEC60730
 * @{
 */

/**
 * @addtogroup STL_IEC60730_CRC32
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
extern uint32_t m_u32CalcCrc32Init;

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup STL_IEC60730_CRC32_Global_Functions
 * @{
 */
uint32_t STL_StartupCalculateCRC32Value(uint32_t u32Crc32Value, uint8_t *pu8Data, uint32_t u32Len);
uint32_t STL_CalculateCRC32Value(uint32_t u32Crc32Value, uint8_t *pu8Data, uint32_t u32Len);
void STL_Crc32Init(void);
uint32_t STL_GetCrc32(uint32_t u32CalcCrc32Value);
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

#endif /* __STL_SW_CRC32_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
