/**
 *******************************************************************************
 * @file  dcu/dcu_triangle_wave_mode/source/main.c
 * @brief This example demonstrates DCU triangle wave mode function.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Delete DAC_DataRegAlignConfig() due to align configure in DAC_Init()
                                    Add DAC_StructInit() before DAC_Init()
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
 * @addtogroup DCU_Triangle_Wave_Mode
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

/* DCU unit definition */
#define DCU_UNIT                        (CM_DCU1)
#define DCU_FCG_ENABLE()                (FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_DCU1, ENABLE))

/* DCU hardware trigger signal number */
#define DCU_TRIG_SEL                    (AOS_DCU1)
#define DCU_TRIG_EVT_SRC                (EVT_SRC_TMR0_1_CMP_A)

/* DCU wave upper limit definition */
#define DCU_WAVE_LOWER_LIMIT            (0U)
#define DCU_WAVE_UPPER_LIMIT            (3999U)

/* DCU interrupt definition */
#define DCU_IRQn                        (INT000_IRQn)
#define DCU_INT_SRC                     (INT_SRC_DCU1)

/* DAC pin definition */
#define DAC_PORT                        (GPIO_PORT_A)
#define DAC_PIN                         (GPIO_PIN_04)

/* DAC unit and channel definition */
#define DAC_UNIT                        (CM_DAC)
#define DAC_CH                          (DAC_CH1)
#define DAC_FCG_ENABLE()                (FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_DAC, ENABLE))

/* DCU reload pin definition */
#define DCU_TOGGLE_PORT                 (GPIO_PORT_E)
#define DCU_TOGGLE_PIN                  (GPIO_PIN_04)

/* TMR0 unit and channel definition */
#define TMR0_UNIT                       (CM_TMR0_1)
#define TMR0_CH                         (TMR0_CH_A)
#define TMR0_FCG_ENABLE()               (FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_1, ENABLE))

/* TMR0 count value per second definition */
#define TMR0_CNT_1S_VAL(div)            ((TMR0_ClockFreq() / (div)))

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
 * @brief  Get TMR0 clock frequency.
 * @param  None
 * @retval TMR0 clock frequency
 */
static uint32_t TMR0_ClockFreq(void)
{
    return CLK_GetBusClockFreq(CLK_BUS_PCLK1);
}

/**
 * @brief  Configure TMR0.
 * @param  None
 * @retval None
 */
static void TMR0_Config(void)
{
    stc_tmr0_init_t stcTmr0Init;

    /* Enable peripheral clock */
    TMR0_FCG_ENABLE();

    /* TMR0 basetimer function initialize */
    (void)TMR0_StructInit(&stcTmr0Init);
    stcTmr0Init.u16CompareValue = (uint16_t)(TMR0_CNT_1S_VAL(1UL) / (4UL * (DCU_WAVE_UPPER_LIMIT - DCU_WAVE_LOWER_LIMIT + 1UL)));
    (void)TMR0_Init(CM_TMR0_1, TMR0_CH_A, &stcTmr0Init);

    /* Start count*/
    TMR0_Start(TMR0_UNIT, TMR0_CH);
}

/**
 * @brief  Configure DAC.
 * @param  None
 * @retval None
 */
static void DAC_Config(void)
{
    stc_dac_init_t stcDacInit;
    stc_gpio_init_t stcGpioInit;

    /* Set DAC pin */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinAttr = PIN_ATTR_ANALOG;
    (void)GPIO_Init(DAC_PORT, DAC_PIN, &stcGpioInit);

    /* Enable peripheral clock. */
    DAC_FCG_ENABLE();

    (void)DAC_StructInit(&stcDacInit);
    /* Initialize DAC */
    stcDacInit.u16Src = DAC_DATA_SRC_DCU;
    (void)DAC_Init(DAC_UNIT, DAC_CH, &stcDacInit);

    /* Enable DAC */
    (void)DAC_Start(DAC_UNIT, DAC_CH);
}

/**
 * @brief  DCU irq callback function.
 * @param  None
 * @retval None
 */
static void DCU_IrqCallback(void)
{
    GPIO_TogglePins(DCU_TOGGLE_PORT, DCU_TOGGLE_PIN);

    DCU_ClearStatus(DCU_UNIT, DCU_FLAG_TRIANGLE_WAVE_BOTTOM);
}

/**
 * @brief  Main function of DCU triangle wave mode project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    stc_dcu_init_t stcDcuInit;
    stc_dcu_wave_config_t stcWaveConfig;
    stc_gpio_init_t stcGpioInit;
    stc_irq_signin_config_t stcIrqConfig;

    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_SEL);

    /* Initialize BSP system clock. */
    BSP_CLK_Init();

    /* Initialize IO for testing DCU reload time. */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    (void)GPIO_Init(DCU_TOGGLE_PORT, DCU_TOGGLE_PIN, &stcGpioInit);

    /* Enable peripheral clock: DCU  */
    DCU_FCG_ENABLE();

    /* Configure DCU wave amplitude && step */
    stcWaveConfig.u32LowerLimit = DCU_WAVE_LOWER_LIMIT;
    stcWaveConfig.u32UpperLimit = DCU_WAVE_UPPER_LIMIT;
    stcWaveConfig.u32Step = 0x1UL;
    (void)DCU_WaveConfig(DCU_UNIT, &stcWaveConfig);

    /* Initialize DCU */
    stcDcuInit.u32Mode = DCU_MD_TRIANGLE_WAVE;
    stcDcuInit.u32DataWidth = DCU_DATA_WIDTH_16BIT;
    (void)DCU_Init(DCU_UNIT, &stcDcuInit);

    /* Register IRQ handler && configure NVIC. */
    stcIrqConfig.enIRQn = DCU_IRQn;
    stcIrqConfig.enIntSrc = DCU_INT_SRC;
    stcIrqConfig.pfnCallback = &DCU_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqConfig);
    NVIC_SetPriority(stcIrqConfig.enIRQn, DDL_IRQ_PRIO_00);
    NVIC_ClearPendingIRQ(stcIrqConfig.enIRQn);
    NVIC_EnableIRQ(stcIrqConfig.enIRQn);

    /* Enable DCU wave mode interrupt */
    DCU_IntCmd(DCU_UNIT, DCU_CATEGORY_WAVE, DCU_INT_TRIANGLE_WAVE_BOTTOM, ENABLE);
    DCU_GlobalIntCmd(DCU_UNIT, ENABLE);

    /* Set hardware trigger source */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);
    AOS_SetTriggerEventSrc(DCU_TRIG_SEL, DCU_TRIG_EVT_SRC);

    /* Initialize DAC. */
    DAC_Config();

    /* Initialize TMR0. */
    TMR0_Config();

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
