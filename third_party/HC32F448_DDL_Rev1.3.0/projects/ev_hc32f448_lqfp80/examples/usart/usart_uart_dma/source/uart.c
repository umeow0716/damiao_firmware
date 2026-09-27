/**
 *******************************************************************************
 * @file  usart/usart_uart_dma/source/uart.c
 * @brief This file provides functions for UART data receive and transfer by DMA.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2025-11-03       CDT             First version
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
#include "uart.h"

/**
 * @addtogroup USART_UART_DMA
 * @{
 */

/**
 * @defgroup UART UART
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/**
 * @defgroup UART_Local_Types UART Local Types
 * @{
 */
/**
 * @defgroup DMA_Register DMA Register
 * @{
 */
#define DMA_REG_ADDR(_REG_)             ((uint32_t)(&(_REG_)))
#define DMA_REG32(_ADDR_)               ((__IO uint32_t *)(_ADDR_))

#define DMA_REG_DTCTL(UNIT, CH)         DMA_REG32(DMA_REG_ADDR((UNIT)->DTCTL0) + ((CH) * 0x40UL))
#define DMA_REG_MONDTCTL(UNIT, CH)      DMA_REG32(DMA_REG_ADDR((UNIT)->MONDTCTL0) + ((CH) * 0x40UL))

#define DMA_DTCTL_CNT_VAL(UNIT, CH)     (READ_REG32(*DMA_REG_DTCTL(UNIT, CH)) >> DMA_DTCTL_CNT_POS)
#define DMA_MONDTCTL_CNT_VAL(UNIT, CH)  (READ_REG32(*DMA_REG_MONDTCTL(UNIT, CH)) >> DMA_MONDTCTL_CNT_POS)
/**
 * @}
 */

/**
 * @defgroup FCG_Clock FCG Clock
 * @{
 */
#define FCG_CLK_USART               FCG_Fcg3PeriphClockCmd
#define FCG_CLK_TMR0                FCG_Fcg2PeriphClockCmd
#define FCG_CLK_DMA                 FCG_Fcg0PeriphClockCmd
/**
 * @}
 */
/**
 * @}
 */

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
 * @defgroup UART_Local_Functions UART Local Functions
 * @{
 */

/**
 * @brief  Stop timeout timer.
 * @param  [in]  TMR0x                  Pointer to TMR0 instance register base.
 *         This parameter can be one of the following values:
 *           @arg  CM_TMR0_x or CM_TMR0
 * @param  [in]  u32Ch                  TMR0 channel.
 *         This parameter can be a value @ref TMR0_Channel
 * @retval None
 */
static void USART_StopTimeoutTimer(CM_TMR0_TypeDef *TMR0x, uint32_t u32Ch)
{
    uint32_t u32ClrMask;
    uint32_t u32SetMask;
    uint32_t u32BitOffset;

    u32BitOffset = 16UL * u32Ch;

    /* Set: TMR0_BCONR.SYNCLKA<B>=1, TMR0_BCONR.SYNA<B>=0 */
    u32ClrMask = (TMR0_BCONR_SYNCLKA | TMR0_BCONR_SYNSA) << u32BitOffset;
    u32SetMask = TMR0_BCONR_SYNCLKA << u32BitOffset;
    MODIFY_REG32(TMR0x->BCONR, u32ClrMask, u32SetMask);

    /* Set: TMR0_BCONR.CSTA<B>=0, TMR0_BCONR.SYNCLKA<B>=0, TMR0_BCONR.SYNSA<B>=1 */
    u32ClrMask = (TMR0_BCONR_SYNCLKA | TMR0_BCONR_SYNSA | TMR0_BCONR_CSTA) << u32BitOffset;
    u32SetMask = TMR0_BCONR_SYNSA << u32BitOffset;
    MODIFY_REG32(TMR0x->BCONR, u32ClrMask, u32SetMask);
}

/**
 * @brief  Configure RX timeout for UART RX.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @retval None
 */
static void UART_RXTO_Config(stc_uart_handle_t *pstcHandle)
{
    CM_TMR0_TypeDef *TMR0x;
    uint32_t u32Ch;
    uint16_t u16Rtb;
    uint32_t u32Alpha;
    uint32_t u32CkDiv;
    uint32_t u32CmpVal;
    stc_tmr0_init_t stcTmr0Init;

    TMR0x = pstcHandle->stcRxto.TMR0x;
    u32Ch = pstcHandle->stcRxto.u32Ch;
    u16Rtb = pstcHandle->stcRxto.u16TimeoutBit;

    /* Enable TMR0 clock */
    FCG_CLK_TMR0(pstcHandle->stcRxto.u32Fcg, ENABLE);

    /* TMR0 initialize */
    TMR0_SetCountValue(TMR0x, u32Ch, 0U);
    (void)TMR0_StructInit(&stcTmr0Init);
    stcTmr0Init.u32ClockDiv = TMR0_CLK_DIV1;
    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_XTAL32;
    if (TMR0_CLK_DIV1 == stcTmr0Init.u32ClockDiv) {
        u32Alpha = 7UL;
    } else if (TMR0_CLK_DIV2 == stcTmr0Init.u32ClockDiv) {
        u32Alpha = 5UL;
    } else if ((TMR0_CLK_DIV4 == stcTmr0Init.u32ClockDiv) || \
               (TMR0_CLK_DIV8 == stcTmr0Init.u32ClockDiv) || \
               (TMR0_CLK_DIV16 == stcTmr0Init.u32ClockDiv)) {
        u32Alpha = 3UL;
    } else {
        u32Alpha = 2UL;
    }

    /* TMR0_CMPA<B>R calculation formula: CMPA<B>R = (RTB / (2 ^ CKDIVA<B>)) - alpha */
    u32CkDiv = (stcTmr0Init.u32ClockDiv >> TMR0_BCONR_CKDIVA_POS);
    u32CmpVal = ((u16Rtb + u32CkDiv - 1UL) >> u32CkDiv) - u32Alpha;
    DDL_ASSERT(u32CmpVal <= 0xFFFFUL);
    stcTmr0Init.u16CompareValue = (uint16_t)(u32CmpVal);
    (void)TMR0_Init(TMR0x, u32Ch, &stcTmr0Init);
    TMR0_HWStartCondCmd(TMR0x, u32Ch, ENABLE);
    TMR0_HWClearCondCmd(TMR0x, u32Ch, ENABLE);
    TMR0_ClearStatus(TMR0x, (uint32_t)(0x1UL << (u32Ch * TMR0_STFLR_CMFB_POS)));

    NVIC_EnableIRQ(pstcHandle->stcRxto.stcIrq.enIRQn);
    USART_ClearStatus(pstcHandle->USARTx, USART_FLAG_RX_TIMEOUT);
    USART_FuncCmd(pstcHandle->USARTx, (USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT), ENABLE);

    (void)INTC_IrqInstallHandle(pstcHandle->stcRxto.stcIrq.enIRQn,  pstcHandle->stcRxto.stcIrq.enIntSrc, \
                                pstcHandle->stcRxto.stcIrq.u16Prio, pstcHandle->stcRxto.stcIrq.pfnIrqCallback);
}

/**
 * @brief  Configure DMA for UART RX.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @retval None
 */
static void UART_RxDMA_Config(stc_uart_handle_t *pstcHandle)
{
    uint32_t u32BufSize;
    uint32_t u32HalfBufSize;
    stc_dma_init_t stcDmaInit;
    stc_dma_llp_init_t stcLlpInit;

    /* Initialize buffer */
    u32BufSize = ARRAY_SZ(pstcHandle->au8RxData);
    u32HalfBufSize = u32BufSize >> 1;
    (void)BUF_Init(&pstcHandle->stcRxRingBuf, pstcHandle->au8RxData, u32BufSize);

    /* Enable DMA clock */
    FCG_CLK_DMA(pstcHandle->stcDmaRx.u32Fcg, ENABLE);

    /* Disable DMA channel */
    (void)DMA_ChCmd(pstcHandle->stcDmaRx.DMAx, pstcHandle->stcDmaRx.u8Ch, DISABLE);

    /* Initialize DMA */
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn       = DMA_INT_ENABLE;
    stcDmaInit.u32SrcAddr     = (uint32_t)(&pstcHandle->USARTx->RDR);
    stcDmaInit.u32DestAddr    = (uint32_t)pstcHandle->stcRxRingBuf.pu8Data;
    stcDmaInit.u32DataWidth   = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32BlockSize   = 1UL;
    stcDmaInit.u32TransCount  = u32HalfBufSize;
    stcDmaInit.u32SrcAddrInc  = DMA_SRC_ADDR_FIX;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_INC;
    (void)DMA_Init(pstcHandle->stcDmaRx.DMAx, pstcHandle->stcDmaRx.u8Ch, &stcDmaInit);

    /* Set DMA remain count */
    pstcHandle->u16DmaRxRemainingCnt = (uint16_t)stcDmaInit.u32TransCount;

    /* Initialize LLP */
    stcLlpInit.u32State   = DMA_LLP_ENABLE;
    stcLlpInit.u32Mode    = DMA_LLP_WAIT;
    stcLlpInit.u32Addr    = (uint32_t)&pstcHandle->stcLlpDesc[0];
    (void)DMA_LlpInit(pstcHandle->stcDmaRx.DMAx, pstcHandle->stcDmaRx.u8Ch, &stcLlpInit);

    /* Configure LLP descriptor */
    pstcHandle->stcLlpDesc[0].SARx   = stcDmaInit.u32SrcAddr;
    pstcHandle->stcLlpDesc[0].DARx   = stcDmaInit.u32DestAddr + stcDmaInit.u32TransCount;
    pstcHandle->stcLlpDesc[0].DTCTLx = (((u32BufSize - u32HalfBufSize) << DMA_DTCTL_CNT_POS) | \
                                        (stcDmaInit.u32BlockSize << DMA_DTCTL_BLKSIZE_POS));
    pstcHandle->stcLlpDesc[0].LLPx  = (uint32_t)&pstcHandle->stcLlpDesc[1];
    pstcHandle->stcLlpDesc[0].CHCTLx = (stcDmaInit.u32SrcAddrInc | stcDmaInit.u32DestAddrInc | stcDmaInit.u32DataWidth | \
                                        stcDmaInit.u32IntEn      | stcLlpInit.u32State       | stcLlpInit.u32Mode);
    pstcHandle->stcLlpDesc[1].SARx  = pstcHandle->stcLlpDesc[0].SARx;
    pstcHandle->stcLlpDesc[1].DARx  = stcDmaInit.u32DestAddr;
    pstcHandle->stcLlpDesc[1].DTCTLx = (stcDmaInit.u32TransCount << DMA_DTCTL_CNT_POS) | (stcDmaInit.u32BlockSize << DMA_DTCTL_BLKSIZE_POS);
    pstcHandle->stcLlpDesc[1].LLPx  = (uint32_t)&pstcHandle->stcLlpDesc[0];
    pstcHandle->stcLlpDesc[1].CHCTLx = pstcHandle->stcLlpDesc[0].CHCTLx;

    /* Enable DMA interrupt */
    NVIC_EnableIRQ(pstcHandle->stcDmaRx.stcIrq.enIRQn);

    /* Enable DMA module */
    DMA_Cmd(pstcHandle->stcDmaRx.DMAx, ENABLE);
    AOS_SetTriggerEventSrc(pstcHandle->stcDmaRx.u32TriggerSel, pstcHandle->stcDmaRx.enTriggerEvent);
    (void)DMA_ChCmd(pstcHandle->stcDmaRx.DMAx, pstcHandle->stcDmaRx.u8Ch, ENABLE);

    (void)INTC_IrqInstallHandle(pstcHandle->stcDmaRx.stcIrq.enIRQn,  pstcHandle->stcDmaRx.stcIrq.enIntSrc, \
                                pstcHandle->stcDmaRx.stcIrq.u16Prio, pstcHandle->stcDmaRx.stcIrq.pfnIrqCallback);
}

/**
 * @brief  Configure DMA for UART TX.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @retval None
 */
static void UART_TxDMA_Config(stc_uart_handle_t *pstcHandle)
{
    stc_dma_init_t stcDmaInit;

    /* Enable DMA clock */
    FCG_CLK_DMA(pstcHandle->stcDmaTx.u32Fcg, ENABLE);

    /* Disable DMA channel */
    (void)DMA_ChCmd(pstcHandle->stcDmaTx.DMAx, pstcHandle->stcDmaTx.u8Ch, DISABLE);

    /* Initialize DMA */
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn       = DMA_INT_DISABLE;
    stcDmaInit.u32SrcAddr     = 0UL;
    stcDmaInit.u32DestAddr    = (uint32_t)(&pstcHandle->USARTx->TDR);
    stcDmaInit.u32DataWidth   = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32BlockSize   = 1UL;
    stcDmaInit.u32TransCount  = 0UL;
    stcDmaInit.u32SrcAddrInc  = DMA_SRC_ADDR_INC;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    (void)DMA_Init(pstcHandle->stcDmaTx.DMAx, pstcHandle->stcDmaTx.u8Ch, &stcDmaInit);

    /* Enable DMA module */
    DMA_Cmd(pstcHandle->stcDmaTx.DMAx, ENABLE);
    AOS_SetTriggerEventSrc(pstcHandle->stcDmaTx.u32TriggerSel, pstcHandle->stcDmaTx.enTriggerEvent);
}

/**
 * @brief  Configure DMA.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @retval None
 */
static void UART_DMA_Config(stc_uart_handle_t *pstcHandle)
{
    /* AOS FCG enable */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    UART_RxDMA_Config(pstcHandle);
    UART_TxDMA_Config(pstcHandle);
}

/**
 * @}
 */

/**
 * @defgroup UART_Global_Functions UART Global Functions
 * @{
 */
/**
 * @brief  Initialize UART in DMA mode.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       pstcHandle/pstcHandle->USARTx is NULL.
 */
int32_t UART_Init(stc_uart_handle_t *pstcHandle)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((NULL != pstcHandle) && (NULL != pstcHandle->USARTx)) {
        /* Enable peripheral clock */
        FCG_CLK_USART(pstcHandle->u32UartFcg, ENABLE);

        (void)USART_DeInit(pstcHandle->USARTx);

        /* Initialize UART */
        i32Ret = USART_UART_Init(pstcHandle->USARTx, &pstcHandle->stcUartInit, NULL);
        if (LL_OK == i32Ret) {
            (void)INTC_IrqInstallHandle(pstcHandle->stcUartRxErrIrq.enIRQn, pstcHandle->stcUartRxErrIrq.enIntSrc, \
                                        pstcHandle->stcUartRxErrIrq.u16Prio, pstcHandle->stcUartRxErrIrq.pfnIrqCallback);

            (void)INTC_IrqInstallHandle(pstcHandle->stcUartTxCmpltIrq.enIRQn, pstcHandle->stcUartTxCmpltIrq.enIntSrc, \
                                        pstcHandle->stcUartTxCmpltIrq.u16Prio, pstcHandle->stcUartTxCmpltIrq.pfnIrqCallback);

            /* Configure UART RX timeout */
            UART_RXTO_Config(pstcHandle);

            /* Configure DMA for UART RX and TX */
            UART_DMA_Config(pstcHandle);

            /* Enable USART RX && RXTO function and interrupt */
            USART_FuncCmd(pstcHandle->USARTx, (USART_RX | USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT), ENABLE);
        }
    }

    return i32Ret;
}

/**
 * @brief  UART transmit by DMA.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @param  [in] pu8Data             The pointer to data transmitted buffer.
 * @param  [in] u16Len              Amount of frame to be sent.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_BUSY:             UART is busy.
 *           - LL_ERR_INVD_PARAM:       If one of following cases matches:
 *                                      - pstcHandle
 *                                      - pu8Data is NULL.
 *                                      - u16Len value is
 */
int32_t UART_Write(stc_uart_handle_t *pstcHandle, uint8_t *pu8Data, uint16_t u16Len)
{
    int32_t i32Ret;

    if ((NULL == pstcHandle) || (NULL == pu8Data) || (0U == u16Len)) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        if (USART_GetStatus(pstcHandle->USARTx, USART_FLAG_TX_CPLT) == RESET) {
            i32Ret = LL_ERR_BUSY;
        } else {
            /* Set DMA TX */
            (void)DMA_SetSrcAddr(pstcHandle->stcDmaTx.DMAx, pstcHandle->stcDmaTx.u8Ch, (uint32_t)pu8Data);
            (void)DMA_SetTransCount(pstcHandle->stcDmaTx.DMAx, pstcHandle->stcDmaTx.u8Ch, u16Len);
            (void)DMA_ChCmd(pstcHandle->stcDmaTx.DMAx, pstcHandle->stcDmaTx.u8Ch, ENABLE);

            /* Enable USART TX and Tx Complete interrupt */
            USART_FuncCmd(pstcHandle->USARTx, USART_TX, DISABLE);
            USART_FuncCmd(pstcHandle->USARTx, USART_TX, ENABLE);
            USART_FuncCmd(pstcHandle->USARTx, USART_INT_TX_CPLT, ENABLE);
            i32Ret = LL_OK;
        }
    }

    return i32Ret;
}

/**
 * @brief  UART read buffer.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @param  [in] pu8Data             The pointer to data buffer.
 * @param  [in] u16Len              Number of bytes to read.
 * @param  [in] pu16LenRead         Number of bytes have been read.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_BUSY:             UART is busy.
 *           - LL_ERR_BUF_EMPTY         Buffer empty
 *           - LL_ERR_INVD_PARAM:       If one of following cases matches:
 *                                      - pstcHandle is NULL.
 *                                      - pu8Data is NULL.
 *                                      - u16LenToRead value is 0.
 *                                      - pu16LenRead is NULL.
 */
int32_t UART_Read(stc_uart_handle_t *pstcHandle, uint8_t *pu8Data, uint16_t u16LenToRead, uint16_t *pu16LenRead)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    uint16_t u16UsedSize;
    uint16_t u16ReadLen;

    if ((NULL != pstcHandle) && (NULL != pu8Data) && (0U != u16LenToRead) && (NULL != pu16LenRead)) {
        if (true == BUF_Empty(&pstcHandle->stcRxRingBuf)) {
            *pu16LenRead = 0U;
            i32Ret = LL_ERR_BUF_EMPTY;
        } else {
            u16UsedSize = (uint16_t)BUF_UsedSize(&pstcHandle->stcRxRingBuf);

            if (u16LenToRead > u16UsedSize) {
                u16LenToRead = u16UsedSize;
            }

            u16ReadLen = (uint16_t)BUF_Read(&pstcHandle->stcRxRingBuf, pu8Data, (uint32_t)u16LenToRead);
            *pu16LenRead = u16ReadLen;
            i32Ret = LL_OK;
        }
    }

    return i32Ret;
}

/**
 * @brief  UART RX error IRQ callback.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @retval None
 * @note USART RX error IRQ handler must call this function.
 */
void UART_RxError_IrqCallback(stc_uart_handle_t *pstcHandle)
{
    (void)USART_ReadData(pstcHandle->USARTx);

    USART_ClearStatus(pstcHandle->USARTx, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

/**
 * @brief  UART_RX DMA transfer complete IRQ callback function.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @retval None
 * @note RX_DMA IRQ handler must call this function.
*/
void UART_RxDma_IrqCallback(stc_uart_handle_t *pstcHandle)
{
    uint16_t u16RecvLen;
    uint16_t u16UpdateLen;

    u16RecvLen = pstcHandle->u16DmaRxRemainingCnt;
    u16UpdateLen = (uint16_t)BUF_UpdateInputIndex(&pstcHandle->stcRxRingBuf, u16RecvLen);
    if (u16UpdateLen != u16RecvLen) {
        /* Warning: au8RxData of stc_uart_handle_t has no enough buffer for saving data,
                    please increase the UART_RX_BUF_SIZE option. */
        DDL_ASSERT(false);
    }

    pstcHandle->u16DmaRxRemainingCnt = (uint16_t)DMA_DTCTL_CNT_VAL(pstcHandle->stcDmaRx.DMAx, pstcHandle->stcDmaRx.u8Ch);

    USART_StopTimeoutTimer(pstcHandle->stcRxto.TMR0x, pstcHandle->stcRxto.u32Ch);

    DMA_ClearTransCompleteStatus(pstcHandle->stcDmaRx.DMAx, pstcHandle->stcDmaRx.u32TransCompleteFlag);
}

/**
 * @brief  UART RX timeout IRQ callback.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @retval None
 * @note USART RX timeout IRQ handler must call this function.
 */
void UART_RxTimeout_IrqCallback(stc_uart_handle_t *pstcHandle)
{
    uint16_t u16RecvLen;
    uint16_t u16UpdateLen;
    uint16_t u16DmaRemainCnt;

    u16DmaRemainCnt = (uint16_t)DMA_MONDTCTL_CNT_VAL(pstcHandle->stcDmaRx.DMAx, pstcHandle->stcDmaRx.u8Ch);

    u16RecvLen = pstcHandle->u16DmaRxRemainingCnt - u16DmaRemainCnt;
    u16UpdateLen = (uint16_t)BUF_UpdateInputIndex(&pstcHandle->stcRxRingBuf, u16RecvLen);
    if (u16UpdateLen != u16RecvLen) {
        /* Warning: au8RxData of stc_uart_handle_t has no enough buffer for saving data,
                    please increase the UART_RX_BUF_SIZE option. */
        DDL_ASSERT(false);
    }

    pstcHandle->u16DmaRxRemainingCnt = u16DmaRemainCnt;

    USART_StopTimeoutTimer(pstcHandle->stcRxto.TMR0x, pstcHandle->stcRxto.u32Ch);

    USART_ClearStatus(pstcHandle->USARTx, USART_FLAG_RX_TIMEOUT);
}

/**
 * @brief  UART TX transfer complete IRQ callback function.
 * @param  [in] pstcHandle          Pointer to a @ref stc_uart_handle_t structure.
 * @retval None
 * @note USART TX complete IRQ handler must call this function.
 */
void UART_TxCmplt_IrqCallback(stc_uart_handle_t *pstcHandle)
{
    USART_FuncCmd(pstcHandle->USARTx, USART_INT_TX_CPLT, DISABLE);
    USART_FuncCmd(pstcHandle->USARTx, USART_TX, DISABLE);
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
