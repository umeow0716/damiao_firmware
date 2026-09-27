/**
 *******************************************************************************
 * @file  usart/usart_uart_dma/source/main.c
 * @brief This example demonstrates UART data receive and transfer by DMA.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Optimize function: USART_TxComplete_IrqCallback
   2025-11-03       CDT             Add function: USART_StopTimeoutTimer
                                    Refine example
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
 * @addtogroup USART_UART_DMA
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

/* DMA definition */
#define RX_DMA_UNIT                     (CM_DMA1)
#define RX_DMA_CH                       (DMA_CH0)
#define RX_DMA_FCG                      (FCG0_PERIPH_DMA1)
#define RX_DMA_TRIG_SEL                 (AOS_DMA1_0)
#define RX_DMA_TRIG_EVT_SRC             (EVT_SRC_USART2_RI)
#define RX_DMA_TC_INT                   (DMA_INT_TC_CH0)
#define RX_DMA_TC_FLAG                  (DMA_FLAG_TC_CH0)
#define RX_DMA_TC_IRQn                  (INT000_IRQn)
#define RX_DMA_TC_INT_SRC               (INT_SRC_DMA1_TC0)

#define TX_DMA_UNIT                     (CM_DMA1)
#define TX_DMA_CH                       (DMA_CH1)
#define TX_DMA_FCG                      (FCG0_PERIPH_DMA1)
#define TX_DMA_TRIG_SEL                 (AOS_DMA1_1)
#define TX_DMA_TRIG_EVT_SRC             (EVT_SRC_USART2_TI)

/* Timer0 unit & channel definition */
#define TMR0_UNIT                       (CM_TMR0_1)
#define TMR0_CH                         (TMR0_CH_B)
#define TMR0_FCG                        (FCG2_PERIPH_TMR0_1)

/* USART RX/TX pin definition */
#define USART_RX_PORT                   (GPIO_PORT_C)   /* PC11: USART2_RX */
#define USART_RX_PIN                    (GPIO_PIN_11)
#define USART_RX_GPIO_FUNC              (GPIO_FUNC_37)

#define USART_TX_PORT                   (GPIO_PORT_C)   /* PC10: USART2_TX */
#define USART_TX_PIN                    (GPIO_PIN_10)
#define USART_TX_GPIO_FUNC              (GPIO_FUNC_36)

/* USART unit definition */
#define USART_UNIT                      (CM_USART2)
#define USART_FCG                       (FCG3_PERIPH_USART2)

/* USART baudrate definition */
#define USART_BAUDRATE                  (115200UL)

/* USART timeout bits definition */
#define USART_TIMEOUT_BIT               (30U)

/* USART interrupt definition */
#define USART_TX_CPLT_IRQn              (INT001_IRQn)
#define USART_TX_CPLT_INT_SRC           (INT_SRC_USART2_TCI)

#define USART_RX_ERR_IRQn               (INT002_IRQn)
#define USART_RX_ERR_INT_SRC            (INT_SRC_USART2_EI)

#define USART_RX_TIMEOUT_IRQn           (INT003_IRQn)
#define USART_RX_TIMEOUT_INT_SRC        (INT_SRC_USART2_RTO)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
void USART_RxError_IrqCallback(void);
void USART_RxDMA_IrqCallback(void);
void USART_RxTimeout_IrqCallback(void);
void USART_TxCmplt_IrqCallback(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static uint8_t m_au8TxData[UART_RX_BUF_SIZE];

static stc_uart_handle_t m_stcUartHandle = {
    .USARTx = USART_UNIT,
    .u32UartFcg = USART_FCG,
    .stcUartInit = {
        .u32Baudrate = USART_BAUDRATE,
        .u32ClockSrc = USART_CLK_SRC_INTERNCLK,
        .u32ClockDiv = USART_CLK_DIV1,
        .u32CKOutput = USART_CK_OUTPUT_ENABLE,
        .u32DataWidth = USART_DATA_WIDTH_8BIT,
        .u32StopBit = USART_STOPBIT_1BIT,
        .u32Parity = USART_PARITY_NONE,
        .u32OverSampleBit = USART_OVER_SAMPLE_8BIT,
        .u32FirstBit = USART_FIRST_BIT_LSB,
        .u32StartBitPolarity = USART_START_BIT_FALLING,
        .u32HWFlowControl = USART_HW_FLOWCTRL_NONE,
    },
    .stcUartRxErrIrq = {
        .enIRQn = USART_RX_ERR_IRQn,
        .enIntSrc = USART_RX_ERR_INT_SRC,
        .u16Prio = DDL_IRQ_PRIO_DEFAULT,
        .pfnIrqCallback = USART_RxError_IrqCallback,
    },
    .stcUartTxCmpltIrq = {
        .enIRQn = USART_TX_CPLT_IRQn,
        .enIntSrc = USART_TX_CPLT_INT_SRC,
        .u16Prio = DDL_IRQ_PRIO_DEFAULT,
        .pfnIrqCallback = USART_TxCmplt_IrqCallback,
    },
    .stcDmaTx = {
        .DMAx = TX_DMA_UNIT,
        .u8Ch = TX_DMA_CH,
        .u32Fcg = TX_DMA_FCG,
        .u32TriggerSel = TX_DMA_TRIG_SEL,
        .enTriggerEvent = TX_DMA_TRIG_EVT_SRC,
    },
    .stcDmaRx = {
        .DMAx = RX_DMA_UNIT,
        .u8Ch = RX_DMA_CH,
        .u32Fcg = RX_DMA_FCG,
        .u32TriggerSel = RX_DMA_TRIG_SEL,
        .enTriggerEvent = RX_DMA_TRIG_EVT_SRC,
        .u32TransCompleteInt = RX_DMA_TC_INT,
        .u32TransCompleteFlag = RX_DMA_TC_FLAG,
        .stcIrq = {
            .enIRQn = RX_DMA_TC_IRQn,
            .enIntSrc = RX_DMA_TC_INT_SRC,
            .u16Prio = DDL_IRQ_PRIO_DEFAULT,
            .pfnIrqCallback = USART_RxDMA_IrqCallback,
        }
    },
    .stcRxto = {
        .TMR0x = TMR0_UNIT,
        .u32Ch = TMR0_CH,
        .u32Fcg = TMR0_FCG,
        .u16TimeoutBit = USART_TIMEOUT_BIT,
        .stcIrq = {
            .enIRQn = USART_RX_TIMEOUT_IRQn,
            .enIntSrc = USART_RX_TIMEOUT_INT_SRC,
            .u16Prio = DDL_IRQ_PRIO_DEFAULT,
            .pfnIrqCallback = USART_RxTimeout_IrqCallback,
        }
    },
};

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  USART RX error IRQ callback.
 * @param  None
 * @retval None
 */
void USART_RxError_IrqCallback(void)
{
    /* Note: must call the function UART_RxError_IrqCallback */
    UART_RxError_IrqCallback(&m_stcUartHandle);
}

/**
 * @brief  RX DMA IRQ callback.
 * @param  None
 * @retval None
 */
void USART_RxDMA_IrqCallback(void)
{
    /* Note: must call the function UART_RxDma_IrqCallback */
    UART_RxDma_IrqCallback(&m_stcUartHandle);
}

/**
 * @brief  USART RX timeout IRQ callback.
 * @param  None
 * @retval None
 */
void USART_RxTimeout_IrqCallback(void)
{
    /* Note: must call the function UART_RxTimeout_IrqCallback */
    UART_RxTimeout_IrqCallback(&m_stcUartHandle);
}

/**
 * @brief  USART TC IRQ callback.
 * @param  None
 * @retval None
 */
void USART_TxCmplt_IrqCallback(void)
{
    /* Note: must call the function UART_RxTimeout_IrqCallback */
    UART_TxCmplt_IrqCallback(&m_stcUartHandle);
}

/**
 * @brief  Main function of UART DMA project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint16_t u16LenRead;

    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_SEL);

    /* Initialize BSP system clock. */
    BSP_CLK_Init();

    /* Initialize BSP expand IO. */
    BSP_IO_Init();

    /* Initialize BSP LED. */
    BSP_LED_Init();

    /* Configure USART RX/TX pin. */
    GPIO_SetFunc(USART_RX_PORT, USART_RX_PIN, USART_RX_GPIO_FUNC);
    GPIO_SetFunc(USART_TX_PORT, USART_TX_PIN, USART_TX_GPIO_FUNC);

    /* Initialize UART for DMA mode. */
    if (LL_OK != UART_Init(&m_stcUartHandle)) {
        BSP_LED_On(LED_RED);
        for (;;) {
        }
    }

    /* MCU Peripheral registers write protected */
    LL_PERIPH_WP(LL_PERIPH_SEL);

    for (;;) {
        if (LL_OK == UART_Read(&m_stcUartHandle, m_au8TxData, (uint16_t)ARRAY_SZ(m_au8TxData), &u16LenRead)) {
            if (LL_OK != UART_Write(&m_stcUartHandle, m_au8TxData, u16LenRead)) {
                BSP_LED_On(LED_RED);
                for (;;) {
                }
            }

            while (USART_GetStatus(m_stcUartHandle.USARTx, USART_FLAG_TX_CPLT) == RESET) {
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
