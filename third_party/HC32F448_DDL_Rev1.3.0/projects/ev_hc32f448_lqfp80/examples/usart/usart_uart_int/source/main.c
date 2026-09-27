/**
 *******************************************************************************
 * @file  usart/usart_uart_int/source/main.c
 * @brief This example demonstrates UART data receive and transfer by interrupt.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Change USART clock division: USART_CLK_DIV64 -> USART_CLK_DIV1
                                    Treat INTC_IrqInstalHandler as library function
                                    Replace MACRO determined function with key selection
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
 * @addtogroup USART_UART_Interrupt
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

/* USART RX/TX pin definition */
#define USART_RX_PORT                   (GPIO_PORT_C)   /* PC11: USART2_RX */
#define USART_RX_PIN                    (GPIO_PIN_11)
#define USART_RX_GPIO_FUNC              (GPIO_FUNC_37)

#define USART_TX_PORT                   (GPIO_PORT_C)   /* PC10: USART2_TX */
#define USART_TX_PIN                    (GPIO_PIN_10)
#define USART_TX_GPIO_FUNC              (GPIO_FUNC_36)

/* USART unit definition */
#define USART_UNIT                      (CM_USART2)
#define USART_FCG_ENABLE()              (FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_USART2, ENABLE))

/* USART interrupt definition */
#define USART_RX_ERR_IRQn               (INT000_IRQn)
#define USART_RX_ERR_INT_SRC            (INT_SRC_USART2_EI)

#define USART_RX_FULL_IRQn              (INT001_IRQn)
#define USART_RX_FULL_INT_SRC           (INT_SRC_USART2_RI)

#define USART_TX_EMPTY_IRQn             (INT002_IRQn)
#define USART_TX_EMPTY_INT_SRC          (INT_SRC_USART2_TI)

#define USART_TX_CPLT_IRQn              (INT003_IRQn)

/* Ring buffer size */
#define RING_BUF_SIZE                   (500UL)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static uint8_t m_au8DataBuf[RING_BUF_SIZE];
static stc_ring_buf_t m_stcRingBuf;
static __IO en_flag_status_t m_enTxCompleteFlag = SET;
static uint32_t m_u32IntrTxCpltFlag = 0UL;
static en_int_src_t m_enIntrTxCpltSrc;
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  USART transmit data register empty IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_TxEmpty_IrqCallback(void)
{
    uint8_t u8Data;

    if (!BUF_Empty(&m_stcRingBuf)) {
        (void)BUF_Read(&m_stcRingBuf, &u8Data, 1UL);
        USART_WriteData(USART_UNIT, (uint16_t)u8Data);
    } else {
        USART_FuncCmd(USART_UNIT, m_u32IntrTxCpltFlag, ENABLE);
    }
}

/**
 * @brief  USART transmit complete IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_TxComplete_IrqCallback(void)
{
    m_enTxCompleteFlag = SET;

    if (USART_INT_TX_CPLT == m_u32IntrTxCpltFlag) {
        USART_FuncCmd(USART_UNIT, (USART_TX | USART_INT_TX_CPLT | USART_INT_TX_EMPTY), DISABLE);
    } else if (USART_INT_TX_END == m_u32IntrTxCpltFlag) {
        USART_ClearStatus(USART_UNIT, USART_FLAG_TX_END);
        USART_FuncCmd(USART_UNIT, (USART_TX | USART_INT_TX_EMPTY), DISABLE);
    } else {
        /* Do nothing */
    }
}

/**
 * @brief  USART RX IRQ callback
 * @param  None
 * @retval None
 */
static void USART_RxFull_IrqCallback(void)
{
    uint8_t u8Data = (uint8_t)USART_ReadData(USART_UNIT);

    (void)BUF_Write(&m_stcRingBuf, &u8Data, 1UL);
}

/**
 * @brief  USART error IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_RxError_IrqCallback(void)
{
    (void)USART_ReadData(USART_UNIT);

    USART_ClearStatus(USART_UNIT, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

/**
 * @brief  Main function of UART interrupt project
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

    /* Configure USART RX/TX pin. */
    GPIO_SetFunc(USART_RX_PORT, USART_RX_PIN, USART_RX_GPIO_FUNC);
    GPIO_SetFunc(USART_TX_PORT, USART_TX_PIN, USART_TX_GPIO_FUNC);

    do {
        if (SET == BSP_KEY_GetStatus(BSP_KEY_1)) {
            m_u32IntrTxCpltFlag = USART_INT_TX_CPLT;
            m_enIntrTxCpltSrc = INT_SRC_USART2_TCI;
        } else if (SET == BSP_KEY_GetStatus(BSP_KEY_2)) {
            m_u32IntrTxCpltFlag = USART_INT_TX_END;
            m_enIntrTxCpltSrc = INT_SRC_USART2_TENDI;
        } else {
            /* rsvd */
        }
    } while (m_u32IntrTxCpltFlag == 0UL);

    /* Enable peripheral clock */
    USART_FCG_ENABLE();

    /* Initialize ring buffer function. */
    (void)BUF_Init(&m_stcRingBuf, m_au8DataBuf, sizeof(m_au8DataBuf));

    /* Initialize UART. */
    (void)USART_UART_StructInit(&stcUartInit);
    stcUartInit.u32Baudrate = 115200UL;
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    if (LL_OK != USART_UART_Init(USART_UNIT, &stcUartInit, NULL)) {
        BSP_LED_On(LED_RED);
        for (;;) {
        }
    }

    /* Register RX error IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_RX_ERR_IRQn, USART_RX_ERR_INT_SRC,
                                DDL_IRQ_PRIO_DEFAULT, USART_RxError_IrqCallback);

    /* Register RX full IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_RX_FULL_IRQn, USART_RX_FULL_INT_SRC,
                                DDL_IRQ_PRIO_DEFAULT, USART_RxFull_IrqCallback);

    /* Register TX empty IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_TX_EMPTY_IRQn, USART_TX_EMPTY_INT_SRC,
                                DDL_IRQ_PRIO_DEFAULT, USART_TxEmpty_IrqCallback);

    /* Register TX complete IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_TX_CPLT_IRQn, m_enIntrTxCpltSrc,
                                DDL_IRQ_PRIO_DEFAULT, USART_TxComplete_IrqCallback);

    /* MCU Peripheral registers write protected */
    LL_PERIPH_WP(LL_PERIPH_SEL);

    /* Enable RX function */
    USART_FuncCmd(USART_UNIT, (USART_RX | USART_INT_RX), ENABLE);

    for (;;) {
        if ((SET == m_enTxCompleteFlag) && !BUF_Empty(&m_stcRingBuf)) {
            m_enTxCompleteFlag = RESET;

            if (USART_INT_TX_END == m_u32IntrTxCpltFlag) {
                USART_FuncCmd(USART_UNIT, USART_INT_TX_END, DISABLE);
            }

            USART_FuncCmd(USART_UNIT, (USART_TX | USART_INT_TX_EMPTY), ENABLE);
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
