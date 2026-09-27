/**
 *******************************************************************************
 * @file  ctc/ctc_ctcref_single_trimming/source/main.c
 * @brief This example demonstrates CTC trimming by CTCREF pin clock source.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Initialize XTAL32 using BSP_XTAL32_Init
   2025-11-03       CDT             Optimize process
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
 * @addtogroup CTC_CTCREF_Single_Trimming
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

/* MCO pin */
#define MCO_SEL                         CLK_MCO1
/* MCO pin of MCO_SEL */
#define MCO_PORT                        (GPIO_PORT_A)
#define MCO_PIN                         (GPIO_PIN_08)
#define MCO_FUNC                        (GPIO_FUNC_1)

/* CTCREF pin */
#define CTCREF_PORT                     (GPIO_PORT_A)
#define CTCREF_PIN                      (GPIO_PIN_09)
#define CTCREF_FUNC                     (GPIO_FUNC_1)

/* CTC reference clock trim time */
#define CTC_REF_CLK_TRIM_TIME           (1.0F/512.0F) /* 512Hz */

/* CTC TRMVAL value */
#define CTC_TRIM_VALUE                  (0x21U)         /* -31 */

/* Function clock gate definition  */
#define CTC_FCG_ENABLE()                (FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_CTC, ENABLE))

/* CTC interrupt definition */
#define CTC_ERR_IRQn                    (INT000_IRQn)

/* Baudrate definition  */
#define PRINTF_BAUDRATE                 (115200UL)

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
 * @brief  BSP clock initialize.
 * @param  None
 * @retval None
 */
void BSP_CLK_Init(void)
{
    stc_clock_xtal_init_t stcXtalInit;

    /************************** Configure XTAL32 clock ************************/
    BSP_XTAL32_Init();

    /************************** Configure XTAL clock **************************/
    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    stcXtalInit.u8State = CLK_XTAL_ON;
    (void)CLK_XtalInit(&stcXtalInit);

    /*********************** Switch system clock to XTAL **********************/
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_XTAL);
}

/**
 * @brief  CTC IRQ handler.
 * @param  None.
 * @retval None
 */
void CTC_IrqHandler(void)
{
    BSP_LED_On(LED_RED);
}

/**
 * @brief  Main function of CTC CTCREF pin clock trimming project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint8_t u8InitTrimValue;
    uint8_t u8TrimmingValue;
    stc_irq_signin_config_t stcIrqConfig;
    const stc_ctc_st_init_t stcCtcInit = {
        .u32HrcClockDiv = CTC_HRC_CLK_DIV1,
        .u32CtcRefEdge = CTC_CTCREF_RISING_RISING,
        .f32CtcRefEdgeTime = CTC_REF_CLK_TRIM_TIME,
        .f32TolerantErrRate = 0.005F,
        .u8TrimValue = CTC_TRIM_VALUE,
    };

    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_SEL);

    /* Initialize BSP system clock. */
    BSP_CLK_Init();

    /* Configure clock output pin */
    GPIO_SetFunc(MCO_PORT, MCO_PIN, MCO_FUNC);

    /* Configure clock output HRC clock */
    CLK_MCOConfig(MCO_SEL, CLK_MCO_SRC_HRC, CLK_MCO_DIV1);

    /* MCO output enable */
    CLK_MCOCmd(MCO_SEL, ENABLE);

    /* HRC output */
    (void)CLK_HrcCmd(ENABLE);

    /* Initialize UART for debug print function. */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, PRINTF_BAUDRATE, BSP_PRINTF_Preinit);

    /* Initialize BSP expand IO. */
    BSP_IO_Init();

    /* Initialize BSP LED. */
    BSP_LED_Init();

    /* Initialize BSP key. */
    BSP_KEY_Init();

    /* Enable peripheral clock */
    CTC_FCG_ENABLE();

    /* Wait CTC stop. */
    while (SET == CTC_GetStatus(CTC_FLAG_BUSY)) {
    }

    /* Initialize CTC function. */
    (void)CTC_ST_Init(&stcCtcInit);

    /* Register CTC error IRQ handler && configure NVIC. */
    stcIrqConfig.enIRQn = CTC_ERR_IRQn;
    stcIrqConfig.enIntSrc = INT_SRC_CTC_ERR;
    stcIrqConfig.pfnCallback = &CTC_IrqHandler;
    (void)INTC_IrqSignIn(&stcIrqConfig);
    NVIC_ClearPendingIRQ(stcIrqConfig.enIRQn);
    NVIC_SetPriority(stcIrqConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqConfig.enIRQn);

    CTC_IntCmd(ENABLE);

    /* User key */
    while (SET != BSP_KEY_GetStatus(BSP_KEY_1)) {
    }

    /* Configure clock output XTAL32 clock */
    CLK_MCOConfig(MCO_SEL, CLK_MCO_SRC_XTAL32, CLK_MCO_DIV64);
    /* Configure CTCREF pin */
    GPIO_SetFunc(CTCREF_PORT, CTCREF_PIN, CTCREF_FUNC);
    /* Wait stable */
    DDL_DelayMS(5);

    u8InitTrimValue = CTC_GetTrimValue();

    for (;;) {
        CTC_Start();

        /* Delay for CTC trimming */
        DDL_DelayMS(5);

        /* Wait CTC stopping */
        while (SET == CTC_GetStatus(CTC_FLAG_BUSY)) {
        }

        /* Check CTC trimming result */
        if (SET == CTC_GetStatus(CTC_FLAG_TRIM_OK)) {
            break;
        }
    }

    /* Disable CTCREF pin */
    GPIO_SetFunc(CTCREF_PORT, CTCREF_PIN, GPIO_FUNC_0);
    /* Configure clock output HRC clock */
    CLK_MCOConfig(MCO_SEL, CLK_MCO_SRC_HRC, CLK_MCO_DIV1);
    /* MCU Peripheral registers write protected */
    LL_PERIPH_WP(LL_PERIPH_SEL);

    u8TrimmingValue = CTC_GetTrimValue();

    DDL_Printf("Init trimming value is 0x%02X; Trimming result is 0x%02X. \r\n", u8InitTrimValue, u8TrimmingValue);

    BSP_LED_On(LED_BLUE);

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
