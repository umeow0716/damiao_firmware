/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/runtime_test.c
 * @brief IEC60730 class B for the runtime test.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-08-01       CDT             Apply certified code
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

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "stl_bsp_conf.h"
#include "stl_utility.h"
#include "stl_test_cpu.h"
#include "stl_test_pc.h"
#include "stl_test_ram.h"
#include "stl_test_runtime.h"
#include "test_impl_adc.h"
#include "test_impl_clk.h"
#include "test_impl_gpio.h"
#include "test_impl_interrupt.h"
#include "test_impl_wdt.h"
#include "test_impl_flash.h"
#include "runtime_test.h"

/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/**
 * @addtogroup Runtime_Test
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define FAIL_HANLDER               (STL_SafetyFailure)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
volatile uint32_t g_u32SysTickCount;

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static const stc_stl_case_runtime_t m_stcPeriodCaseTable[] = {
    /* User implement the below test cases. */
    STL_RUNTIME_CASE(ClockFcm,  STL_ClkFcmRuntimeInit, STL_ClkFcmRuntimeTest, NULL, FAIL_HANLDER),
    STL_RUNTIME_CASE(Interrupt, STL_IntRuntimeInit, STL_IntRuntimeTest, NULL, FAIL_HANLDER),
};

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Self-test initialization in runtime.
 * @param  None
 * @retval None
 */
void STL_RuntimeTestInit(void)
{
#if (STL_PRINT_ENABLE == STL_ON)
    /* Initialize runtime debug print */
    (void)STL_PrintfInit();
#endif

    STL_Printf("********    Self-test runtime initialize       ********\r\n");

    if (STL_FlashRuntimeInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: Flash        ********\r\n");
    }

    if (STL_RamRuntimeInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: RAM          ********\r\n");
    }

    if (STL_StackRuntimeInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: Stack        ********\r\n");
    }

    if (STL_AdcRuntimeInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: Adc          ********\r\n");
    }

    if (STL_GpioInputRuntimeInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: Gpio in      ********\r\n");
    }

    if (STL_GpioOutputRuntimeInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: Gpio out     ********\r\n");
    }

    if (STL_WdtRuntimeInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: Wdt          ********\r\n");
    }

    if (STL_ClockTestInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: Clock&INT    ********\r\n");
    }

    if (STL_ClkFcmRuntimeInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: ClockFcm     ********\r\n");
    }

    if (STL_IntRuntimeInit() != STL_OK) {
        STL_Printf("********    Init fail in runtime: Interrupt    ********\r\n");
    }
}

/**
 * @brief  Self-test on runtime.
 * @param  None
 * @retval None
 */
void STL_RuntimeTest(void)
{
    STL_WdtRuntimeFeed();

    if (STL_CpuTestRuntime() != STL_OK) {
        STL_Printf("********    Test fail in runtime: CPU          ********\r\n");
        FAIL_HANLDER();
    }

    if (STL_PcTest() != STL_OK) {
        STL_Printf("********    Test fail in runtime: PC           ********\r\n");
        FAIL_HANLDER();
    }

    if (STL_FlashRuntimeTest(&g_stcFlashTestPara0) != STL_OK) {
        STL_Printf("********    Test fail in runtime: Flash        ********\r\n");
        FAIL_HANLDER();
    }

    __disable_irq();    /* Disable Interrupt */
    if (STL_RamRuntimeTest() != STL_OK) {
        STL_Printf("********    Test fail in runtime: RAM          ********\r\n");
        FAIL_HANLDER();
    }
    __enable_irq();     /* Enable Interrupt */

    if (STL_StackRuntimeTest() != STL_OK) {
        STL_Printf("********    Test fail in runtime: Stack        ********\r\n");
        FAIL_HANLDER();
    }

    if (STL_AdcRuntimeTest() != STL_OK) {
        STL_Printf("********    Test fail in runtime: Adc          ********\r\n");
        FAIL_HANLDER();
    }

    if (STL_GpioInputRuntimeTest() != STL_OK) {
        STL_Printf("********    Test fail in runtime: Gpio in      ********\r\n");
        FAIL_HANLDER();
    }

    if (STL_GpioOutputRuntimeTest() != STL_OK) {
        STL_Printf("********    Test fail in runtime: Gpio out     ********\r\n");
        FAIL_HANLDER();
    }

    if (STL_WdtRuntimeFeed() != STL_OK) {
        STL_Printf("********    Test fail in runtime: Wdt          ********\r\n");
        FAIL_HANLDER();
    }

    if (STL_ClockRuntimeTest() != STL_OK) {
        STL_Printf("********    Test fail in runtime: Clock&INT    ********\r\n");
        FAIL_HANLDER();
    }

    if (STL_NonIntTestRuntime() != STL_OK) {
        STL_Printf("********    Test fail in runtime: NonInterrupt ********\r\n");
        FAIL_HANLDER();
    }
}

/**
 * @brief  Test feed watchdog.
 * @param  None
 * @retval None
 */
void TEST_FeedWatchdog(void)
{
    (void)STL_WdtRuntimeFeed();
}

/**
 * @brief  SysTick interrupt callback function.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
    g_u32SysTickCount++;

    STL_RuntimeTestCase(m_stcPeriodCaseTable,  ARRAY_SZ(m_stcPeriodCaseTable));

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
