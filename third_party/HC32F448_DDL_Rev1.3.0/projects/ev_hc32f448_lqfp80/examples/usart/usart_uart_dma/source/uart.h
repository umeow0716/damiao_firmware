/**
 *******************************************************************************
 * @file  usart/usart_uart_dma/source/uart.h
 * @brief This file contains the APIs for UART data receive and transfer by DMA.
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
#ifndef __UART_H__
#define __UART_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll.h"
#include "ring_buf.h"

/**
 * @addtogroup USART_UART_DMA
 * @{
 */

/**
 * @addtogroup UART
 * @{
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup UART_Global_Macros UART Global Macros
 * @{
 */
#ifndef UART_RX_BUF_SIZE
#define UART_RX_BUF_SIZE            (500)
#endif

#if UART_RX_BUF_SIZE < 2
#error "please configure UART_RX_BUF_SIZE >= 2"
#endif
/**
 * @}
 */

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/**
 * @defgroup UART_Global_Types UART Global Types
 * @{
 */

/**
 * @brief IRQ configure structure
 */
typedef struct {
    IRQn_Type                   enIRQn;             /*!< IRQ number       */
    uint16_t                    u16Prio;            /*!< IRQ priority     */
    en_int_src_t                enIntSrc;           /*!< Interrutp soucre */
    func_ptr_t                  pfnIrqCallback;     /*!< IRQ callback     */
} stc_irq_config_t;

/**
 * @brief DMA configure structure
 */
typedef struct {
    CM_DMA_TypeDef              *DMAx;                  /*!< Pointer to DMA instance register base */
    uint8_t                     u8Ch;                   /*!< Specifies the DMA channel             */
    uint32_t                    u32Fcg;                 /*!< Specifies the DMA FCG clock           */
    uint32_t                    u32TriggerSel;          /*!< Specifies the DMA_TRGSEL              */
    en_event_src_t              enTriggerEvent;         /*!< Specifies the trigger event           */
    uint32_t                    u32TransCompleteInt;    /*!< DMA transfer complete interrupt       */
    uint32_t                    u32TransCompleteFlag;   /*!< DMA transfer complete flag            */
    stc_irq_config_t            stcIrq;                 /*!< DMA IRQ configure                     */
} stc_dma_config_t;

/**
 * @brief UART RX timeout configure structure
 */
typedef struct {
    CM_TMR0_TypeDef             *TMR0x;             /*!< Pointer to TMR0 instance register base */
    uint32_t                    u32Ch;              /*!< Specifies the TMR0 channel             */
    uint32_t                    u32Fcg;             /*!< Specifies the TMR0 FCG clock           */
    uint16_t                    u16TimeoutBit;      /*!< Specifies the timeout bit    */
    stc_irq_config_t            stcIrq;             /*!< UART timeout IRQ configure  */
} stc_uart_rxto_t;

/**
 * @brief UART configure structure
 */
typedef struct {
    CM_USART_TypeDef            *USARTx;                        /*!< Pointer to TMR0 instance register base */
    uint32_t                    u32UartFcg;                     /*!< Specifies the USART FCG clock */
    stc_usart_uart_init_t       stcUartInit;                    /*!< UART initialization */
    stc_irq_config_t            stcUartRxErrIrq;                /*!< UART RX error IRQ configure */
    stc_irq_config_t            stcUartTxCmpltIrq;              /*!< UART TX complete IRQ configure */

    stc_dma_config_t            stcDmaTx;                       /*!< DMA TX configure */

    stc_dma_config_t            stcDmaRx;                       /*!< DMA RX configure */
    stc_uart_rxto_t             stcRxto;                        /*!< UART RX timeout configure */
    stc_dma_llp_descriptor_t    stcLlpDesc[2];                  /*!< DMA RX LLP descriptor */
    uint16_t                    u16DmaRxRemainingCnt;           /*!< DMA RX remain count*/

    stc_ring_buf_t              stcRxRingBuf;                   /*!< Ring buffer */
    uint8_t                     au8RxData[UART_RX_BUF_SIZE];    /*!< RX data buffer */
} stc_uart_handle_t;

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @defgroup UART_Global_Macros UART Global Macros
 * @{
 */
int32_t UART_Init(stc_uart_handle_t *pstcHandle);

int32_t UART_Write(stc_uart_handle_t *pstcHandle, uint8_t *pu8Data, uint16_t u16Len);
int32_t UART_Read(stc_uart_handle_t *pstcHandle, uint8_t *pu8Data, uint16_t u16LenToRead, uint16_t *pu16LenRead);

void UART_RxError_IrqCallback(stc_uart_handle_t *pstcHandle);
void UART_RxDma_IrqCallback(stc_uart_handle_t *pstcHandle);
void UART_RxTimeout_IrqCallback(stc_uart_handle_t *pstcHandle);
void UART_TxCmplt_IrqCallback(stc_uart_handle_t *pstcHandle);
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
#endif /* __UART_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
