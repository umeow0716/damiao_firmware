/**
 *******************************************************************************
 * @file  timer6/timer6_sw_sync/source/main.c
 * @brief This example demonstrates Timer6 software synchronize trigger function.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
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
 * @addtogroup TIMER6_Software_Synchronous
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* unlock/lock peripheral */
#define EXAMPLE_PERIPH_WE               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU)
#define EXAMPLE_PERIPH_WP               (LL_PERIPH_EFM | LL_PERIPH_FCG)

#define EXAMPLE_IRQN                    (INT002_IRQn)
#define EXAMPLE_TMR6_INT_SRC            (INT_SRC_TMR6_1_UDF)

#define TMR6_1_PWMA_PORT                (GPIO_PORT_A)
#define TMR6_1_PWMA_PIN                 (GPIO_PIN_08)
#define TMR6_1_PWMA_FUNC                (GPIO_FUNC_3)
#define TMR6_1_PWMB_PORT                (GPIO_PORT_A)
#define TMR6_1_PWMB_PIN                 (GPIO_PIN_07)
#define TMR6_1_PWMB_FUNC                (GPIO_FUNC_3)

#define TMR6_2_PWMA_PORT                (GPIO_PORT_A)
#define TMR6_2_PWMA_PIN                 (GPIO_PIN_09)
#define TMR6_2_PWMA_FUNC                (GPIO_FUNC_3)
#define TMR6_2_PWMB_PORT                (GPIO_PORT_B)
#define TMR6_2_PWMB_PIN                 (GPIO_PIN_00)
#define TMR6_2_PWMB_FUNC                (GPIO_FUNC_3)

#define TEST_TMR6_SW_SYNC               (TMR6_SW_SYNC_U1 | TMR6_SW_SYNC_U2)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/


/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  TIMER6 interrupt handler callback.
 * @param  None
 * @retval None
 */
static void Tmr6_UnderFlow_CallBack(void)
{
    static uint8_t i = 0U;

    if (0U == i) {
        TMR6_SetCompareValue(CM_TMR6_1, TMR6_CMP_REG_C, 0x2000U);
        TMR6_SetCompareValue(CM_TMR6_2, TMR6_CMP_REG_C, 0x2000U);
        i = 1U;
    } else {
        TMR6_SetCompareValue(CM_TMR6_1, TMR6_CMP_REG_C, 0x4000U);
        TMR6_SetCompareValue(CM_TMR6_2, TMR6_CMP_REG_C, 0x4000U);
        i = 0U;
    }
}

/**
 * @brief  Main function of TIMER6 compare output mode project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint32_t u32Compare;
    stc_tmr6_init_t stcTmr6Init;
    stc_tmr6_pwm_init_t stcPwmInit;
    stc_irq_signin_config_t stcIrqConfig;
    stc_tmr6_buf_config_t stcBufConfig;

    /* Unlock peripherals or registers */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    /* Configure BSP */
    BSP_CLK_Init();

    (void)TMR6_StructInit(&stcTmr6Init);
    (void)TMR6_PWM_StructInit(&stcPwmInit);
    (void)TMR6_BufFuncStructInit(&stcBufConfig);

    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR6_1 | FCG2_PERIPH_TMR6_2, ENABLE);

    /* Timer6 PWM pin configuration */
    GPIO_SetFunc(TMR6_1_PWMA_PORT, TMR6_1_PWMA_PIN, TMR6_1_PWMA_FUNC);
    GPIO_SetFunc(TMR6_1_PWMB_PORT, TMR6_1_PWMB_PIN, TMR6_1_PWMB_FUNC);

    GPIO_SetFunc(TMR6_2_PWMA_PORT, TMR6_2_PWMA_PIN, TMR6_2_PWMA_FUNC);
    GPIO_SetFunc(TMR6_2_PWMB_PORT, TMR6_2_PWMB_PIN, TMR6_2_PWMB_FUNC);

    TMR6_DeInit(CM_TMR6_1);
    TMR6_DeInit(CM_TMR6_2);

    /* Timer6 general count function configuration */
    stcTmr6Init.sw_count.u32CountMode = TMR6_MD_TRIANGLE;
    stcTmr6Init.sw_count.u32ClockDiv = TMR6_CLK_DIV1;
    stcTmr6Init.u32PeriodValue = 0x8340U;
    (void)TMR6_Init(CM_TMR6_1, &stcTmr6Init);
    (void)TMR6_Init(CM_TMR6_2, &stcTmr6Init);

    /* General compare buffer function configure */
    stcBufConfig.u32BufNum = TMR6_BUF_DUAL;
    stcBufConfig.u32BufTransCond = TMR6_BUF_TRANS_OVF;
    (void)TMR6_GeneralBufConfig(CM_TMR6_1, TMR6_CH_A, &stcBufConfig);
    (void)TMR6_GeneralBufConfig(CM_TMR6_2, TMR6_CH_A, &stcBufConfig);
    /* General compare buffer function command */
    TMR6_GeneralBufCmd(CM_TMR6_1, TMR6_CH_A, ENABLE);
    TMR6_GeneralBufCmd(CM_TMR6_2, TMR6_CH_A, ENABLE);
    /* Compare register set */
    u32Compare = 0x2000U;
    TMR6_SetCompareValue(CM_TMR6_1, TMR6_CMP_REG_C, u32Compare);
    TMR6_SetCompareValue(CM_TMR6_2, TMR6_CMP_REG_C, u32Compare);

    /* Configure PWM output */
    stcPwmInit.u32CompareValue = 0x2000U;
    stcPwmInit.u32StartPolarity = TMR6_PWM_LOW;
    stcPwmInit.u32StopPolarity = TMR6_PWM_LOW;
    stcPwmInit.u32CountDownMatchBPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32CountUpMatchBPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32CountDownMatchAPolarity = TMR6_PWM_INVT;
    stcPwmInit.u32CountUpMatchAPolarity = TMR6_PWM_INVT;
    stcPwmInit.u32UdfPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32OvfPolarity = TMR6_PWM_HOLD;
    (void)TMR6_PWM_Init(CM_TMR6_1, TMR6_CH_A, &stcPwmInit);
    (void)TMR6_PWM_Init(CM_TMR6_2, TMR6_CH_A, &stcPwmInit);
    stcPwmInit.u32CompareValue = 0x6000U;
    stcPwmInit.u32CountDownMatchBPolarity = TMR6_PWM_INVT;
    stcPwmInit.u32CountUpMatchBPolarity = TMR6_PWM_INVT;
    stcPwmInit.u32CountDownMatchAPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32CountUpMatchAPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32StartPolarity = TMR6_PWM_HIGH;
    (void)TMR6_PWM_Init(CM_TMR6_1, TMR6_CH_B, &stcPwmInit);
    (void)TMR6_PWM_Init(CM_TMR6_2, TMR6_CH_B, &stcPwmInit);

    /* PWM output command */
    TMR6_PWM_OutputCmd(CM_TMR6_1, TMR6_CH_A, ENABLE);
    TMR6_PWM_OutputCmd(CM_TMR6_2, TMR6_CH_A, ENABLE);
    TMR6_PWM_OutputCmd(CM_TMR6_1, TMR6_CH_B, ENABLE);
    TMR6_PWM_OutputCmd(CM_TMR6_2, TMR6_CH_B, ENABLE);

    /* Set PWMA PWMB output */
    TMR6_SetFunc(CM_TMR6_1, TMR6_CH_A, TMR6_PIN_CMP_OUTPUT);
    TMR6_SetFunc(CM_TMR6_2, TMR6_CH_A, TMR6_PIN_CMP_OUTPUT);
    TMR6_SetFunc(CM_TMR6_1, TMR6_CH_B, TMR6_PIN_CMP_OUTPUT);
    TMR6_SetFunc(CM_TMR6_2, TMR6_CH_B, TMR6_PIN_CMP_OUTPUT);

    /* ENABLE interrupt */
    TMR6_IntCmd(CM_TMR6_1, TMR6_INT_UDF, ENABLE);
    /* NVIC */
    stcIrqConfig.enIRQn = EXAMPLE_IRQN;
    stcIrqConfig.enIntSrc = EXAMPLE_TMR6_INT_SRC;
    stcIrqConfig.pfnCallback = &Tmr6_UnderFlow_CallBack;
    (void)INTC_IrqSignIn(&stcIrqConfig);
    NVIC_ClearPendingIRQ(EXAMPLE_IRQN);
    NVIC_SetPriority(EXAMPLE_IRQN, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(EXAMPLE_IRQN);

    for (;;) {
        /* Start timer6 */
        TMR6_SWSyncStart(TEST_TMR6_SW_SYNC);
        DDL_DelayMS(500UL);

        /* Stop timer6 */
        TMR6_SWSyncStop(TEST_TMR6_SW_SYNC);
        DDL_DelayMS(500UL);

        /* Clear timer6 count register */
        TMR6_SWSyncClear(TEST_TMR6_SW_SYNC);
        DDL_DelayMS(500UL);
    }

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
