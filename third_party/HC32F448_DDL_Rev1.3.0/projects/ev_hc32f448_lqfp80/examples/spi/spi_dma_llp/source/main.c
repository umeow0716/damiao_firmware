/**
 *******************************************************************************
 * @file  spi/spi_dma_llp/source/main.c
 * @brief Main program SPI tx/rx dma llp for the Device Driver Library.
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
 * @addtogroup SPI_Dma_Llp
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* unlock/lock peripheral */
#define EXAMPLE_PERIPH_WE               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | \
                                         LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)
#define EXAMPLE_PERIPH_WP               (LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_SRAM)

/* Configuration for Example */
#define EXAMPLE_SPI_BUF_LEN             (128U)
#define EXAMPLE_SPI_TANS_CNT            (2U)

/* SPI definition */
#define SPI_UNIT                        (CM_SPI1)
#define SPI_CLK                         (FCG1_PERIPH_SPI1)
#define SPI_TX_EVT_SRC                  (EVT_SRC_SPI1_SPTI)
#define SPI_RX_EVT_SRC                  (EVT_SRC_SPI1_SPRI)

/* DMA definition */
#define DMA_UNIT                        (CM_DMA1)
#define DMA_CLK                         (FCG0_PERIPH_DMA1 | FCG0_PERIPH_AOS)
#define DMA_TX_CH                       (DMA_CH0)
#define DMA_TX_TRIG_CH                  (AOS_DMA1_0)

#define DMA_RX_CH                       (DMA_CH1)
#define DMA_RX_TC_FLAG                  (DMA_FLAG_TC_CH1)
#define DMA_RX_TRIG_CH                  (AOS_DMA1_1)
#define DMA_RX_INT_SRC                  (INT_SRC_DMA1_TC1)
#define DMA_RX_IRQ_NUM                  (INT006_IRQn)

/* SS = PA9 */
#define SPI_SS_PORT                     (GPIO_PORT_A)
#define SPI_SS_PIN                      (GPIO_PIN_09)
#define SPI_SS_FUNC                     (GPIO_FUNC_42)
/* SCK = PA8 */
#define SPI_SCK_PORT                    (GPIO_PORT_A)
#define SPI_SCK_PIN                     (GPIO_PIN_08)
#define SPI_SCK_FUNC                    (GPIO_FUNC_43)
/* MOSI = PA3 */
#define SPI_MOSI_PORT                   (GPIO_PORT_A)
#define SPI_MOSI_PIN                    (GPIO_PIN_03)
#define SPI_MOSI_FUNC                   (GPIO_FUNC_40)
/* MISO = PA11 */
#define SPI_MISO_PORT                   (GPIO_PORT_A)
#define SPI_MISO_PIN                    (GPIO_PIN_11)
#define SPI_MISO_FUNC                   (GPIO_FUNC_41)

/* SPI communication mode */
#define SPI_COMM_MD                     (SPI_COMM_MD_CONT)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static const char m_au8TxBuf[EXAMPLE_SPI_TANS_CNT][EXAMPLE_SPI_BUF_LEN] = {
    "1st round: SPI Master/Slave example: Communication between two boards!",
    "2nd round: SPI Master/Slave example: Communication between two boards!",
};
static char m_au8RxBuf[EXAMPLE_SPI_TANS_CNT][EXAMPLE_SPI_BUF_LEN] = {0};
static stc_dma_llp_descriptor_t m_stcLlpDesc_TX[EXAMPLE_SPI_TANS_CNT] = {0};
static stc_dma_llp_descriptor_t m_stcLlpDesc_RX[EXAMPLE_SPI_TANS_CNT] = {0};
static uint32_t m_u32TansCnt = 0UL;
static __IO en_flag_status_t m_enRxCompleteFlag = RESET;
static __IO uint32_t m_u32SPIMode = 0xFFFFFFFFUL;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  DMA recive complete callback.
 * @param  None
 * @retval None
 */
static void DMA_ReciveCompleteCallback(void)
{
    m_enRxCompleteFlag = SET;
    DMA_ClearTransCompleteStatus(DMA_UNIT, DMA_RX_TC_FLAG);
}

/**
 * @brief  SPI configure.
 * @param  None
 * @retval None
 */
static void SPI_Config(void)
{
    stc_spi_init_t stcSpiInit;
    stc_gpio_init_t stcGpioInit;

    do {
        if (SET == BSP_KEY_GetStatus(BSP_KEY_1)) {
            m_u32SPIMode = SPI_MASTER;
        } else if (SET == BSP_KEY_GetStatus(BSP_KEY_2)) {
            m_u32SPIMode = SPI_SLAVE;
        } else {
            /* rsvd */
        }
    } while (0xFFFFFFFFUL == m_u32SPIMode);

    /* Configure Port */
    (void)GPIO_StructInit(&stcGpioInit);
    if (SPI_MASTER == m_u32SPIMode) {
        stcGpioInit.u16PinDrv       = PIN_HIGH_DRV;
        (void)GPIO_Init(SPI_SS_PORT,   SPI_SS_PIN,   &stcGpioInit);
        (void)GPIO_Init(SPI_SCK_PORT,  SPI_SCK_PIN,  &stcGpioInit);
        (void)GPIO_Init(SPI_MOSI_PORT, SPI_MOSI_PIN, &stcGpioInit);
        stcGpioInit.u16PinDrv       = PIN_LOW_DRV;
        stcGpioInit.u16PinInputType = PIN_IN_TYPE_CMOS;
        (void)GPIO_Init(SPI_MISO_PORT, SPI_MISO_PIN, &stcGpioInit);
    } else {
        stcGpioInit.u16PinInputType = PIN_IN_TYPE_CMOS;
        (void)GPIO_Init(SPI_SS_PORT,   SPI_SS_PIN,   &stcGpioInit);
        (void)GPIO_Init(SPI_SCK_PORT,  SPI_SCK_PIN,  &stcGpioInit);
        (void)GPIO_Init(SPI_MOSI_PORT, SPI_MOSI_PIN, &stcGpioInit);
        stcGpioInit.u16PinDrv       = PIN_HIGH_DRV;
        stcGpioInit.u16PinInputType = PIN_IN_TYPE_SMT;
        (void)GPIO_Init(SPI_MISO_PORT, SPI_MISO_PIN, &stcGpioInit);
    }

    GPIO_SetFunc(SPI_SS_PORT,   SPI_SS_PIN,   SPI_SS_FUNC);
    GPIO_SetFunc(SPI_SCK_PORT,  SPI_SCK_PIN,  SPI_SCK_FUNC);
    GPIO_SetFunc(SPI_MOSI_PORT, SPI_MOSI_PIN, SPI_MOSI_FUNC);
    GPIO_SetFunc(SPI_MISO_PORT, SPI_MISO_PIN, SPI_MISO_FUNC);

    /* Configuration SPI */
    FCG_Fcg1PeriphClockCmd(SPI_CLK, ENABLE);
    (void)SPI_StructInit(&stcSpiInit);
    stcSpiInit.u32MasterSlave       = m_u32SPIMode;
    stcSpiInit.u32WireMode          = SPI_4_WIRE;
    stcSpiInit.u32TransMode         = SPI_FULL_DUPLEX;
    stcSpiInit.u32Parity            = SPI_PARITY_INVD;
    stcSpiInit.u32SpiMode           = SPI_MD_1;
    stcSpiInit.u32BaudRatePrescaler = SPI_BR_CLK_DIV64;
    stcSpiInit.u32DataBits          = SPI_DATA_SIZE_8BIT;
    stcSpiInit.u32FirstBit          = SPI_FIRST_MSB;
    stcSpiInit.u32FrameLevel        = SPI_1_FRAME;
    (void)SPI_Init(SPI_UNIT, &stcSpiInit);
}

/**
 * @brief  DMA configure.
 * @param  None
 * @retval None
 */
static void DMA_Config(void)
{
    stc_dma_init_t stcDmaInit;

    stc_dma_llp_init_t stcDmaLlpInit;
    /* DMA configuration */
    FCG_Fcg0PeriphClockCmd(DMA_CLK, ENABLE);
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32BlockSize  = 1UL;
    stcDmaInit.u32TransCount = EXAMPLE_SPI_BUF_LEN;
    stcDmaInit.u32DataWidth  = DMA_DATAWIDTH_8BIT;

    /* Configure TX */
    stcDmaInit.u32SrcAddrInc  = DMA_SRC_ADDR_INC;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    stcDmaInit.u32SrcAddr     = (uint32_t)(&m_au8TxBuf[0]);
    stcDmaInit.u32DestAddr    = (uint32_t)(&SPI_UNIT->DR);
    if (LL_OK != DMA_Init(DMA_UNIT, DMA_TX_CH, &stcDmaInit)) {
        for (;;) {
        }
    }
    AOS_SetTriggerEventSrc(DMA_TX_TRIG_CH, SPI_TX_EVT_SRC);
    /* Configure LLP descriptor. */
    m_stcLlpDesc_TX[0].SARx = (uint32_t)(&m_au8TxBuf[0]);
    m_stcLlpDesc_TX[0].DARx = (uint32_t)(&SPI_UNIT->DR);
    m_stcLlpDesc_TX[0].LLPx = (uint32_t)(&m_stcLlpDesc_TX[1]);
    m_stcLlpDesc_TX[0].DTCTLx = (EXAMPLE_SPI_BUF_LEN << DMA_DTCTL_CNT_POS) | (1UL << DMA_DTCTL_BLKSIZE_POS);
    m_stcLlpDesc_TX[0].CHCTLx = DMA_SRC_ADDR_INC | DMA_DEST_ADDR_FIX | DMA_DATAWIDTH_8BIT | \
                                DMA_LLP_ENABLE | DMA_INT_ENABLE | DMA_LLP_WAIT;

    m_stcLlpDesc_TX[1].SARx = (uint32_t)(&m_au8TxBuf[1]);
    m_stcLlpDesc_TX[1].DARx = (uint32_t)(&SPI_UNIT->DR);
    m_stcLlpDesc_TX[1].LLPx = (uint32_t)(&m_stcLlpDesc_TX[0]);
    m_stcLlpDesc_TX[1].DTCTLx = (EXAMPLE_SPI_BUF_LEN << DMA_DTCTL_CNT_POS) | (1UL << DMA_DTCTL_BLKSIZE_POS);
    m_stcLlpDesc_TX[1].CHCTLx = DMA_SRC_ADDR_INC | DMA_DEST_ADDR_FIX | DMA_DATAWIDTH_8BIT | \
                                DMA_LLP_ENABLE | DMA_INT_ENABLE | DMA_LLP_WAIT;

    (void)DMA_LlpStructInit(&stcDmaLlpInit);
    stcDmaLlpInit.u32State = DMA_LLP_ENABLE;
    stcDmaLlpInit.u32Mode = DMA_LLP_WAIT;
    stcDmaLlpInit.u32Addr = (uint32_t)(&m_stcLlpDesc_TX[1]);
    (void)DMA_LlpInit(DMA_UNIT, DMA_TX_CH, &stcDmaLlpInit);

    /* Configure RX */
    stcDmaInit.u32IntEn       = DMA_INT_ENABLE;
    stcDmaInit.u32SrcAddrInc  = DMA_SRC_ADDR_FIX;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_INC;
    stcDmaInit.u32SrcAddr     = (uint32_t)(&SPI_UNIT->DR);
    stcDmaInit.u32DestAddr    = (uint32_t)(&m_au8RxBuf[0]);
    if (LL_OK != DMA_Init(DMA_UNIT, DMA_RX_CH, &stcDmaInit)) {
        for (;;) {
        }
    }
    AOS_SetTriggerEventSrc(DMA_RX_TRIG_CH, SPI_RX_EVT_SRC);
    /* Configure LLP descriptor. */
    m_stcLlpDesc_RX[0].SARx = (uint32_t)(&SPI_UNIT->DR);
    m_stcLlpDesc_RX[0].DARx = (uint32_t)(&m_au8RxBuf[0]);
    m_stcLlpDesc_RX[0].LLPx = (uint32_t)(&m_stcLlpDesc_RX[1]);
    m_stcLlpDesc_RX[0].DTCTLx = (EXAMPLE_SPI_BUF_LEN << DMA_DTCTL_CNT_POS) | (1UL << DMA_DTCTL_BLKSIZE_POS);
    m_stcLlpDesc_RX[0].CHCTLx = DMA_SRC_ADDR_FIX | DMA_DEST_ADDR_INC | DMA_DATAWIDTH_8BIT | \
                                DMA_LLP_ENABLE | DMA_INT_ENABLE | DMA_LLP_WAIT;

    m_stcLlpDesc_RX[1].SARx = (uint32_t)(&SPI_UNIT->DR);
    m_stcLlpDesc_RX[1].DARx = (uint32_t)(&m_au8RxBuf[1]);
    m_stcLlpDesc_RX[1].LLPx = (uint32_t)(&m_stcLlpDesc_RX[0]);
    m_stcLlpDesc_RX[1].DTCTLx = (EXAMPLE_SPI_BUF_LEN << DMA_DTCTL_CNT_POS) | (1UL << DMA_DTCTL_BLKSIZE_POS);
    m_stcLlpDesc_RX[1].CHCTLx = DMA_SRC_ADDR_FIX | DMA_DEST_ADDR_INC | DMA_DATAWIDTH_8BIT | \
                                DMA_LLP_ENABLE | DMA_INT_ENABLE | DMA_LLP_WAIT;

    stcDmaLlpInit.u32Addr = (uint32_t)(&m_stcLlpDesc_RX[1]);
    (void)DMA_LlpInit(DMA_UNIT, DMA_RX_CH, &stcDmaLlpInit);

    /* DMA receive NVIC configure */
    DMA_ClearTransCompleteStatus(DMA_UNIT, DMA_FLAG_BTC_CH1);
    (void)INTC_IrqInstallHandle(DMA_RX_IRQ_NUM, DMA_RX_INT_SRC,
                                DDL_IRQ_PRIO_DEFAULT, DMA_ReciveCompleteCallback);

    /* Enable DMA and channel */
    DMA_Cmd(DMA_UNIT, ENABLE);
    (void)DMA_ChCmd(DMA_UNIT, DMA_TX_CH, ENABLE);
    (void)DMA_ChCmd(DMA_UNIT, DMA_RX_CH, ENABLE);
}

/**
 * @brief  Main function of SPI tx/rx dma project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint32_t index = 0;

    /* Peripheral registers write unprotected */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    /* Configure BSP */
    BSP_CLK_Init();
    BSP_IO_Init();
    BSP_LED_Init();
    BSP_KEY_Init();
    /* Configure SPI */
    SPI_Config();
    /* Configure DMA */
    DMA_Config();
    /* Peripheral registers write protected */
    LL_PERIPH_WP(EXAMPLE_PERIPH_WP);

    /* Wait key trigger in master mode */
    if (SPI_MASTER == m_u32SPIMode) {
        while (RESET == BSP_KEY_GetStatus(BSP_KEY_2)) {
        }
    }
#if (SPI_COMM_MD == SPI_COMM_MD_CONT)
    SPI_SetCommMode(SPI_UNIT, SPI_COMM_MD_CONT);
#endif
    SPI_Cmd(SPI_UNIT, ENABLE);

    for (;;) {
        if (SET == m_enRxCompleteFlag) {
            m_enRxCompleteFlag = RESET;
#if (SPI_COMM_MD == SPI_COMM_MD_CONT)
            if (SPI_SLAVE == m_u32SPIMode) {
                SPI_ClearStatus(SPI_UNIT, SPI_FLAG_MD_FAULT | SPI_FLAG_UNDERRUN);
                SPI_Cmd(SPI_UNIT, ENABLE);
            }
#endif
            /* Compare Tx and Rx buffer */
            if (0 == memcmp(m_au8TxBuf[index], m_au8RxBuf[index], EXAMPLE_SPI_BUF_LEN)) {
                (void)memset(m_au8RxBuf[index], 0, EXAMPLE_SPI_BUF_LEN);
                if (m_u32TansCnt++ >= 30UL) {
                    m_u32TansCnt = 0UL;
                    BSP_LED_Toggle(LED_BLUE);
                }
                BSP_LED_Off(LED_RED);
            } else {
                SPI_Cmd(SPI_UNIT, DISABLE);
                BSP_LED_Off(LED_BLUE);
                for (;;) {
                    BSP_LED_Toggle(LED_RED);
                    DDL_DelayMS(500);
                }
            }
            if (++index >= EXAMPLE_SPI_TANS_CNT) {
                index = 0;
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
