/**
 *******************************************************************************
 * @file  usart/usart_lin/source/lin.c
 * @brief This midware file provides firmware functions to manage the LIN.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
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
#include "hc32_ll_fcg.h"
#include "hc32_ll_gpio.h"
#include "hc32_ll_interrupts.h"
#include "hc32_ll_utility.h"
#include "lin.h"

/**
 * @addtogroup USART_LIN
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup LIN_Local_Macros LIN Local Macros
 * @{
 */

/* USART LIN channel */
#define IS_VALID_USART_LIN(x)                                                  \
(   (CM_USART3 == (x)) ||                                                      \
    (CM_USART6 == (x)))

/**
 * @defgroup USART_LIN_Channel_Index USART LIN Channel Index
 * @{
 */
#define USART_LIN_CH3                           (0UL)
#define USART_LIN_CH6                           (1UL)
/**
 * @}
 */

/* BIT operation */
#define LIN_BIT(x, pos)                         (((x) >> (pos)) & 0x01U)

/* LIN wakeup/break/sync data definition */
#define LIN_WAKEUP_DATA                         (0x80U)
#define LIN_BREAK_DATA                          (0x00U)
#define LIN_SYNC_DATA                           (0x55U)

/**
 * @defgroup LIN_Frame_PID_Length_Mask LIN frame PID length mask
 * @{
 */
#define LIN_FRAME_PID_LEN_MASK                  (0x30U)
/**
 * @}
 */

/**
 * @defgroup LIN_Delay LIN Delay
 * @{
 */
#define LIN_INTER_FRAME_SPACE                   (10UL) /* Pause (Frame->Frame)   (ca. 10ms) */
#define LIN_FRAME_RESPONSE_SPACE                (1UL)  /* Pause (Header->Data)   (ca.  1ms) */
#define LIN_FRAME_INTER_BYTE_SPACE              (1UL)  /* Pause (Data->Data)     (ca.  1ms) */
/**
 * @}
 */

/**
 * @defgroup USART_LIN_Interrupt_Source_Number USART LIN Interrupt Source Number
 * @{
 */
#define LIN_RX_INT_SRC(Idx)                                                    \
(   (USART_LIN_CH6 == (Idx)) ? INT_SRC_USART6_RI : INT_SRC_USART3_RI)

#define LIN_RXERR_INT_SRC(Idx)                                                 \
(   (USART_LIN_CH6 == (Idx)) ? INT_SRC_USART6_EI : INT_SRC_USART3_EI)

#define LIN_BRKWKP_INT_SRC(Idx)                                                \
(   (USART_LIN_CH6 == (Idx)) ? INT_SRC_USART6_BRKWKPI : INT_SRC_USART3_BRKWKPI)

#define LIN_TX_INT_SRC(Idx)                                                    \
(   (USART_LIN_CH6 == (Idx)) ? INT_SRC_USART6_TI : INT_SRC_USART3_TI)

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
/**
 * @addtogroup LIN_Local_Functions
 * @{
 */
static void LIN_SendBreak(const stc_lin_handle_t *pstcLinHandle);
static void LIN_SendChar(const stc_lin_handle_t *pstcLinHandle, uint8_t u8Char);
static int32_t LIN_MASTER_SendFrameHeader(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame);
static uint8_t LIN_CalculateChecksum(uint8_t u8PID, const uint8_t au8Data[], uint8_t u8Len);
static uint8_t LIN_CalculatePIDParity(uint8_t u8ID);
static uint8_t LIN_GetFrameDataLenbyFrameID(uint8_t u8ID);
static void USART3_RxFull_IrqCallback(void);
static void USART3_RxError_IrqCallback(void);
static void USART3_LinWakeupBreak_IrqCallback(void);
static void USART6_RxError_IrqCallback(void);
static void USART6_RxFull_IrqCallback(void);
static void USART6_LinWakeupBreak_IrqCallback(void);
static void INTC_InstalIrqHandler(const stc_irq_signin_config_t *pstcConfig, uint32_t u32Priority);

/**
 * @}
 */

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/**
 * @defgroup LIN_Local_Variables LIN Local Variables
 * @{
 */
static stc_lin_handle_t *m_apstcLinHandle[2] = {NULL, NULL};

static func_ptr_t m_apfnLinRxFullIrqCb[2] = {&USART3_RxFull_IrqCallback, &USART6_RxFull_IrqCallback};
static func_ptr_t m_apfnLinRxErrorIrqCb[2] = {&USART3_RxError_IrqCallback, &USART6_RxError_IrqCallback};
static func_ptr_t m_apfnLinBreakWakeupIrqCb[2] = {&USART3_LinWakeupBreak_IrqCallback, &USART6_LinWakeupBreak_IrqCallback};
/**
 * @}
 */

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @defgroup LIN_Global_Functions LIN Global Functions
 * @{
 */

/**
 * @brief  Initialize LIN function.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @retval int32_t:
 *           - LL_OK:                   Initialize successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle value is NULL.
 */
int32_t LIN_Init(stc_lin_handle_t *pstcLinHandle)
{
    uint32_t u32UsartFcg;
    uint32_t u32UsartLinIndex;
    stc_irq_signin_config_t stcIrqSigninConfig;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcLinHandle) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        if (CM_USART3 == pstcLinHandle->USARTx) {
            u32UsartFcg = FCG3_PERIPH_USART3;
            u32UsartLinIndex = USART_LIN_CH3;
        } else {
            u32UsartFcg = FCG3_PERIPH_USART6;
            u32UsartLinIndex = USART_LIN_CH6;
        }
        m_apstcLinHandle[u32UsartLinIndex] = pstcLinHandle;

        /* Configure USART RX/TX pin. */
        GPIO_SetFunc(pstcLinHandle->stcPinConfig.u8RxPort, pstcLinHandle->stcPinConfig.u16RxPin, \
                     pstcLinHandle->stcPinConfig.u16RxPinFunc);
        GPIO_SetFunc(pstcLinHandle->stcPinConfig.u8TxPort, pstcLinHandle->stcPinConfig.u16TxPin, \
                     pstcLinHandle->stcPinConfig.u16TxPinFunc);

        /* Enable peripheral clock */
        FCG_Fcg3PeriphClockCmd(u32UsartFcg, ENABLE);

        /* Initialize LIN */
        (void)USART_LIN_Init(pstcLinHandle->USARTx, &pstcLinHandle->stcLinInit, NULL);

        /* Register USART RX IRQ handler && configure NVIC. */
        stcIrqSigninConfig.enIntSrc = LIN_RX_INT_SRC(u32UsartLinIndex);
        stcIrqSigninConfig.pfnCallback = m_apfnLinRxFullIrqCb[u32UsartLinIndex];
        stcIrqSigninConfig.enIRQn = pstcLinHandle->stcIrqConfig.RxFullIRQn;
        INTC_InstalIrqHandler(&stcIrqSigninConfig, DDL_IRQ_PRIO_DEFAULT);

        /* Register USART error IRQ handler && configure NVIC. */
        stcIrqSigninConfig.enIntSrc = LIN_RXERR_INT_SRC(u32UsartLinIndex);
        stcIrqSigninConfig.pfnCallback = m_apfnLinRxErrorIrqCb[u32UsartLinIndex];
        stcIrqSigninConfig.enIRQn = pstcLinHandle->stcIrqConfig.RxErrorIRQn;
        INTC_InstalIrqHandler(&stcIrqSigninConfig, DDL_IRQ_PRIO_DEFAULT);

        USART_FuncCmd(pstcLinHandle->USARTx, USART_TX, ENABLE);

        if (LIN_SLAVE == pstcLinHandle->u32LinMode) {
            /* Register USART LIN break detection interrupt IRQ handler && configure NVIC. */
            stcIrqSigninConfig.pfnCallback = m_apfnLinBreakWakeupIrqCb[u32UsartLinIndex];
            stcIrqSigninConfig.enIntSrc = LIN_BRKWKP_INT_SRC(u32UsartLinIndex);
            stcIrqSigninConfig.enIRQn = pstcLinHandle->stcIrqConfig.BreakWkupIRQn;
            INTC_InstalIrqHandler(&stcIrqSigninConfig, DDL_IRQ_PRIO_DEFAULT);

            USART_FuncCmd(pstcLinHandle->USARTx, USART_LIN_INT_BREAK, ENABLE);

            if (LIN_STATE_SLEEP == pstcLinHandle->enLinState) {
                (void)LIN_Sleep(pstcLinHandle);
            }
        }

        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Set LIN state.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @param  [in] enState                 LIN state
 * @retval int32_t:
 *           - LL_OK:                   Set successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle value is NULL.
 */
int32_t LIN_SetState(stc_lin_handle_t *pstcLinHandle, en_lin_state_t enState)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (pstcLinHandle != NULL) {
        pstcLinHandle->enLinState = enState;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Get LIN state.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @retval LIN state
 */
en_lin_state_t LIN_GetState(const stc_lin_handle_t *pstcLinHandle)
{
    return pstcLinHandle->enLinState;
}

/**
 * @brief  LIN sleep.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @retval int32_t:
 *           - LL_OK:                   Set successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle value is NULL.
 */
int32_t LIN_Sleep(stc_lin_handle_t *pstcLinHandle)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (pstcLinHandle != NULL) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        USART_FuncCmd(pstcLinHandle->USARTx, USART_RX, ENABLE);
        USART_FuncCmd(pstcLinHandle->USARTx, (USART_LIN_WKUP | USART_LIN_INT_WKUP), ENABLE);
        pstcLinHandle->enLinState = LIN_STATE_SLEEP;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  LIN send wakeup signal.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @retval int32_t:
 *           - LL_OK:                   Send successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle value is NULL.
 */
int32_t LIN_SendWakeupSignal(const stc_lin_handle_t *pstcLinHandle)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcLinHandle) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        while (RESET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_TX_EMPTY)) {
        }

        USART_WriteData(pstcLinHandle->USARTx, LIN_WAKEUP_DATA);
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  LIN master send frame.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @param  [in] pstcFrame               Pointer to a @ref stc_lin_frame_t structure
 * @retval int32_t:
 *           - LL_OK:                   Send successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle or pstcFrame value is NULL.
 */
int32_t LIN_MASTER_SendFrame(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame)
{
    int32_t i32Ret = LL_OK;

    if ((NULL != pstcFrame) && (NULL != pstcLinHandle)) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        i32Ret = LIN_MASTER_SendFrameHeader(pstcLinHandle, pstcFrame);
        if (LL_OK == i32Ret) {
            USART_FuncCmd(pstcLinHandle->USARTx, (USART_RX | USART_INT_RX), DISABLE);

            /* Send data */
            pstcFrame->u8XferCount = 0U;
            while (pstcFrame->u8XferCount < pstcFrame->u8Len) {
                LIN_SendChar(pstcLinHandle, pstcFrame->au8Data[pstcFrame->u8XferCount]);
                pstcFrame->u8XferCount++;
            }

            /* Calculate Checksum */
            pstcFrame->u8Checksum = LIN_CalculateChecksum(pstcFrame->u8PID, pstcFrame->au8Data, pstcFrame->u8Len);

            LIN_SendChar(pstcLinHandle, pstcFrame->u8Checksum);
            while (RESET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_TX_CPLT)) {
            }

            USART_FuncCmd(pstcLinHandle->USARTx, (USART_RX | USART_INT_RX), ENABLE);
        }
    }

    return i32Ret;
}

/**
 * @brief  LIN master receive frame(blocking mode).
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @param  [in] pstcFrame               Pointer to a @ref stc_lin_frame_t structure
 * @param  [in] i32Timeout              Timeout duration
 * @retval int32_t:
 *           - LL_OK:                   Receive successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle or pstcFrame value is NULL.
 */
int32_t LIN_MASTER_ReceiveFrame(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame, int32_t i32Timeout)
{
    uint8_t i;
    uint8_t u8Checksum;
    __IO uint32_t u32Cycle;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((NULL != pstcFrame) && (NULL != pstcLinHandle)) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        /* Clear frame data */
        pstcFrame->u8XferCount = 0U;
        pstcFrame->u8Checksum = 0U;
        pstcFrame->enError = LIN_OK;
        for (i = 0U; i < 8U; i++) {
            pstcFrame->au8Data[i] = 0U;
        }

        i32Ret = LIN_MASTER_SendFrameHeader(pstcLinHandle, pstcFrame);
        if (LL_OK == i32Ret) {
            u32Cycle = (i32Timeout < 0) ? 0UL : (uint32_t)i32Timeout * (HCLK_VALUE / 10000UL);  /* i32Timeout * 1ms */

            /* Wait checksum */
            while ((u32Cycle > 0UL) || (i32Timeout < 0)) {
                u32Cycle--;
                if (LIN_FRAME_STATE_CHKSUM == pstcFrame->enState) {
                    break;
                }
            }

            if ((0UL == u32Cycle) && (i32Timeout >= 0)) {
                i32Ret = LL_ERR_TIMEOUT;
            } else {
                u8Checksum = LIN_CalculateChecksum(pstcFrame->u8PID, pstcFrame->au8Data, pstcFrame->u8Len);
                if (u8Checksum != pstcFrame->u8Checksum) {
                    i32Ret = LL_ERR;
                }
                DDL_DelayMS(LIN_INTER_FRAME_SPACE);
            }
        }
    }

    return i32Ret;
}

/**
 * @brief  LIN slave receive frame response data(blocking mode).
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @param  [in] pstcFrame               Pointer to a @ref stc_lin_frame_t structure
 * @param  [in] i32Timeout              Timeout duration
 * @retval int32_t:
 *           - LL_OK:                   Receive successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle or pstcFrame value is NULL.
 */
int32_t LIN_SLAVE_ReceiveFrame(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame, int32_t i32Timeout)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((NULL != pstcLinHandle) && (NULL != pstcFrame)) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        /* Start LIN slave receive frame header function(blocking mode). */
        i32Ret = LIN_SLAVE_ReceiveFrameHeader(pstcLinHandle, pstcFrame, i32Timeout);
        if (LL_OK == i32Ret) {
            /* Start LIN slave receive frame data function. */
            i32Ret = LIN_SLAVE_ReceiveFrameResponse(pstcLinHandle, i32Timeout);
        }
    }

    return i32Ret;
}

/**
 * @brief  LIN slave send frame(blocking mode).
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @param  [in] pstcFrame               Pointer to a @ref stc_lin_frame_t structure
 * @param  [in] i32Timeout              Timeout duration
 * @retval int32_t:
 *           - LL_OK:                   Send successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle or pstcFrame value is NULL.
 */
int32_t LIN_SLAVE_SendFrame(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame, int32_t i32Timeout)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((NULL != pstcLinHandle) && (NULL != pstcFrame)) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        /* Start LIN slave receive frame header function(blocking mode). */
        i32Ret = LIN_SLAVE_ReceiveFrameHeader(pstcLinHandle, pstcFrame, i32Timeout);
        if (LL_OK == i32Ret) {
            /* LIN slave send frame response. */
            i32Ret = LIN_SLAVE_SendFrameResponse(pstcLinHandle);
        }
    }

    return i32Ret;
}

/**
 * @brief  LIN slave receive frame header(blocking mode).
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @param  [in] pstcFrame               Pointer to a @ref stc_lin_frame_t structure
 * @param  [in] i32Timeout              Timeout duration
 * @retval int32_t:
 *           - LL_OK:                   Receive successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle or pstcFrame value is NULL.
 */
int32_t LIN_SLAVE_ReceiveFrameHeader(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame,
                                     int32_t i32Timeout)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    __IO uint32_t u32Cyc = (i32Timeout < 0) ? 0UL : ((uint32_t)i32Timeout) * (HCLK_VALUE / 10000UL);  /* i32Timeout * 1ms */

    if ((NULL != pstcFrame) && (NULL != pstcLinHandle)) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        pstcFrame->u8PID = 0U;
        pstcFrame->enError = LIN_OK;
        pstcFrame->u8Len = 0U;
        pstcFrame->u8XferCount = 0U;
        pstcFrame->u8Checksum = 0U;
        pstcFrame->enState = LIN_FRAME_STATE_IDLE;

        pstcLinHandle->pstcFrame = pstcFrame;

        /* Data state */
        USART_FuncCmd(pstcLinHandle->USARTx, USART_RX, ENABLE);
        USART_FuncCmd(pstcLinHandle->USARTx, USART_LIN_INT_BREAK, ENABLE);

        while ((u32Cyc > 0UL) || (i32Timeout < 0)) {
            u32Cyc--;
            if (pstcFrame->enState >= LIN_FRAME_STATE_PID) {
                break;
            }
        }

        if ((0UL == u32Cyc) && (i32Timeout >= 0)) {
            i32Ret = LL_ERR_TIMEOUT;
        } else {
            i32Ret = LL_OK;
        }
    }

    return i32Ret;
}

/**
 * @brief  LIN slave receive frame response data(blocking mode).
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @param  [in] i32Timeout              Timeout duration
 * @retval int32_t:
 *           - LL_OK:                   Receive successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle value is NULL.
 * @note   Firstly call LIN_SLAVE_ReceiveFrameHeader(), and then call this function.
 */
int32_t LIN_SLAVE_ReceiveFrameResponse(const stc_lin_handle_t *pstcLinHandle, int32_t i32Timeout)
{
    uint8_t u8Checksum;
    stc_lin_frame_t *pstcFrame;
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    __IO uint32_t u32Cyc = (i32Timeout < 0) ? 0UL : ((uint32_t)i32Timeout) * (HCLK_VALUE / 10000UL);  /* i32Timeout * 1ms */

    if (NULL != pstcLinHandle) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));
        DDL_ASSERT(NULL != pstcLinHandle->pstcFrame);
        DDL_ASSERT(LIN_FRAME_STATE_PID == pstcLinHandle->pstcFrame->enState);

        pstcFrame = pstcLinHandle->pstcFrame;
        while ((u32Cyc > 0UL) || (i32Timeout < 0)) {
            u32Cyc--;

            if (LIN_FRAME_STATE_CHKSUM == pstcFrame->enState) {
                u8Checksum = LIN_CalculateChecksum(pstcFrame->u8PID, pstcFrame->au8Data, pstcFrame->u8Len);
                if (u8Checksum == pstcFrame->u8Checksum) {
                    i32Ret = LL_OK;
                    pstcFrame->enState = LIN_FRAME_STATE_IDLE;
                } else {
                    i32Ret = LL_ERR;
                    pstcFrame->enError = LIN_ERR_CHKSUM;
                }
                break;
            }
        }

        if ((0UL == u32Cyc) && (i32Ret != LL_OK) && (i32Timeout >= 0)) {
            i32Ret = LL_ERR_TIMEOUT;
        } else {
            i32Ret = LL_OK;
        }
    }

    return i32Ret;
}

/**
 * @brief  LIN slave send frame response data.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @retval int32_t:
 *           - LL_OK:                   Send successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle value is NULL.
 * @note   Firstly call LIN_SLAVE_ReceiveFrameHeader(), and then call this function.
 */
int32_t LIN_SLAVE_SendFrameResponse(stc_lin_handle_t *pstcLinHandle)
{
    uint8_t u8Checksum;
    stc_lin_frame_t *pstcFrame;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((NULL != pstcLinHandle) && (NULL != pstcLinHandle->pstcFrame)) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));
        DDL_ASSERT(LIN_FRAME_STATE_PID == pstcLinHandle->pstcFrame->enState);

        pstcFrame = pstcLinHandle->pstcFrame;
        DDL_DelayMS(LIN_FRAME_RESPONSE_SPACE);

        USART_FuncCmd(pstcLinHandle->USARTx, (USART_RX | USART_INT_RX), DISABLE);

        /* Send data */
        pstcFrame->u8XferCount = 0U;
        while (pstcFrame->u8XferCount < pstcFrame->u8Len) {
            LIN_SendChar(pstcLinHandle, pstcFrame->au8Data[pstcFrame->u8XferCount]);
            pstcFrame->u8XferCount++;
        }
        pstcLinHandle->pstcFrame->enState = LIN_FRAME_STATE_DATA;

        /* Calculate Checksum */
        u8Checksum = LIN_CalculateChecksum(pstcFrame->u8ID, pstcFrame->au8Data, pstcFrame->u8Len);

        LIN_SendChar(pstcLinHandle, u8Checksum);
        while (RESET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_TX_CPLT)) {
        }

        USART_FuncCmd(pstcLinHandle->USARTx, (USART_RX | USART_INT_RX), ENABLE);
        pstcLinHandle->pstcFrame->enState = LIN_FRAME_STATE_CHKSUM;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @}
 */

/**
 * @defgroup LIN_Local_Functions LIN Local Functions
 * @{
 */

/**
 * @brief  Master send frame header field.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @param  [in] pstcFrame               Pointer to a @ref stc_lin_frame_t structure
 * @retval int32_t:
 *           - LL_OK:                   Send successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcLinHandle or pstcFrame value is NULL.
 */
static int32_t LIN_MASTER_SendFrameHeader(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if ((pstcFrame != NULL) && (pstcLinHandle != NULL)) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));
        DDL_ASSERT((pstcFrame->u8ID != 0xFFU) && (pstcFrame->u8ID != 0x00U));

        pstcLinHandle->pstcFrame = pstcFrame;

        /* Idle state */
        if (SET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN)) {
            i32Ret = LL_ERR;
        } else {
            USART_FuncCmd(pstcLinHandle->USARTx, (USART_RX | USART_INT_RX), DISABLE);

            /* Send break field */
            LIN_SendBreak(pstcLinHandle);
            pstcFrame->enState = LIN_FRAME_STATE_BREAK;

            /* Send sync field */
            LIN_SendChar(pstcLinHandle, LIN_SYNC_DATA);
            pstcFrame->enState = LIN_FRAME_STATE_SYNC;
            while (RESET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_TX_CPLT)) {
            }
            DDL_DelayMS(LIN_FRAME_INTER_BYTE_SPACE);

            /* Calculate PID */
            pstcFrame->u8PID = LIN_CalculatePIDParity(pstcFrame->u8ID);

            /* Send PID */
            LIN_SendChar(pstcLinHandle, pstcFrame->u8PID);
            while (RESET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_TX_CPLT)) {
            }

            USART_FuncCmd(pstcLinHandle->USARTx, (USART_RX | USART_INT_RX), ENABLE);

            pstcFrame->enState = LIN_FRAME_STATE_PID;
            pstcFrame->u8Len = LIN_GetFrameDataLenbyFrameID(pstcFrame->u8ID);
            i32Ret = LL_OK;
        }
    }

    return i32Ret;
}

/**
 * @brief  Master send break field.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @retval None
 */
static void LIN_SendBreak(const stc_lin_handle_t *pstcLinHandle)
{
    DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

    while (RESET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_TX_EMPTY)) {
    }

    USART_LIN_RequestBreakSending(pstcLinHandle->USARTx);

    if (USART_LIN_SEND_BREAK_MD_TDR == USART_LIN_GetBreakMode(pstcLinHandle->USARTx)) {
        USART_WriteData(pstcLinHandle->USARTx, LIN_BREAK_DATA);
    }

    while (SET == USART_LIN_GetRequestBreakStatus(pstcLinHandle->USARTx)) {
    }
}

/**
 * @brief  Lin send character.
 * @param  [in] pstcLinHandle           Pointer to a @ref stc_lin_handle_t structure
 * @param  [in] u8Char                  Character
 * @retval None
 */
static void LIN_SendChar(const stc_lin_handle_t *pstcLinHandle, uint8_t u8Char)
{
    DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

    while (RESET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_TX_EMPTY)) {
    }

    USART_WriteData(pstcLinHandle->USARTx, (uint16_t)u8Char);
}

/**
 * @brief  Calculate PID && data field checksum.
 * @param  [in] u8PID                   PID number
 * @param  [in] au8Data                 Data buffer
 * @param  [in] u8Len                   Data length
 * @retval Checksum
 */
static uint8_t LIN_CalculateChecksum(uint8_t u8PID, const uint8_t au8Data[], uint8_t u8Len)
{
    uint8_t i;
    uint8_t u8ID = u8PID & 0x3FU;
    uint16_t u16Checksum;

    /* 0x3C 0x3D Classic Checksum */
    if ((u8ID == 0x3CU) || (u8ID == 0x3DU)) {
        u16Checksum = 0U;
    } else {
        u16Checksum  = (uint16_t)u8PID; /* Enhanced Checksum */
    }

    for (i = 0U; i < u8Len; i++) {
        u16Checksum += au8Data[i];

        /* Carry bit */
        if (u16Checksum > 0xFFU) {
            u16Checksum -= 0xFFU;
        }
    }

    return (uint8_t)((uint16_t)(~u16Checksum)); /* reversed */
}

/**
 * @brief  Calculate Protected ID parity.
 * @param  [in] u8ID                    ID number
 * @retval Protected ID
 */
static uint8_t LIN_CalculatePIDParity(uint8_t u8ID)
{
    uint8_t u8P0;
    uint8_t u8P1;
    uint8_t u8Parity;

    u8Parity = u8ID;
    u8P0 = (LIN_BIT(u8Parity, 0U) ^ LIN_BIT(u8Parity, 1U) ^ LIN_BIT(u8Parity, 2U) ^ LIN_BIT(u8Parity, 4U)) << 6U;
    u8P1 = (((LIN_BIT(u8Parity, 1U) ^ LIN_BIT(u8Parity, 3U) ^ LIN_BIT(u8Parity, 4U) ^ LIN_BIT(u8Parity, 5U)) > 0U) ? 0U : 1U);
    u8P1 = u8P1 << 7U;
    u8Parity |= (u8P0 | u8P1);

    return u8Parity;
}

/**
 * @brief  Get LIN frame data length by frame ID.
 * @param  [in] u8ID                    Frame PID
 * @retval Frame data length
 */
static uint8_t LIN_GetFrameDataLenbyFrameID(uint8_t u8ID)
{
    uint8_t u8DataLen;

    /*------------------------------------+
    | ID5 | ID4 | Frame Data Filed Length |
    |-------------------------------------|
    |  0  |  0  |            2            |
    |-----|-------------------------------|
    |  0  |  1  |            2            |
    |-----|-------------------------------|
    |  1  |  0  |            4            |
    |-----|-------------------------------|
    |  1  |  1  |            8            |
    +-------------------------------------*/

    switch (u8ID & LIN_FRAME_PID_LEN_MASK) {
        case LIN_FRAME_DATA_4BYTE:
            u8DataLen = 4U;
            break;
        case LIN_FRAME_DATA_8BYTE:
            u8DataLen = 8U;
            break;
        default:
            u8DataLen = 2U;
            break;
    }

    return u8DataLen;
}

/**
 * @brief  USART RX IRQ callback
 * @param  [in] pstcLinHandle           Pointer to struct @ref stc_lin_handle_t
 * @retval None
 */
static void USART_RxFull_IrqCallback(stc_lin_handle_t *pstcLinHandle)
{
    uint8_t u8Data;
    uint32_t u32Baudrate;

    if (NULL != pstcLinHandle) {
        DDL_ASSERT(NULL != pstcLinHandle->pstcFrame);
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        u8Data = (uint8_t)USART_ReadData(pstcLinHandle->USARTx);

        switch (pstcLinHandle->pstcFrame->enState) {
            case LIN_FRAME_STATE_BREAK:
                u32Baudrate = USART_LIN_GetMeasureBaudrate(pstcLinHandle->USARTx);
                pstcLinHandle->stcSlaveConfig.u32AdjustBaudrate = u32Baudrate;
                USART_FuncCmd(pstcLinHandle->USARTx, (USART_RX | USART_TX), DISABLE);
                (void)USART_SetBaudrate(pstcLinHandle->USARTx, u32Baudrate, NULL);
                USART_FuncCmd(pstcLinHandle->USARTx, (USART_RX | USART_TX), ENABLE);
                pstcLinHandle->pstcFrame->enState = LIN_FRAME_STATE_SYNC;
                break;
            case LIN_FRAME_STATE_SYNC:
                if (u8Data == LIN_CalculatePIDParity(u8Data & 0x3FU)) {
                    pstcLinHandle->pstcFrame->u8PID = u8Data;
                    pstcLinHandle->pstcFrame->u8ID = pstcLinHandle->pstcFrame->u8PID & 0x3FU;
                    pstcLinHandle->pstcFrame->u8Len = LIN_GetFrameDataLenbyFrameID(pstcLinHandle->pstcFrame->u8ID);
                    pstcLinHandle->pstcFrame->enState = LIN_FRAME_STATE_PID;
                } else {
                    pstcLinHandle->pstcFrame->enError = LIN_ERR_PID;
                }
                break;
            case LIN_FRAME_STATE_PID:
                if (pstcLinHandle->pstcFrame->u8XferCount < pstcLinHandle->pstcFrame->u8Len) {
                    pstcLinHandle->pstcFrame->au8Data[pstcLinHandle->pstcFrame->u8XferCount++] = u8Data;
                }

                if (pstcLinHandle->pstcFrame->u8XferCount == pstcLinHandle->pstcFrame->u8Len) {
                    pstcLinHandle->pstcFrame->enState = LIN_FRAME_STATE_DATA;
                }
                break;
            case LIN_FRAME_STATE_DATA:
                pstcLinHandle->pstcFrame->u8Checksum = u8Data;
                pstcLinHandle->pstcFrame->enState = LIN_FRAME_STATE_CHKSUM;
                break;
            default:
                break;
        }
    }
}

/**
 * @brief  USART error IRQ callback.
 * @param  [in] pstcLinHandle           Pointer to struct @ref stc_lin_handle_t
 * @retval None
 */
static void USART_RxError_IrqCallback(const stc_lin_handle_t *pstcLinHandle)
{
    if (NULL != pstcLinHandle) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        if (SET == USART_GetStatus(pstcLinHandle->USARTx, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR))) {
            (void)USART_ReadData(pstcLinHandle->USARTx);
        }

        USART_ClearStatus(pstcLinHandle->USARTx, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
    }
}

/**
 * @brief  USART LIN break detection && wakeup IRQ callback.
 * @param  [in] pstcLinHandle           Pointer to struct @ref stc_lin_handle_t
 * @retval None
 */
static void USART_LinWakeupBreak_IrqCallback(stc_lin_handle_t *pstcLinHandle)
{
    if (NULL != pstcLinHandle) {
        DDL_ASSERT(IS_VALID_USART_LIN(pstcLinHandle->USARTx));

        if (LIN_SLAVE == pstcLinHandle->u32LinMode) {
            if (LIN_STATE_SLEEP == pstcLinHandle->enLinState) {
                if (SET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_LIN_WKUP)) {
                    USART_FuncCmd(pstcLinHandle->USARTx, USART_LIN_WKUP, DISABLE);
                    pstcLinHandle->enLinState = LIN_STATE_WAKEUP;
                }
            } else {
                if (SET == USART_GetStatus(pstcLinHandle->USARTx, USART_FLAG_LIN_BREAK)) {
                    USART_FuncCmd(pstcLinHandle->USARTx, USART_INT_RX, ENABLE);
                    USART_FuncCmd(pstcLinHandle->USARTx, USART_LIN_INT_BREAK, DISABLE);
                    pstcLinHandle->pstcFrame->enState = LIN_FRAME_STATE_BREAK;
                }
            }
        }

        USART_ClearStatus(pstcLinHandle->USARTx, (USART_FLAG_LIN_BREAK | USART_FLAG_LIN_WKUP));
    }
}

/**
 * @brief  USART channel 3 RX full IRQ callback
 * @param  None
 * @retval None
 */
static void USART3_RxFull_IrqCallback(void)
{
    USART_RxFull_IrqCallback(m_apstcLinHandle[USART_LIN_CH3]);
}

/**
 * @brief  USART channel 6 RX full IRQ callback
 * @param  None
 * @retval None
 */
static void USART6_RxFull_IrqCallback(void)
{
    USART_RxFull_IrqCallback(m_apstcLinHandle[USART_LIN_CH6]);
}

/**
 * @brief  USART channel 3 RX error IRQ callback.
 * @param  None
 * @retval None
 */
static void USART3_RxError_IrqCallback(void)
{
    USART_RxError_IrqCallback(m_apstcLinHandle[USART_LIN_CH3]);
}

/**
 * @brief  USART channel 6 RX error IRQ callback.
 * @param  None
 * @retval None
 */
static void USART6_RxError_IrqCallback(void)
{
    USART_RxError_IrqCallback(m_apstcLinHandle[USART_LIN_CH6]);
}

/**
 * @brief  USART channel 3 LIN break detection && wakeup IRQ callback.
 * @param  None
 * @retval None
 */
static void USART3_LinWakeupBreak_IrqCallback(void)
{
    USART_LinWakeupBreak_IrqCallback(m_apstcLinHandle[USART_LIN_CH3]);
}

/**
 * @brief  USART channel 6 LIN break detection && wakeup IRQ callback.
 * @param  None
 * @retval None
 */
static void USART6_LinWakeupBreak_IrqCallback(void)
{
    USART_LinWakeupBreak_IrqCallback(m_apstcLinHandle[USART_LIN_CH6]);
}

/**
 * @brief  Instal IRQ handler.
 * @param  [in] pstcConfig              Pointer to struct @ref stc_irq_signin_config_t
 * @param  [in] u32Priority             Interrupt priority
 * @retval None
 */
static void INTC_InstalIrqHandler(const stc_irq_signin_config_t *pstcConfig, uint32_t u32Priority)
{
    if (NULL != pstcConfig) {
        (void)INTC_IrqSignIn(pstcConfig);
        NVIC_ClearPendingIRQ(pstcConfig->enIRQn);
        NVIC_SetPriority(pstcConfig->enIRQn, u32Priority);
        NVIC_EnableIRQ(pstcConfig->enIRQn);
    }
}

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
