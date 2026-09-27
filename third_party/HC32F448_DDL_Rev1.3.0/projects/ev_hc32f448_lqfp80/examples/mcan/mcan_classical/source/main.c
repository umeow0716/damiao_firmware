/**
 *******************************************************************************
 * @file  mcan/mcan_classical/source/main.c
 * @brief Main program of MCAN classical for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Updates related to MCAN driver optimization.
                                    Peripheral SRAMC not used, removed related code.
                                    Code and comments optimized.
   2025-11-03       CDT             Corrected the comment of dedicated Rx buffer.
                                    Use BSP_PRINTF_BAUDRATE to replace immediate value 115200.
                                    Typo: MCAU -> MCAN.
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
 * @addtogroup MCAN
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define MCAN_UNIT1                      (1U)
#define MCAN_UNIT2                      (2U)
#define MCAN_UNIT_SEL                   MCAN_UNIT1

#if (MCAN_UNIT_SEL == MCAN_UNIT1)
#define MCAN_UNIT                       (CM_MCAN1)
#define MCAN_PERIPH_CLK                 (FCG1_PERIPH_MCAN1)
#define MCAN_CLK_UNIT                   (CLK_MCAN1)
#define MCAN_CLK_SRC                    (CLK_MCANCLK_SYSCLK_DIV5)
/* Pin */
#define MCAN_TX_PORT                    (GPIO_PORT_C)
#define MCAN_TX_PIN                     (GPIO_PIN_12)
#define MCAN_TX_PIN_FUNC                (GPIO_FUNC_56)
#define MCAN_RX_PORT                    (GPIO_PORT_D)
#define MCAN_RX_PIN                     (GPIO_PIN_00)
#define MCAN_RX_PIN_FUNC                (GPIO_FUNC_57)
/* IRQ */
#define MCAN_INT0_PRIO                  (DDL_IRQ_PRIO_03)
#define MCAN_INT0_SRC                   (INT_SRC_MCAN1_INT0)
#define MCAN_INT0_IRQn                  (MCAN1_INT0_IRQn)
#define MCAN_INT_LINE0_HANDLER          MCAN1_INT0_Handler
#define MCAN_INT1_PRIO                  (DDL_IRQ_PRIO_03)
#define MCAN_INT1_SRC                   (INT_SRC_MCAN1_INT1)
#define MCAN_INT1_IRQn                  (MCAN1_INT1_IRQn)
#define MCAN_INT_LINE1_HANDLER          MCAN1_INT1_Handler

#elif (MCAN_UNIT_SEL == MCAN_UNIT2)
#define MCAN_UNIT                       (CM_MCAN2)
#define MCAN_PERIPH_CLK                 (FCG1_PERIPH_MCAN2)
#define MCAN_CLK_UNIT                   (CLK_MCAN2)
#define MCAN_CLK_SRC                    (CLK_MCANCLK_SYSCLK_DIV5)
/* Pin */
#define MCAN_TX_PORT                    (GPIO_PORT_H)
#define MCAN_TX_PIN                     (GPIO_PIN_02)
#define MCAN_TX_PIN_FUNC                (GPIO_FUNC_56)
#define MCAN_RX_PORT                    (GPIO_PORT_E)
#define MCAN_RX_PIN                     (GPIO_PIN_04)
#define MCAN_RX_PIN_FUNC                (GPIO_FUNC_57)
/* IRQ */
#define MCAN_INT0_PRIO                  (DDL_IRQ_PRIO_03)
#define MCAN_INT0_SRC                   (INT_SRC_MCAN2_INT0)
#define MCAN_INT0_IRQn                  (MCAN2_INT0_IRQn)
#define MCAN_INT_LINE0_HANDLER          MCAN2_INT0_Handler
#define MCAN_INT1_PRIO                  (DDL_IRQ_PRIO_03)
#define MCAN_INT1_SRC                   (INT_SRC_MCAN2_INT1)
#define MCAN_INT1_IRQn                  (MCAN2_INT1_IRQn)
#define MCAN_INT_LINE1_HANDLER          MCAN2_INT1_Handler

#else
#error "Not supported selection."
#endif

/* Interrupt */
#define MCAN_RX_INT_SEL                 (MCAN_INT_RX_FIFO0_NEW_MSG | \
                                         MCAN_INT_RX_FIFO0_WATERMARK | \
                                         MCAN_INT_RX_FIFO0_FULL | \
                                         MCAN_INT_RX_FIFO0_MSG_LOST | \
                                         MCAN_INT_RX_FIFO1_NEW_MSG | \
                                         MCAN_INT_RX_FIFO1_WATERMARK | \
                                         MCAN_INT_RX_FIFO1_FULL | \
                                         MCAN_INT_RX_FIFO1_MSG_LOST | \
                                         MCAN_INT_RX_BUF_NEW_MSG)
#define MCAN_TX_INT_SEL                 (MCAN_INT_TX_CPLT | \
                                         MCAN_INT_TX_EVT_FIFO_NEW_DATA | \
                                         MCAN_INT_BUS_OFF)

#define MCAN_INT0_SEL                   MCAN_RX_INT_SEL
#define MCAN_INT1_SEL                   MCAN_TX_INT_SEL

/* Message RAM */
/* Each standard filter element size is 4 bytes */
#define MCAN_STD_FILTER_NUM             (4U)
/* Each extended filter element size is 8 bytes */
#define MCAN_EXT_FILTER_NUM             (4U)
/* Each Rx FIFO0 element size is 8+8 bytes */
#define MCAN_RX_FIFO0_NUM               (10U)
#define MCAN_RX_FIFO0_WATERMARK         (8U)
#define MCAN_RX_FIFO0_DATA_FIELD_SIZE   MCAN_DATA_SIZE_8BYTE
/* Each Rx FIFO1 element size is 8+8 bytes */
#define MCAN_RX_FIFO1_NUM               (10U)
#define MCAN_RX_FIFO1_WATERMARK         (7U)
#define MCAN_RX_FIFO1_DATA_FIELD_SIZE   MCAN_DATA_SIZE_8BYTE
/* Each Rx buffer element size is 8+8 bytes */
#define MCAN_RX_BUF_NUM                 (6U)
#define MCAN_RX_BUF_DATA_FIELD_SIZE     MCAN_DATA_SIZE_8BYTE
/* Each Tx buffer element size is 8+8 bytes */
#define MCAN_TX_BUF_NUM                 (12U)
#define MCAN_TX_FIFO_NUM                (6U)
#define MCAN_TX_BUF_DATA_FIELD_SIZE     MCAN_DATA_SIZE_8BYTE
#define MCAN_TX_NOTIFICATION_BUF        ((1UL << (MCAN_TX_BUF_NUM + MCAN_TX_FIFO_NUM)) - 1U)
/* Each extended filter element size is 8 bytes */
#define MCAN_TX_EVT_NUM                 (12U)

/* Filter */
#define MCAN_CFG_IGNOR                  (0U)
/* Accept standard frames with ID from 0x110 to 0x11F and store to Rx FIFO0 */
#define MCAN_STD_FILTER0                {.u32IdType = MCAN_STD_ID, .u32FilterType = MCAN_FILTER_RANGE, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO0, .u32FilterId1 = 0x110UL, \
                                         .u32FilterId2 = 0x11FUL,}
/* Accept standard frames with ID 0x130~0x132 and store to dedicated Rx buffer 0~2 */
#define MCAN_STD_FILTER1                {.u32IdType = MCAN_STD_ID, .u32FilterType = MCAN_CFG_IGNOR, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_BUF, .u32FilterId1 = 0x130UL, \
                                         .u32FilterId2 = MCAN_CFG_IGNOR, .u32RxBufferIndex = MCAN_RX_BUF0}
#define MCAN_STD_FILTER2                {.u32IdType = MCAN_STD_ID, .u32FilterType = MCAN_CFG_IGNOR, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_BUF, .u32FilterId1 = 0x131UL, \
                                         .u32FilterId2 = MCAN_CFG_IGNOR, .u32RxBufferIndex = MCAN_RX_BUF1}
#define MCAN_STD_FILTER3                {.u32IdType = MCAN_STD_ID, .u32FilterType = MCAN_CFG_IGNOR, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_BUF, .u32FilterId1 = 0x132UL, \
                                         .u32FilterId2 = MCAN_CFG_IGNOR, .u32RxBufferIndex = MCAN_RX_BUF2}

/* Accept extended frames with ID from 0x12345110 to 0x1234511F and store to Rx FIFO1 */
#define MCAN_EXT_FILTER0                {.u32IdType = MCAN_EXT_ID, .u32FilterType = MCAN_FILTER_MASK, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO1, .u32FilterId1 = 0x12345110UL, \
                                         .u32FilterId2 = 0x1FFFFFF0UL,}
/* Accept extended frames with ID 0x12345130~0x12345132 and store to dedicated Rx buffer 3~5 */
#define MCAN_EXT_FILTER1                {.u32IdType = MCAN_EXT_ID, .u32FilterType = MCAN_CFG_IGNOR, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_BUF, .u32FilterId1 = 0x12345130UL, \
                                         .u32FilterId2 = MCAN_CFG_IGNOR, .u32RxBufferIndex = MCAN_RX_BUF3}
#define MCAN_EXT_FILTER2                {.u32IdType = MCAN_EXT_ID, .u32FilterType = MCAN_CFG_IGNOR, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_BUF, .u32FilterId1 = 0x12345131UL, \
                                         .u32FilterId2 = MCAN_CFG_IGNOR, .u32RxBufferIndex = MCAN_RX_BUF4}
#define MCAN_EXT_FILTER3                {.u32IdType = MCAN_EXT_ID, .u32FilterType = MCAN_CFG_IGNOR, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_BUF, .u32FilterId1 = 0x12345132UL, \
                                         .u32FilterId2 = MCAN_CFG_IGNOR, .u32RxBufferIndex = MCAN_RX_BUF5}
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void McanCommClockConfig(void);
static void McanInitConfig(void);
static void McanIrqConfig(void);
static void McanPinConfig(void);
static void McanPhyEnable(void);

static void McanSampleTx(void);

static void McanLoadTxMsg(stc_mcan_tx_msg_t *pstcTxMsg, stc_mcan_rx_msg_t *pstcRxMsg);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Main function of mcan_classical project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_INTC | LL_PERIPH_PWC_CLK_RMU);

    BSP_CLK_Init();
    BSP_IO_Init();
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);

    McanCommClockConfig();
    McanInitConfig();
    McanIrqConfig();
    McanPinConfig();
    McanPhyEnable();

    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_INTC | LL_PERIPH_PWC_CLK_RMU);

    /*********************************************************************************************/

    /* Start the MCAN module */
    MCAN_Start(MCAN_UNIT);

    /*********************************************************************************************/
    McanSampleTx();

    for (;;) {
    }
}

/**
 * @brief  Specifies communication clock.
 * @param  None
 * @retval None
 */
static void McanCommClockConfig(void)
{
    CLK_SetCANClockSrc(MCAN_CLK_UNIT, MCAN_CLK_SRC);
}

/**
 * @brief  MCAN initial configuration.
 * @param  None
 * @retval None
 */
static void McanInitConfig(void)
{
    stc_mcan_init_t stcMcanInit;
    stc_mcan_filter_t stcStdFilterList[MCAN_STD_FILTER_NUM] = {
        MCAN_STD_FILTER0, MCAN_STD_FILTER1, MCAN_STD_FILTER2, MCAN_STD_FILTER3
    };
    stc_mcan_filter_t stcExtFilterList[MCAN_EXT_FILTER_NUM] = {
        MCAN_EXT_FILTER0, MCAN_EXT_FILTER1, MCAN_EXT_FILTER2, MCAN_EXT_FILTER3
    };

    (void)MCAN_StructInit(&stcMcanInit);
    stcMcanInit.u32Mode = MCAN_MD_NORMAL;
    stcMcanInit.u32FrameFormat = MCAN_FRAME_CLASSIC;
    /* Classic CAN. Baudrate 1Mbps, sample point 80% */
    stcMcanInit.stcBitTime.u32NominalPrescaler     = 2U;
    stcMcanInit.stcBitTime.u32NominalTimeSeg1      = 16U;
    stcMcanInit.stcBitTime.u32NominalTimeSeg2      = 4U;
    stcMcanInit.stcBitTime.u32NominalSyncJumpWidth = 4U;
    /* Message RAM */
    stcMcanInit.stcMsgRam.u32AddrOffset        = 0U;
    stcMcanInit.stcMsgRam.u32StdFilterNum      = MCAN_STD_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32ExtFilterNum      = MCAN_EXT_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0Num        = MCAN_RX_FIFO0_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0DataSize   = MCAN_RX_FIFO0_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32RxFifo1Num        = MCAN_RX_FIFO1_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo1DataSize   = MCAN_RX_FIFO1_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32RxBufferNum       = MCAN_RX_BUF_NUM;
    stcMcanInit.stcMsgRam.u32RxBufferDataSize  = MCAN_RX_BUF_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32TxBufferNum       = MCAN_TX_BUF_NUM;
    stcMcanInit.stcMsgRam.u32TxFifoQueueNum    = MCAN_TX_FIFO_NUM;
    stcMcanInit.stcMsgRam.u32TxFifoQueueMode   = MCAN_TX_FIFO_MD;
    stcMcanInit.stcMsgRam.u32TxDataSize        = MCAN_TX_BUF_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32TxEventNum        = MCAN_TX_EVT_NUM;
    /* Acceptance filter */
    stcMcanInit.stcFilter.pstcStdFilterList     = stcStdFilterList;
    stcMcanInit.stcFilter.pstcExtFilterList     = stcExtFilterList;
    stcMcanInit.stcFilter.u32StdFilterConfigNum = stcMcanInit.stcMsgRam.u32StdFilterNum;
    stcMcanInit.stcFilter.u32ExtFilterConfigNum = stcMcanInit.stcMsgRam.u32ExtFilterNum;

    FCG_Fcg1PeriphClockCmd(MCAN_PERIPH_CLK, ENABLE);
    (void)MCAN_Init(MCAN_UNIT, &stcMcanInit);

    /* Watermark if needed */
    MCAN_SetFifoWatermark(MCAN_UNIT, MCAN_WATERMARK_RX_FIFO0, MCAN_RX_FIFO0_WATERMARK);
    MCAN_SetFifoWatermark(MCAN_UNIT, MCAN_WATERMARK_RX_FIFO1, MCAN_RX_FIFO1_WATERMARK);

    /* Configure Rx FIFO0 operation mode if needed.
       If new message received when Rx FIFO0 is full, the new message
       will not be stored and Message Lost Interrupt will be generated. */
    MCAN_RxFifoOperationModeConfig(MCAN_UNIT, MCAN_RX_FIFO0, MCAN_RX_FIFO_BLOCKING);
    /* Configure Rx FIFO1 operation mode if needed.
       If new message received when Rx FIFO1 is full, the new received message
       will overwrite the oldest received message and RF1N Interrupt generated. */
    MCAN_RxFifoOperationModeConfig(MCAN_UNIT, MCAN_RX_FIFO1, MCAN_RX_FIFO_OVERWRITE);

    /* Configures timestamp if needed */
    MCAN_TimestampCounterConfig(MCAN_UNIT, 1U);
    MCAN_TimestampCounterCmd(MCAN_UNIT, ENABLE);

    /* The Tx buffer can cause transmission completed interrupt
       only when its own transmission completed interrupt is enabled. */
    MCAN_TxBufferNotificationCmd(MCAN_UNIT, MCAN_TX_NOTIFICATION_BUF, MCAN_INT_TX_CPLT, ENABLE);

    MCAN_IntCmd(MCAN_UNIT, MCAN_INT0_SEL, MCAN_INT_LINE0, ENABLE);
    MCAN_IntCmd(MCAN_UNIT, MCAN_INT1_SEL, MCAN_INT_LINE1, ENABLE);
}

/**
 * @brief  CAN interrupt configuration.
 * @param  None
 * @retval None
 */
static void McanIrqConfig(void)
{
    NVIC_ClearPendingIRQ(MCAN_INT0_IRQn);
    NVIC_SetPriority(MCAN_INT0_IRQn, MCAN_INT0_PRIO);
    NVIC_EnableIRQ(MCAN_INT0_IRQn);

    NVIC_ClearPendingIRQ(MCAN_INT1_IRQn);
    NVIC_SetPriority(MCAN_INT1_IRQn, MCAN_INT0_PRIO);
    NVIC_EnableIRQ(MCAN_INT1_IRQn);
}

/**
 * @brief  Specifies pin function for TXD and RXD.
 * @param  None
 * @retval None
 */
static void McanPinConfig(void)
{
    GPIO_SetFunc(MCAN_TX_PORT, MCAN_TX_PIN, MCAN_TX_PIN_FUNC);
    GPIO_SetFunc(MCAN_RX_PORT, MCAN_RX_PIN, MCAN_RX_PIN_FUNC);
}

/**
 * @brief  Set CAN PHY STB pin as low.
 * @param  None
 * @retval None
 */
static void McanPhyEnable(void)
{
    BSP_CAN_STB_IO_Init();
    /* Set PHY STB pin as low. */
    BSP_CAN_STBCmd(EIO_PIN_RESET);
}

/**
 * @brief  MCAN transmit.
 * @param  None
 * @retval None
 */
static void McanSampleTx(void)
{
    stc_mcan_tx_msg_t stcTxMsg = {
        .ID = 0x777UL,
        .IDE = 0U,
        .DLC = MCAN_DLC8,
        .u32TxBuffer = MCAN_TX_BUF0,
        .au8Data = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U},
    };
    (void)MCAN_AddMsgToTxBuffer(MCAN_UNIT, &stcTxMsg);
    MCAN_EnableTxBufferRequest(MCAN_UNIT, stcTxMsg.u32TxBuffer);

    stcTxMsg.ID = 0x17777777UL;
    stcTxMsg.IDE = 1U;
    (void)MCAN_AddMsgToTxFifoQueue(MCAN_UNIT, &stcTxMsg);
}

/**
 * @brief  MCAN interrupt line 0 IRQ handler.
 * @param  None
 * @retval None
 */
void MCAN_INT_LINE0_HANDLER(void)
{
    uint32_t u32RxBuffer;
    uint32_t u32TxBufferIndex;
    uint32_t u32TxBuffer = 0U;
    stc_mcan_rx_msg_t stcRxMsg;
    stc_mcan_tx_msg_t stcTxMsg;
    stc_mcan_rx_msg_t stcRxMsgRxFifo1[MCAN_RX_FIFO1_NUM];

    /* Rx FIFO0 related interrupts */
    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO0_NEW_MSG) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO0_NEW_MSG);
        /* Messages stored in Rx FIFO0 can be read here */
        DDL_Printf("Rx FIFO 0 new message\r\n");
        if (MCAN_GetRxMsg(MCAN_UNIT, MCAN_RX_FIFO0, &stcRxMsg) == LL_OK) {
            McanLoadTxMsg(&stcTxMsg, &stcRxMsg);
            stcTxMsg.u32TxBuffer = MCAN_TX_BUF0;
            /* Transmit message via dedicated Tx buffer */
            if (MCAN_AddMsgToTxBuffer(MCAN_UNIT, &stcTxMsg) == LL_OK) {
                MCAN_EnableTxBufferRequest(MCAN_UNIT, stcTxMsg.u32TxBuffer);
            } else {
                /* The specified dedicated Tx buffer is busy. */
            }
        } else {
            /* Exception */
        }
    }

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO0_WATERMARK) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO0_WATERMARK);
        DDL_Printf("Rx FIFO0 reached watermark\r\n");
        /* Messages stored in Rx FIFO0 can be read here */
    }

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO0_FULL) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO0_FULL);
        DDL_Printf("Rx FIFO0 full\r\n");
        /* Messages stored in Rx FIFO0 can be read here */
    }

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO0_MSG_LOST) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO0_MSG_LOST);
        DDL_Printf("Rx FIFO0 message lost\r\n");
    }

    /* Rx FIFO1 related interrupts */
    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_NEW_MSG) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_NEW_MSG);
        DDL_Printf("Rx FIFO 1 new message\r\n");
        /* Messages stored in Rx FIFO1 can be read here */
    }

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_WATERMARK) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_WATERMARK);
        DDL_Printf("Rx FIFO1 reached watermark\r\n");
        /* Messages stored in Rx FIFO1 can be read here */
    }

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_FULL) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_FULL);
        DDL_Printf("Rx FIFO1 full\r\n");
        /* Messages stored in Rx FIFO1 can be read here */
        u32RxBuffer = 0U;
        /* Read all messages that stored in Rx FIFO1 */
        while (MCAN_GetRxMsg(MCAN_UNIT, MCAN_RX_FIFO1, &stcRxMsgRxFifo1[u32RxBuffer]) == LL_OK) {
            u32RxBuffer++;
        }
        /* Transmit message via dedicated Tx buffer */
        for (u32TxBufferIndex = 0U; u32TxBufferIndex < u32RxBuffer; u32TxBufferIndex++) {
            McanLoadTxMsg(&stcTxMsg, &stcRxMsgRxFifo1[u32TxBufferIndex]);
            stcTxMsg.u32TxBuffer = 1UL << u32TxBufferIndex;
            if (MCAN_AddMsgToTxBuffer(MCAN_UNIT, &stcTxMsg) == LL_OK) {
                u32TxBuffer |= stcTxMsg.u32TxBuffer;
            } else {
                /* The specified dedicated Tx buffer is busy. */
            }
        }
        /* All dedicated Tx buffer are filled. Add transmission requests. */
        MCAN_EnableTxBufferRequest(MCAN_UNIT, u32TxBuffer);
    }

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_MSG_LOST) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_MSG_LOST);
        DDL_Printf("Rx FIFO1 message lost\r\n");
    }

    /* Dedicated Rx buffer related interrupts */
    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_BUF_NEW_MSG) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_BUF_NEW_MSG);
        /* Message stored to Dedicated Rx Buffer */
        DDL_Printf("Message stored to dedicated Rx buffer\r\n");
        /* Read received message that stored in the dedicated Rx buffer.
           When a message is stored in a dedicated Rx buffer, the the interrupt flag IR.DRX and
           the corresponding bit in NDAT1/2 are set. When bits in NDAT1/2 are set the respective
           Rx buffer is locked (will not be overwritten by a new message) and the corresponding
           filter will not match. */
        for (u32RxBuffer = MCAN_RX_BUF0; u32RxBuffer < MCAN_RX_BUF_NUM; u32RxBuffer++) {
            if (MCAN_CheckRxBufferMsgAvailable(MCAN_UNIT, u32RxBuffer) == LL_OK) {
                if (MCAN_GetRxMsg(MCAN_UNIT, u32RxBuffer, &stcRxMsg) == LL_OK) {
                    DDL_Printf("Dedicated Rx buffer%u received\r\n", (uint8_t)u32RxBuffer);
                    McanLoadTxMsg(&stcTxMsg, &stcRxMsg);
                    /* Transmit message via Tx FIFO/Queue */
                    if (MCAN_AddMsgToTxFifoQueue(MCAN_UNIT, &stcTxMsg) != LL_OK) {
                        /* Tx FIFO full */
                        DDL_Printf("Tx FIFO full\r\n");
                    }
                } else {
                    /* Exception */
                }
            }
        }
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  MCAN interrupt line 1 IRQ handler.
 * @param  None
 * @retval None
 */
void MCAN_INT_LINE1_HANDLER(void)
{
    stc_mcan_tx_event_t stcTxEvent;

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_TX_CPLT) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_TX_CPLT);
        /* Transmission Completed */
        DDL_Printf("Transmission completed\r\n");
        /* User code if needed */
    }

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_TX_EVT_FIFO_NEW_DATA) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_TX_EVT_FIFO_NEW_DATA);
        /* Tx Event FIFO New Entry */
        DDL_Printf("Tx event FIFO new entry\r\n");
        if (MCAN_GetTxEvent(MCAN_UNIT, &stcTxEvent) == LL_OK) {
            /* Use the Tx event if needed */
            DDL_Printf("Tx event: ID %.8x\r\n", (unsigned int)stcTxEvent.ID);
            DDL_Printf("Tx event: message marker %u\r\n", (uint8_t)stcTxEvent.u32MsgMarker);
            DDL_Printf("Tx event: Tx timestamp %u\r\n", (uint16_t)stcTxEvent.u32TxTimestamp);
        }
    }

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_BUS_OFF) == SET) {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_BUS_OFF);
        if (MCAN_GetProtocolFlagStatus(MCAN_UNIT, MCAN_PROTOCOL_FLAG_BUS_OFF) == SET) {
            /* If the device goes Bus_Off, it will set CCCR.INIT of its own accord, stopping all bus activities.
               The application should clear CCCR.INIT, then the device can resume normal operation.
               Once CCCR.INIT has been cleared by the CPU, the device will then wait for 129 occurrences of
               Bus Idle(129 * 11 consecutive recessive bits) before resuming normal operation. */
            MCAN_Start(MCAN_UNIT);
        }
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  Load Tx message from received message
 * @param  [in]  pstcTxMsg              Pointer to the message to be transmitted.
 * @param  [in]  pstcRxMsg              Pointer to the received message.
 * @retval None
 */
static void McanLoadTxMsg(stc_mcan_tx_msg_t *pstcTxMsg, stc_mcan_rx_msg_t *pstcRxMsg)
{
    static uint8_t u8TxMarker = 0U;
    *pstcTxMsg = *((stc_mcan_tx_msg_t *)pstcRxMsg);
    pstcTxMsg->au8Data[0U] = u8TxMarker;
    pstcTxMsg->ID &= 0xFFFFF0FFUL;
    pstcTxMsg->ID |= 0x300UL;
    pstcTxMsg->u32StoreTxEvent = 1U;
    pstcTxMsg->u32MsgMarker = u8TxMarker++;
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
