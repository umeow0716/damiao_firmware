/**
 *******************************************************************************
 * @file  timera/timera_pwm/source/main.c
 * @brief Main program TimerA PWM for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
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
 * @addtogroup TIMERA_PWM
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define TMRA_UNIT1                          (CM_TMRA_1)
#define TMRA_UNIT2                          (CM_TMRA_2)
#define TMRA_UNIT3                          (CM_TMRA_3)
#define TMRA_FCG_PERIPH                     (FCG2_PERIPH_TMRA_1 | FCG2_PERIPH_TMRA_2 | FCG2_PERIPH_TMRA_3)
#define TMRA_PERIPH_ENABLE()                FCG_Fcg2PeriphClockCmd(TMRA_FCG_PERIPH, ENABLE)

/* Function of this example. */
/* TimerA1: normal single PWM
   TimerA2: single edge aligned PWM
   TimerA3: two edge symmetric PWM */

/**
 * Define the configurations of PWM according to the function that selected.
 * In this example:
 *   1. System clock is 200MHz.
 *   2. Clock source of TimerA is PCLK0(200MHz by default) and divided by 1.
 *   3. About PWM:
 *      TimerA1 normal single PWM: frequency 200KHz, high duty 30%
 *      TimerA2 single edge aligned PWM: frequency 100KH, high duty 30%, 55%
 *      TimerA3 two edge symmetric PWM: frequency 50KHz, high duty 50%, 40%
 *
 * Sawtooth mode:
 *   Calculate the period value according to the frequency:
 *     PeriodVal = (TimerAClockFrequency(Hz) / PWMFreq) - 1
 *   Calculate the compare value according to the duty ratio:
 *     CmpVal = ((PeriodVal + 1) * Duty) - 1
 *
 * Triangle mode:
 *   Calculate the period value according to the frequency:
 *     PeriodVal = (TimerAClockFrequency(Hz) / (PWMFreq * 2))
 *   Calculate the compare value according to the duty ratio:
 *     CmpVal = (PeriodVal * Duty)
 */
/* Definitions for Normal Single PWM */
#define TMRA1_PWM_CH                    (TMRA_CH1)
#define TMRA1_PWM_PORT                  (GPIO_PORT_A)
#define TMRA1_PWM_PIN                   (GPIO_PIN_08)
#define TMRA1_PWM_PIN_FUNC              (GPIO_FUNC_4)

#define TMRA1_MD                        (TMRA_MD_SAWTOOTH)
#define TMRA1_DIR                       (TMRA_DIR_UP)
#define TMRA1_PERIOD_VAL                (1000U - 1U)
#define TMRA1_PWM_CMP_VAL               (300U - 1U)

/* Definitions for Single Edge Aligned PWM */
#define TMRA2_PWMX_CH                   (TMRA_CH1)
#define TMRA2_PWMY_CH                   (TMRA_CH2)

#define TMRA2_PWMX_PORT                 (GPIO_PORT_A)
#define TMRA2_PWMX_PIN                  (GPIO_PIN_00)
#define TMRA2_PWMX_PIN_FUNC             (GPIO_FUNC_4)
#define TMRA2_PWMY_PORT                 (GPIO_PORT_A)
#define TMRA2_PWMY_PIN                  (GPIO_PIN_01)
#define TMRA2_PWMY_PIN_FUNC             (GPIO_FUNC_4)

#define TMRA2_MD                        (TMRA_MD_SAWTOOTH)
#define TMRA2_DIR                       (TMRA_DIR_UP)
#define TMRA2_PERIOD_VAL                (2000U - 1U)
#define TMRA2_PWMX_CMP_VAL              (600U - 1U)
#define TMRA2_PWMY_CMP_VAL              (1100U - 1U)

/* Definitions for Two Edges Symmetric PWM */
#define TMRA3_PWMX_CH                   (TMRA_CH1)
#define TMRA3_PWMY_CH                   (TMRA_CH2)

#define TMRA3_PWMX_PORT                 (GPIO_PORT_A)
#define TMRA3_PWMX_PIN                  (GPIO_PIN_06)
#define TMRA3_PWMX_PIN_FUNC             (GPIO_FUNC_5)
#define TMRA3_PWMY_PORT                 (GPIO_PORT_A)
#define TMRA3_PWMY_PIN                  (GPIO_PIN_07)
#define TMRA3_PWMY_PIN_FUNC             (GPIO_FUNC_5)

#define TMRA3_MD                        (TMRA_MD_TRIANGLE)
#define TMRA3_DIR                       (TMRA_DIR_UP)
#define TMRA3_PERIOD_VAL                (2000U)
#define TMRA3_PWMX_CMP_VAL              (1000U)
#define TMRA3_PWMY_CMP_VAL              (1200U)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void TmrAConfig(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Main function of timera_pwm project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_EFM);
    /* BSP clock init. */
    BSP_CLK_Init();
    /* Configures TimerA. */
    TmrAConfig();
    /* MCU Peripheral registers write protected. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_EFM);
    /* Starts TimerA. */
    TMRA_Start(TMRA_UNIT1);
    TMRA_Start(TMRA_UNIT2);
    TMRA_Start(TMRA_UNIT3);

    /***************** Configuration end, application start **************/
    for (;;) {
        /**
         * Stop PWM output:
         *   TMRA_Stop(TMRA_UNIT);
         *   or
         *   TMRA_PWM_OutputCmd(TMRA_UNIT, TMRA_PWM_x_CH, DISABLE);
         */
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
    stc_tmra_pwm_init_t stcPwmInit;

    /* Enable TimerA peripheral clock. */
    TMRA_PERIPH_ENABLE();

    /* Set a default initialization value for stcTmraInit. */
    (void)TMRA_StructInit(&stcTmraInit);

    /* Initializes TimerA1. */
    stcTmraInit.sw_count.u8CountMode = TMRA1_MD;
    stcTmraInit.sw_count.u8CountDir  = TMRA1_DIR;
    stcTmraInit.u32PeriodValue = TMRA1_PERIOD_VAL;
    (void)TMRA_Init(TMRA_UNIT1, &stcTmraInit);

    (void)TMRA_PWM_StructInit(&stcPwmInit);
    stcPwmInit.u32CompareValue = TMRA1_PWM_CMP_VAL;
    GPIO_SetFunc(TMRA1_PWM_PORT, TMRA1_PWM_PIN, TMRA1_PWM_PIN_FUNC);
    (void)TMRA_PWM_Init(TMRA_UNIT1, TMRA1_PWM_CH, &stcPwmInit);
    TMRA_PWM_OutputCmd(TMRA_UNIT1, TMRA1_PWM_CH, ENABLE);

    /* Initializes TimerA2. */
    stcTmraInit.sw_count.u8CountMode = TMRA2_MD;
    stcTmraInit.sw_count.u8CountDir  = TMRA2_DIR;
    stcTmraInit.u32PeriodValue = TMRA2_PERIOD_VAL;
    (void)TMRA_Init(TMRA_UNIT2, &stcTmraInit);

    (void)TMRA_PWM_StructInit(&stcPwmInit);
    stcPwmInit.u32CompareValue = TMRA2_PWMX_CMP_VAL;
    GPIO_SetFunc(TMRA2_PWMX_PORT, TMRA2_PWMX_PIN, TMRA2_PWMX_PIN_FUNC);
    (void)TMRA_PWM_Init(TMRA_UNIT2, TMRA2_PWMX_CH, &stcPwmInit);
    TMRA_PWM_OutputCmd(TMRA_UNIT2, TMRA2_PWMX_CH, ENABLE);

    (void)TMRA_PWM_StructInit(&stcPwmInit);
    stcPwmInit.u32CompareValue = TMRA2_PWMY_CMP_VAL;
    GPIO_SetFunc(TMRA2_PWMY_PORT, TMRA2_PWMY_PIN, TMRA2_PWMY_PIN_FUNC);
    (void)TMRA_PWM_Init(TMRA_UNIT2, TMRA2_PWMY_CH, &stcPwmInit);
    TMRA_PWM_OutputCmd(TMRA_UNIT2, TMRA2_PWMY_CH, ENABLE);

    /* Initializes TimerA3. */
    stcTmraInit.sw_count.u8CountMode = TMRA3_MD;
    stcTmraInit.sw_count.u8CountDir  = TMRA3_DIR;
    stcTmraInit.u32PeriodValue = TMRA3_PERIOD_VAL;
    (void)TMRA_Init(TMRA_UNIT3, &stcTmraInit);
    /* Initializes counter value as 1 while triangle counting mode */
    TMRA_SetCountValue(TMRA_UNIT3, 1UL);

    (void)TMRA_PWM_StructInit(&stcPwmInit);
    stcPwmInit.u32CompareValue        = TMRA3_PWMX_CMP_VAL;
    stcPwmInit.u16StartPolarity       = TMRA_PWM_HIGH;
    stcPwmInit.u16StopPolarity        = TMRA_PWM_HIGH;
    stcPwmInit.u16PeriodMatchPolarity = TMRA_PWM_HOLD;
    GPIO_SetFunc(TMRA3_PWMX_PORT, TMRA3_PWMX_PIN, TMRA3_PWMX_PIN_FUNC);
    (void)TMRA_PWM_Init(TMRA_UNIT3, TMRA3_PWMX_CH, &stcPwmInit);
    TMRA_PWM_OutputCmd(TMRA_UNIT3, TMRA3_PWMX_CH, ENABLE);

    stcPwmInit.u32CompareValue  = TMRA3_PWMY_CMP_VAL;
    stcPwmInit.u16StartPolarity = TMRA_PWM_LOW;
    stcPwmInit.u16StopPolarity  = TMRA_PWM_LOW;
    GPIO_SetFunc(TMRA3_PWMY_PORT, TMRA3_PWMY_PIN, TMRA3_PWMY_PIN_FUNC);
    (void)TMRA_PWM_Init(TMRA_UNIT3, TMRA3_PWMY_CH, &stcPwmInit);
    TMRA_PWM_OutputCmd(TMRA_UNIT3, TMRA3_PWMY_CH, ENABLE);
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
