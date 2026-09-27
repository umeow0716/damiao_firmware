/**
 *******************************************************************************
 * @file  timera/timera_compare_value_buffer/source/main.c
 * @brief Main program TimerA compare value buffer for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Set XTAL as system clock source
   2025-11-03       CDT             Example optimized.
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
#include "main.h"

/**
 * @addtogroup HC32F448_DDL_Examples
 * @{
 */

/**
 * @addtogroup TIMERA_Compare_Value_Buffer
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * TimerA unit and channel definitions for this example.
 *    |----------------|----------------|
 *    |  TMRA_BUF_CH   |   TMRA_DEST_CH |
 *    |----------------|----------------|
 *    |  TMRA_CH2      |   TMRA_CH1     |
 *    |----------------|----------------|
 *    |  TMRA_CH4      |   TMRA_CH3     |
 *    |----------------|----------------|
 *    |  TMRA_CH6      |   TMRA_CH5     |
 *    |----------------|----------------|
 *    |  TMRA_CH8      |   TMRA_CH7     |
 *    |----------------|----------------|
 */
#define TMRA_UNIT1                      (CM_TMRA_1)
#define TMRA_UNIT2                      (CM_TMRA_2)
#define TMRA_PERIPH_ENABLE()            FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMRA_1 | FCG2_PERIPH_TMRA_2, ENABLE)

#define TMRA1_BUF_CH                    (TMRA_CH2)
#define TMRA1_DEST_CH                   (TMRA_CH1)

#define TMRA2_BUF_CH                    (TMRA_CH2)
#define TMRA2_DEST_CH                   (TMRA_CH1)

#define TMRA1_CNT_MD                    TMRA_MD_SAWTOOTH
#define TMRA2_CNT_MD                    TMRA_MD_TRIANGLE

/* The divider of the clock source. @ref TMRA_Clock_Divider */
#define TMRA_CLK_DIV                    (TMRA_CLK_DIV1024)

/* Period value and compare value. */
#define TMRA1_PERIOD_VAL                (0x2000U)
#define TMRA1_SRC_CMP_VAL               (0x1000U)
#define TMRA2_PERIOD_VAL                (0x3000U)
#define TMRA2_SRC_CMP_VAL               (0x2000U)

/**
 * Compare value buffer condition.
 * 'TMRA_CMP_BUF_TRANS_COND' can be defined as a value of @ref TMRA_Cmp_Value_Buf_Trans_Cond
 */
#define TMRA1_CMP_BUF_TRANS_COND        (TMRA_BUF_TRANS_COND_OVF_UDF_CLR)
#define TMRA2_CMP_BUF_TRANS_COND        (TMRA_BUF_TRANS_COND_PEAK | TMRA_BUF_TRANS_COND_VALLEY)

/* Definitions about TimerA interrupt for the example. */
#define TMRA1_INT_OVF_TYPE              (TMRA_INT_OVF)
#define TMRA1_INT_OVF_FLAG              (TMRA_FLAG_OVF)
#define TMRA1_INT_OVF_PRIO              (DDL_IRQ_PRIO_DEFAULT)
#define TMRA1_INT_OVF_SRC               (INT_SRC_TMRA_1_OVF)
#define TMRA1_INT_OVF_IRQn              (INT010_IRQn)

#define TMRA2_INT_OVF_TYPE              (TMRA_INT_OVF)
#define TMRA2_INT_OVF_FLAG              (TMRA_FLAG_OVF)
#define TMRA2_INT_OVF_PRIO              (DDL_IRQ_PRIO_DEFAULT)
#define TMRA2_INT_OVF_SRC               (INT_SRC_TMRA_2_OVF)
#define TMRA2_INT_OVF_IRQn              (INT011_IRQn)

#define TMRA2_INT_UDF_TYPE              (TMRA_INT_UDF)
#define TMRA2_INT_UDF_FLAG              (TMRA_FLAG_UDF)
#define TMRA2_INT_UDF_PRIO              (DDL_IRQ_PRIO_DEFAULT)
#define TMRA2_INT_UDF_SRC               (INT_SRC_TMRA_2_UDF)
#define TMRA2_INT_UDF_IRQn              (INT012_IRQn)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void TmrAConfig(void);
static void TmrAIrqConfig(void);

static void TMRA1_Ovf_IrqCallback(void);
static void TMRA2_Ovf_IrqCallback(void);
static void TMRA2_Udf_IrqCallback(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
__IO static uint16_t m_u16CompareValueBuffer1 = TMRA1_SRC_CMP_VAL;
__IO static uint16_t m_u16CompareValueBuffer2 = TMRA2_SRC_CMP_VAL;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Main function of timera_compare_value_buffer project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_EFM);
    /* Initializes UART for debug printing. Baudrate is 115200. */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
    /* Configures TimerA. */
    TmrAConfig();
    /* MCU Peripheral registers write protected. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_EFM);

    /* Starts TimerA. */
    TMRA_Start(TMRA_UNIT1);
    TMRA_Start(TMRA_UNIT2);

    /***************** Configuration end, application start **************/
    for (;;) {
        /* See IrqCallback in this file. */
    }
}

/**
 * @brief  TimerA configuration.
 * @param  None
 * @retval None
 */
static void TmrAConfig(void)
{
    stc_tmra_init_t stcTmraInit;

    /* 1. Enable TimerA peripheral clock. */
    TMRA_PERIPH_ENABLE();

    /* 2. Initialize TimerA unit . */
    (void)TMRA_StructInit(&stcTmraInit);
    stcTmraInit.sw_count.u8ClockDiv  = TMRA_CLK_DIV;
    stcTmraInit.sw_count.u8CountMode = TMRA1_CNT_MD;
    stcTmraInit.sw_count.u8CountDir  = TMRA_DIR_UP;
    stcTmraInit.u32PeriodValue       = TMRA1_PERIOD_VAL;
    (void)TMRA_Init(TMRA_UNIT1, &stcTmraInit);
    TMRA_SetCompareValue(TMRA_UNIT1, TMRA1_BUF_CH, TMRA1_SRC_CMP_VAL);

    stcTmraInit.sw_count.u8CountMode = TMRA2_CNT_MD;
    stcTmraInit.u32PeriodValue       = TMRA2_PERIOD_VAL;
    (void)TMRA_Init(TMRA_UNIT2, &stcTmraInit);
    TMRA_SetCompareValue(TMRA_UNIT2, TMRA2_BUF_CH, TMRA2_SRC_CMP_VAL);
    TMRA_SetCountValue(TMRA_UNIT2, 1U);

    /* 3. Condition of compare value buffer transmission. */
    TMRA_SetCompareBufCond(TMRA_UNIT1, TMRA1_DEST_CH, TMRA1_CMP_BUF_TRANS_COND);
    TMRA_CompareBufCmd(TMRA_UNIT1, TMRA1_DEST_CH, ENABLE);
    TMRA_SetCompareBufCond(TMRA_UNIT2, TMRA2_DEST_CH, TMRA2_CMP_BUF_TRANS_COND);
    TMRA_CompareBufCmd(TMRA_UNIT2, TMRA2_DEST_CH, ENABLE);

    /* 4. Configures IRQ if needed. */
    TMRA_IntCmd(TMRA_UNIT1, TMRA1_INT_OVF_TYPE, ENABLE);
    TMRA_IntCmd(TMRA_UNIT2, TMRA2_INT_OVF_TYPE | TMRA2_INT_UDF_TYPE, ENABLE);
    TmrAIrqConfig();
}

/**
 * @brief  TimerA interrupt configuration.
 * @param  None
 * @retval None
 */
static void TmrAIrqConfig(void)
{
    stc_irq_signin_config_t stcIrq;

    stcIrq.enIntSrc    = TMRA1_INT_OVF_SRC;
    stcIrq.enIRQn      = TMRA1_INT_OVF_IRQn;
    stcIrq.pfnCallback = &TMRA1_Ovf_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrq);
    NVIC_ClearPendingIRQ(stcIrq.enIRQn);
    NVIC_SetPriority(stcIrq.enIRQn, TMRA1_INT_OVF_PRIO);
    NVIC_EnableIRQ(stcIrq.enIRQn);

    stcIrq.enIntSrc    = TMRA2_INT_OVF_SRC;
    stcIrq.enIRQn      = TMRA2_INT_OVF_IRQn;
    stcIrq.pfnCallback = &TMRA2_Ovf_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrq);
    NVIC_ClearPendingIRQ(stcIrq.enIRQn);
    NVIC_SetPriority(stcIrq.enIRQn, TMRA2_INT_OVF_PRIO);
    NVIC_EnableIRQ(stcIrq.enIRQn);

    stcIrq.enIntSrc    = TMRA2_INT_UDF_SRC;
    stcIrq.enIRQn      = TMRA2_INT_UDF_IRQn;
    stcIrq.pfnCallback = &TMRA2_Udf_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrq);
    NVIC_ClearPendingIRQ(stcIrq.enIRQn);
    NVIC_SetPriority(stcIrq.enIRQn, TMRA2_INT_UDF_PRIO);
    NVIC_EnableIRQ(stcIrq.enIRQn);
}

/**
 * @brief  TimerA1 counter overflow interrupt callback function.
 * @param  None
 * @retval None
 */
static void TMRA1_Ovf_IrqCallback(void)
{
    TMRA_ClearStatus(TMRA_UNIT1, TMRA1_INT_OVF_FLAG);
    DDL_Printf("TimerA1 OVF Irq callback get current compare value: %u\r\n",
               (unsigned int)TMRA_GetCompareValue(TMRA_UNIT1, TMRA1_DEST_CH));
    m_u16CompareValueBuffer1++;
    TMRA_SetCompareValue(TMRA_UNIT1, TMRA1_BUF_CH, m_u16CompareValueBuffer1);
    DDL_Printf("TimerA1 OVF Irq callback set compare value buffer: %u\r\n", m_u16CompareValueBuffer1);
}

/**
 * @brief  TimerA2 counter overflow interrupt callback function.
 * @param  None
 * @retval None
 */
static void TMRA2_Ovf_IrqCallback(void)
{
    TMRA_ClearStatus(TMRA_UNIT2, TMRA2_INT_OVF_FLAG);
    DDL_Printf("TimerA2 OVF Irq callback get current compare value: %u\r\n",
               (unsigned int)TMRA_GetCompareValue(TMRA_UNIT2, TMRA2_DEST_CH));
    m_u16CompareValueBuffer2++;
    TMRA_SetCompareValue(TMRA_UNIT2, TMRA2_BUF_CH, m_u16CompareValueBuffer2);
    DDL_Printf("TimerA2 OVF Irq callback set compare value buffer: %u\r\n", m_u16CompareValueBuffer2);
}

/**
 * @brief  TimerA2 counter underflow interrupt callback function.
 * @param  None
 * @retval None
 */
static void TMRA2_Udf_IrqCallback(void)
{
    TMRA_ClearStatus(TMRA_UNIT2, TMRA2_INT_UDF_FLAG);
    DDL_Printf("TimerA2 UDF Irq callback get current compare value: %u\r\n",
               (unsigned int)TMRA_GetCompareValue(TMRA_UNIT2, TMRA2_DEST_CH));
    m_u16CompareValueBuffer2++;
    TMRA_SetCompareValue(TMRA_UNIT2, TMRA2_BUF_CH, m_u16CompareValueBuffer2);
    DDL_Printf("TimerA2 UDF Irq callback set compare value buffer: %u\r\n", m_u16CompareValueBuffer2);
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
