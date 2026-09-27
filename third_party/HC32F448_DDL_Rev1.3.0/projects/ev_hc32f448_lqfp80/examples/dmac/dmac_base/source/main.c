/**
 *******************************************************************************
 * @file  dmac/dmac_base/source/main.c
 * @brief This example demonstrates DMA basic transfer function.
@verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Optimize DMA2_Error_Handler()
   2025-11-03       CDT             Add DMA transform tigger by software
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
 * @addtogroup DMAC_Base
 * @{
 */
/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* DMAC */
#define DMA_UNIT            (CM_DMA2)
#define DMA_CH_SW           (DMA_CH1)
#define DMA_INT_SRC_SW      (INT_SRC_DMA2_TC1)
#define DMA_IRQn_SW         (INT001_IRQn)
#define DMA_CH_AOS          (DMA_CH4)
#define DMA_INT_SRC_AOS     (INT_SRC_DMA2_TC4)
#define DMA_IRQn_AOS        (INT000_IRQn)
#define DMA_TRIG_CH         (AOS_DMA2_4)
#define DMA_TC              (4UL)
#define DMA_BC              (5UL)
#define DMA_DW              (DMA_DATAWIDTH_32BIT)
#define DMA_ERR_IRQn        (DMA2_ERR_IRQn)
#define DMA_ERR_FLAG        (DMA_FLAG_TRANS_ERR_CH4)
#define DMA_TRANS_TIME      (0x1000U)
#define DMA_CH_MAX          (DMA_CH5)
#define SW_TRIG_OK          (0x01UL)
#define AOS_TRIG_OK         (0x02UL)
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
__IO static en_flag_status_t m_u8DmaCH1TcEnd = RESET;
__IO static en_flag_status_t m_u8DmaCH4TcEnd = RESET;

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static uint32_t u32Result = 0;
static const uint32_t u32SrcBuf[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                       11, 12, 13, 14, 15, 16, 17, 18,
                                       19, 20
                                      };
static uint32_t u32Dest1Buf[20] = {0};
static uint32_t u32Dest2Buf[20] = {0};
static uint32_t u32ExpectDestBufData[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                            11, 12, 13, 14, 15, 16, 17, 18,
                                            19, 20
                                           };

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  DMA basic function init
 * @param  None
 * @retval None
 */
static void DmaInit(void)
{
    stc_dma_init_t stcDmaInit;

    AOS_SetTriggerEventSrc(DMA_TRIG_CH, EVT_SRC_AOS_STRG);

    (void)DMA_StructInit(&stcDmaInit);

    stcDmaInit.u32IntEn      = DMA_INT_ENABLE;
    stcDmaInit.u32BlockSize  = DMA_BC;
    stcDmaInit.u32TransCount = DMA_TC;
    stcDmaInit.u32DataWidth  = DMA_DW;
    stcDmaInit.u32DestAddr   = (uint32_t)(&u32Dest1Buf[0]);
    stcDmaInit.u32SrcAddr    = (uint32_t)(&u32SrcBuf[0]);
    stcDmaInit.u32SrcAddrInc     = DMA_SRC_ADDR_INC;
    stcDmaInit.u32DestAddrInc    = DMA_DEST_ADDR_INC;
    (void)DMA_Init(DMA_UNIT, DMA_CH_SW, &stcDmaInit);

    stcDmaInit.u32DestAddr = (uint32_t)(&u32Dest2Buf[0]);
    (void)DMA_Init(DMA_UNIT, DMA_CH_AOS, &stcDmaInit);
}

/**
 * @brief  DMA ch.1 transfer complete IRQ callback
 * @param  None
 * @retval None
 */
static void DMA_CH1_TranEnd_IrqCallback(void)
{
    m_u8DmaCH1TcEnd = SET;
    DMA_ClearTransCompleteStatus(DMA_UNIT, DMA_FLAG_TC_CH1);
}

/**
 * @brief  DMA ch.4 transfer complete IRQ callback
 * @param  None
 * @retval None
 */
static void DMA2_CH4_TransEnd_IrqCallback(void)
{
    m_u8DmaCH4TcEnd = SET;
    DMA_ClearTransCompleteStatus(DMA_UNIT, DMA_FLAG_TC_CH4);
}

/**
 * @brief  DMA basic function interrupt init
 * @param  None
 * @retval None
 */
static void DmaIntInit(void)
{
    stc_irq_signin_config_t stcIrqSignConfig;

    /* set interrupt for software trigger DMA */
    stcIrqSignConfig.enIntSrc   = DMA_INT_SRC_SW;
    stcIrqSignConfig.enIRQn     = DMA_IRQn_SW;
    stcIrqSignConfig.pfnCallback = &DMA_CH1_TranEnd_IrqCallback;

    (void)INTC_IrqSignIn(&stcIrqSignConfig);
    DMA_ClearTransCompleteStatus(DMA_UNIT, DMA_FLAG_TC_CH1);

    /* NVIC setting */
    NVIC_ClearPendingIRQ(DMA_IRQn_SW);
    NVIC_SetPriority(DMA_IRQn_SW, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(DMA_IRQn_SW);

    stcIrqSignConfig.enIntSrc   = DMA_INT_SRC_AOS;
    stcIrqSignConfig.enIRQn     = DMA_IRQn_AOS;
    stcIrqSignConfig.pfnCallback = &DMA2_CH4_TransEnd_IrqCallback;

    (void)INTC_IrqSignIn(&stcIrqSignConfig);
    DMA_ClearTransCompleteStatus(DMA_UNIT, DMA_FLAG_TC_CH4);

    /* NVIC setting */
    NVIC_ClearPendingIRQ(DMA_IRQn_AOS);
    NVIC_SetPriority(DMA_IRQn_AOS, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(DMA_IRQn_AOS);

    /* Enable DMA error interrupt */
    NVIC_ClearPendingIRQ(DMA_ERR_IRQn);
    NVIC_SetPriority(DMA_ERR_IRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(DMA_ERR_IRQn);
}

/**
 * @brief  DMA2 error handle
 * @param  None
 * @retval None
 */
void DMA2_Error_Handler(void)
{
    uint8_t u8Pos = 0U;
    uint32_t u32TimeOut = 0UL;
    uint32_t u32ErrCH;

    u32ErrCH = READ_REG32_BIT(DMA_UNIT->INTSTAT0, DMA_INTSTAT0_TRNERR);
    if (0x0UL != u32ErrCH) {
        DMA_ClearErrStatus(DMA_UNIT, u32ErrCH);
        while (0x0UL != READ_REG32_BIT(DMA_UNIT->CHSTAT, DMA_CHSTAT_CHACT)) {
            if ((0x1UL << u8Pos) == u32ErrCH) {
                u8Pos++;
                if (u8Pos > DMA_CH_MAX) {
                    break;
                }
            }
            while (SET == DMA_GetTransStatus(DMA_UNIT, DMA_STAT_TRANS_CH0 << u8Pos)) {
                u32TimeOut++;
                if (u32TimeOut > DMA_TRANS_TIME) {
                    /* error handling */
                    NVIC_SystemReset();
                }
            }
            u8Pos++;
            u32TimeOut = 0UL;
            if (u8Pos > DMA_CH_MAX) {
                break;
            }
        }
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  Main function of DMAC project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_EFM);
    /* System clock init */
    BSP_CLK_Init();
    /* Expand IO init */
    BSP_IO_Init();
    /* LED init */
    BSP_LED_Init();
    /* DMA/AOS FCG enable */
    FCG_Fcg0PeriphClockCmd((FCG0_PERIPH_DMA2 | FCG0_PERIPH_AOS), ENABLE);

    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_EFM);

    /* Config DMA */
    DmaInit();
    /* DMA interrupt config */
    DmaIntInit();

    /* DMA module enable */
    DMA_Cmd(DMA_UNIT, ENABLE);

    /* DMA channel enable */
    (void)DMA_ChCmd(DMA_UNIT, DMA_CH_SW, ENABLE);

    /* 1st DMA SW */
    DMA_MxChSWTrigger(DMA_UNIT, DMA_MX_CH1);

    while (RESET == m_u8DmaCH1TcEnd) {
        if (SET == DMA_GetTransCompleteStatus(DMA_UNIT, DMA_FLAG_BTC_CH1)) {
            DMA_ClearTransCompleteStatus(DMA_UNIT, DMA_FLAG_BTC_CH1);
            DMA_MxChSWTrigger(DMA_UNIT, DMA_MX_CH1);
        }
    }
    if (0 == memcmp(u32Dest1Buf, u32ExpectDestBufData, sizeof(u32Dest1Buf))) {
        u32Result |= SW_TRIG_OK;
    }

    /* DMA aos trigger channel enable */
    (void)DMA_ChCmd(DMA_UNIT, DMA_CH_AOS, ENABLE);

    /* 1st AOS SW */
    AOS_SW_Trigger();
    while (RESET == m_u8DmaCH4TcEnd) {
        if (SET == DMA_GetTransCompleteStatus(DMA_UNIT, DMA_FLAG_BTC_CH4)) {
            DMA_ClearTransCompleteStatus(DMA_UNIT, DMA_FLAG_BTC_CH4);
            AOS_SW_Trigger();
        }
    }

    if (0 == memcmp(u32Dest2Buf, u32ExpectDestBufData, sizeof(u32Dest2Buf))) {
        u32Result |= AOS_TRIG_OK;
    }

    if ((SW_TRIG_OK | AOS_TRIG_OK) == u32Result) {
        /* LED_BLUE On, as excepted */
        BSP_LED_On(LED_BLUE);
    } else {
        /* LED_RED On, something wrong */
        BSP_LED_On(LED_RED);
    }

    for (;;) {
        ;
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
