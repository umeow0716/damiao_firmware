/**
 *******************************************************************************
 * @file  stl_test_adc.h
 * @brief This file contains all the functions prototypes of the adc test.
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

#ifndef __STL_ADC_TEST_H__
#define __STL_ADC_TEST_H__

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
 * @addtogroup STL_IEC60730_ADC
 * @{
 */

/*******************************************************************************
 * Global type definitions ('typedef')
*******************************************************************************/
/**
 * @defgroup STL_IEC60730_ADC_Global_Type STL IEC60730 ADC Global Type
 * @{
 */
typedef struct {
    uint32_t u32AdcFlagReg;      /* unit num */
    uint32_t u32AdcFlagMsk;
    uint32_t u32AdcFlagClrReg;   /* pointer to AD channel num*/
    uint32_t u32AdcFlagClrMsk;   /* sample times */
    uint32_t u32AdcResult0Reg;   /* expected lower value */
    uint32_t u32AdcResult1Reg;   /* expected upper value */
    uint32_t u32AdcResult2Reg;
    uint32_t u32AdcResult3Reg;
    uint32_t u32AdcTestUpperLimit;
    uint32_t u32AdcTestLowerLimit;
} stc_stl_adc_test_para_t;
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
 * @addtogroup STL_IEC60730_ADC_Global_Functions
 * @{
 */
uint32_t STL_AdcTestStartup(void);
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

#endif /* __STL_ADC_TEST_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
