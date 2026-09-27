/**
 *******************************************************************************
 * @file  cmp/cmp_normal_int/source/main.c
 * @brief Main program of CMP for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Delete DAC_DataRegAlignConfig() due to align configure in DAC_Init()
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
 * @addtogroup CMP_Normal_Int
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* unlock/lock peripheral */
#define EXAMPLE_PERIPH_WE               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | \
                                         LL_PERIPH_PWC_CLK_RMU)
#define EXAMPLE_PERIPH_WP               (LL_PERIPH_EFM | LL_PERIPH_FCG)

#define CMP_TEST_UNIT                   (CM_CMP1)
#define CMP_PERIP_CLK                   (FCG3_PERIPH_CMP1_2)
/* Define port and pin for CMP */
/* CMP compare voltage CMP1_INP4(PA3) */
#define CMP1_INP4_PORT                  (GPIO_PORT_A)
#define CMP1_INP4_PIN                   (GPIO_PIN_03)
/* CMP1_OUT */
#define CMP1_OUT_PORT                   (GPIO_PORT_B)
#define CMP1_OUT_PIN                    (GPIO_PIN_12)
#define CMP1_OUT_FUNC                   (GPIO_FUNC_1)
/* DAC */
#define DAC_PERIP_CLK                   (PWC_FCG3_DAC)
#define DAC_TEST_UNIT                   (CM_DAC)
#define DAC_CH                          (DAC_CH2)
#define DAC_VOL_1P1V                    (0x1000U/33U*11U)
#define DAC_DATA_ALIGN                  (DAC_DATA_ALIGN_RIGHT)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void CmpConfig(void);
static void DAO2Config(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Main function of cmp_normal_int project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Unlock peripherals or registers */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    /* Configure BSP */
    BSP_CLK_Init();
    BSP_IO_Init();
    BSP_LED_Init();

    /* Configure DAO2 */
    DAO2Config();
    /* Configure CMP */
    CmpConfig();

    /*NVIC configuration for interrupt */
    NVIC_EnableIRQ(CMP1_IRQn);
    /* Peripheral registers write protected */
    LL_PERIPH_WP(EXAMPLE_PERIPH_WP);
    /* Configuration finished */
    for (;;) {
        ;
    }
}

/**
 * @brief  CMP interrupt call back
 * @param  None
 * @retval None
 */
void CMP1_Handler(void)
{
    if (SET == CMP_GetStatus(CMP_TEST_UNIT)) {
        BSP_LED_On(LED_RED);
        BSP_LED_Off(LED_BLUE);
    } else {
        BSP_LED_On(LED_BLUE);
        BSP_LED_Off(LED_RED);
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  Configure CMP.
 * @param  None
 * @retval None
 */
static void CmpConfig(void)
{
    stc_cmp_init_t stcCmpInit;
    stc_gpio_init_t stcGpioInit;

    /* Enable peripheral Clock */
    FCG_Fcg3PeriphClockCmd(CMP_PERIP_CLK, ENABLE);

    /* Port function configuration for CMP */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinAttr = PIN_ATTR_ANALOG;
    (void)GPIO_Init(CMP1_INP4_PORT, CMP1_INP4_PIN, &stcGpioInit);
    GPIO_SetFunc(CMP1_OUT_PORT, CMP1_OUT_PIN, CMP1_OUT_FUNC);

    /* Clear structure */
    (void)CMP_StructInit(&stcCmpInit);
    stcCmpInit.u16PositiveInput = CMP_POSITIVE_INP4;
    stcCmpInit.u16NegativeInput = CMP_NEGATIVE_INM4;  /* DAO2 */
    stcCmpInit.u16OutPolarity = CMP_OUT_INVT_OFF;
    stcCmpInit.u16OutDetectEdge = CMP_DETECT_EDGS_BOTH;
    stcCmpInit.u16OutFilter = CMP_OUT_FILTER_CLK_DIV32;
    (void)CMP_NormalModeInit(CMP_TEST_UNIT, &stcCmpInit);

    /* Enable interrupt if need */
    CMP_IntCmd(CMP_TEST_UNIT, ENABLE);
    /* Enable CMP output */
    CMP_CompareOutCmd(CMP_TEST_UNIT, ENABLE);
    /* Enable VCOUT */
    CMP_PinVcoutCmd(CMP_TEST_UNIT, ENABLE);
}

/**
 * @brief  Configure DAO2 enable.
 * @param  None
 * @retval None
 */
static void DAO2Config(void)
{
    stc_dac_init_t stcDacInit;

    /* Enable DAC peripheral clock. */
    FCG_Fcg3PeriphClockCmd(DAC_PERIP_CLK, ENABLE);

    /* Init DAC by default value: source from data register and output enabled*/
    DAC_DeInit(DAC_TEST_UNIT);
    (void)DAC_StructInit(&stcDacInit);
    stcDacInit.u16Align = DAC_DATA_ALIGN;
    (void)DAC_Init(DAC_TEST_UNIT, DAC_CH, &stcDacInit);

    DAC_SetChData(DAC_TEST_UNIT, DAC_CH, DAC_VOL_1P1V);
    (void)DAC_Start(DAC_TEST_UNIT, DAC_CH);
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
