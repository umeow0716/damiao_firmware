/**
 *******************************************************************************
 * @file  exmc/exmc_smc_sram_is61lv6416_dma/source/main.c
 * @brief This example demonstrates DMA access SMC SRAM function.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-12-15       CDT             First version
   2025-11-03       CDT             Use BSP_XTAL_PIN to replace BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN
                                    MEMORY_Access8/16/32 use LED show status
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
 * @addtogroup EXMC_SMC_SRAM_IS61LV6416_DMA
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

/* SMC DMA configuration define */
#define SMC_DMA_UNIT                    (CM_DMA2)
#define SMC_DMA_CLK                     (FCG0_PERIPH_DMA2 | FCG0_PERIPH_AOS)
#define SMC_DMA_CH                      (DMA_CH0)
#define SMC_DMA_TRIG_CH                 (AOS_DMA2_0)
#define SMC_DMA_INT_CH                  (DMA_INT_TC_CH0)
#define SMC_DMA_INT_SRC                 (INT_SRC_DMA2_TC0)
#define SMC_DMA_IRQn                    (INT006_IRQn)

#define SMC_DMA_BLOCK_SIZE              (1024U)
#define SMC_DMA_TRANS_CNT               (1U)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static uint32_t m_u32StartAddr = 0UL;
static uint32_t m_u32ByteSize = 0UL;

static uint8_t m_au8ReadData[SMC_DMA_BLOCK_SIZE];
static uint8_t m_au8WriteData[SMC_DMA_BLOCK_SIZE];

static uint16_t m_au16ReadData[SMC_DMA_BLOCK_SIZE];
static uint16_t m_au16WriteData[SMC_DMA_BLOCK_SIZE];

static uint32_t m_au32ReadData[SMC_DMA_BLOCK_SIZE];
static uint32_t m_au32WriteData[SMC_DMA_BLOCK_SIZE];

static uint32_t m_u32TestCnt = 0UL;
static uint32_t m_u32ByteTestErrorCnt = 0UL;
static uint32_t m_u32HalfwordTestErrorCnt = 0UL;
static uint32_t m_u32WordTestErrorCnt = 0UL;

__IO static en_flag_status_t m_u8DmaTcEnd = RESET;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  BSP clock initialize.
 *         SET board system clock to PLLH@160MHz
 *         Flash: 3 wait
 *         SRAM_H: 0 wait
 *         SRAM_B: 1 wait
 *         PCLK0: 160MHz
 *         PCLK1: 80MHz
 *         PCLK2: 40MHz
 *         PCLK3: 40MHz
 *         PCLK4: 80MHz
 *         EXCLK: 40MHz
 *         HCLK:  160MHz
 * @param  None
 * @retval None
 */
void BSP_CLK_Init(void)
{
    stc_clock_xtal_init_t stcXtalInit;
    stc_clock_pll_init_t stcPLLHInit;

    /* PCLK0, HCLK Max 200MHz */
    /* PCLK1, PCLK4 Max 100MHz */
    /* PCLK2, EXCLK Max 60MHz */
    /* PCLK3 Max 50MHz */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL,
                    (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV4 |
                     CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2 | CLK_EXCLK_DIV4 |
                     CLK_HCLK_DIV1));

    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    /* Config Xtal and enable Xtal */
    stcXtalInit.u8Mode   = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv    = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8State  = CLK_XTAL_ON;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);

    (void)CLK_PLLStructInit(&stcPLLHInit);
    /* VCO = (8/1)*80 = 640MHz*/
    stcPLLHInit.u8PLLState = CLK_PLL_ON;
    stcPLLHInit.PLLCFGR = 0UL;
    stcPLLHInit.PLLCFGR_f.PLLM = 1UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLN = 80UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLP = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLQ = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLR = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcPLLHInit);

    /* 3 cycles for 150 ~ 200MHz */
    (void)EFM_SetWaitCycle(EFM_WAIT_CYCLE3);
    /* 3 cycles for 150 ~ 200MHz */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT3);
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);
}

/**
 * @brief  Initialize test data.
 * @param  None
 * @retval None
 */
static void InitTestData(void)
{
    uint32_t i;

    /* Clear count value */
    m_u32ByteTestErrorCnt = 0UL;
    m_u32HalfwordTestErrorCnt = 0UL;
    m_u32WordTestErrorCnt = 0UL;

    /* Initialize test data. */
    for (i = 0UL; i < SMC_DMA_BLOCK_SIZE; i++) {
        m_au8ReadData[i] = 0U;
        m_au8WriteData[i] = (uint8_t)rand();
        m_au16ReadData[i] = 0U;
        m_au16WriteData[i] = (uint16_t)rand();
        m_au32ReadData[i] = 0UL;
        m_au32WriteData[i] = (uint32_t)rand();
    }
}


/**
 * @brief  DMA transfer complete interrupt callback function.
 * @param  None
 * @retval None
 */
static void DMA_TransCompleteCallBack(void)
{
    m_u8DmaTcEnd = SET;
    DMA_ClearTransCompleteStatus(SMC_DMA_UNIT, SMC_DMA_INT_CH);
}

/**
 * @brief  DMA init configuration.
 * @param  None
 * @retval None
 */
static void DMA_InitConfig(void)
{
    stc_dma_init_t stcDmaInit;
    stc_irq_signin_config_t stcIrqSignConfig;

    /* SMC DMA configuration */
    FCG_Fcg0PeriphClockCmd(SMC_DMA_CLK, ENABLE);
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn         = DMA_INT_ENABLE;
    stcDmaInit.u32DataWidth     = DMA_DATAWIDTH_32BIT;
    stcDmaInit.u32BlockSize     = SMC_DMA_BLOCK_SIZE;
    stcDmaInit.u32TransCount    = SMC_DMA_TRANS_CNT;
    /* Set source & destination address mode */
    stcDmaInit.u32SrcAddrInc    = DMA_SRC_ADDR_INC;
    stcDmaInit.u32DestAddrInc   = DMA_DEST_ADDR_INC;
    stcDmaInit.u32DestAddr  = (uint32_t)(&m_au32ReadData[0]);
    stcDmaInit.u32SrcAddr   = (uint32_t)(&m_au32WriteData[0]);
    if (LL_OK != DMA_Init(SMC_DMA_UNIT, SMC_DMA_CH, &stcDmaInit)) {
        for (;;) {
        }
    }
    /* Enable AHB HPROT buffer and cache when DMA transfer for SMC */
    DMA_AHB_HProtBufCacheCmd(SMC_DMA_UNIT, SMC_DMA_CH, ENABLE);

    AOS_SetTriggerEventSrc(SMC_DMA_TRIG_CH, EVT_SRC_AOS_STRG);

    /* Interrupt configuration */
    stcIrqSignConfig.enIntSrc    = SMC_DMA_INT_SRC;
    stcIrqSignConfig.enIRQn      = SMC_DMA_IRQn;
    stcIrqSignConfig.pfnCallback = &DMA_TransCompleteCallBack;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);

    /* Enable DMA */
    DMA_Cmd(SMC_DMA_UNIT, ENABLE);
}

/**
 * @brief  DMA transfer data.
 * @param  [in] u32SrcAddr:               DMA source address
 * @param  [in] u32DestAddr:              DMA destination address
 * @param  [in] u32BlockSize:             DMA transfer block size
 * @retval None
 */
static void DMA_TransData(uint32_t u32SrcAddr, uint32_t u32DestAddr, uint32_t u32BlockSize)
{
    m_u8DmaTcEnd = RESET;

    DMA_SetSrcAddr(SMC_DMA_UNIT, SMC_DMA_CH, u32SrcAddr);
    DMA_SetDestAddr(SMC_DMA_UNIT, SMC_DMA_CH, u32DestAddr);
    DMA_SetBlockSize(SMC_DMA_UNIT, SMC_DMA_CH, u32BlockSize);
    DMA_SetTransCount(SMC_DMA_UNIT, SMC_DMA_CH, SMC_DMA_TRANS_CNT);
    (void)DMA_ChCmd(SMC_DMA_UNIT, SMC_DMA_CH, ENABLE);

    AOS_SW_Trigger();

    /* Wait transfer complete */
    while (RESET == m_u8DmaTcEnd) {
    };
}

/**
 * @brief  Access memory for byte with DMA.
 * @param  [in] u3AccessAddr:           Memory address
 * @param  [in] NumBytes:               Access size(unit: byte)
 * @retval count for reading and writing data error
 */
static uint32_t MEMORY_DMA_Access8(uint32_t u3AccessAddr, uint32_t NumBytes)
{
    uint32_t i;
    uint32_t j;
    uint32_t u32TestErrCnt = 0UL;
    uint32_t u32LedToggle = 0UL;
    uint32_t u32MemoryAddr = u3AccessAddr;

    /* Configure DMA data width */
    DMA_SetDataWidth(SMC_DMA_UNIT, SMC_DMA_CH, DMA_DATAWIDTH_8BIT);

    for (i = 0UL; i < NumBytes; i += SMC_DMA_BLOCK_SIZE) {
        /* Write */
        DMA_TransData((uint32_t)(&m_au8WriteData[0]), u32MemoryAddr, SMC_DMA_BLOCK_SIZE);
        /* Read */
        DMA_TransData(u32MemoryAddr, (uint32_t)(&m_au8ReadData[0]), SMC_DMA_BLOCK_SIZE);

        /* Verify data. */
        for (j = 0UL; j < SMC_DMA_BLOCK_SIZE; j++) {
            if (m_au8WriteData[j] != m_au8ReadData[j]) {
                u32TestErrCnt++;
                DDL_Printf("DMA Byte read/write error: address = 0x%.8x; write data = 0x%x; read data = 0x%x\r\n",
                           (unsigned int)(u32MemoryAddr + j * sizeof(m_au8ReadData[0])), (unsigned int)m_au8WriteData[j], (unsigned int)m_au8ReadData[j]);
            }
        }

        u32MemoryAddr += (SMC_DMA_BLOCK_SIZE * sizeof(m_au8ReadData[0]));
        (void)memset(m_au8ReadData, 0, (SMC_DMA_BLOCK_SIZE * sizeof(m_au8ReadData[0])));

        if (u32LedToggle++ == 100UL) {
            BSP_LED_Toggle(LED_BLUE);
            u32LedToggle = 0UL;
        }
    }
    return u32TestErrCnt;
}

/**
 * @brief  Access memory for half-word with DMA.
 * @param  [in] u3AccessAddr:           Memory address
 * @param  [in] NumHalfwords:           Access size(unit: half-word)
 * @retval count for reading and writing data error
 */
static uint32_t MEMORY_DMA_Access16(uint32_t u3AccessAddr, uint32_t NumHalfwords)
{
    uint32_t i;
    uint32_t j;
    uint32_t u32TestErrCnt = 0UL;
    uint32_t u32LedToggle = 0UL;
    uint32_t u32MemoryAddr = u3AccessAddr;

    /* Configure DMA data width */
    DMA_SetDataWidth(SMC_DMA_UNIT, SMC_DMA_CH, DMA_DATAWIDTH_16BIT);

    for (i = 0UL; i < NumHalfwords; i += SMC_DMA_BLOCK_SIZE) {
        /* Write */
        DMA_TransData((uint32_t)(&m_au16WriteData[0]), u32MemoryAddr, SMC_DMA_BLOCK_SIZE);
        /* Read */
        DMA_TransData(u32MemoryAddr, (uint32_t)(&m_au16ReadData[0]), SMC_DMA_BLOCK_SIZE);

        /* Verify data. */
        for (j = 0UL; j < SMC_DMA_BLOCK_SIZE; j++) {
            if (m_au16WriteData[j] != m_au16ReadData[j]) {
                u32TestErrCnt++;
                DDL_Printf("DMA Halfword read/write error: address = 0x%.8x; write data = 0x%x; read data = 0x%x\r\n",
                           (unsigned int)(u32MemoryAddr + j * sizeof(m_au16ReadData[0])), (unsigned int)m_au16WriteData[j], (unsigned int)m_au16ReadData[j]);
            }
        }

        u32MemoryAddr += (SMC_DMA_BLOCK_SIZE * sizeof(m_au16ReadData[0]));
        (void)memset(m_au16ReadData, 0, (SMC_DMA_BLOCK_SIZE * sizeof(m_au16ReadData[0])));

        if (u32LedToggle++ == 100UL) {
            BSP_LED_Toggle(LED_BLUE);
            u32LedToggle = 0UL;
        }
    }
    return u32TestErrCnt;
}

/**
 * @brief  Access memory for word with DMA.
 * @param  [in] u3AccessAddr:           Memory address
 * @param  [in] NumWords:               Access size(unit: word)
 * @retval count for reading and writing data error
 */
static uint32_t MEMORY_DMA_Access32(uint32_t u3AccessAddr, uint32_t NumWords)
{
    uint32_t i;
    uint32_t j;
    uint32_t u32TestErrCnt = 0UL;
    uint32_t u32LedToggle = 0UL;
    uint32_t u32MemoryAddr = u3AccessAddr;

    /* Configure DMA data width */
    DMA_SetDataWidth(SMC_DMA_UNIT, SMC_DMA_CH, DMA_DATAWIDTH_32BIT);

    for (i = 0UL; i < NumWords; i += SMC_DMA_BLOCK_SIZE) {
        /* Write */
        DMA_TransData((uint32_t)(&m_au32WriteData[0]), u32MemoryAddr, SMC_DMA_BLOCK_SIZE);
        /* Read */
        DMA_TransData(u32MemoryAddr, (uint32_t)(&m_au32ReadData[0]), SMC_DMA_BLOCK_SIZE);

        /* Verify data. */
        for (j = 0UL; j < SMC_DMA_BLOCK_SIZE; j++) {
            if (m_au32WriteData[j] != m_au32ReadData[j]) {
                u32TestErrCnt++;
                DDL_Printf("DMA Word read/write error: address = 0x%.8x; write data = 0x%.8x; read data = 0x%.8x\r\n",
                           (unsigned int)(u32MemoryAddr + j * sizeof(m_au32ReadData[0])), (unsigned int)m_au32WriteData[j], (unsigned int)m_au32ReadData[j]);
            }
        }

        u32MemoryAddr += (SMC_DMA_BLOCK_SIZE * sizeof(m_au32ReadData[0]));
        (void)memset(m_au32ReadData, 0, (SMC_DMA_BLOCK_SIZE * sizeof(m_au32ReadData[0])));

        if (u32LedToggle++ == 100UL) {
            BSP_LED_Toggle(LED_BLUE);
            u32LedToggle = 0UL;
        }
    }
    return u32TestErrCnt;
}

/**
 * @brief  Main function
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_SEL);

    /* Initialize system clock: */
    BSP_CLK_Init();

    /* Initialize UART print */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);

    /* Initialize LED */
    BSP_IO_Init();
    BSP_LED_Init();

    /* Configure SRAM. */
    (void)BSP_IS61LV6416_Init();
    BSP_IS61LV6416_GetMemInfo(&m_u32StartAddr, &m_u32ByteSize);

    /* Configure DMA */
    DMA_InitConfig();

    /* MCU Peripheral registers write protected */
    LL_PERIPH_WP(LL_PERIPH_SEL);

    DDL_Printf("Memory start address: 0x%.8X \r\n", (unsigned int)m_u32StartAddr);
    DDL_Printf("Memory end   address: 0x%.8X \r\n", (unsigned int)(m_u32StartAddr + m_u32ByteSize - 1UL));
    DDL_Printf("Memory size  (Bytes): 0x%.8X \r\n\r\n", (unsigned int)m_u32ByteSize);

    for (;;) {
        m_u32TestCnt++;
        DDL_Printf("********DMA Write/read test times: %u ********\r\n", (unsigned int)m_u32TestCnt);

        /* Initialize test data. */
        InitTestData();

        /****************** Access data width: Byte *****************/
        m_u32ByteTestErrorCnt = MEMORY_DMA_Access8(m_u32StartAddr, m_u32ByteSize);
        DDL_Printf("     DMA Byte read/write error data count: %u \r\n", (unsigned int)m_u32ByteTestErrorCnt);

        /****************** Access data width: Halfword *************/
        m_u32HalfwordTestErrorCnt = MEMORY_DMA_Access16(m_u32StartAddr, m_u32ByteSize / 2UL);
        DDL_Printf(" DMA Halfword read/write error data count: %u \r\n", (unsigned int)m_u32HalfwordTestErrorCnt);

        /****************** Access data width: Word *****************/
        m_u32WordTestErrorCnt = MEMORY_DMA_Access32(m_u32StartAddr, m_u32ByteSize / 4UL);
        DDL_Printf("     DMA Word read/write error data count: %u \r\n\r\n", (unsigned int)m_u32WordTestErrorCnt);

        /****************** Error check ******************/
        if ((m_u32ByteTestErrorCnt > 0UL) || (m_u32HalfwordTestErrorCnt > 0UL) || (m_u32WordTestErrorCnt > 0UL)) {
            BSP_LED_On(LED_RED);
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
