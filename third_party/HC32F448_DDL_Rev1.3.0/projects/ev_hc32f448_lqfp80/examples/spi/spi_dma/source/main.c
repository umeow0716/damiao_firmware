/**
 *******************************************************************************
 * @file  spi/spi_dma/source/main.c
 * @brief Main program SPI tx/rx dma for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Use key to determine master and slave device
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
 * @addtogroup SPI_DMA
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
                                         LL_PERIPH_PWC_CLK_RMU)
#define EXAMPLE_PERIPH_WP               (LL_PERIPH_EFM | LL_PERIPH_FCG)

/* Configuration for Example */
#define EXAMPLE_SPI_BUF_LEN             (128U)

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
#define DMA_RX_INT_CH                   (DMA_INT_TC_CH1)
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
static char m_au8TxBuf[EXAMPLE_SPI_BUF_LEN] = "SPI Master/Slave example: Communication between two boards!";
static char m_au8RxBuf[EXAMPLE_SPI_BUF_LEN];
static __IO en_flag_status_t m_enRxCompleteFlag = RESET;
static __IO uint32_t m_u32SPIMode = 0xFFFFFFFFUL;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  DMA transmit complete callback.
 * @param  None
 * @retval None
 */
static void DMA_TransCompleteCallback(void)
{
    m_enRxCompleteFlag = SET;
    DMA_ClearTransCompleteStatus(DMA_UNIT, DMA_RX_INT_CH);
}

#if SPI_COMM_MD == SPI_COMM_MD_CONT
/**
 * @brief  SPI master release delay (1 SCK).
 * @param  [in]  SPIx               SPI unit
 *   @arg CM_SPIx or CM_SPI
 * @retval None
 * @note   The delay while CPHA = 0 in continue communication mode.
 */
static void SPI_MasterReleaseDelay(CM_SPI_TypeDef *SPIx)
{
    uint32_t u32BusFreq, u32SpiDiv;
    uint32_t u32Speed, u32DelayTime = 1U;

    if (0U == READ_REG32_BIT(SPIx->CFG2, SPI_CFG2_CPHA)) {
        u32BusFreq = CLK_GetBusClockFreq(CLK_BUS_PCLK1);
        u32SpiDiv = (((READ_REG32_BIT(SPIx->CFG1, SPI_CFG1_CLKDIV) >> SPI_CFG1_CLKDIV_POS) + 1U) << 1U);
        u32SpiDiv *= (1UL << (READ_REG32_BIT(SPIx->CFG2, SPI_CFG2_MBR) >> SPI_CFG2_MBR_POS));
        u32Speed = u32BusFreq / u32SpiDiv;
        /* Communication speed below 1Mhz */
        if ((u32Speed < 1000000UL) && (u32Speed > 0U)) {
            u32DelayTime = 1000000UL / u32Speed;
            if (0U != (1000000UL % u32Speed)) {
                u32DelayTime += 1U;
            }
        }
        DDL_DelayUS(u32DelayTime);
    }
}
#endif

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

    /* DMA receive NVIC configure */
    (void)INTC_IrqInstallHandle(DMA_RX_IRQ_NUM, DMA_RX_INT_SRC, DDL_IRQ_PRIO_DEFAULT, DMA_TransCompleteCallback);

    /* Enable DMA and channel */
    DMA_Cmd(DMA_UNIT, ENABLE);
    (void)DMA_ChCmd(DMA_UNIT, DMA_TX_CH, ENABLE);
    (void)DMA_ChCmd(DMA_UNIT, DMA_RX_CH, ENABLE);
}

/**
 * @brief  SPI configure.
 * @param  None
 * @retval None
 */
static void DMA_ReloadConfig(void)
{
    (void)DMA_SetSrcAddr(DMA_UNIT, DMA_TX_CH, (uint32_t)(&m_au8TxBuf[0]));
    (void)DMA_SetTransCount(DMA_UNIT, DMA_TX_CH, EXAMPLE_SPI_BUF_LEN);
    (void)DMA_SetDestAddr(DMA_UNIT, DMA_RX_CH, (uint32_t)(&m_au8RxBuf[0]));
    (void)DMA_SetTransCount(DMA_UNIT, DMA_RX_CH, EXAMPLE_SPI_BUF_LEN);
    /* Enable DMA channel */
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
    /* Peripheral registers write unprotected */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    /* Configure BSP */
    BSP_CLK_Init();
    BSP_IO_Init();
    BSP_LED_Init();
    BSP_KEY_Init();
    /* Configure SPI */
    SPI_Config();
    /* DMA configuration */
    DMA_Config();
    /* Peripheral registers write protected */
    LL_PERIPH_WP(EXAMPLE_PERIPH_WP);

    /* Wait key trigger in master mode */
    if (SPI_MASTER == m_u32SPIMode) {
        while (RESET == BSP_KEY_GetStatus(BSP_KEY_2)) {
        }
    }

    for (;;) {
        m_enRxCompleteFlag = RESET;
        (void)memset(m_au8RxBuf, 0, EXAMPLE_SPI_BUF_LEN);
        DMA_ReloadConfig();

#if SPI_COMM_MD == SPI_COMM_MD_CONT
        SPI_SetCommMode(SPI_UNIT, SPI_COMM_MD_CONT);
#endif
        /* Enable SPI */
        SPI_Cmd(SPI_UNIT, ENABLE);
        /* Waiting for completion of reception */
        while (RESET == m_enRxCompleteFlag) {
        }

#if SPI_COMM_MD == SPI_COMM_MD_CONT
        if (SPI_MASTER == m_u32SPIMode) {
            SPI_MasterReleaseDelay(SPI_UNIT);
        }
        SPI_SetCommMode(SPI_UNIT, SPI_COMM_MD_NORMAL);
        if (SPI_SLAVE == m_u32SPIMode) {
            SPI_ClearStatus(SPI_UNIT, SPI_FLAG_MD_FAULT | SPI_FLAG_UNDERRUN);
        }
#endif
        /* Disable SPI */
        SPI_Cmd(SPI_UNIT, DISABLE);

        /* Compare Tx and Rx buffer */
        if (0 == memcmp(m_au8TxBuf, m_au8RxBuf, EXAMPLE_SPI_BUF_LEN)) {
            BSP_LED_On(LED_BLUE);
            BSP_LED_Off(LED_RED);
        } else {
            BSP_LED_On(LED_RED);
            BSP_LED_Off(LED_BLUE);
        }
        if (SPI_MASTER == m_u32SPIMode) {
            /* Wait for the slave to be ready */
            DDL_DelayMS(10U);
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
