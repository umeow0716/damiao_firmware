/**
 *******************************************************************************
 * @file  timera/timera_hard_trigger/source/main.c
 * @brief Main program TimerA hardware trigger for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2025-11-03       CDT             First version
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
 * @addtogroup TIMERA_Base_Timer
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* TimerA unit definitions for this example. */
#define TMRA_UNIT                       (CM_TMRA_1)
#define TMRA_PERIPH_ENABLE()            FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMRA_1, ENABLE)

/* The divider of the clock source. @ref TMRA_Clock_Divider */
#define TMRA_CLK_DIV                    (TMRA_CLK_DIV8)

/* The counting mode of TimerA. @ref TMRA_Count_Mode */
#define TMRA_MD                         (TMRA_MD_SAWTOOTH)

/* The counting direction of TimerA. @ref TMRA_Count_Dir */
#define TMRA_DIR                        (TMRA_DIR_DOWN)

/**
 * In this example:
 *   System clock is MRC@8MHz.
 *   TimerA clock is 8MHz.
 *   Timing period is 1ms.
 * A simple formula for calculating the compare value is:
 *   Sawtooth mode:
 *     TmrAPeriodVal = (TmrAPeriod(us) * [TmrAClockSource(MHz) / TmrAClockDivider]) - 1.
 *   Triangle mode:
 *     TmrAPeriodVal = (TmrAPeriod(us) * [TmrAClockSource(MHz) / TmrAClockDivider]) / 2.
 */
#define TMRA_PERIOD_VAL                 (1000U - 1U)

/**
 * Definitions about TimerA interrupt for the example.
 */
#define TMRA_INT_PRIO                   (DDL_IRQ_PRIO_DEFAULT)
#define TMRA_INT_IRQn                   (TMRA_1_OVF_UDF_IRQn)
#define TMRA_OVF_UDF_IRQ_HANDLER        TMRA_1_Ovf_Udf_Handler
#if (TMRA_DIR == TMRA_DIR_UP)
#define TMRA_INT_TYPE                   (TMRA_INT_OVF)
#define TMRA_INT_FLAG                   (TMRA_FLAG_OVF)
#elif (TMRA_DIR == TMRA_DIR_DOWN)
#define TMRA_INT_TYPE                   (TMRA_INT_UDF)
#define TMRA_INT_FLAG                   (TMRA_FLAG_UDF)
#endif

#define TMRA_START_COND                 (TMRA_START_COND_EVT)
/* BSP key K1 can generate the event */
#define TMRA_TRIG_EVT                   (BSP_KEY1_EVT)

#define TMRA_STOP_COND                  (TMRA_STOP_COND_TRIG_FALLING)
#define TMRA_TRIG_PORT                  (GPIO_PORT_A)
#define TMRA_TRIG_PIN                   (GPIO_PIN_12)
#define TMRA_TRIG_PIN_FUNC              (GPIO_FUNC_4)

/* Indicate pin definition in this example. */
#define INDICATE_PORT                   (GPIO_PORT_A)
#define INDICATE_PIN                    (GPIO_PIN_09)
#define INDICATE_OUT_TOGGLE()           (GPIO_TogglePins(INDICATE_PORT, INDICATE_PIN))

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void IndicateConfig(void);

static void TmrAConfig(void);
static void TmrAIrqConfig(void);
static void TmrATriggerCondConfig(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Main function of timera_hard_trigger project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);
    /* BSP key K1 can trigger TimerA start. */
    BSP_KEY_Init();
    /* Configures indicator. */
    IndicateConfig();
    /* Configures TimerA. */
    TmrAConfig();
    /* MCU Peripheral registers write protected. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);

    /* Starts TimerA. */
    /* Press BSP key K1 to start TimerA. */

    /***************** Configuration end, application start **************/

    for (;;) {
        /* See TMRA_IrqCallback in this file. */
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

    /* 2. Set a default initialization value for stcTmraInit. */
    (void)TMRA_StructInit(&stcTmraInit);

    /* 3. Modifies the initialization values depends on the application. */
    stcTmraInit.sw_count.u8ClockDiv  = TMRA_CLK_DIV;
    stcTmraInit.sw_count.u8CountMode = TMRA_MD;
    stcTmraInit.sw_count.u8CountDir  = TMRA_DIR;
    stcTmraInit.u32PeriodValue       = TMRA_PERIOD_VAL;
    (void)TMRA_Init(TMRA_UNIT, &stcTmraInit);
    /* Set the counter as period value while sawtooth mode count down */
    TMRA_SetCountValue(TMRA_UNIT, TMRA_PERIOD_VAL);

    /* 4. Configures IRQ if needed. */
    TmrAIrqConfig();

    /* 5. Configures hardware trigger condition if needed. */
    TMRA_IntCmd(TMRA_UNIT, TMRA_INT_TYPE, ENABLE);
    TmrATriggerCondConfig();
}

/**
 * @brief  TimerA interrupt configuration.
 * @param  None
 * @retval None
 */
static void TmrAIrqConfig(void)
{
    NVIC_ClearPendingIRQ(TMRA_INT_IRQn);
    NVIC_SetPriority(TMRA_INT_IRQn, TMRA_INT_PRIO);
    NVIC_EnableIRQ(TMRA_INT_IRQn);
}

/**
 * @brief  TimerA counter overflow/underflow IRQ handler.
 * @param  None
 * @retval None
 */
void TMRA_OVF_UDF_IRQ_HANDLER(void)
{
    if (TMRA_GetStatus(TMRA_UNIT, TMRA_INT_FLAG) == SET) {
        TMRA_ClearStatus(TMRA_UNIT, TMRA_INT_FLAG);
        INDICATE_OUT_TOGGLE();
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  Trigger condition configuration.
 * @param  None
 * @retval None
 */
static void TmrATriggerCondConfig(void)
{
    /* TimerA start-condition. */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);
    AOS_SetTriggerEventSrc(AOS_TMRA_0, TMRA_TRIG_EVT);
    TMRA_HWStartCondCmd(TMRA_UNIT, TMRA_START_COND, ENABLE);

    /* TimerA stop-condition. */
    GPIO_SetFunc(TMRA_TRIG_PORT, TMRA_TRIG_PIN, TMRA_TRIG_PIN_FUNC);
    /* Configures the filter of pin TRIG if needed. */
    TMRA_SetFilterClockDiv(TMRA_UNIT, TMRA_PIN_TRIG, TMRA_FILTER_CLK_DIV64);
    TMRA_FilterCmd(TMRA_UNIT, TMRA_PIN_TRIG, ENABLE);
    TMRA_HWStopCondCmd(TMRA_UNIT, TMRA_STOP_COND, ENABLE);
}

/**
 * @brief  Indicator configuration.
 * @param  None
 * @retval None
 */
static void IndicateConfig(void)
{
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    (void)GPIO_Init(INDICATE_PORT, INDICATE_PIN, &stcGpioInit);
    /* Output enable */
    GPIO_OutputCmd(INDICATE_PORT, INDICATE_PIN, ENABLE);
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
