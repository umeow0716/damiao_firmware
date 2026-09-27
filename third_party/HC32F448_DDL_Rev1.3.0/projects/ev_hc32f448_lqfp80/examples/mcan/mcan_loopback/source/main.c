/**
 *******************************************************************************
 * @file  mcan/mcan_loopback/source/main.c
 * @brief Main program of MCAN loopback for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Optimized the example.
   2025-11-03       CDT             Use BSP_PRINTF_BAUDRATE to replace immediate value 115200.
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
#include <stdlib.h>
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

#define MCAN_TX_PORT                    (GPIO_PORT_C)
#define MCAN_TX_PIN                     (GPIO_PIN_12)
#define MCAN_TX_PIN_FUNC                (GPIO_FUNC_56)
#define MCAN_RX_PORT                    (GPIO_PORT_D)
#define MCAN_RX_PIN                     (GPIO_PIN_00)
#define MCAN_RX_PIN_FUNC                (GPIO_FUNC_57)

#elif (MCAN_UNIT_SEL == MCAN_UNIT2)
#define MCAN_UNIT                       (CM_MCAN2)
#define MCAN_PERIPH_CLK                 (FCG1_PERIPH_MCAN2)
#define MCAN_CLK_UNIT                   (CLK_MCAN2)
#define MCAN_CLK_SRC                    (CLK_MCANCLK_SYSCLK_DIV5)

#define MCAN_TX_PORT                    (GPIO_PORT_H)
#define MCAN_TX_PIN                     (GPIO_PIN_02)
#define MCAN_TX_PIN_FUNC                (GPIO_FUNC_56)
#define MCAN_RX_PORT                    (GPIO_PORT_E)
#define MCAN_RX_PIN                     (GPIO_PIN_04)
#define MCAN_RX_PIN_FUNC                (GPIO_FUNC_57)

#else
#error "Not supported selection."
#endif

/* Message RAM */
/* Each standard filter element size is 4 bytes */
#define MCAN_STD_FILTER_NUM             (1U)
/* Each extended filter element size is 8 bytes */
#define MCAN_EXT_FILTER_NUM             (1U)
/* Each Rx FIFO0 element size is 64+8 bytes */
#define MCAN_RX_FIFO0_NUM               (1U)
#define MCAN_RX_FIFO0_DATA_FIELD_SIZE   MCAN_DATA_SIZE_64BYTE
/* Each Rx FIFO1 element size is 64+8 bytes */
#define MCAN_RX_FIFO1_NUM               (1U)
#define MCAN_RX_FIFO1_DATA_FIELD_SIZE   MCAN_DATA_SIZE_64BYTE
/* Each Tx buffer element size is 64+8 bytes */
#define MCAN_TX_BUF_NUM                 (1U)
#define MCAN_TX_BUF_DATA_FIELD_SIZE     MCAN_DATA_SIZE_64BYTE

#define MCAN_STD_ID_MASK                (0x7FFUL)
#define MCAN_EXT_ID_MASK                (0x1FFFFFFFUL)
#define MCAN_STD_ID_MAX                 MCAN_STD_ID_MASK

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void McanCommClockConfig(void);
static void McanInitConfig(void);
static void McanPinConfig(void);
static void McanPhyEnable(void);

static void McanLoopback(void);
static void McanVerifyRxMsg(stc_mcan_rx_msg_t *pstcRxMsg, stc_mcan_tx_msg_t *pstcTxMsg);

static void GetRandom(uint32_t *pu32Data, uint32_t u32WordLen);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static uint32_t m_au32StdFilterAddrList[MCAN_STD_FILTER_NUM];
static uint32_t m_au32ExtFilterAddrList[MCAN_EXT_FILTER_NUM];
static uint32_t m_au32RxFifo0AddrList[MCAN_RX_FIFO0_NUM];
static uint32_t m_au32RxFifo1AddrList[MCAN_RX_FIFO1_NUM];
static uint32_t m_au32TxBufferAddrList[MCAN_TX_BUF_NUM];

static stc_mcan_msg_ram_addr_t m_stcMsgRamAddrList = {
    .pu32StdFilterAddrList = m_au32StdFilterAddrList,
    .pu32ExtFilterAddrList = m_au32ExtFilterAddrList,
    .pu32RxFifo0AddrList = m_au32RxFifo0AddrList,
    .pu32RxFifo1AddrList = m_au32RxFifo1AddrList,
    .pu32RxBufferAddrList = NULL,
    .pu32TxEventAddrList = NULL,
    .pu32TxBufferAddrList = m_au32TxBufferAddrList,
    .pu32TxFifoQueueAddrList = NULL,
};

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Main function of mcan_loopback project
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
    McanPinConfig();
    McanPhyEnable();

    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_INTC | LL_PERIPH_PWC_CLK_RMU);

    /******************************* Application start *******************************************/
    /* Get each element RAM address if needed */
    MCAN_GetMsgRamAddr(MCAN_UNIT, &m_stcMsgRamAddrList);

    /* Start the MCAN module */
    MCAN_Start(MCAN_UNIT);

    /*********************************************************************************************/

    for (;;) {
        McanLoopback();
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
    stc_mcan_filter_t stcStdFilter = {
        .u32IdType       = MCAN_STD_ID,
        .u32FilterType   = MCAN_FILTER_RANGE,
        .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO0,
        .u32FilterId1    = 0x0UL,
        .u32FilterId2    = MCAN_STD_ID_MAX,
    };
    stc_mcan_filter_t stcExtFilter = {
        .u32IdType       = MCAN_EXT_ID,
        .u32FilterType   = MCAN_FILTER_MASK,
        .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO1,
        .u32FilterId1    = 0x0UL,
        .u32FilterId2    = 0x0UL,
    };

    /* Default bit rate based on 40MHz clock: nominal bit rate 500Kbps, data bit rate 4Mbps. */
    (void)MCAN_StructInit(&stcMcanInit);
    stcMcanInit.u32Mode = MCAN_MD_EXTERN_LOOPBACK;

    /* Message RAM */
    stcMcanInit.stcMsgRam.u32StdFilterNum      = MCAN_STD_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32ExtFilterNum      = MCAN_EXT_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0Num        = MCAN_RX_FIFO0_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0DataSize   = MCAN_RX_FIFO0_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32RxFifo1Num        = MCAN_RX_FIFO1_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo1DataSize   = MCAN_RX_FIFO1_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32TxBufferNum       = MCAN_TX_BUF_NUM;
    stcMcanInit.stcMsgRam.u32TxDataSize        = MCAN_TX_BUF_DATA_FIELD_SIZE;
    /* Acceptance filter */
    stcMcanInit.stcFilter.pstcStdFilterList     = &stcStdFilter;
    stcMcanInit.stcFilter.pstcExtFilterList     = &stcExtFilter;
    stcMcanInit.stcFilter.u32StdFilterConfigNum = stcMcanInit.stcMsgRam.u32StdFilterNum;
    stcMcanInit.stcFilter.u32ExtFilterConfigNum = stcMcanInit.stcMsgRam.u32ExtFilterNum;

    FCG_Fcg1PeriphClockCmd(MCAN_PERIPH_CLK, ENABLE);
    (void)MCAN_Init(MCAN_UNIT, &stcMcanInit);
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
 * @brief  MCAN loopback test entity.
 * @param  None
 * @retval None
 */
static void McanLoopback(void)
{
    uint32_t u32Ctrl;
    uint32_t u32RxFlag;
    uint32_t u32RxFifo;
    stc_mcan_tx_msg_t stcTxMsg = {
        .u32TxBuffer = MCAN_TX_BUF0,
    };
    stc_mcan_rx_msg_t stcRxMsg;

    for (;;) {
        GetRandom((uint32_t *)stcTxMsg.au8Data, 16U);
        GetRandom((uint32_t *)&stcTxMsg.ID, 1U);
        GetRandom((uint32_t *)&u32Ctrl, 1U);

        stcTxMsg.IDE = (u32Ctrl & 0x01UL);
        stcTxMsg.DLC = (u32Ctrl & 0xFUL);
        stcTxMsg.FDF = ((u32Ctrl >> 1U) & 0x01UL);
        /* BRS based on FDF */
        if (stcTxMsg.FDF != 0U) {
            stcTxMsg.BRS = ((u32Ctrl >> 2U) & 0x01UL);
        } else {
            stcTxMsg.BRS = 0U;
        }

        if (stcTxMsg.IDE != 0U) {
            /* Extended ID to Rx FIFO1 */
            stcTxMsg.ID &= MCAN_EXT_ID_MASK;
            u32RxFlag = MCAN_FLAG_RX_FIFO1_NEW_MSG;
            u32RxFifo = MCAN_RX_FIFO1;
        } else {
            /* Standard ID to Rx FIFO0 */
            stcTxMsg.ID &= MCAN_STD_ID_MASK;
            u32RxFlag = MCAN_FLAG_RX_FIFO0_NEW_MSG;
            u32RxFifo = MCAN_RX_FIFO0;
        }

        (void)MCAN_AddMsgToTxBuffer(MCAN_UNIT, &stcTxMsg);
        MCAN_EnableTxBufferRequest(MCAN_UNIT, stcTxMsg.u32TxBuffer);
        /* Wait Tx completed */
        while (MCAN_CheckTxOccurred(MCAN_UNIT, stcTxMsg.u32TxBuffer) != LL_OK) {
            __NOP();
        }
        /* Check Rx FIFO0 New Message flag */
        if (MCAN_GetStatus(MCAN_UNIT, u32RxFlag) == SET) {
            MCAN_ClearStatus(MCAN_UNIT, u32RxFlag);
            if (MCAN_GetRxMsg(MCAN_UNIT, u32RxFifo, &stcRxMsg) == LL_OK) {
                /* Verify Rx message */
                McanVerifyRxMsg(&stcRxMsg, &stcTxMsg);
            } else {
                /* Exception occurred */
                while (1U);
            }
        } else {
            /* Exception occurred */
            while (1U);
        }
    }
}

/**
 * @brief Configure the transmitter delay compensation
 * @param  [in]  pstcRxMsg              Pointer to the received message.
 * @param  [in]  pstcTxMsg              Pointer to the transmitted message.
 * @retval None
 */
static void McanVerifyRxMsg(stc_mcan_rx_msg_t *pstcRxMsg, stc_mcan_tx_msg_t *pstcTxMsg)
{
    uint32_t i;
    int32_t i32Ret = LL_OK;

    if ((pstcRxMsg->ID != pstcTxMsg->ID) || \
        (pstcRxMsg->IDE != pstcTxMsg->IDE) || \
        (pstcRxMsg->RTR != pstcTxMsg->RTR) || \
        (pstcRxMsg->DLC != pstcTxMsg->DLC) || \
        (pstcRxMsg->ESI != pstcTxMsg->ESI) || \
        (pstcRxMsg->BRS != pstcTxMsg->BRS) || \
        (pstcRxMsg->FDF != pstcTxMsg->FDF)) {
        i32Ret = LL_ERR;
    }

    if (i32Ret == LL_OK) {
        DDL_Printf(">>>> ---------------------- <<<<\r\n");
        if (pstcRxMsg->IDE == 1U) {
            DDL_Printf("Rx ID: %.8x, ", (unsigned int)pstcRxMsg->ID);
        } else {
            DDL_Printf("Rx ID: %.3x, ", (unsigned int)pstcRxMsg->ID);
        }
        DDL_Printf("Data:");
        for (i = 0U; i < pstcRxMsg->u32DataSize; i++) {
            DDL_Printf(" %.2x", pstcRxMsg->au8Data[i]);
            if (pstcRxMsg->au8Data[i] != pstcTxMsg->au8Data[i]) {
                i32Ret = LL_ERR;
            }
        }
        DDL_Printf("\r\n");
        if (i32Ret != LL_OK) {
            DDL_Printf("ERROR: Rx data field verify failure!\r\n");
            DDL_Printf("Expected data:");
            for (i = 0U; i < pstcRxMsg->u32DataSize; i++) {
                DDL_Printf(" %.2x", pstcTxMsg->au8Data[i]);
            }
            DDL_Printf("\r\n");
        } else {
            DDL_Printf("This is the self-transmitted frame. CAN loopback OK!\r\n");
        }
    }

    if (i32Ret != LL_OK) {
        /* Exception occurred */
        DDL_Printf("This is not the self-transmitted frame!!! CAN loopback test END!!!\r\n");
        while (1U);
    }
}

/**
 * @brief Get random number for test.
 * @param  [in]  pu32Data               Pointer to an uint32_t type data buffer.
 * @param  [in]  u32WordLen             The data buffer length in word.
 * @retval None
 */
static void GetRandom(uint32_t *pu32Data, uint32_t u32WordLen)
{
    uint32_t i;
    for (i = 0; i < u32WordLen; i++) {
        pu32Data[i] = rand();
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
