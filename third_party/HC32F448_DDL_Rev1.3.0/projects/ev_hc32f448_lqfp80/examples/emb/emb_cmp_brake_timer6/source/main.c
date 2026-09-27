/**
 *******************************************************************************
 * @file  emb/emb_cmp_brake_timer6/source/main.c
 * @brief This example demonstrates how to use CMP brake function of EMB
 *        function.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Fix magic number
   2025-11-03       CDT             Delete DAC_DataRegAlignConfig() due to align configure in DAC_Init()
                                    Add DAC_StructInit() before DAC_Init()
                                    Optimize EMB IRQ callback register
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
 * @addtogroup EMB_CMP_Brake_TMR6
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/* Peripheral register WE/WP selection */
#define LL_PERIPH_SEL                   (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                                         LL_PERIPH_EFM)

/* CMP pin definition */
#define CMP_INP_PORT                    (GPIO_PORT_A)
#define CMP_INP_PIN                     (GPIO_PIN_03)

/* CMP unit definition */
#define CMP_UNIT                        (CM_CMP1)
#define CMP_FCG_ENABLE()                (FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_CMP1_2, ENABLE))

/* DAC unit definition */
#define DAC_UNIT                        (CM_DAC)
#define DAC_CH                          (DAC_CH2)
#define DAC_FCG_ENABLE()                (FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_DAC, ENABLE))
#define DAC_VOL_1P65V                   (0x1000U/2U)

/* TMR6 PWM pin definition */
#define TIM6_PWMA_PORT                  (GPIO_PORT_A)
#define TIM6_PWMA_PIN                   (GPIO_PIN_08)
#define TIM6_PWMA_GPIO_FUNC             (GPIO_FUNC_3)

#define TIM6_PWMB_PORT                  (GPIO_PORT_A)
#define TIM6_PWMB_PIN                   (GPIO_PIN_07)
#define TIM6_PWMB_GPIO_FUNC             (GPIO_FUNC_3)

/* TMR6 unit definition */
#define TMR6_UNIT                       (CM_TMR6_1)
#define TMR6_FCG_ENABLE()               (FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR6_1, ENABLE))

/* EMB unit definition */
#define EMB_GROUP                       (CM_EMB0)
#define EMB_FCG_ENABLE()                (FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_EMB, ENABLE))

/* EMB interrupt definition */
#define EMB_INT_IRQn                    (INT000_IRQn)
#define EMB_INT_SRC                     (INT_SRC_EMB_GR0)

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
 * @brief  Configure CMP
 * @param  None
 * @retval None
 */
static void CMP_Config(void)
{
    stc_cmp_init_t stcCmpInit;
    stc_gpio_init_t stcGpioInit;

    /* CMP compare voltage pin */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinAttr = PIN_ATTR_ANALOG;
    (void)GPIO_Init(CMP_INP_PORT, CMP_INP_PIN, &stcGpioInit);

    /* Enable peripheral Clock */
    CMP_FCG_ENABLE();

    /* Configuration for normal compare function */
    (void)CMP_StructInit(&stcCmpInit);
    stcCmpInit.u16PositiveInput = CMP_POSITIVE_INP4;
    stcCmpInit.u16NegativeInput = CMP_NEGATIVE_INM4;  /* DAO2 */
    stcCmpInit.u16OutPolarity = CMP_OUT_INVT_OFF;
    stcCmpInit.u16OutDetectEdge = CMP_DETECT_EDGS_BOTH;
    stcCmpInit.u16OutFilter = CMP_OUT_FILTER_CLK_DIV32;
    (void)CMP_NormalModeInit(CMP_UNIT, &stcCmpInit);

    /* Enable CMP output */
    CMP_CompareOutCmd(CMP_UNIT, ENABLE);

    CMP_PinVcoutCmd(CMP_UNIT, ENABLE);
}

/**
 * @brief  Configure DAC.
 * @param  None
 * @retval None
 */
static void DAC_Config(void)
{
    stc_dac_init_t stcDacInit;

    /* Enable peripheral Clock */
    DAC_FCG_ENABLE();

    (void)DAC_StructInit(&stcDacInit);
    /* Initialize DAC */
    (void)DAC_Init(DAC_UNIT, DAC_CH, &stcDacInit);

    /* Write Data :V = (Conversion Data / 4096) * VREFH */
    DAC_SetChData(DAC_UNIT, DAC_CH, DAC_VOL_1P65V);

    /* Start Convert */
    (void)DAC_Start(DAC_UNIT, DAC_CH);
}

/**
 * @brief  Get TMR6 clock frequency.
 * @param  None
 * @retval TMR6 clock frequency
 */
static uint32_t TMR6_ClockFreq(void)
{
    return CLK_GetBusClockFreq(CLK_BUS_PCLK0);
}

/**
 * @brief  Configure TMR6 PWM
 * @param  None
 * @retval None
 */
static void TMR6_PwmConfig(void)
{
    stc_tmr6_init_t stcTmr6Init;
    stc_tmr6_pwm_init_t stcTmr6PwmInit;

    /* Initialize PWM I/O */
    GPIO_SetFunc(TIM6_PWMA_PORT, TIM6_PWMA_PIN, TIM6_PWMA_GPIO_FUNC);
    GPIO_SetFunc(TIM6_PWMB_PORT, TIM6_PWMB_PIN, TIM6_PWMB_GPIO_FUNC);

    /* Enable TMR6 peripheral clock */
    TMR6_FCG_ENABLE();

    /* TMR6 general count function configuration */
    (void)TMR6_StructInit(&stcTmr6Init);
    stcTmr6Init.sw_count.u32ClockDiv = TMR6_CLK_DIV256;
    stcTmr6Init.u32PeriodValue = TMR6_ClockFreq() / (32UL * (1UL << (uint32_t)(stcTmr6Init.sw_count.u32ClockDiv >> \
                                                                               TMR6_GCONR_CKDIV_POS))) - 1UL;
    (void)TMR6_Init(TMR6_UNIT, &stcTmr6Init);

    /* Configure PWM output */
    stcTmr6PwmInit.u32CompareValue = stcTmr6Init.u32PeriodValue / 2UL;
    stcTmr6PwmInit.u32CountDownMatchAPolarity = TMR6_PWM_HOLD;
    stcTmr6PwmInit.u32CountUpMatchAPolarity = TMR6_PWM_HIGH;
    stcTmr6PwmInit.u32CountUpMatchBPolarity = TMR6_PWM_HOLD;
    stcTmr6PwmInit.u32CountDownMatchBPolarity = TMR6_PWM_HOLD;
    stcTmr6PwmInit.u32UdfPolarity = TMR6_PWM_HOLD;
    stcTmr6PwmInit.u32OvfPolarity = TMR6_PWM_LOW;
    stcTmr6PwmInit.u32StopPolarity = TMR6_PWM_LOW;
    stcTmr6PwmInit.u32StartPolarity = TMR6_PWM_LOW;
    (void)TMR6_PWM_Init(TMR6_UNIT, TMR6_CH_A, &stcTmr6PwmInit);

    stcTmr6PwmInit.u32CompareValue = stcTmr6Init.u32PeriodValue / 2UL;
    stcTmr6PwmInit.u32CountDownMatchAPolarity = TMR6_PWM_HOLD;
    stcTmr6PwmInit.u32CountUpMatchAPolarity = TMR6_PWM_HOLD;
    stcTmr6PwmInit.u32CountUpMatchBPolarity = TMR6_PWM_LOW;
    stcTmr6PwmInit.u32CountDownMatchBPolarity = TMR6_PWM_HOLD;
    stcTmr6PwmInit.u32UdfPolarity = TMR6_PWM_HOLD;
    stcTmr6PwmInit.u32OvfPolarity = TMR6_PWM_HIGH;
    stcTmr6PwmInit.u32StopPolarity = TMR6_PWM_HIGH;
    stcTmr6PwmInit.u32StartPolarity = TMR6_PWM_HIGH;
    (void)TMR6_PWM_Init(TMR6_UNIT, TMR6_CH_B, &stcTmr6PwmInit);

    /* PWM pin function set */
    TMR6_SetFunc(TMR6_UNIT, TMR6_CH_A, TMR6_PIN_CMP_OUTPUT);
    TMR6_SetFunc(TMR6_UNIT, TMR6_CH_B, TMR6_PIN_CMP_OUTPUT);

    /* PWM output command */
    TMR6_PWM_OutputCmd(TMR6_UNIT, TMR6_CH_A, ENABLE);
    TMR6_PWM_OutputCmd(TMR6_UNIT, TMR6_CH_B, ENABLE);

    /* Start TMR6 count. */
    TMR6_Start(TMR6_UNIT);
}

/**
 * @brief  EMB IRQ Callback.
 * @param  None
 * @retval None
 */
void EMB_IrqCallback(void)
{
    if (SET == EMB_GetStatus(EMB_GROUP, EMB_FLAG_CMP)) {
        /* Clear the EMB CMP status. */
        EMB_ClearStatus(EMB_GROUP, EMB_FLAG_CMP);
    }
}

/**
 * @brief  Main function of EMB CMP brake
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    stc_emb_tmr6_init_t stcEmbInit;
    stc_tmr6_emb_config_t stcTmr6EmbConfig;

    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_SEL);

    /* Configure DAC. */
    DAC_Config();

    /* Configure CMP. */
    CMP_Config();

    /* Configure TMR6 PWM. */
    TMR6_PwmConfig();

    /* Configure TMR6 EMB. */
    (void)TMR6_EMBConfigStructInit(&stcTmr6EmbConfig);
    stcTmr6EmbConfig.u32ValidCh = TMR6_EMB_EVT_CH0;
    stcTmr6EmbConfig.u32ReleaseMode = TMR6_EMB_RELEASE_IMMED;
    stcTmr6EmbConfig.u32PinStatus = TMR6_EMB_PIN_LOW;
    (void)TMR6_EMBConfig(TMR6_UNIT, TMR6_CH_A, &stcTmr6EmbConfig);
    (void)TMR6_EMBConfig(TMR6_UNIT, TMR6_CH_B, &stcTmr6EmbConfig);

    /* Enable EMB peripheral clock */
    EMB_FCG_ENABLE();

    /* EMB: initialize */
    (void)EMB_TMR6_StructInit(&stcEmbInit);
    stcEmbInit.stcCmp.u32Cmp1State = EMB_CMP1_ENABLE;
    (void)EMB_TMR6_Init(EMB_GROUP, &stcEmbInit);

    /* EMB: enable interrupt */
    EMB_IntCmd(EMB_GROUP, EMB_INT_CMP, ENABLE);

    /* EMB: set release PWM condition */
    EMB_SetReleasePwmCond(EMB_GROUP, EMB_EVT_CMP, EMB_RELEASE_PWM_COND_FLAG_ZERO);

    /* EMB: register IRQ callback. */
    (void)INTC_IrqInstallHandle(EMB_INT_IRQn, EMB_INT_SRC, DDL_IRQ_PRIO_DEFAULT, EMB_IrqCallback);

    /* MCU Peripheral registers write protected */
    LL_PERIPH_WP(LL_PERIPH_SEL);

    for (;;) {
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
