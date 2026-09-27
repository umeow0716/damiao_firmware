/**
 *******************************************************************************
 * @file  cmp/cmp_window/source/main.c
 * @brief Main program of CMP for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Delete DAC_DataRegAlignConfig() due to align configure in DAC_Init()
                                    Modify DAC port define
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
 * @addtogroup CMP_Window
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

#define DAC_TEST_UNIT                   (CM_DAC)
#define DAC_PERIP_CLK                   (FCG3_PERIPH_DAC)

#define CMP_PERIP_CLK                   (FCG3_PERIPH_CMP3_4)
/* Define port and pin of CMP */
/* Window compare voltage PB1(CMP3_INP2+CMP4_INP2) */
#define WIN_POSITIVE_PORT               (GPIO_PORT_B)
#define WIN_POSITIVE_PIN                (GPIO_PIN_01)
/* CMP4_OUT */
#define CMP4_OUT_PORT                   (GPIO_PORT_C)
#define CMP4_OUT_PIN                    (GPIO_PIN_13)
#define CMP4_OUT_FUNC                   (GPIO_FUNC_2)

#define DAC_VOL_1P3V                    (0x1000U/33U*13U)
#define DAC_VOL_2V                      (0x1000U/33U*20U)

#define DAC_CH1_PORT                    (GPIO_PORT_A)
#define DAC_CH1_PIN                     (GPIO_PIN_04)
#define DAC_CH2_PORT                    (GPIO_PORT_A)
#define DAC_CH2_PIN                     (GPIO_PIN_05)

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
 * @brief  Configure CMP.
 * @param  None
 * @retval None
 */
static void CmpConfig(void)
{
    stc_cmp_window_init_t stcWindowModeInit;
    stc_gpio_init_t stcGpioInit;
    /* Enable peripheral Clock */
    FCG_Fcg3PeriphClockCmd(CMP_PERIP_CLK, ENABLE);
    /* Port function configuration for CMP*/
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinAttr = PIN_ATTR_ANALOG;
    (void)GPIO_Init(DAC_CH1_PORT, DAC_CH1_PIN, &stcGpioInit);
    (void)GPIO_Init(DAC_CH2_PORT, DAC_CH2_PIN, &stcGpioInit);
    (void)GPIO_Init(WIN_POSITIVE_PORT, WIN_POSITIVE_PIN, &stcGpioInit);
    GPIO_SetFunc(CMP4_OUT_PORT, CMP4_OUT_PIN, CMP4_OUT_FUNC);

    /* Configuration for normal compare function */
    (void)CMP_WindowStructInit(&stcWindowModeInit);
    stcWindowModeInit.u16PositiveInput = CMP_WIN_POSITIVE_INP2; /* PB1 */
    stcWindowModeInit.u32WinVolLow = CMP_WIN_LOW_INM3;   /* DAO1 */
    stcWindowModeInit.u32WinVolHigh = CMP_WIN_HIGH_INM4; /* DAO2 */
    stcWindowModeInit.u16OutPolarity = CMP_OUT_INVT_OFF;
    stcWindowModeInit.u16OutDetectEdge = CMP_DETECT_EDGS_BOTH;
    stcWindowModeInit.u16OutFilter = CMP_OUT_FILTER_CLK_DIV32;
    (void)CMP_WindowModeInit(CMP_WIN_CMP34, &stcWindowModeInit);

    /* Enable CMP output */
    CMP_CompareOutCmd(CM_CMP3, ENABLE);
    CMP_CompareOutCmd(CM_CMP4, ENABLE);

    /* Enable VCOUT */
    CMP_PinVcoutCmd(CM_CMP4, ENABLE);
}

/**
 * @brief  Configure DAC.
 * @param  None
 * @retval None
 */
static void DacConfig(void)
{
    stc_dac_init_t stcDacInit;
    /* Enable peripheral Clock */
    FCG_Fcg3PeriphClockCmd(DAC_PERIP_CLK, ENABLE);

    DAC_DeInit(DAC_TEST_UNIT);
    (void)DAC_StructInit(&stcDacInit);
    (void)DAC_Init(DAC_TEST_UNIT, DAC_CH1, &stcDacInit);
    (void)DAC_Init(DAC_TEST_UNIT, DAC_CH2, &stcDacInit);

    /* Clear data register */
    DAC_SetChData(DAC_TEST_UNIT, DAC_CH1, 0U);
    DAC_SetChData(DAC_TEST_UNIT, DAC_CH2, 0U);

    (void)DAC_AMPCmd(DAC_TEST_UNIT, DAC_CH1, ENABLE);
    (void)DAC_AMPCmd(DAC_TEST_UNIT, DAC_CH2, ENABLE);

    /* Output Enable */
    DAC_StartDualCh(DAC_TEST_UNIT);

    /* Write Data  V = (Conversion Data / 4096) * VREFH */
    DAC_SetChData(DAC_TEST_UNIT, DAC_CH1, DAC_VOL_1P3V);
    DAC_SetChData(DAC_TEST_UNIT, DAC_CH2, DAC_VOL_2V);
}

/**
 * @brief  Main function of cmp_window project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Unlock peripherals or registers */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);

    BSP_CLK_Init();

    /* Configure DAC */
    DacConfig();
    /* Configure CMP */
    CmpConfig();
    /* Lock peripherals or registers */
    LL_PERIPH_WP(EXAMPLE_PERIPH_WP);
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
