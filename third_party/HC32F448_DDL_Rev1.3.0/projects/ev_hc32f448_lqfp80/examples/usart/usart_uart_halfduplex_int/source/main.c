/**
 *******************************************************************************
 * @file  usart/usart_uart_halfduplex_int/source/main.c
 * @brief This example demonstrates UART half-duplex data receive and transfer
 *        by interrupt mode.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Change USART clock division: USART_CLK_DIV64 -> USART_CLK_DIV16
                                    Treat INTC_IrqInstalHandler as library function
                                    Use key to determine master and slave device
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
 * @addtogroup USART_UART_HalfDuplex_Interrupt
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

/* USART TX pin definition */
#define USART_TX_PORT                   (GPIO_PORT_A)   /* PA9: USART1_TX */
#define USART_TX_PIN                    (GPIO_PIN_09)
#define USART_TX_GPIO_FUNC              (GPIO_FUNC_32)

/* USART unit definition */
#define USART_UNIT                      (CM_USART1)
#define USART_FCG_ENABLE()              (FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_USART1, ENABLE))

/* USART interrupt definition */
#define USART_RX_ERR_IRQn               (INT000_IRQn)
#define USART_RX_ERR_INT_SRC            (INT_SRC_USART1_EI)

#define USART_RX_FULL_IRQn              (INT001_IRQn)
#define USART_RX_FULL_INT_SRC           (INT_SRC_USART1_RI)

#define USART_TX_EMPTY_IRQn             (INT002_IRQn)
#define USART_TX_EMPTY_INT_SRC          (INT_SRC_USART1_TI)

#define USART_TX_CPLT_IRQn              (INT003_IRQn)
#define USART_TX_CPLT_INT_SRC           (INT_SRC_USART1_TCI)

/* Communication data size */
#define COM_DATA_LEN                    (ARRAY_SZ(m_au8TxData))

/* master / slave device */
#define EXAMPLE_USART_MODE_MASTER       (0)
#define EXAMPLE_USART_MODE_SLAVE        (1)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static uint32_t m_u32TxIndex;
static const uint8_t m_au8TxData[] = "USART half-duplux test.";
static __IO en_flag_status_t m_enTxCompleteFlag = RESET;

static uint32_t m_u32RxIndex;
static uint8_t m_au8RxData[COM_DATA_LEN];
static __IO en_flag_status_t m_enRxCompleteFlag = RESET;

static __IO int8_t m_i8USARTMode = -1;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  UART master unit TX IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_TxEmpty_IrqCallback(void)
{
    const uint8_t *pu8TxData;

    if (EXAMPLE_USART_MODE_MASTER == m_i8USARTMode) {
        pu8TxData = m_au8TxData;
    } else {
        pu8TxData = m_au8RxData;
    }

    if (m_u32TxIndex < COM_DATA_LEN) {
        USART_WriteData(USART_UNIT, pu8TxData[m_u32TxIndex]);
        m_u32TxIndex += 1UL;
    } else {
        USART_FuncCmd(USART_UNIT, USART_INT_TX_CPLT, ENABLE);
    }
}

/**
 * @brief  UART master unit TX Complete IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_TxComplete_IrqCallback(void)
{
    m_enTxCompleteFlag = SET;

    /* Disable TX & interrupt function*/
    USART_FuncCmd(USART_UNIT, (USART_TX | USART_INT_TX_CPLT | USART_INT_TX_EMPTY), DISABLE);
}

/**
 * @brief  UART RX full IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_RxFull_IrqCallback(void)
{
    if (m_u32RxIndex < COM_DATA_LEN) {
        m_au8RxData[m_u32RxIndex] = (uint8_t)USART_ReadData(USART_UNIT);
        m_u32RxIndex += 1UL;

        if (m_u32RxIndex == COM_DATA_LEN) {
            m_enRxCompleteFlag = SET;
            /* Disable RX & RX no empty interrupt function */
            USART_FuncCmd(USART_UNIT, (USART_RX | USART_INT_RX), DISABLE);
        }
    }
}

/**
 * @brief  UART RX error IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_RxError_IrqCallback(void)
{
    (void)USART_ReadData(USART_UNIT);

    USART_ClearStatus(USART_UNIT, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

/**
 * @brief  Main function of UART halfduplex interrupt project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    stc_usart_uart_init_t stcUartInit;

    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_SEL);

    /* Initialize BSP system clock. */
    BSP_CLK_Init();

    /* Initialize BSP expand IO. */
    BSP_IO_Init();

    /* Initialize BSP LED. */
    BSP_LED_Init();

    /* Initialize BSP key. */
    BSP_KEY_Init();

    /* Configure USART TX pin. */
    GPIO_SetFunc(USART_TX_PORT, USART_TX_PIN, USART_TX_GPIO_FUNC);

    /* Enable peripheral clock */
    USART_FCG_ENABLE();

    /* Initialize UART half-duplex. */
    (void)USART_UART_StructInit(&stcUartInit);
    stcUartInit.u32ClockDiv = USART_CLK_DIV16;
    stcUartInit.u32Baudrate = 9600UL;
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    if (LL_OK != USART_HalfDuplex_Init(USART_UNIT, &stcUartInit, NULL)) {
        BSP_LED_On(LED_RED);
        for (;;) {
        }
    }

    /* Register RX IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_RX_FULL_IRQn, USART_RX_FULL_INT_SRC,
                                DDL_IRQ_PRIO_DEFAULT, USART_RxFull_IrqCallback);

    /* Register RX error IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_RX_ERR_IRQn, USART_RX_ERR_INT_SRC,
                                DDL_IRQ_PRIO_DEFAULT, USART_RxError_IrqCallback);

    /* Register TX IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_TX_EMPTY_IRQn, USART_TX_EMPTY_INT_SRC,
                                DDL_IRQ_PRIO_DEFAULT, USART_TxEmpty_IrqCallback);

    /* Register TC IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_TX_CPLT_IRQn, USART_TX_CPLT_INT_SRC,
                                DDL_IRQ_PRIO_DEFAULT, USART_TxComplete_IrqCallback);

    /* MCU Peripheral registers write protected */
    LL_PERIPH_WP(LL_PERIPH_SEL);

    do {
        if (SET == BSP_KEY_GetStatus(BSP_KEY_1)) {
            m_i8USARTMode = EXAMPLE_USART_MODE_MASTER;
        } else if (SET == BSP_KEY_GetStatus(BSP_KEY_2)) {
            m_i8USARTMode = EXAMPLE_USART_MODE_SLAVE;
        } else {
            /* rsvd */
        }
    } while (m_i8USARTMode == -1);

    if (EXAMPLE_USART_MODE_MASTER == m_i8USARTMode) {
        /* Master send data */
        USART_FuncCmd(USART_UNIT, (USART_TX | USART_INT_TX_EMPTY), ENABLE);
        while (RESET == m_enTxCompleteFlag) {
        }

        /* Master receive data */
        USART_FuncCmd(USART_UNIT, (USART_RX | USART_INT_RX), ENABLE);
        while (RESET == m_enRxCompleteFlag) {
        }
    } else {
        /* Slave receive data */
        USART_FuncCmd(USART_UNIT, (USART_RX | USART_INT_RX), ENABLE);
        while (RESET == m_enRxCompleteFlag) {
        }

        /* Slave send data */
        USART_FuncCmd(USART_UNIT, (USART_TX | USART_INT_TX_EMPTY), ENABLE);
        while (RESET == m_enTxCompleteFlag) {
        }
    }

    /* Compare data */
    if (0 == memcmp(m_au8RxData, m_au8TxData, COM_DATA_LEN)) {
        BSP_LED_On(LED_BLUE);
    } else {
        BSP_LED_On(LED_RED);
    }

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
