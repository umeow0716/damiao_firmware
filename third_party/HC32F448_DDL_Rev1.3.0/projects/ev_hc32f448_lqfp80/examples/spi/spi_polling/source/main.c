/**
 *******************************************************************************
 * @file  spi/spi_polling/source/main.c
 * @brief Main program SPI tx/rx polling for the Device Driver Library.
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
 * @addtogroup SPI_Polling
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

/* SPI communication timeout */
#define SPI_COMM_TIMEOUT_VAL            (0x20000000UL)

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
static __IO uint32_t m_u32SPIMode = 0xFFFFFFFFUL;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
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
    SPI_Cmd(SPI_UNIT, ENABLE);
}

/**
 * @brief  Main function of SPI tx/rx polling project
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
    /* Peripheral registers write protected */
    LL_PERIPH_WP(EXAMPLE_PERIPH_WP);

    /* Wait key trigger in master mode */
    if (SPI_MASTER == m_u32SPIMode) {
        while (RESET == BSP_KEY_GetStatus(BSP_KEY_2)) {
        }
    }

    for (;;) {
        (void)memset(m_au8RxBuf, 0, EXAMPLE_SPI_BUF_LEN);
#if SPI_COMM_MD == SPI_COMM_MD_CONT
        SPI_SetCommMode(SPI_UNIT, SPI_COMM_MD_CONT);
#endif
        /* Send and receive data */
        (void)SPI_TransReceive(SPI_UNIT, m_au8TxBuf, m_au8RxBuf, EXAMPLE_SPI_BUF_LEN, SPI_COMM_TIMEOUT_VAL);
#if SPI_COMM_MD == SPI_COMM_MD_CONT
        SPI_SetCommMode(SPI_UNIT, SPI_COMM_MD_NORMAL);
        if (SPI_SLAVE == m_u32SPIMode) {
            SPI_ClearStatus(SPI_UNIT, SPI_FLAG_MD_FAULT | SPI_FLAG_UNDERRUN);
            SPI_Cmd(SPI_UNIT, ENABLE);
        }
#endif
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
