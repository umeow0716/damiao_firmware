/**
 *******************************************************************************
 * @file  usart/usart_clocksync_int/source/main.c
 * @brief This example demonstrates clock sync data receive and transfer by interrupt.
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
 * @addtogroup USART_ClockSync_Interrupt
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
/**
 * @brief Buffer handle structure definition
 */
typedef struct {
    const uint8_t *pu8TxData;   /*!< Pointer to TX buffer */
    uint32_t      u32TxSize;    /*!< TX buffer size       */
    __IO uint32_t u32TxCount;   /*!< TX count             */
    uint8_t       *pu8RxData;   /*!< Pointer to RX buffer */
    uint32_t      u32RxSize;    /*!< RX buffer size */
    __IO uint32_t u32RxCount;   /*!< RX count       */
} stc_buf_handle_t;

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* Peripheral register WE/WP selection */
#define LL_PERIPH_SEL                   (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                                         LL_PERIPH_EFM)

/* USART CK/RX/TX pin definition */
#define USART_CK_PORT                   (GPIO_PORT_A)   /* PA8: USART1_CK */
#define USART_CK_PIN                    (GPIO_PIN_08)
#define USART_CK_GPIO_FUNC              (GPIO_FUNC_7)

#define USART_RX_PORT                   (GPIO_PORT_A)   /* PA10: USART1_RX */
#define USART_RX_PIN                    (GPIO_PIN_10)
#define USART_RX_GPIO_FUNC              (GPIO_FUNC_33)

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

/* master/slave device */
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
static stc_buf_handle_t m_stcBufHandle;

static const uint8_t m_au8TxData[] = "USART clock-sync test.";
static uint8_t m_au8RxData[COM_DATA_LEN];

static __IO int8_t m_i8USARTMode = -1;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Enable send&&receive an amount of data(non-blocking).
 * @param  [in] USARTx                  pointer to a USART instance.
 * @param  [in] pstcBufHandle           pointer to a stc_buf_handle_t structure.
 * @param  [in] pu8TxData               Pointer to data transmitted buffer
 * @param  [out] pu8RxData              Pointer to data received buffer
 * @param  [in] u32Size                 Amount of data to be received
 * @retval int32_t:
 *           - LL_OK:                   Enable successfully.
 *           - LL_ERR_INVD_PARAM:       Invalid parameter
 */
static int32_t CLOCKSYNC_TransReceive_INT(CM_USART_TypeDef *USARTx,
                                          stc_buf_handle_t *pstcBufHandle,
                                          const uint8_t *pu8TxData,
                                          uint8_t *pu8RxData,
                                          uint32_t u32Size)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    const uint32_t u32Func = (USART_RX | USART_INT_RX | USART_TX);

    if ((USARTx != NULL) && (pstcBufHandle != NULL) && \
        (pu8TxData != NULL) && (pu8RxData != NULL) && (u32Size > 0UL)) {
        pstcBufHandle->pu8RxData = pu8RxData;
        pstcBufHandle->u32RxSize = u32Size;
        pstcBufHandle->u32RxCount = 0UL;
        pstcBufHandle->pu8TxData = pu8TxData;
        pstcBufHandle->u32TxSize = u32Size;
        pstcBufHandle->u32TxCount = 0UL;

        USART_FuncCmd(USARTx, (u32Func | USART_INT_TX_EMPTY | USART_INT_TX_CPLT), DISABLE);
        if (USART_CLK_SRC_EXTCLK == USART_GetClockSrc(USARTx)) {
            USART_FuncCmd(USARTx, (u32Func | USART_INT_TX_EMPTY), ENABLE);
        } else {
            USART_FuncCmd(USARTx, (u32Func | USART_INT_TX_CPLT), ENABLE);
        }
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Send receive an amount of data in full-duplex mode (non-blocking) in IRQ handler.
 * @param  [in] USARTx                  pointer to a USART instance.
 * @param  [in] pstcBufHandle           pointer to a stc_buf_handle_t structure.
 * @retval None
 */
static void CLOCKSYNC_TransReceiveCallback(CM_USART_TypeDef *USARTx, stc_buf_handle_t *pstcBufHandle)
{
    if (pstcBufHandle->u32RxCount != pstcBufHandle->u32RxSize) {
        if (USART_GetStatus(USARTx, USART_FLAG_RX_FULL) != RESET) {
            pstcBufHandle->pu8RxData[pstcBufHandle->u32RxCount] = (uint8_t)USART_ReadData(USARTx);
            pstcBufHandle->u32RxCount++;
        }
    }

    /* Check the latest data received */
    if (pstcBufHandle->u32RxCount == pstcBufHandle->u32RxSize) {
        /* Disable the USART RXNE && Error Interrupt */
        USART_FuncCmd(USARTx, USART_INT_RX, DISABLE);
    } else {
        if (pstcBufHandle->u32TxCount != pstcBufHandle->u32TxSize) {
            if (USART_GetStatus(USARTx, USART_FLAG_TX_EMPTY) == SET) {
                USART_WriteData(USARTx, (uint16_t)(pstcBufHandle->pu8TxData[pstcBufHandle->u32TxCount]));
                pstcBufHandle->u32TxCount++;

                /* Check the latest data transmitted */
                if (pstcBufHandle->u32TxCount == pstcBufHandle->u32TxSize) {
                    USART_FuncCmd(USARTx, (USART_INT_TX_EMPTY | USART_INT_TX_CPLT), DISABLE);
                }
            }
        }
    }
}

/**
 * @brief  USART TX Complete IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_TxComplete_IrqCallback(void)
{
    CLOCKSYNC_TransReceiveCallback(USART_UNIT, &m_stcBufHandle);
}

/**
 * @brief  USART TX Empty IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_TxEmpty_IrqCallback(void)
{
    CLOCKSYNC_TransReceiveCallback(USART_UNIT, &m_stcBufHandle);
}

/**
 * @brief  USART RX IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_RxFull_IrqCallback(void)
{
    CLOCKSYNC_TransReceiveCallback(USART_UNIT, &m_stcBufHandle);
}

/**
 * @brief  USART RX Error IRQ callback.
 * @param  None
 * @retval None
 */
static void USART_RxError_IrqCallback(void)
{
    (void)USART_ReadData(USART_UNIT);

    USART_ClearStatus(USART_UNIT, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

/**
 * @brief  Main function of clock-sync interrupt project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint32_t u32TxXferCount;
    uint32_t u32RxXferCount;
    stc_usart_clocksync_init_t stcClockSyncInit;

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
    GPIO_SetFunc(USART_CK_PORT, USART_CK_PIN, USART_CK_GPIO_FUNC);
    GPIO_SetFunc(USART_RX_PORT, USART_RX_PIN, USART_RX_GPIO_FUNC);
    GPIO_SetFunc(USART_TX_PORT, USART_TX_PIN, USART_TX_GPIO_FUNC);

    do {
        if (SET == BSP_KEY_GetStatus(BSP_KEY_1)) {
            m_i8USARTMode = EXAMPLE_USART_MODE_MASTER;
        } else if (SET == BSP_KEY_GetStatus(BSP_KEY_2)) {
            m_i8USARTMode = EXAMPLE_USART_MODE_SLAVE;
        } else {
            /* rsvd */
        }
    } while (m_i8USARTMode == -1);

    /* Enable peripheral clock */
    USART_FCG_ENABLE();

    /* Initialize CLKSYNC function. */
    (void)USART_ClockSync_StructInit(&stcClockSyncInit);
    if (EXAMPLE_USART_MODE_MASTER == m_i8USARTMode) {
        stcClockSyncInit.u32ClockSrc = USART_CLK_SRC_INTERNCLK;
        stcClockSyncInit.u32ClockDiv = USART_CLK_DIV16;
        stcClockSyncInit.u32Baudrate = 9600UL;
    } else {
        stcClockSyncInit.u32ClockSrc = USART_CLK_SRC_EXTCLK;
    }

    if (LL_OK != USART_ClockSync_Init(USART_UNIT, &stcClockSyncInit, NULL)) {
        BSP_LED_On(LED_RED);
        for (;;) {
        }
    }

    /* Register RX error IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_RX_ERR_IRQn, USART_RX_ERR_INT_SRC,
                                DDL_IRQ_PRIO_03, USART_RxError_IrqCallback);

    /* Register RX full IRQ handler && configure NVIC. */
    (void)INTC_IrqInstallHandle(USART_RX_FULL_IRQn, USART_RX_FULL_INT_SRC,
                                DDL_IRQ_PRIO_00, USART_RxFull_IrqCallback);

    if (EXAMPLE_USART_MODE_MASTER == m_i8USARTMode) {
        /* Register TX complete IRQ handler && configure NVIC. */
        (void)INTC_IrqInstallHandle(USART_TX_CPLT_IRQn, USART_TX_CPLT_INT_SRC,
                                    DDL_IRQ_PRIO_02, USART_TxComplete_IrqCallback);
    } else {
        /* Register TX empty IRQ handler && configure NVIC. */
        (void)INTC_IrqInstallHandle(USART_TX_EMPTY_IRQn, USART_TX_EMPTY_INT_SRC,
                                    DDL_IRQ_PRIO_01, USART_TxEmpty_IrqCallback);
    }
    /* MCU Peripheral registers write protected */
    LL_PERIPH_WP(LL_PERIPH_SEL);

    /* Wait key trigger in master mode */
    if (EXAMPLE_USART_MODE_MASTER == m_i8USARTMode) {
        while (RESET == BSP_KEY_GetStatus(BSP_KEY_2)) {
        }
    }
    /* Start the transmission process*/
    (void)CLOCKSYNC_TransReceive_INT(USART_UNIT, &m_stcBufHandle, m_au8TxData, m_au8RxData, COM_DATA_LEN);

    /* Wait tranmission complete */
    do {
        u32TxXferCount = m_stcBufHandle.u32TxCount;
        u32RxXferCount = m_stcBufHandle.u32RxCount;
    } while ((u32TxXferCount != COM_DATA_LEN) || (u32RxXferCount != COM_DATA_LEN));

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
