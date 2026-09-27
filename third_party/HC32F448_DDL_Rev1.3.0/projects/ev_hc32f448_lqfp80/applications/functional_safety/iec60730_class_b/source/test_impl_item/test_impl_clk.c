/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/test_impl_item/test_impl_clk.c
 * @brief This file provides firmware functions to implement the clock test.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Initialize XTAL32 using BSP_XTAL32_Init
   2025-11-03       CDT             FCM interface add instance
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
#include "test_impl_clk.h"


/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/**
 * @defgroup Test_Implement_Clock Test Implement Clock
 * @{
 */

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define REF_CLK_FREQ                (XTAL32_VALUE)
#define REF_CLK_DIV                 (32UL)

#define TARGET_CLK_DIV              (32UL)
#define TARGET_CLK_FREQ             (SystemCoreClock)

#define TMR_CONFIG_DLY              (300UL)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
stc_clock_test_para_t stcClockTestPara;

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void TMR0_Cmp_IrqCallback(void);
static void TMRA_Ovf_IrqCallback(void);
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static __IO uint32_t m_u32ClockErrCount = 0UL;
static __IO uint8_t m_u32FmcActived = STL_OFF;

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @defgroup Test_Implement_Clock_Global_Functions Test Implement Clock Global Functions
 * @{
 */

/**
 * @brief  FCM reference clock XTAL32 initialize
 * @param  None
 * @retval None
 */
static void RefClockInit(void)
{
    BSP_XTAL32_Init();
    FCM_DeInit(CM_FCM);
}

/**
 * @brief  FCM frequency error IRQ callback
 * @param  None
 * @retval None
 */
static void FCM_Error_IrqCallback(void)
{
    FCM_Cmd(CM_FCM, DISABLE);

    m_u32ClockErrCount++;
    m_u32FmcActived = STL_OFF;

    FCM_ClearStatus(CM_FCM, FCM_FLAG_ERR);
}

/**
 * @brief  FCM measure counter overflow IRQ callback
 * @param  None
 * @retval None
 */
static void FCM_Ovf_IrqCallback(void)
{
    FCM_Cmd(CM_FCM, DISABLE);

    m_u32ClockErrCount++;
    m_u32FmcActived = STL_OFF;

    FCM_ClearStatus(CM_FCM, FCM_FLAG_OVF);
}

/**
 * @brief  Clock test initialize in runtime.
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          Initialize successfully.
 */
uint32_t STL_ClkFcmRuntimeInit(void)
{
    stc_fcm_init_t stcFcmInit;
    stc_irq_signin_config_t stcIrqSignConfig;

    RefClockInit();

    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_FCM, ENABLE);

    (void)FCM_StructInit(&stcFcmInit);
    stcFcmInit.u32RefClock     = FCM_REF_CLK_XTAL32;
    stcFcmInit.u32RefClockDiv  = FCM_REF_CLK_DIV32;
    stcFcmInit.u32RefClockEdge = FCM_REF_CLK_RISING;
    stcFcmInit.u32TargetClock  = FCM_TARGET_CLK_PLLHP;
    stcFcmInit.u32ExceptionType = FCM_EXP_TYPE_INT;
    stcFcmInit.u32TargetClockDiv = FCM_TARGET_CLK_DIV32;

    /* Idea count value = (targ_freq/tar_div)/(ref_freq/ref_div) */
    stcFcmInit.u16LowerLimit = (uint16_t)((((TARGET_CLK_FREQ / TARGET_CLK_DIV) / (REF_CLK_FREQ / REF_CLK_DIV)) * 97UL) / 100UL);
    stcFcmInit.u16UpperLimit = (uint16_t)((((TARGET_CLK_FREQ / TARGET_CLK_DIV) / (REF_CLK_FREQ / REF_CLK_DIV)) * 103UL) / 100UL);

    (void)FCM_Init(CM_FCM, &stcFcmInit);
    FCM_IntCmd(CM_FCM, (FCM_INT_OVF | FCM_INT_ERR), ENABLE);

    stcIrqSignConfig.enIntSrc = INT_SRC_FCMFERRI;
    stcIrqSignConfig.enIRQn   = STL_FCM_ERR_INT_IRQn;
    stcIrqSignConfig.pfnCallback = &FCM_Error_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, STL_FCM_ERR_IRQ_PRIO);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);

    stcIrqSignConfig.enIntSrc = INT_SRC_FCMCOVFI;
    stcIrqSignConfig.enIRQn   = STL_FCM_OVF_INT_IRQn;
    stcIrqSignConfig.pfnCallback = &FCM_Ovf_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, STL_FCM_OVF_IRQ_PRIO);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);

    return STL_OK;
}

/**
 * @brief  Clock test in runtime.
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          Test pass.
 *           - STL_ERR:         Test fail.
 */
uint32_t STL_ClkFcmRuntimeTest(void)
{
    if (STL_OFF == m_u32FmcActived) {
        m_u32ClockErrCount = 0UL;
        m_u32FmcActived = STL_ON;

        FCM_Cmd(CM_FCM, ENABLE);
    }

    return (m_u32ClockErrCount == 0UL) ? STL_OK : STL_ERR;
}

/**
 * @brief  Clock pamameter initialize
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          Initialize successfully.
 */
static uint32_t STL_ClkParaInit(void)
{
    stcClockTestPara.u16ClkTestUpperLimit = (uint16_t)(STL_TMRA_COUNT_HIGH);
    stcClockTestPara.u16ClkTestLowerLimit = (uint16_t)(STL_TMRA_COUNT_LOW);
    stcClockTestPara.m_u32TmrCount = 0UL;
    stcClockTestPara.u8TestFailJudge = 1UL;
    stcClockTestPara.u16TestFailCnt  = 0UL;
    stcClockTestPara.u8FlagClockTestFail = 0UL;
    stcClockTestPara.u16ClkTestIrqCnt = 0UL;
    stcClockTestPara.u32NoneIntTestCnt = 0UL;
    stcClockTestPara.u32NonIntTestJudge = 0x100000UL;
    return STL_OK;
}


/**
 * @brief  Clock test initialize in startup.
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          Initialize successfully.
 */
uint32_t STL_ClockTestInit(void)
{
    stc_tmr0_init_t stcTmr0Init;
    stc_tmra_init_t stcTmraInit;
    stc_irq_signin_config_t stcIrq;

    /* Initialize parameters */
    STL_ClkParaInit();

    /* Enable LRC */
    CLK_LrcCmd(ENABLE);

    /* Stop count when core halt */
    DBGC_PeriphCmd(DBGC_PERIPH_TMR0_2, DISABLE);
    DBGC_Periph2Cmd(DBGC_PERIPH_TMRA_1, DISABLE);
    /*******************Ref Timer Init****************************/
    /* Enable Timer0 clock */
    STL_TMR0_FCG_ENABLE();

    /* Timer0 configuration */
    (void)TMR0_StructInit(&stcTmr0Init);
    stcTmr0Init.u32ClockSrc     = TMR0_CLK_SRC_LRC;
    stcTmr0Init.u32ClockDiv     = STL_TMR0_CLK_DIV;
    stcTmr0Init.u32Func         = TMR0_FUNC_CMP;
    stcTmr0Init.u16CompareValue = STL_TMR0_PERIOD_VALUE;
    (void)TMR0_Init(STL_TMR0_UNIT, STL_TMR0_CH, &stcTmr0Init);
    /* Delay 6 asyn clock at least for Timer0 asyn counter */
    STL_DelayUS(TMR_CONFIG_DLY);

    TMR0_IntCmd(STL_TMR0_UNIT, STL_TMR0_INT, ENABLE);
    /* Delay 6 asyn clock at least for Timer0 asyn counter */
    STL_DelayUS(TMR_CONFIG_DLY);

    /* Interrupt configuration */
    stcIrq.enIntSrc    = STL_TMR0_OVF_INT_SRC;
    stcIrq.enIRQn      = STL_TMR0_OVF_INT_IRQn;
    stcIrq.pfnCallback = &TMR0_Cmp_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrq);
    NVIC_ClearPendingIRQ(stcIrq.enIRQn);
    NVIC_SetPriority(stcIrq.enIRQn, STL_TMR0_OVF_IRQ_PRIO);
    NVIC_EnableIRQ(stcIrq.enIRQn);

    /*******************Sys Timer Init****************************/
    /* Enable TimerA clock */
    STL_TMRA_FCG_ENABLE();

    /* TimerA configuration */
    (void)TMRA_StructInit(&stcTmraInit);
    stcTmraInit.sw_count.u8ClockDiv  = STL_TMRA_CLK_DIV;
    stcTmraInit.sw_count.u8CountMode = STL_TMRA_MD;
    stcTmraInit.sw_count.u8CountDir  = STL_TMRA_DIR;
    stcTmraInit.u32PeriodValue       = STL_TMRA_PERIOD_VALUE;
    (void)TMRA_Init(STL_TMRA_UNIT, &stcTmraInit);
    TMRA_IntCmd(STL_TMRA_UNIT, STL_TMRA_INT, ENABLE);

    /* Interrupt configuration */
    stcIrq.enIntSrc    = STL_TMRA_OVF_INT_SRC;
    stcIrq.enIRQn      = STL_TMRA_OVF_INT_IRQn;
    stcIrq.pfnCallback = &TMRA_Ovf_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrq);
    NVIC_ClearPendingIRQ(stcIrq.enIRQn);
    NVIC_SetPriority(stcIrq.enIRQn, STL_TMRA_OVF_IRQ_PRIO);
    NVIC_EnableIRQ(stcIrq.enIRQn);

    /* Start Timer */
    TMR0_Start(STL_TMR0_UNIT, STL_TMR0_CH);
    TMRA_Start(STL_TMRA_UNIT);
    return STL_OK;
}

/**
 * @brief  Clock test de-initialize.
 * @param  None
 * @retval uint32_t:
 *           - STL_OK:          De-initialize successfully.
 */
uint32_t STL_ClockTestDeInit(void)
{
    TMRA_Stop(STL_TMRA_UNIT);
    TMR0_Stop(STL_TMR0_UNIT, STL_TMR0_CH);
    /* Delay 6 asyn clock at least for Timer0 asyn counter */
    STL_DelayUS(TMR_CONFIG_DLY);

    TMRA_DeInit(STL_TMRA_UNIT);
    TMR0_DeInit(STL_TMR0_UNIT);
    return STL_OK;
}

/**
 * @brief  Timer0 compare IRQ callback
 * @param  None
 * @retval None
 */
static void TMR0_Cmp_IrqCallback(void)
{
    if (STL_ERR == STL_ClockTest()) {
        stcClockTestPara.u8FlagClockTestFail = 1UL;
    }
    TMR0_ClearStatus(STL_TMR0_UNIT, STL_TMR0_FLAG);
}

/**
 * @brief  TimerA overflow IRQ callback
 * @param  None
 * @retval None
 */
static void TMRA_Ovf_IrqCallback(void)
{
    STL_SysTimerIrq();

    TMRA_ClearStatus(STL_TMRA_UNIT, STL_TMRA_FLAG);
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
