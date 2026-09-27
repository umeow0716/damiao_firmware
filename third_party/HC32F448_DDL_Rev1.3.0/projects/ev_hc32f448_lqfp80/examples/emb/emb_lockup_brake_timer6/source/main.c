/**
 *******************************************************************************
 * @file  emb/emb_lockup_brake_timer6/source/main.c
 * @brief This example demonstrates how to use lockup brake function of EMB
 *        function.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Optimize comments: HardFault_Generate and HardFault_Handler
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
 * @addtogroup EMB_Lockup_Brake_TMR6
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

/* Key pin definition */
#define KEY_PORT                        (GPIO_PORT_B)
#define KEY_PIN                         (GPIO_PIN_06)

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
 * @brief  Get key state
 * @param  None
 * @retval An @ref en_flag_status_t enumeration type value.
 */
static en_flag_status_t KEY_State(void)
{
    en_flag_status_t enKeyState = RESET;

    if (PIN_RESET == GPIO_ReadInputPins(KEY_PORT, KEY_PIN)) {
        DDL_DelayMS(50UL);

        if (PIN_RESET == GPIO_ReadInputPins(KEY_PORT, KEY_PIN)) {
            while (PIN_RESET == GPIO_ReadInputPins(KEY_PORT, KEY_PIN)) {
            }
            enKeyState = SET;
        }
    }

    return enKeyState;
}

/**
 * @brief  Generate hard-fault.
 * @param  None
 * @retval None
 */
#if defined (__ICCARM__)
#pragma optimize= none
#endif
static void HardFault_Generate(void)
{
    /* MemManage fault escalated into HardFault : MemManage fault occurs
       and the MemManage handler is not enabled */
    ((func_ptr_t)0xFFFFFFFFUL)();
}

/**
 * @brief  Hard-fault IRQ Handler.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
    /* The processor enters a lockup state if a fault occurs
       when executing the NMI or HardFault handlers. */
    HardFault_Generate();
    HardFault_Generate();

    __DSB();  /* Arm Errata 838869 */
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
 * @brief  Main function of EMB lockup brake
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    stc_emb_tmr6_init_t stcEmbInit;
    stc_tmr6_emb_config_t stcTmr6EmbConfig;

    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_SEL);

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
    stcEmbInit.stcSys.u32Lockup = EMB_LOCKUP_ENABLE;
    (void)EMB_TMR6_Init(EMB_GROUP, &stcEmbInit);

    for (;;) {
        /* Wait key pressed */
        while (RESET == KEY_State()) {
        }

        /* Generate hard-fault interrupt */
        HardFault_Generate();
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
