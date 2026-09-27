/**
 *******************************************************************************
 * @file  timer6/timer6_pwm_dynamic_dutycycle/source/main.c
 * @brief This example demonstrates how to dynamically modify the duty cycle output using timer6.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-12-15       CDT             First version
   2025-11-03       CDT             Optimize the Duty_Set() function
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
 * @addtogroup TIMER6_Pwm_Dynamic_Dutycycle
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* unlock/lock peripheral */
#define EXAMPLE_PERIPH_WE               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |\
                                         LL_PERIPH_SRAM)
#define EXAMPLE_PERIPH_WP               (LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_SRAM)

#define TMR6_1_PWMA_PORT                (GPIO_PORT_A)
#define TMR6_1_PWMA_PIN                 (GPIO_PIN_08)
#define TMR6_1_PWMA_FUNC                (GPIO_FUNC_3)
#define TMR6_1_PWMB_PORT                (GPIO_PORT_A)
#define TMR6_1_PWMB_PIN                 (GPIO_PIN_07)
#define TMR6_1_PWMB_FUNC                (GPIO_FUNC_3)

#define PWM_PERIODVALUE                 (3125U)    /* Pwm period = 1KHz = 200MHz/32/1000/2 (/2 because of triangle mode) */

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static uint8_t u8Duty_pos = 1U;
static uint16_t au16Cmp_buf[] = {PWM_PERIODVALUE / 10, PWM_PERIODVALUE / 10, 0, PWM_PERIODVALUE / 10, PWM_PERIODVALUE / 10, PWM_PERIODVALUE};
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Duty cycle setting.
 * @param  [in]  TMR6x                  Pointer to TMR6 instance register base.
 * @param  [in]  u32Index               General compare register to be set. @ref TMR6_Compare_Reg_Index_Define
 * @param  [in]  cmp_value              The comparison value to be set.
 * @retval None
 */
static void Duty_Set(CM_TMR6_TypeDef *TMR6x, uint32_t u32Index, uint16_t cmp_value)
{
    if (TMR6_GetCompareValue(TMR6x, u32Index) != cmp_value) {
        TMR6_SetCompareValue(TMR6x, u32Index, cmp_value);
        if (cmp_value == 0U) {
            if (u32Index == TMR6_CMP_REG_D) {
                TMR6_PWM_SetPolarity(TMR6x, TMR6_CH_B, TMR6_STAT_DOWN_CNT_MATCH_B, TMR6_PWM_HOLD);
            } else {
                TMR6_PWM_SetPolarity(TMR6x, TMR6_CH_A, TMR6_STAT_DOWN_CNT_MATCH_A, TMR6_PWM_HOLD);
            }
        } else if (cmp_value == PWM_PERIODVALUE) {
            if (u32Index == TMR6_CMP_REG_D) {
                TMR6_PWM_SetPolarity(TMR6x, TMR6_CH_B, TMR6_STAT_OVF, TMR6_PWM_INVT);
            } else {
                TMR6_PWM_SetPolarity(TMR6x, TMR6_CH_A, TMR6_STAT_OVF, TMR6_PWM_INVT);
            }
        } else {
            if (u32Index == TMR6_CMP_REG_D) {
                TMR6_PWM_SetPolarity(TMR6x, TMR6_CH_B, TMR6_STAT_OVF, TMR6_PWM_HOLD);
                TMR6_PWM_SetPolarity(TMR6x, TMR6_CH_B, TMR6_STAT_DOWN_CNT_MATCH_B, TMR6_PWM_HIGH);
            } else {
                TMR6_PWM_SetPolarity(TMR6x, TMR6_CH_A, TMR6_STAT_OVF, TMR6_PWM_HOLD);
                TMR6_PWM_SetPolarity(TMR6x, TMR6_CH_A, TMR6_STAT_DOWN_CNT_MATCH_A, TMR6_PWM_HIGH);
            }
        }
    }
}

/**
 * @brief  TIMER6 underflow interrupt handler callback.
 * @param  None
 * @retval None
 */
static void Tmr6_UnderFlow_CallBack(void)
{
    TMR6_ClearStatus(CM_TMR6_1, TMR6_FLAG_UDF);
    Duty_Set(CM_TMR6_1, TMR6_CMP_REG_C, au16Cmp_buf[u8Duty_pos]);
    Duty_Set(CM_TMR6_1, TMR6_CMP_REG_D, au16Cmp_buf[u8Duty_pos]);
    if (++u8Duty_pos >= sizeof(au16Cmp_buf) / sizeof(au16Cmp_buf[0])) {
        u8Duty_pos = 0;
    }
}

/**
 * @brief  Main function of TIMER6 compare output mode project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    stc_tmr6_init_t stcTmr6Init;
    stc_tmr6_pwm_init_t stcPwmInit;
    stc_irq_signin_config_t stcIrqRegiConf;
    stc_tmr6_buf_config_t stcBufConfig;

    /* Unlock peripherals or registers */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    /* Configure BSP */
    BSP_CLK_Init();

    (void)TMR6_StructInit(&stcTmr6Init);
    (void)TMR6_PWM_StructInit(&stcPwmInit);
    (void)TMR6_BufFuncStructInit(&stcBufConfig);

    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR6_1, ENABLE);

    /* Timer6 PWM output port configuration */
    GPIO_SetFunc(TMR6_1_PWMA_PORT, TMR6_1_PWMA_PIN, TMR6_1_PWMA_FUNC);
    GPIO_SetFunc(TMR6_1_PWMB_PORT, TMR6_1_PWMB_PIN, TMR6_1_PWMB_FUNC);

    TMR6_DeInit(CM_TMR6_1);
    /* Timer6 general count function configuration */
    stcTmr6Init.sw_count.u32CountMode = TMR6_MD_TRIANGLE;
    stcTmr6Init.sw_count.u32ClockDiv = TMR6_CLK_DIV32;
    stcTmr6Init.u32PeriodValue = PWM_PERIODVALUE;
    (void)TMR6_Init(CM_TMR6_1, &stcTmr6Init);

    /* General compare buffer function configure */
    stcBufConfig.u32BufNum = TMR6_BUF_SINGLE;
    stcBufConfig.u32BufTransCond = TMR6_BUF_TRANS_OVF;
    (void)TMR6_GeneralBufConfig(CM_TMR6_1, TMR6_CH_A, &stcBufConfig);
    (void)TMR6_GeneralBufConfig(CM_TMR6_1, TMR6_CH_B, &stcBufConfig);
    TMR6_GeneralBufCmd(CM_TMR6_1, TMR6_CH_A, ENABLE);
    TMR6_GeneralBufCmd(CM_TMR6_1, TMR6_CH_B, ENABLE);
    /* Set General Compare RegisterA Value */
    TMR6_SetCompareValue(CM_TMR6_1, TMR6_CMP_REG_C, au16Cmp_buf[0]); /* General comprare register C, buffer for GCMAR */
    TMR6_SetCompareValue(CM_TMR6_1, TMR6_CMP_REG_D, au16Cmp_buf[0]); /* General comprare register D, buffer for GCMBR */

    /* Configure PWM output CHA */
    stcPwmInit.u32CompareValue = au16Cmp_buf[0];
    stcPwmInit.u32CountDownMatchBPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32CountUpMatchBPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32CountDownMatchAPolarity = TMR6_PWM_HIGH;
    stcPwmInit.u32CountUpMatchAPolarity = TMR6_PWM_LOW;
    stcPwmInit.u32UdfPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32OvfPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32StopPolarity = TMR6_PWM_LOW;
    stcPwmInit.u32StartPolarity = TMR6_PWM_LOW;
    (void)TMR6_PWM_Init(CM_TMR6_1, TMR6_CH_A, &stcPwmInit);
    /* CHB */
    stcPwmInit.u32CompareValue = au16Cmp_buf[0];
    stcPwmInit.u32CountDownMatchBPolarity = TMR6_PWM_HIGH;
    stcPwmInit.u32CountUpMatchBPolarity = TMR6_PWM_LOW;
    stcPwmInit.u32CountDownMatchAPolarity = TMR6_PWM_HOLD;
    stcPwmInit.u32CountUpMatchAPolarity = TMR6_PWM_HOLD;
    (void)TMR6_PWM_Init(CM_TMR6_1, TMR6_CH_B, &stcPwmInit);
    /* PWM pin function set */
    TMR6_SetFunc(CM_TMR6_1, TMR6_CH_A, TMR6_PIN_CMP_OUTPUT);
    TMR6_SetFunc(CM_TMR6_1, TMR6_CH_B, TMR6_PIN_CMP_OUTPUT);
    /* PWM output command */
    TMR6_PWM_OutputCmd(CM_TMR6_1, TMR6_CH_A, ENABLE);
    TMR6_PWM_OutputCmd(CM_TMR6_1, TMR6_CH_B, ENABLE);

    /* Enable interrupt */
    TMR6_IntCmd(CM_TMR6_1, TMR6_INT_UDF, ENABLE);

    stcIrqRegiConf.enIRQn = INT002_IRQn;
    stcIrqRegiConf.enIntSrc = INT_SRC_TMR6_1_UDF;
    stcIrqRegiConf.pfnCallback = &Tmr6_UnderFlow_CallBack;
    (void)INTC_IrqSignIn(&stcIrqRegiConf);
    NVIC_ClearPendingIRQ(stcIrqRegiConf.enIRQn);
    NVIC_SetPriority(stcIrqRegiConf.enIRQn, DDL_IRQ_PRIO_15);
    NVIC_EnableIRQ(stcIrqRegiConf.enIRQn);

    /* Start timer6 */
    TMR6_Start(CM_TMR6_1);

    for (;;) {
        ;
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
