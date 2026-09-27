/**
 *******************************************************************************
 * @file  usart/usart_lin/source/main.c
 * @brief This example demonstrates LIN data receive and transfer.
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
 * @addtogroup USART_LIN
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* Peripheral register WE/WP selection */
#define LL_PERIPH_WE_SEL                (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                                         LL_PERIPH_EFM)
#define LL_PERIPH_WP_SEL                (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                                         LL_PERIPH_EFM)

/* USART RX/TX pin definition */
#define USART_RX_PORT                   (GPIO_PORT_D)   /* PD1: USART6_RX */
#define USART_RX_PIN                    (GPIO_PIN_01)
#define USART_RX_GPIO_FUNC              (GPIO_FUNC_55)

#define USART_TX_PORT                   (GPIO_PORT_D)   /* PD2: USART6_TX */
#define USART_TX_PIN                    (GPIO_PIN_02)
#define USART_TX_GPIO_FUNC              (GPIO_FUNC_54)

/* USART unit definition */
#define USART_UNIT                      (CM_USART6)

/* Interrupt number definition */
#define USART_RX_FULL_IRQn              (INT000_IRQn)
#define USART_RX_ERR_IRQn               (INT001_IRQn)
#define USART_LIN_BREAKWKUP_IRQn        (INT002_IRQn)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static stc_lin_frame_t m_stcSlaveRxFrame;

static stc_lin_handle_t m_stcLinHandle = {
    .USARTx = USART_UNIT,
    .stcLinInit = {
        .u32Baudrate = 19200UL,
        .u32ClockSrc = USART_CLK_SRC_INTERNCLK,
        .u32ClockDiv = USART_CLK_DIV16,
        .u32CKOutput = USART_CK_OUTPUT_DISABLE,
        .u32OverSampleBit = USART_OVER_SAMPLE_8BIT,
        .u32BmcClockDiv = USART_LIN_BMC_CLK_DIV8,
        .u32DetectBreakLen = USART_LIN_DETECT_BREAK_10BIT,
        .u32SendBreakLen = USART_LIN_SEND_BREAK_13BIT,
        .u32SendBreakMode = USART_LIN_SEND_BREAK_MD_SBK,
    },
    .stcPinConfig = {
        .u8RxPort = USART_RX_PORT,
        .u16RxPin = USART_RX_PIN,
        .u16RxPinFunc = USART_RX_GPIO_FUNC,
        .u8TxPort = USART_TX_PORT,
        .u16TxPin = USART_TX_PIN,
        .u16TxPinFunc = USART_TX_GPIO_FUNC,
    },
    .stcIrqConfig = {
        .RxFullIRQn = USART_RX_FULL_IRQn,
        .RxErrorIRQn = USART_RX_ERR_IRQn,
        .BreakWkupIRQn = USART_LIN_BREAKWKUP_IRQn,
    },
};

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Main function of LIN project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    int32_t i32Ret;

    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_WE_SEL);

    /* Initialize BSP system clock. */
    BSP_CLK_Init();

    /* Initialize BSP expand IO. */
    BSP_IO_Init();

    /* Initialize BSP LED. */
    BSP_LED_Init();

    /* Configure LIN transceiver chip sleep pin. */
    BSP_IO_WritePortPin(LIN_SLEEP_PORT, LIN_SLEEP_PIN, 1U);
    BSP_IO_ConfigPortPin(LIN_SLEEP_PORT, LIN_SLEEP_PIN, EIO_DIR_OUT);

    /* Initialize LIN function. */
    m_stcLinHandle.u32LinMode = LIN_SLAVE;
    m_stcLinHandle.enLinState = LIN_STATE_WAKEUP;
    if (LL_OK != LIN_Init(&m_stcLinHandle)) {
        BSP_LED_On(LED_RED);
        for (;;) {
        }
    }

    /* MCU Peripheral registers write protected */
    LL_PERIPH_WP(LL_PERIPH_WP_SEL);

    for (;;) {
        /* LIN slave receive frame */
        i32Ret = LIN_SLAVE_ReceiveFrame(&m_stcLinHandle, &m_stcSlaveRxFrame, LIN_REC_WAITING_FOREVER);
        if (LL_OK == i32Ret) {
            BSP_LED_Off(LED_BLUE | LED_RED);

            /* LIN slave send frame */
            i32Ret = LIN_SLAVE_SendFrame(&m_stcLinHandle, &m_stcSlaveRxFrame, LIN_REC_WAITING_FOREVER);
            if (LL_OK == i32Ret) {
                BSP_LED_On(LED_BLUE);
            } else {
                BSP_LED_On(LED_RED);
            }
        }
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
