/**
 *******************************************************************************
 * @file  usart/usart_clocksync_dma/source/main.c
 * @brief This example demonstrates clock sync data receive and transfer by DMA.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Fix bug: possible null pointer for ClockSync_DMAConfig parameter pstcHandle
   2025-11-03       CDT             Fixed the return value type of ClockSync_Receive_DMA() and ClockSync_TransReceive_DMA()
                                    Change USART clock division: USART_CLK_DIV64 -> USART_CLK_DIV16
                                    Fix logic operation bug
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
 * @addtogroup USART_ClockSync_DMA
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/**
 * @brief IRQ configure structure
 */
typedef struct stc_irq_config {
    IRQn_Type                   enIrqNum;
    uint32_t                    u32IrqPrio;
    en_int_src_t                enIntSrc;
} stc_irq_config_t;

/**
 * @brief DMA configure structure
 */
typedef struct stc_dma_config {
    CM_DMA_TypeDef              *DMAx;
    uint8_t                     u8Ch;
    uint32_t                    u32Fcg;
    uint32_t                    u32TriggerSel;
    en_event_src_t              enTriggerEvent;
    uint32_t                    u32TransCompleteInt;
    stc_irq_config_t            stcIrqConfig;
    func_ptr_t                  pfnIrqCallback;
} stc_dma_config_t;

/**
 * @brief Clock synchronization mode handle
 */
typedef struct {
    CM_USART_TypeDef            *USARTx;

    stc_usart_clocksync_init_t  *pstcInit;

    const uint8_t               *pu8TxData;       /*!< Pointer to TX buffer       */

    uint16_t                    u16TxSize;        /*!< Tx size                    */

    __IO uint16_t               u16TxCount;       /*!< Tx counter                 */

    uint8_t                     *pu8RxData;       /*!< Pointer to RX buffer       */

    uint16_t                    u16RxSize;        /*!< Rx size                    */

    __IO uint16_t               u16RxCount;       /*!< Rx counter                 */

    stc_dma_config_t            *pstcRxDmaConfig; /*!< Tx DMA handle parameters   */

    stc_dma_config_t            *pstcTxDmaConfig; /*!< Rx DMA handle parameters   */
} stc_clocksync_handle_t;

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* Peripheral register WE/WP selection */
#define LL_PERIPH_SEL                   (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                                         LL_PERIPH_EFM)

/* DMA definition */
#define RX_DMA_UNIT                     (CM_DMA1)
#define RX_DMA_CH                       (DMA_CH0)
#define RX_DMA_FCG                      (FCG0_PERIPH_DMA1)
#define RX_DMA_TRIG_SEL                 (AOS_DMA1_0)
#define RX_DMA_TRIG_EVT_SRC             (EVT_SRC_USART1_RI)
#define RX_DMA_TC_INT                   (DMA_INT_TC_CH0)
#define RX_DMA_TC_FLAG                  (DMA_FLAG_TC_CH0)
#define RX_DMA_TC_IRQn                  (INT000_IRQn)
#define RX_DMA_TC_IRQ_PRIO              (DDL_IRQ_PRIO_DEFAULT)
#define RX_DMA_TC_INT_SRC               (INT_SRC_DMA1_TC0)

#define TX_DMA_UNIT                     (CM_DMA2)
#define TX_DMA_CH                       (DMA_CH0)
#define TX_DMA_FCG                      (FCG0_PERIPH_DMA2)
#define TX_DMA_TRIG_SEL                 (AOS_DMA2_0)
#define TX_DMA_TRIG_EVT_SRC             (EVT_SRC_USART1_TI)
#define TX_DMA_TC_INT                   (DMA_INT_TC_CH0)
#define TX_DMA_TC_FLAG                  (DMA_FLAG_TC_CH0)
#define TX_DMA_TC_IRQn                  (INT001_IRQn)
#define TX_DMA_TC_IRQ_PRIO              (DDL_IRQ_PRIO_DEFAULT)
#define TX_DMA_TC_INT_SRC               (INT_SRC_DMA2_TC0)

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
#define USART_FCG                       (FCG3_PERIPH_USART1)

/* master/slave device */
#define EXAMPLE_USART_MODE_MASTER       (0)
#define EXAMPLE_USART_MODE_SLAVE        (1)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void RX_DMA_TC_IrqCallback(void);
static void TX_DMA_TC_IrqCallback(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/* Data buffer */
static const uint8_t m_au8TxData[] = "USART clock-sync test based on DMA.";
static uint8_t m_au8RxData[ARRAY_SZ(m_au8TxData)];

/* Clock-sync handle */
stc_clocksync_handle_t m_stcClkSyncHandle;

/* Clock-sync initialization parameters */
stc_usart_clocksync_init_t m_stcClkSyncInit = {
    USART_CLK_SRC_INTERNCLK,
    USART_CLK_DIV16,
    9600UL,
    USART_FIRST_BIT_LSB,
    USART_HW_FLOWCTRL_RTS,
};

/* RX DMA configure parameters */
stc_dma_config_t m_stcRxDmaConfig = {
    RX_DMA_UNIT,
    RX_DMA_CH,
    RX_DMA_FCG,
    RX_DMA_TRIG_SEL,
    RX_DMA_TRIG_EVT_SRC,
    RX_DMA_TC_INT,
    {
        RX_DMA_TC_IRQn,
        RX_DMA_TC_IRQ_PRIO,
        RX_DMA_TC_INT_SRC,
    },
    RX_DMA_TC_IrqCallback,
};

/* TX DMA configure parameters */
stc_dma_config_t m_stcTxDmaConfig = {
    TX_DMA_UNIT,
    TX_DMA_CH,
    TX_DMA_FCG,
    TX_DMA_TRIG_SEL,
    TX_DMA_TRIG_EVT_SRC,
    TX_DMA_TC_INT,
    {
        TX_DMA_TC_IRQn,
        TX_DMA_TC_IRQ_PRIO,
        TX_DMA_TC_INT_SRC,
    },
    TX_DMA_TC_IrqCallback,
};

static __IO int8_t m_i8USARTMode = -1;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  RX DMA transfer complete IRQ callback function.
 * @param  None
 * @retval None
 */
void RX_DMA_TC_IrqCallback(void)
{
    m_stcClkSyncHandle.u16RxCount = m_stcClkSyncHandle.u16RxSize;
    DMA_ClearTransCompleteStatus(RX_DMA_UNIT, RX_DMA_TC_FLAG);
}

/**
 * @brief  TX DMA transfer complete IRQ callback function.
 * @param  None
 * @retval None
 */
void TX_DMA_TC_IrqCallback(void)
{
    m_stcClkSyncHandle.u16TxCount = m_stcClkSyncHandle.u16TxSize;
    DMA_ClearTransCompleteStatus(TX_DMA_UNIT, TX_DMA_TC_FLAG);
}

/**
 * @brief  Initialize DMA.
 * @param  None
 * @retval int32_t:
 *           - LL_OK:                   Initialize successfully.
 *           - LL_ERR_INVD_PARAM:       Initialization parameters is invalid.
 */
int32_t ClockSync_DMAConfig(stc_clocksync_handle_t *pstcHandle)
{
    stc_dma_init_t stcDmaInit;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    /* DMA&AOS FCG enable */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    if (pstcHandle != NULL) {
        if (pstcHandle->pstcRxDmaConfig != NULL) {
            FCG_Fcg0PeriphClockCmd(pstcHandle->pstcRxDmaConfig->u32Fcg, ENABLE);
            /* USART_RX_DMA */
            (void)DMA_StructInit(&stcDmaInit);
            stcDmaInit.u32IntEn = DMA_INT_ENABLE;
            stcDmaInit.u32BlockSize = 1UL;
            stcDmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
            stcDmaInit.u32SrcAddr = (uint32_t)(&pstcHandle->USARTx->RDR);
            stcDmaInit.u32SrcAddrInc = DMA_SRC_ADDR_FIX;
            stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_INC;
            i32Ret = DMA_Init(pstcHandle->pstcRxDmaConfig->DMAx, pstcHandle->pstcRxDmaConfig->u8Ch, &stcDmaInit);
            if (LL_OK == i32Ret) {
                (void)INTC_IrqInstallHandle(pstcHandle->pstcRxDmaConfig->stcIrqConfig.enIrqNum,
                                            pstcHandle->pstcRxDmaConfig->stcIrqConfig.enIntSrc,
                                            pstcHandle->pstcRxDmaConfig->stcIrqConfig.u32IrqPrio,
                                            pstcHandle->pstcRxDmaConfig->pfnIrqCallback);

                AOS_SetTriggerEventSrc(pstcHandle->pstcRxDmaConfig->u32TriggerSel,
                                       pstcHandle->pstcRxDmaConfig->enTriggerEvent);

                DMA_Cmd(RX_DMA_UNIT, ENABLE);
                DMA_Cmd(pstcHandle->pstcRxDmaConfig->DMAx, ENABLE);
                DMA_TransCompleteIntCmd(pstcHandle->pstcRxDmaConfig->DMAx,
                                        pstcHandle->pstcRxDmaConfig->u32TransCompleteInt,
                                        ENABLE);
            }
        }

        if (pstcHandle->pstcTxDmaConfig != NULL) {
            FCG_Fcg0PeriphClockCmd(pstcHandle->pstcTxDmaConfig->u32Fcg, ENABLE);
            /* USART_TX_DMA */
            (void)DMA_StructInit(&stcDmaInit);
            stcDmaInit.u32IntEn = DMA_INT_ENABLE;
            stcDmaInit.u32BlockSize = 1UL;
            stcDmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
            stcDmaInit.u32DestAddr = (uint32_t)(&pstcHandle->USARTx->TDR);
            stcDmaInit.u32SrcAddrInc = DMA_SRC_ADDR_INC;
            stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_FIX;
            i32Ret = DMA_Init(pstcHandle->pstcTxDmaConfig->DMAx, pstcHandle->pstcTxDmaConfig->u8Ch, &stcDmaInit);
            if (LL_OK == i32Ret) {
                (void)INTC_IrqInstallHandle(pstcHandle->pstcTxDmaConfig->stcIrqConfig.enIrqNum,
                                            pstcHandle->pstcTxDmaConfig->stcIrqConfig.enIntSrc,
                                            pstcHandle->pstcTxDmaConfig->stcIrqConfig.u32IrqPrio,
                                            pstcHandle->pstcTxDmaConfig->pfnIrqCallback);

                AOS_SetTriggerEventSrc(pstcHandle->pstcTxDmaConfig->u32TriggerSel,
                                       pstcHandle->pstcTxDmaConfig->enTriggerEvent);

                DMA_Cmd(pstcHandle->pstcTxDmaConfig->DMAx, ENABLE);
                DMA_TransCompleteIntCmd(pstcHandle->pstcTxDmaConfig->DMAx,
                                        pstcHandle->pstcTxDmaConfig->u32TransCompleteInt, ENABLE);
            }
        }

        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Configure clock-sync in DMA mode.
 * @param  [in]  pstcHandle             Pointer to a @ref stc_clocksync_handle_t structure.
 * @param  [in]  USARTx                 Pointer to USART instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_USARTx:            USART unit instance register base
 * @param  [in]  u32Fcg                 USART clock gate
 * @param  [in]  pstcInit               Pointer to a @ref stc_usart_clocksync_init_t structure.
 * @param  [out] pstcRxDmaConfig        Pointer to a @ref stc_dma_config_t structure.
 * @param  [in]  pstcTxDmaConfig        Pointer to a @ref stc_dma_config_t structure.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       Parameters invalid.
 */
int32_t ClockSync_Config(stc_clocksync_handle_t *pstcHandle, CM_USART_TypeDef *USARTx, uint32_t u32Fcg,
                         stc_usart_clocksync_init_t *pstcInit, stc_dma_config_t *pstcRxDmaConfig, stc_dma_config_t *pstcTxDmaConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((pstcHandle != NULL) && (USARTx != NULL) && (pstcInit != NULL)) {
        /* Bind parameters */
        pstcHandle->USARTx = USARTx;
        pstcHandle->pstcInit = pstcInit;
        pstcHandle->pstcRxDmaConfig = pstcRxDmaConfig;
        pstcHandle->pstcTxDmaConfig = pstcTxDmaConfig;

        /* Configure DMA */
        ClockSync_DMAConfig(pstcHandle);

        /* Enable peripheral clock */
        FCG_Fcg3PeriphClockCmd(u32Fcg, ENABLE);

        if (EXAMPLE_USART_MODE_MASTER == m_i8USARTMode) {
            pstcInit->u32ClockSrc = USART_CLK_SRC_INTERNCLK;
        } else {
            pstcInit->u32ClockSrc = USART_CLK_SRC_EXTCLK;
        }
        /* Initialize clock sync function. */
        i32Ret = USART_ClockSync_Init(USARTx, pstcInit, NULL);
        DDL_ASSERT(i32Ret == LL_OK);
    }

    return i32Ret;
}

/**
 * @brief  Clock-sync receive data in DMA mode.
 * @param  [in] pstcHandle              Pointer to a @ref stc_clocksync_handle_t structure.
 * @param  [in] pu8Data                 The pointer to data received buffer
 * @param  [in] u16Len                  Amount of data to be received.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       Parameters invalid.
 */
int32_t ClockSync_Receive_DMA(stc_clocksync_handle_t *pstcHandle, uint8_t *pu8Data, uint16_t u16Len)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((NULL != pstcHandle) && (NULL != pu8Data) && (0UL != u16Len)) {
        if (NULL != pstcHandle->pstcRxDmaConfig) {
            /***************************** RX_DMA *****************************/
            /* Configure buffer parameters */
            pstcHandle->pu8RxData = pu8Data;
            pstcHandle->u16RxCount = 0U;
            pstcHandle->u16RxSize = u16Len;

            /* Configure DMA destination address */
            DMA_SetDestAddr(pstcHandle->pstcRxDmaConfig->DMAx, pstcHandle->pstcRxDmaConfig->u8Ch, (uint32_t)pu8Data);

            /* Configure DMA transfer count */
            DMA_SetTransCount(pstcHandle->pstcRxDmaConfig->DMAx, pstcHandle->pstcRxDmaConfig->u8Ch, u16Len);

            /* Enable DMA */
            DMA_ChCmd(pstcHandle->pstcRxDmaConfig->DMAx, pstcHandle->pstcRxDmaConfig->u8Ch, ENABLE);

            /***************************** Enable USART TX ********************/
            /* Clear USART_CR.TE and RE.
               For Write 1 to USART_CR.TE or RE only when USART_CR.TE and RE value is 0 in clock-sync mode */
            USART_FuncCmd(pstcHandle->USARTx, (USART_RX | USART_TX), DISABLE);

            USART_FuncCmd(pstcHandle->USARTx, USART_RX, ENABLE);
            i32Ret = LL_OK;
        }
    }

    return i32Ret;
}

/**
 * @brief  Clock-sync transmit data in DMA mode.
 * @param  [in] pstcHandle              Pointer to a @ref stc_clocksync_handle_t structure.
 * @param  [in] pu8Data                 The pointer to data transmitted buffer
 * @param  [in] u16Len                  Amount of data to be transmitted.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       Parameters invalid.
 */
int32_t ClockSync_Trans_DMA(stc_clocksync_handle_t *pstcHandle, const uint8_t *pu8Data, uint16_t u16Len)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((NULL != pstcHandle) && (NULL != pu8Data) && (0UL != u16Len)) {
        if (NULL != pstcHandle->pstcTxDmaConfig) {
            /***************************** TX_DMA *****************************/
            /* Configure buffer parameters */
            pstcHandle->pu8TxData = pu8Data;
            pstcHandle->u16TxCount = 0U;
            pstcHandle->u16TxSize = u16Len;

            /* Configure DMA source address */
            DMA_SetSrcAddr(pstcHandle->pstcTxDmaConfig->DMAx, pstcHandle->pstcTxDmaConfig->u8Ch, (uint32_t)pu8Data);

            /* Configure DMA transfer count */
            DMA_SetTransCount(pstcHandle->pstcTxDmaConfig->DMAx, pstcHandle->pstcTxDmaConfig->u8Ch, u16Len);

            /* Enable DMA */
            DMA_ChCmd(pstcHandle->pstcTxDmaConfig->DMAx, pstcHandle->pstcTxDmaConfig->u8Ch, ENABLE);

            /***************************** Enable USART TX ********************/
            /* Clear USART_CR.TE and RE.
               For Write 1 to USART_CR.TE or RE only when USART_CR.TE and RE value is 0 in clock-sync mode */
            USART_FuncCmd(pstcHandle->USARTx, (USART_RX | USART_TX), DISABLE);

            USART_FuncCmd(pstcHandle->USARTx, USART_TX, ENABLE);
            i32Ret = LL_OK;
        }
    }

    return i32Ret;
}

/**
 * @brief  Clock-sync transmit && receive data in DMA mode.
 * @param  [in]  pstcHandle             Pointer to a @ref stc_clocksync_handle_t structure.
 * @param  [in]  pu8TxData              The pointer to data transmitted buffer
 * @param  [out] pu8RxData              The pointer to data received buffer
 * @param  [in]  u32Len                 Amount of data to be sent and received.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       Parameters invalid.
 */
int32_t ClockSync_TransReceive_DMA(stc_clocksync_handle_t *pstcHandle, const uint8_t *pu8TxData,
                                   uint8_t *pu8RxData, uint16_t u16Len)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((NULL != pstcHandle) && (NULL != pu8TxData) && (NULL != pu8RxData) && (0UL != u16Len)) {
        if ((NULL != pstcHandle->pstcRxDmaConfig) && (NULL != pstcHandle->pstcTxDmaConfig)) {
            /***************************** RX_DMA *****************************/
            /* Configure buffer parameters */
            pstcHandle->pu8RxData = pu8RxData;
            pstcHandle->u16RxCount = 0U;
            pstcHandle->u16RxSize = u16Len;

            /* Configure DMA destination address */
            DMA_SetDestAddr(pstcHandle->pstcRxDmaConfig->DMAx, pstcHandle->pstcRxDmaConfig->u8Ch, (uint32_t)pu8RxData);

            /* Configure DMA transfer count */
            DMA_SetTransCount(pstcHandle->pstcRxDmaConfig->DMAx, pstcHandle->pstcRxDmaConfig->u8Ch, u16Len);

            /* Enable DMA */
            DMA_ChCmd(pstcHandle->pstcRxDmaConfig->DMAx, pstcHandle->pstcRxDmaConfig->u8Ch, ENABLE);

            /***************************** TX_DMA *****************************/
            /* Configure buffer parameters */
            pstcHandle->pu8TxData = pu8TxData;
            pstcHandle->u16TxCount = 0U;
            pstcHandle->u16TxSize = u16Len;

            /* Configure DMA source address */
            DMA_SetSrcAddr(pstcHandle->pstcTxDmaConfig->DMAx, pstcHandle->pstcTxDmaConfig->u8Ch, (uint32_t)pu8TxData);

            /* Configure DMA transfer count */
            DMA_SetTransCount(pstcHandle->pstcTxDmaConfig->DMAx, pstcHandle->pstcTxDmaConfig->u8Ch, u16Len);

            /* Enable DMA */
            DMA_ChCmd(pstcHandle->pstcTxDmaConfig->DMAx, pstcHandle->pstcTxDmaConfig->u8Ch, ENABLE);

            /********************* Enable USART TX and RX *********************/
            /* Clear USART_CR.TE and RE.
               For Write 1 to USART_CR.TE or RE only when USART_CR.TE and RE value is 0 in clock-sync mode */
            USART_FuncCmd(pstcHandle->USARTx, (USART_RX | USART_TX), DISABLE);

            USART_FuncCmd(pstcHandle->USARTx, (USART_RX | USART_TX), ENABLE);
            i32Ret = LL_OK;
        }
    }

    return i32Ret;
}

/**
 * @brief  Main function of clock-sync DMA project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    en_flag_status_t enStatus = RESET;

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

    /* Configure USART clock sync function. */
    ClockSync_Config(&m_stcClkSyncHandle, USART_UNIT, USART_FCG,
                     &m_stcClkSyncInit, &m_stcRxDmaConfig, &m_stcTxDmaConfig);

    /* Wait key trigger in master mode */
    if (EXAMPLE_USART_MODE_MASTER == m_i8USARTMode) {
        while (RESET == BSP_KEY_GetStatus(BSP_KEY_2)) {
        }
    }

    if (EXAMPLE_USART_MODE_MASTER == m_i8USARTMode) {
        /********************************* Master *********************************/
        /* Transmit data by DMA */
        ClockSync_Trans_DMA(&m_stcClkSyncHandle, m_au8TxData, ARRAY_SZ(m_au8TxData));

        /* Wait transmit completely */
        while ((m_stcClkSyncHandle.u16TxCount != m_stcClkSyncHandle.u16TxSize) || (RESET == enStatus)) {
            enStatus = USART_GetStatus(m_stcClkSyncHandle.USARTx, USART_FLAG_TX_CPLT);
        }

        /* Delay for configuring slave node */
        DDL_DelayMS(100UL);
    } else {
        /********************************* Slave **********************************/
        /* Receive data by DMA */
        ClockSync_Receive_DMA(&m_stcClkSyncHandle, m_au8RxData, ARRAY_SZ(m_au8RxData));

        /* Wait transmit completely */
        while (m_stcClkSyncHandle.u16RxCount != m_stcClkSyncHandle.u16RxSize) {
        }

        /* Compare data */
        if (0 != memcmp(m_au8RxData, m_au8TxData, m_stcClkSyncHandle.u16RxSize)) {
            BSP_LED_On(LED_RED);
        }
    }

    /* Transmit and receive data by DMA */
    ClockSync_TransReceive_DMA(&m_stcClkSyncHandle, m_au8TxData, m_au8RxData, ARRAY_SZ(m_au8RxData));

    /* Wait transmit and receive completely */
    while ((m_stcClkSyncHandle.u16RxCount != m_stcClkSyncHandle.u16RxSize) || \
           (m_stcClkSyncHandle.u16TxCount != m_stcClkSyncHandle.u16TxSize)) {
    }

    /* Compare data */
    if (0 != memcmp(m_au8RxData, m_au8TxData, m_stcClkSyncHandle.u16RxSize)) {
        BSP_LED_On(LED_RED);
    } else {
        BSP_LED_On(LED_BLUE);
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
