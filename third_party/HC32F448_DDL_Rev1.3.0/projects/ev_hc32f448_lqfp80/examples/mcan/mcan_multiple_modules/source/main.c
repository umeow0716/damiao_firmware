/**
 *******************************************************************************
 * @file  mcan/mcan_multiple_modules/source/main.c
 * @brief Main program of MCAN multiple modules for the Device Driver Library.
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
/* MCAN1 configuration */
#define MCAN1_UNIT                      (CM_MCAN1)
#define MCAN1_PERIPH_CLK                (FCG1_PERIPH_MCAN1)
#define MCAN1_CLK_UNIT                  (CLK_MCAN1)
#define MCAN1_CLK_SRC                   (CLK_MCANCLK_SYSCLK_DIV5)
/* Pin */
#define MCAN1_TX_PORT                   (GPIO_PORT_C)
#define MCAN1_TX_PIN                    (GPIO_PIN_12)
#define MCAN1_TX_PIN_FUNC               (GPIO_FUNC_56)
#define MCAN1_RX_PORT                   (GPIO_PORT_D)
#define MCAN1_RX_PIN                    (GPIO_PIN_00)
#define MCAN1_RX_PIN_FUNC               (GPIO_FUNC_57)
/* IRQ */
#define MCAN1_INT0_PRIO                 (DDL_IRQ_PRIO_03)
#define MCAN1_INT0_SRC                  (INT_SRC_MCAN1_INT0)
#define MCAN1_INT0_IRQn                 (MCAN1_INT0_IRQn)
#define MCAN1_INT1_PRIO                 (DDL_IRQ_PRIO_03)
#define MCAN1_INT1_SRC                  (INT_SRC_MCAN1_INT1)
#define MCAN1_INT1_IRQn                 (MCAN1_INT1_IRQn)

/* Interrupts of MCAN1 */
#define MCAN1_RX_INT_SEL                (MCAN_INT_RX_FIFO0_NEW_MSG | MCAN_INT_RX_FIFO1_NEW_MSG)
#define MCAN1_TX_INT_SEL                (MCAN_INT_TX_CPLT | MCAN_INT_BUS_OFF)
#define MCAN1_INT0_SEL                   MCAN1_RX_INT_SEL
#define MCAN1_INT1_SEL                   MCAN1_TX_INT_SEL

/* MCAN1 Message RAM */
/* Each standard filter element size is 4 bytes */
#define MCAN1_STD_FILTER_NUM            (1U)
/* Each extended filter element size is 8 bytes */
#define MCAN1_EXT_FILTER_NUM            (1U)
/* Each Rx FIFO0 element size is 64+8 bytes */
#define MCAN1_RX_FIFO0_NUM              (3U)
#define MCAN1_RX_FIFO0_DATA_FIELD_SIZE   MCAN_DATA_SIZE_64BYTE
/* Each Rx FIFO1 element size is 64+8 bytes */
#define MCAN1_RX_FIFO1_NUM              (3U)
#define MCAN1_RX_FIFO1_DATA_FIELD_SIZE   MCAN_DATA_SIZE_64BYTE
/* Each Tx buffer element size is 64+8 bytes */
#define MCAN1_TX_BUF_NUM                (0U)
#define MCAN1_TX_FIFO_NUM               (3U)
#define MCAN1_TX_BUF_DATA_FIELD_SIZE    MCAN_DATA_SIZE_64BYTE
#define MCAN1_TX_NOTIFICATION_BUF       ((1UL << (MCAN1_TX_BUF_NUM + MCAN1_TX_FIFO_NUM)) - 1U)
/* Each extended filter element size is 8 bytes */
#define MCAN1_TX_EVT_NUM                (0U)

/* MCAN1 Filter */
/* Accept standard frames with ID from 0x110 to 0x11F and store to Rx FIFO0 */
#define MCAN1_STD_FILTER0               {.u32IdType = MCAN_STD_ID, .u32FilterType = MCAN_FILTER_RANGE, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO0, .u32FilterId1 = 0x110UL, \
                                         .u32FilterId2 = 0x11FUL,}
#define MCAN1_STD_FILTER_LIST           {MCAN1_STD_FILTER0}

/* Accept extended frames with ID from 0x12345110 to 0x1234511F and store to Rx FIFO1 */
#define MCAN1_EXT_FILTER0               {.u32IdType = MCAN_EXT_ID, .u32FilterType = MCAN_FILTER_MASK, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO1, .u32FilterId1 = 0x12345110UL, \
                                         .u32FilterId2 = 0x1FFFFFF0UL,}
#define MCAN1_EXT_FILTER_LIST           {MCAN1_EXT_FILTER0}

/*********************************************************************************************************/
/*********************************************************************************************************/
/* MCAN2 configuration */
#define MCAN2_UNIT                      (CM_MCAN2)
#define MCAN2_PERIPH_CLK                (FCG1_PERIPH_MCAN2)
#define MCAN2_CLK_UNIT                  (CLK_MCAN2)
#define MCAN2_CLK_SRC                   (CLK_MCANCLK_SYSCLK_DIV5)
/* Pin */
#define MCAN2_TX_PORT                   (GPIO_PORT_H)
#define MCAN2_TX_PIN                    (GPIO_PIN_02)
#define MCAN2_TX_PIN_FUNC               (GPIO_FUNC_56)
#define MCAN2_RX_PORT                   (GPIO_PORT_E)
#define MCAN2_RX_PIN                    (GPIO_PIN_04)
#define MCAN2_RX_PIN_FUNC               (GPIO_FUNC_57)
/* IRQ */
#define MCAN2_INT0_PRIO                 (DDL_IRQ_PRIO_03)
#define MCAN2_INT0_SRC                  (INT_SRC_MCAN2_INT0)
#define MCAN2_INT0_IRQn                 (MCAN2_INT0_IRQn)
#define MCAN2_INT1_PRIO                 (DDL_IRQ_PRIO_03)
#define MCAN2_INT1_SRC                  (INT_SRC_MCAN2_INT1)
#define MCAN2_INT1_IRQn                 (MCAN2_INT1_IRQn)

/* Interrupts of MCAN2 */
#define MCAN2_RX_INT_SEL                (MCAN_INT_RX_FIFO_NEW_MSG | MCAN_INT_RX_FIFO1_NEW_MSG)
#define MCAN2_TX_INT_SEL                (MCAN_INT_TX_CPLT | MCAN_INT_BUS_OFF)
#define MCAN2_INT0_SEL                   MCAN1_RX_INT_SEL
#define MCAN2_INT1_SEL                   MCAN1_TX_INT_SEL

/* MCAN2 Message RAM */
/* Each standard filter element size is 4 bytes */
#define MCAN2_STD_FILTER_NUM            (1U)
/* Each extended filter element size is 8 bytes */
#define MCAN2_EXT_FILTER_NUM            (1U)
/* Each Rx FIFO0 element size is 64+8 bytes */
#define MCAN2_RX_FIFO0_NUM              (3U)
#define MCAN2_RX_FIFO0_DATA_FIELD_SIZE   MCAN_DATA_SIZE_64BYTE
/* Each Rx FIFO1 element size is 64+8 bytes */
#define MCAN2_RX_FIFO1_NUM              (3U)
#define MCAN2_RX_FIFO1_DATA_FIELD_SIZE   MCAN_DATA_SIZE_64BYTE
/* Each Tx buffer element size is 64+8 bytes */
#define MCAN2_TX_BUF_NUM                (0U)
#define MCAN2_TX_FIFO_NUM               (3U)
#define MCAN2_TX_BUF_DATA_FIELD_SIZE    MCAN_DATA_SIZE_64BYTE
#define MCAN2_TX_NOTIFICATION_BUF       ((1UL << (MCAN1_TX_BUF_NUM + MCAN1_TX_FIFO_NUM)) - 1U)
/* Each extended filter element size is 8 bytes */
#define MCAN2_TX_EVT_NUM                (0U)

/* MCAN2 Filter */
/* Accept standard frames with ID from 0x120 to 0x12F and store to Rx FIFO0 */
#define MCAN2_STD_FILTER0               {.u32IdType = MCAN_STD_ID, .u32FilterType = MCAN_FILTER_RANGE, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO0, .u32FilterId1 = 0x120UL, \
                                         .u32FilterId2 = 0x12FUL,}
#define MCAN2_STD_FILTER_LIST           {MCAN2_STD_FILTER0}

/* Accept extended frames with ID from 0x12345120 to 0x1234512F and store to Rx FIFO1 */
#define MCAN2_EXT_FILTER0               {.u32IdType = MCAN_EXT_ID, .u32FilterType = MCAN_FILTER_MASK, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO1, .u32FilterId1 = 0x12345120UL, \
                                         .u32FilterId2 = 0x1FFFFFF0UL,}
#define MCAN2_EXT_FILTER_LIST           {MCAN2_EXT_FILTER0}

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
 * @brief  Main function of mcan_multiple_modules project
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

    /* Start the MCAN modules */
    MCAN_Start(MCAN1_UNIT);
    MCAN_Start(MCAN2_UNIT);

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
    CLK_SetCANClockSrc(MCAN1_CLK_UNIT, MCAN1_CLK_SRC);
    CLK_SetCANClockSrc(MCAN2_CLK_UNIT, MCAN2_CLK_SRC);
}

/**
 * @brief  MCAN initial configuration.
 * @param  None
 * @retval None
 */
static void McanInitConfig(void)
{
    stc_mcan_init_t stcMcanInit;

    stc_mcan_filter_t stcMcan1StdFilterList[] = MCAN1_STD_FILTER_LIST;
    stc_mcan_filter_t stcMcan1ExtFilterList[] = MCAN1_EXT_FILTER_LIST;

    stc_mcan_filter_t stcMcan2StdFilterList[] = MCAN2_STD_FILTER_LIST;
    stc_mcan_filter_t stcMcan2ExtFilterList[] = MCAN2_EXT_FILTER_LIST;

    (void)MCAN_StructInit(&stcMcanInit);
    stcMcanInit.u32Mode = MCAN_MD_NORMAL;
    stcMcanInit.u32FrameFormat = MCAN_FRAME_ISO_FD_BRS;
    /* CAN FD arbitration phase. Baudrate 1Mbps, sample point 80% */
    stcMcanInit.stcBitTime.u32NominalPrescaler     = 1U;
    stcMcanInit.stcBitTime.u32NominalTimeSeg1      = 32U;
    stcMcanInit.stcBitTime.u32NominalTimeSeg2      = 8U;
    stcMcanInit.stcBitTime.u32NominalSyncJumpWidth = 8U;
    /* CAN FD data phase. Baudrate 4Mbps, sample point 80% */
    stcMcanInit.stcBitTime.u32DataPrescaler      = 1U;
    stcMcanInit.stcBitTime.u32DataTimeSeg1       = 8U;
    stcMcanInit.stcBitTime.u32DataTimeSeg2       = 2U;
    stcMcanInit.stcBitTime.u32DataSyncJumpWidth  = 2U;
    stcMcanInit.stcBitTime.u32TDC                = MCAN_FD_TDC_ENABLE;
    stcMcanInit.stcBitTime.u32SspOffset          = 8U;
    stcMcanInit.stcBitTime.u32TdcFilter          = 0U;
    /* Message RAM */
    stcMcanInit.stcMsgRam.u32AddrOffset        = 0U;
    stcMcanInit.stcMsgRam.u32StdFilterNum      = MCAN1_STD_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32ExtFilterNum      = MCAN1_EXT_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0Num        = MCAN1_RX_FIFO0_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0DataSize   = MCAN1_RX_FIFO0_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32RxFifo1Num        = MCAN1_RX_FIFO1_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo1DataSize   = MCAN1_RX_FIFO1_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32RxBufferNum       = 0U;
    stcMcanInit.stcMsgRam.u32TxBufferNum       = MCAN1_TX_BUF_NUM;
    stcMcanInit.stcMsgRam.u32TxFifoQueueNum    = MCAN1_TX_FIFO_NUM;
    stcMcanInit.stcMsgRam.u32TxFifoQueueMode   = MCAN_TX_FIFO_MD;
    stcMcanInit.stcMsgRam.u32TxDataSize        = MCAN1_TX_BUF_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32TxEventNum        = MCAN1_TX_EVT_NUM;
    /* Acceptance filter */
    stcMcanInit.stcFilter.pstcStdFilterList     = stcMcan1StdFilterList;
    stcMcanInit.stcFilter.pstcExtFilterList     = stcMcan1ExtFilterList;
    stcMcanInit.stcFilter.u32StdFilterConfigNum = stcMcanInit.stcMsgRam.u32StdFilterNum;
    stcMcanInit.stcFilter.u32ExtFilterConfigNum = stcMcanInit.stcMsgRam.u32ExtFilterNum;

    /* Initializes MCAN1 */
    FCG_Fcg1PeriphClockCmd(MCAN1_PERIPH_CLK, ENABLE);
    (void)MCAN_Init(MCAN1_UNIT, &stcMcanInit);
    /* The Tx buffer can cause transmission completed interrupt
       only when its own transmission completed interrupt is enabled. */
    MCAN_TxBufferNotificationCmd(MCAN1_UNIT, MCAN1_TX_NOTIFICATION_BUF, MCAN_INT_TX_CPLT, ENABLE);
    MCAN_IntCmd(MCAN1_UNIT, MCAN1_INT0_SEL, MCAN_INT_LINE0, ENABLE);
    MCAN_IntCmd(MCAN1_UNIT, MCAN1_INT1_SEL, MCAN_INT_LINE1, ENABLE);

    /* Initializes MCAN2 */
    /* Work mode and frame format: same with MCAN1
       Bit rate: same with MCAN1 */
    /* Message RAM
       The "u32AddrOffset+u32AllocatedSize" of the previous configured MCAN is the minimum start
       address(u32AddrOffset) of the next MCAN to be configured. */
    stcMcanInit.stcMsgRam.u32AddrOffset        = stcMcanInit.stcMsgRam.u32AddrOffset + stcMcanInit.stcMsgRam.u32AllocatedSize;
    stcMcanInit.stcMsgRam.u32StdFilterNum      = MCAN2_STD_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32ExtFilterNum      = MCAN2_EXT_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0Num        = MCAN2_RX_FIFO0_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0DataSize   = MCAN2_RX_FIFO0_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32RxFifo1Num        = MCAN2_RX_FIFO1_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo1DataSize   = MCAN2_RX_FIFO1_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32RxBufferNum       = 0U;
    stcMcanInit.stcMsgRam.u32TxBufferNum       = MCAN2_TX_BUF_NUM;
    stcMcanInit.stcMsgRam.u32TxFifoQueueNum    = MCAN2_TX_FIFO_NUM;
    stcMcanInit.stcMsgRam.u32TxFifoQueueMode   = MCAN_TX_FIFO_MD;
    stcMcanInit.stcMsgRam.u32TxDataSize        = MCAN2_TX_BUF_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32TxEventNum        = MCAN2_TX_EVT_NUM;
    /* Acceptance filter */
    stcMcanInit.stcFilter.pstcStdFilterList     = stcMcan2StdFilterList;
    stcMcanInit.stcFilter.pstcExtFilterList     = stcMcan2ExtFilterList;
    stcMcanInit.stcFilter.u32StdFilterConfigNum = stcMcanInit.stcMsgRam.u32StdFilterNum;
    stcMcanInit.stcFilter.u32ExtFilterConfigNum = stcMcanInit.stcMsgRam.u32ExtFilterNum;

    FCG_Fcg1PeriphClockCmd(MCAN2_PERIPH_CLK, ENABLE);
    (void)MCAN_Init(MCAN2_UNIT, &stcMcanInit);
    /* The Tx buffer can cause transmission completed interrupt
       only when its own transmission completed interrupt is enabled. */
    MCAN_TxBufferNotificationCmd(MCAN2_UNIT, MCAN2_TX_NOTIFICATION_BUF, MCAN_INT_TX_CPLT, ENABLE);
    MCAN_IntCmd(MCAN2_UNIT, MCAN2_INT0_SEL, MCAN_INT_LINE0, ENABLE);
    MCAN_IntCmd(MCAN2_UNIT, MCAN2_INT1_SEL, MCAN_INT_LINE1, ENABLE);
}

/**
 * @brief  CAN interrupt configuration.
 * @param  None
 * @retval None
 */
static void McanIrqConfig(void)
{
    /* MCAN1 IRQ configuration */
    NVIC_ClearPendingIRQ(MCAN1_INT0_IRQn);
    NVIC_SetPriority(MCAN1_INT0_IRQn, MCAN1_INT0_PRIO);
    NVIC_EnableIRQ(MCAN1_INT0_IRQn);

    NVIC_ClearPendingIRQ(MCAN1_INT1_IRQn);
    NVIC_SetPriority(MCAN1_INT1_IRQn, MCAN1_INT0_PRIO);
    NVIC_EnableIRQ(MCAN1_INT1_IRQn);

    /* MCAN2 IRQ configuration */
    NVIC_ClearPendingIRQ(MCAN2_INT0_IRQn);
    NVIC_SetPriority(MCAN2_INT0_IRQn, MCAN2_INT0_PRIO);
    NVIC_EnableIRQ(MCAN2_INT0_IRQn);

    NVIC_ClearPendingIRQ(MCAN2_INT1_IRQn);
    NVIC_SetPriority(MCAN2_INT1_IRQn, MCAN2_INT0_PRIO);
    NVIC_EnableIRQ(MCAN2_INT1_IRQn);
}

/**
 * @brief  Specifies pin function for TXD and RXD.
 * @param  None
 * @retval None
 */
static void McanPinConfig(void)
{
    GPIO_SetFunc(MCAN1_TX_PORT, MCAN1_TX_PIN, MCAN1_TX_PIN_FUNC);
    GPIO_SetFunc(MCAN1_RX_PORT, MCAN1_RX_PIN, MCAN1_RX_PIN_FUNC);

    GPIO_SetFunc(MCAN2_TX_PORT, MCAN2_TX_PIN, MCAN2_TX_PIN_FUNC);
    GPIO_SetFunc(MCAN2_RX_PORT, MCAN2_RX_PIN, MCAN2_RX_PIN_FUNC);
}

/**
 * @brief  Set CAN PHY STB pin as low.
 * @param  None
 * @retval None
 */
static void McanPhyEnable(void)
{
    BSP_CAN_STB_IO_Init();
    /* Set PYH STB pin as low. */
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
        /* Classical CAN frame */
        .ID = 0x111UL,
        .IDE = 0U,
        .DLC = MCAN_DLC8,
        .au8Data = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U},
    };
    (void)MCAN_AddMsgToTxFifoQueue(MCAN1_UNIT, &stcTxMsg);
    stcTxMsg.ID = 0x222UL;
    (void)MCAN_AddMsgToTxFifoQueue(MCAN2_UNIT, &stcTxMsg);
    /* CAN FD with BRS frame */
    stcTxMsg.ID = 0x11111111UL;
    stcTxMsg.IDE = 1U;
    stcTxMsg.FDF = 1U;
    stcTxMsg.BRS = 1U;
    (void)MCAN_AddMsgToTxFifoQueue(MCAN1_UNIT, &stcTxMsg);
    stcTxMsg.ID = 0x12222222UL;
    (void)MCAN_AddMsgToTxFifoQueue(MCAN2_UNIT, &stcTxMsg);
}

/**
 * @brief  MCAN1 interrupt line 0 IRQ handler.
 * @param  None
 * @retval None
 */
void MCAN1_INT0_Handler(void)
{
    stc_mcan_rx_msg_t stcRxMsg;
    stc_mcan_tx_msg_t stcTxMsg;

    if (MCAN_GetStatus(MCAN1_UNIT, MCAN_FLAG_RX_FIFO0_NEW_MSG) == SET) {
        MCAN_ClearStatus(MCAN1_UNIT, MCAN_FLAG_RX_FIFO0_NEW_MSG);
        /* Rx FIFO 0 New Message */
        DDL_Printf("MCAN1 Rx FIFO 0 new message\r\n");
        if (MCAN_GetRxMsg(MCAN1_UNIT, MCAN_RX_FIFO0, &stcRxMsg) == LL_OK) {
            McanLoadTxMsg(&stcTxMsg, &stcRxMsg);
            /* Transmit message via dedicated Tx FIFO */
            if (MCAN_AddMsgToTxFifoQueue(MCAN1_UNIT, &stcTxMsg) == LL_OK) {
                DDL_Printf("MCAN1 send out the message that received by Rx FIFO 0\r\n");
            } else {
                DDL_Printf("MCAN1 Tx FIFO full\r\n");
            }
        } else {
            /* Exception */
        }
    }

    if (MCAN_GetStatus(MCAN1_UNIT, MCAN_FLAG_RX_FIFO1_NEW_MSG) == SET) {
        MCAN_ClearStatus(MCAN1_UNIT, MCAN_FLAG_RX_FIFO1_NEW_MSG);
        /* Rx FIFO 1 New Message */
        DDL_Printf("MCAN1 Rx FIFO 1 new message\r\n");
        if (MCAN_GetRxMsg(MCAN1_UNIT, MCAN_RX_FIFO1, &stcRxMsg) == LL_OK) {
            McanLoadTxMsg(&stcTxMsg, &stcRxMsg);
            /* Transmit message via dedicated Tx FIFO */
            if (MCAN_AddMsgToTxFifoQueue(MCAN1_UNIT, &stcTxMsg) == LL_OK) {
                DDL_Printf("MCAN1 send out the message that received by Rx FIFO 1\r\n");
            } else {
                DDL_Printf("MCAN1 Tx FIFO full\r\n");
            }
        } else {
            /* Exception */
        }
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  MCAN1 interrupt line 1 IRQ handler.
 * @param  None
 * @retval None
 */
void MCAN1_INT1_Handler(void)
{
    if (MCAN_GetStatus(MCAN1_UNIT, MCAN_FLAG_TX_CPLT) == SET) {
        MCAN_ClearStatus(MCAN1_UNIT, MCAN_FLAG_TX_CPLT);
        /* Transmission Completed */
        DDL_Printf("MCAN1 transmission completed\r\n");
        /* User code if needed */
    }

    if (MCAN_GetStatus(MCAN1_UNIT, MCAN_FLAG_BUS_OFF) == SET) {
        MCAN_ClearStatus(MCAN1_UNIT, MCAN_FLAG_BUS_OFF);
        if (MCAN_GetProtocolFlagStatus(MCAN1_UNIT, MCAN_PROTOCOL_FLAG_BUS_OFF) == SET) {
            DDL_Printf("MCAN1 bus-off\r\n");
            /* If the device goes Bus_Off, it will set CCCR.INIT of its own accord, stopping all bus activities.
               The application should clear CCCR.INIT, then the device can resume normal operation.
               Once CCCR.INIT has been cleared by the CPU, the device will then wait for 129 occurrences of
               Bus Idle(129 * 11 consecutive recessive bits) before resuming normal operation. */
            MCAN_Start(MCAN1_UNIT);
        }
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  MCAN2 interrupt line 0 IRQ handler.
 * @param  None
 * @retval None
 */
void MCAN2_INT0_Handler(void)
{
    stc_mcan_rx_msg_t stcRxMsg;
    stc_mcan_tx_msg_t stcTxMsg;

    if (MCAN_GetStatus(MCAN2_UNIT, MCAN_FLAG_RX_FIFO0_NEW_MSG) == SET) {
        MCAN_ClearStatus(MCAN2_UNIT, MCAN_FLAG_RX_FIFO0_NEW_MSG);
        /* Rx FIFO 0 New Message */
        DDL_Printf("MCAN2 Rx FIFO 0 new message\r\n");
        if (MCAN_GetRxMsg(MCAN2_UNIT, MCAN_RX_FIFO0, &stcRxMsg) == LL_OK) {
            McanLoadTxMsg(&stcTxMsg, &stcRxMsg);
            /* Transmit message via dedicated Tx FIFO */
            if (MCAN_AddMsgToTxFifoQueue(MCAN2_UNIT, &stcTxMsg) == LL_OK) {
                DDL_Printf("MCAN2 send out the message that received by Rx FIFO 0\r\n");
            } else {
                DDL_Printf("MCAN2 Tx FIFO full\r\n");
            }
        } else {
            /* Exception */
        }
    }

    if (MCAN_GetStatus(MCAN2_UNIT, MCAN_FLAG_RX_FIFO1_NEW_MSG) == SET) {
        MCAN_ClearStatus(MCAN2_UNIT, MCAN_FLAG_RX_FIFO1_NEW_MSG);
        /* Rx FIFO 1 New Message */
        DDL_Printf("MCAN2 Rx FIFO 1 new message\r\n");
        if (MCAN_GetRxMsg(MCAN2_UNIT, MCAN_RX_FIFO1, &stcRxMsg) == LL_OK) {
            McanLoadTxMsg(&stcTxMsg, &stcRxMsg);
            /* Transmit message via dedicated Tx FIFO */
            if (MCAN_AddMsgToTxFifoQueue(MCAN2_UNIT, &stcTxMsg) == LL_OK) {
                DDL_Printf("MCAN2 send out the message that received by Rx FIFO 1\r\n");
            } else {
                DDL_Printf("MCAN2 Tx FIFO full\r\n");
            }
        } else {
            /* Exception */
        }
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  MCAN2 interrupt line 1 IRQ handler.
 * @param  None
 * @retval None
 */
void MCAN2_INT1_Handler(void)
{
    if (MCAN_GetStatus(MCAN2_UNIT, MCAN_FLAG_TX_CPLT) == SET) {
        MCAN_ClearStatus(MCAN2_UNIT, MCAN_FLAG_TX_CPLT);
        /* Transmission Completed */
        DDL_Printf("MCAN2 transmission completed\r\n");
        /* User code if needed */
    }

    if (MCAN_GetStatus(MCAN2_UNIT, MCAN_FLAG_BUS_OFF) == SET) {
        MCAN_ClearStatus(MCAN2_UNIT, MCAN_FLAG_BUS_OFF);
        if (MCAN_GetProtocolFlagStatus(MCAN2_UNIT, MCAN_PROTOCOL_FLAG_BUS_OFF) == SET) {
            DDL_Printf("MCAN2 bus-off\r\n");
            /* If the device goes Bus_Off, it will set CCCR.INIT of its own accord, stopping all bus activities.
               The application should clear CCCR.INIT, then the device can resume normal operation.
               Once CCCR.INIT has been cleared by the CPU, the device will then wait for 129 occurrences of
               Bus Idle(129 * 11 consecutive recessive bits) before resuming normal operation. */
            MCAN_Start(MCAN2_UNIT);
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
    *pstcTxMsg = *((stc_mcan_tx_msg_t *)pstcRxMsg);
    pstcTxMsg->ESI = 0U;
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
