/**
 *******************************************************************************
 * @file  i2c/i2c_slave_dma/source/main.c
 * @brief Main program of I2C slave polling for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Remove redundant process for slave address commands
                                    Add definition I2C_ADDR_MD as address condition select
                                    Configure DMA interrupt disable in I2C_DMA_Initialize() function
   2025-11-03       CDT             Clear DMA transfer completed flag before DMA start
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
 * @addtogroup I2C_slave_dma
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

/* Define I2C unit used for the example */
#define I2C_UNIT                        (CM_I2C1)
#define I2C_FCG_USE                     (FCG1_PERIPH_I2C1)
/* Define slave device address for example */
#define DEVICE_ADDR                     (0x06U)
/* I2C address mode */
#define I2C_ADDR_MD_7BIT                (0U)
#define I2C_ADDR_MD_10BIT               (1U)
/* Config I2C address mode: I2C_ADDR_MD_7BIT or I2C_ADDR_MD_10BIT */
#define I2C_ADDR_MD                     (I2C_ADDR_MD_7BIT)

/* Define port and pin for SDA and SCL */
#define I2C_SCL_PORT                    (GPIO_PORT_E)
#define I2C_SCL_PIN                     (GPIO_PIN_01)
#define I2C_SDA_PORT                    (GPIO_PORT_E)
#define I2C_SDA_PIN                     (GPIO_PIN_00)
#define I2C_GPIO_SCL_FUNC               (GPIO_FUNC_49)
#define I2C_GPIO_SDA_FUNC               (GPIO_FUNC_48)

/* Define DMA unit and channel */
#define DMA_FCG_USE                     (FCG0_PERIPH_DMA1 | FCG0_PERIPH_AOS)

#define I2C_TX_EVT_SRC                  (EVT_SRC_I2C1_TEI)
#define I2C_RX_EVT_SRC                  (EVT_SRC_I2C1_RXI)

#define DMA_TX_UNIT                     (CM_DMA1)
#define DMA_TX_CH                       (DMA_CH0)
#define DMA_TX_TRIG_CH                  (AOS_DMA1_0)
#define DMA_RX_UNIT                     (CM_DMA1)
#define DMA_RX_CH                       (DMA_CH1)
#define DMA_RX_INT_CH                   (DMA_INT_TC_CH1)
#define DMA_RX_TRIG_CH                  (AOS_DMA1_1)

#define DMA_RX_TC_FLAG                  (DMA_FLAG_TC_CH1)
#define DMA_TX_TC_FLAG                  (DMA_FLAG_TC_CH0)

#define TIMEOUT                         (0x40000UL)

/* Define Write and read data length for the example */
#define TEST_DATA_LEN                   (256U)
/* Define i2c baudrate */
#define I2C_BAUDRATE                    (400000UL)

#define LED_GREEN_PORT                  (GPIO_PORT_A)
#define LED_GREEN_PIN                   (GPIO_PIN_02)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static uint8_t u8RxBuf[TEST_DATA_LEN];

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Initialize LED.
 * @param  None
 * @retval None
 */
static void LedInit(void)
{
    stc_gpio_init_t stcGpioInit;

    /* RGB LED initialize */
    (void)GPIO_StructInit(&stcGpioInit);
    (void)GPIO_Init(LED_GREEN_PORT, LED_GREEN_PIN, &stcGpioInit);

    /* "Turn off" LED before set to output */
    GPIO_SetPins(LED_GREEN_PORT, LED_GREEN_PIN);

    /* Output enable */
    GPIO_OutputCmd(LED_GREEN_PORT, LED_GREEN_PIN, ENABLE);
}

/**
 * @brief  Toggle LED.
 * @param  None
 * @retval None
 */
static void LedToggle(void)
{
    GPIO_TogglePins(LED_GREEN_PORT, LED_GREEN_PIN);
}

/**
 * @brief  SysTick interrupt handler function.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
    SysTick_IncTick();

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  DMA initialize for I2C slave
 * @param  None
 * @retval None
 */
static void I2C_DMA_Initialize(void)
{
    stc_dma_init_t stcDmaInit;

    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32BlockSize  = 1UL;
    stcDmaInit.u32TransCount = 0UL;
    stcDmaInit.u32DataWidth  = DMA_DATAWIDTH_8BIT;
    /* Configure TX */
    stcDmaInit.u32SrcAddrInc  = DMA_SRC_ADDR_INC;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    stcDmaInit.u32SrcAddr     = (uint32_t)NULL;
    stcDmaInit.u32DestAddr    = (uint32_t)(&I2C_UNIT->DTR);
    if (LL_OK != DMA_Init(DMA_TX_UNIT, DMA_TX_CH, &stcDmaInit)) {
        for (;;) {
        }
    }
    AOS_SetTriggerEventSrc(DMA_TX_TRIG_CH, I2C_TX_EVT_SRC);
    /* Configure RX */
    stcDmaInit.u32IntEn       = DMA_INT_DISABLE;
    stcDmaInit.u32SrcAddrInc  = DMA_SRC_ADDR_FIX;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_INC;
    stcDmaInit.u32SrcAddr     = (uint32_t)(&I2C_UNIT->DRR);
    stcDmaInit.u32DestAddr    = (uint32_t)NULL;
    if (LL_OK != DMA_Init(DMA_RX_UNIT, DMA_RX_CH, &stcDmaInit)) {
        for (;;) {
        }
    }
    AOS_SetTriggerEventSrc(DMA_RX_TRIG_CH, I2C_RX_EVT_SRC);

    /* Enable DMA unit */
    DMA_Cmd(DMA_TX_UNIT, ENABLE);
    DMA_Cmd(DMA_RX_UNIT, ENABLE);
}

/**
 * @brief  Slave receive data DMA start
 * @param  au8Data                  Data array
 * @param  u32Size                  Data size
 * @retval None
 */
static void I2C_DMA_RxStart(uint8_t au8Data[], uint32_t u32Size)
{
    DMA_ClearTransCompleteStatus(DMA_RX_UNIT, DMA_RX_TC_FLAG);
    (void)DMA_SetTransCount(DMA_RX_UNIT, DMA_RX_CH, u32Size);
    DMA_SetDestAddr(DMA_RX_UNIT, DMA_RX_CH, (uint32_t)(&au8Data[0]));
    DMA_ChCmd(DMA_RX_UNIT, DMA_RX_CH, ENABLE);
}

/**
 * @brief  Slave transmit data DMA start
 * @param  au8Data                  Data array
 * @param  u32Size                  Data size
 * @retval None
 */
static void I2C_DMA_TxStart(uint8_t au8Data[], uint32_t u32Size)
{
    DMA_ClearTransCompleteStatus(DMA_TX_UNIT, DMA_TX_TC_FLAG);
    (void)DMA_SetTransCount(DMA_TX_UNIT, DMA_TX_CH, u32Size);
    DMA_SetSrcAddr(DMA_TX_UNIT, DMA_TX_CH, (uint32_t)(&au8Data[0]));
    DMA_ChCmd(DMA_TX_UNIT, DMA_TX_CH, ENABLE);
}

/**
 * @brief  Slave receive data
 * @param  au8Data                  Data array
 * @param  u32Size                  Data size
 * @param  u32Timeout               Time out count
 * @retval int32_t:
 *            - LL_OK:              Success
 *            - LL_ERR:             Failed
 *            - LL_ERR_TIMEOUT:     Time out
 */
static int32_t I2C_Slave_Receive(uint8_t au8Data[], uint32_t u32Size, uint32_t u32Timeout)
{
    int32_t i32Ret;

    I2C_DMA_RxStart(au8Data, u32Size);

    I2C_Cmd(I2C_UNIT, ENABLE);

    /* Clear status */
    I2C_ClearStatus(I2C_UNIT, I2C_CLR_STOPFCLR | I2C_CLR_NACKFCLR);

    /* Wait slave address matched */
    while (RESET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_MATCH_ADDR0)) {
        ;
    }
    I2C_ClearStatus(I2C_UNIT, I2C_CLR_SLADDR0FCLR);

    if (RESET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_TRA)) {
        /* Slave receive data via DMA */
        /* Wait DMA finish */
        while (RESET == DMA_GetTransCompleteStatus(DMA_RX_UNIT, DMA_RX_TC_FLAG)) {
            /* Need add timeout release process here */
            i32Ret = LL_ERR_TIMEOUT;
        }
        i32Ret = I2C_WaitStatus(I2C_UNIT, I2C_FLAG_STOP, SET, u32Timeout);
    } else {
        i32Ret = LL_ERR;
    }

    I2C_Cmd(I2C_UNIT, DISABLE);
    return i32Ret;
}

/**
 * @brief  Slave transmit data
 * @param  au8Data                  Data array
 * @param  u32Size                  Data size
 * @param  u32Timeout               Time out count
 * @retval int32_t:
 *            - LL_OK:              Success
 *            - LL_ERR:             Failed
 *            - LL_ERR_TIMEOUT:     Time out
 */
static int32_t I2C_Slave_Transmit(uint8_t au8Data[], uint32_t u32Size, uint32_t u32Timeout)
{
    int32_t i32Ret;

    if (u32Size > 1U) {
        I2C_DMA_TxStart(au8Data + 1U, u32Size - 1U);
    }

    I2C_Cmd(I2C_UNIT, ENABLE);

    /* Clear status */
    I2C_ClearStatus(I2C_UNIT, I2C_CLR_STOPFCLR | I2C_CLR_NACKFCLR);

    /* Wait slave address matched */
    while (RESET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_MATCH_ADDR0)) {
        ;
    }
    I2C_ClearStatus(I2C_UNIT, I2C_CLR_SLADDR0FCLR);

#if (I2C_ADDR_MD == I2C_ADDR_MD_10BIT)
    /* Wait slave address matched */
    while (RESET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_MATCH_ADDR0)) {
        ;
    }
    I2C_ClearStatus(I2C_UNIT, I2C_CLR_SLADDR0FCLR);
#endif

    if (RESET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_TRA)) {
        i32Ret = LL_ERR;
    } else {
        /* Send first data */
        I2C_WriteData(I2C_UNIT, au8Data[0]);

        if (u32Size > 1U) {
            /* Wait DMA finish */
            while (RESET == DMA_GetTransCompleteStatus(DMA_TX_UNIT, DMA_TX_TC_FLAG)) {
                /* Need add timeout release process here */
                i32Ret = LL_ERR_TIMEOUT;
            }
        }
        i32Ret = I2C_WaitStatus(I2C_UNIT, I2C_FLAG_TX_CPLT, SET, u32Timeout);

        if ((LL_OK == i32Ret) || (LL_ERR_TIMEOUT == i32Ret)) {
            /* Release SCL pin */
            (void)I2C_ReadData(I2C_UNIT);

            /* Wait stop condition */
            i32Ret = I2C_WaitStatus(I2C_UNIT, I2C_FLAG_STOP, SET, u32Timeout);
        }
    }

    I2C_Cmd(I2C_UNIT, DISABLE);
    return i32Ret;
}


/**
 * @brief  Initialize the I2C peripheral for slave
 * @param  None
 * @retval int32_t:
 *            - LL_OK:                  Success
 *            - LL_ERR_INVD_PARAM:      Invalid parameter
 */
static int32_t Slave_Initialize(void)
{
    int32_t i32Ret;
    stc_i2c_init_t stcI2cInit;
    float32_t fErr;

    (void)I2C_DeInit(I2C_UNIT);

    (void)I2C_StructInit(&stcI2cInit);
    stcI2cInit.u32ClockDiv = I2C_CLK_DIV1;
    stcI2cInit.u32Baudrate = I2C_BAUDRATE;
    stcI2cInit.u32SclTime = 33UL;
    i32Ret = I2C_Init(I2C_UNIT, &stcI2cInit, &fErr);

    if (LL_OK == i32Ret) {
        /* Set slave address*/
#if (I2C_ADDR_MD == I2C_ADDR_MD_10BIT)
        I2C_SlaveAddrConfig(I2C_UNIT, I2C_ADDR0, I2C_ADDR_10BIT, DEVICE_ADDR);
#else
        I2C_SlaveAddrConfig(I2C_UNIT, I2C_ADDR0, I2C_ADDR_7BIT, DEVICE_ADDR);
#endif
    }

    /* I2C FIFO function config */
    I2C_FIFO_Cmd(I2C_UNIT, ENABLE);
    return i32Ret;
}

/**
 * @brief  Main function
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Unlock peripherals or registers */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    /* BSP initialization */
    BSP_CLK_Init();
    LedInit();
    /* SysTick configuration */
    (void)SysTick_Init(1000U);

    (void)memset(u8RxBuf, (int32_t)0x00U, TEST_DATA_LEN);

    /* Initialize I2C port*/
    GPIO_SetFunc(I2C_SCL_PORT, I2C_SCL_PIN, I2C_GPIO_SCL_FUNC);
    GPIO_SetFunc(I2C_SDA_PORT, I2C_SDA_PIN, I2C_GPIO_SDA_FUNC);

    /* Enable I2C and Peripheral*/
    FCG_Fcg1PeriphClockCmd(I2C_FCG_USE, ENABLE);
    /* Initialize I2C peripheral and enable function*/
    if (LL_OK != Slave_Initialize()) {
        /* Peripheral registers write protected */
        LL_PERIPH_WP(EXAMPLE_PERIPH_WP);
        /* Initialize error*/
        for (;;) {
            LedToggle();
            SysTick_Delay(200UL);
        }
    }

    /* DMA/AOS FCG enable */
    FCG_Fcg0PeriphClockCmd(DMA_FCG_USE, ENABLE);
    I2C_DMA_Initialize();

    /* Peripheral registers write protected */
    LL_PERIPH_WP(EXAMPLE_PERIPH_WP);

    for (;;) {
        if (LL_OK == I2C_Slave_Receive(u8RxBuf, TEST_DATA_LEN, TIMEOUT)) {
            if (LL_OK != I2C_Slave_Transmit(u8RxBuf, TEST_DATA_LEN, TIMEOUT)) {
                /* Failed */
                break;
            }
        } else {
            /* Failed */
            break;
        }
    }

    /* Communication failed */
    for (;;) {
        LedToggle();
        SysTick_Delay(200UL);
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
