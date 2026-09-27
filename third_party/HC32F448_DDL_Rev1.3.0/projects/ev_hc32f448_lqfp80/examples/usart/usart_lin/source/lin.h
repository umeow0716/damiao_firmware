/**
 *******************************************************************************
 * @file  usart/usart_lin/source/lin.h
 * @brief This file contains all the functions prototypes of the LIN midware
 *        library.
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
#ifndef __LIN_H__
#define __LIN_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_usart.h"

/**
 * @addtogroup USART_LIN
 * @{
 */

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup LIN_Global_Types LIN Global Types
 * @{
 */

/**
 * @brief LIN state enumeration definition
 */
typedef enum {
    LIN_STATE_SLEEP  = 0U,              /*!< Idle */
    LIN_STATE_WAKEUP = 1U,              /*!< Frame Break field */
} en_lin_state_t;

/**
 * @brief LIN frame state enumeration definition
 */
typedef enum {
    LIN_FRAME_STATE_IDLE   = 0U,        /*!< Frame idle */
    LIN_FRAME_STATE_BREAK  = 1U,        /*!< Frame Break field */
    LIN_FRAME_STATE_SYNC   = 2U,        /*!< Frame Sync field  */
    LIN_FRAME_STATE_PID    = 3U,        /*!< Frame PID field  */
    LIN_FRAME_STATE_DATA   = 4U,        /*!< Frame Data  */
    LIN_FRAME_STATE_CHKSUM = 5U,        /*!< Frame Checksum */
} en_lin_frame_state_t;

/**
 * @brief LIN error code enumeration definition
 */
typedef enum {
    LIN_OK         = 0U,                /*!< No error */
    LIN_ERR_BREAK  = 1U,                /*!< Error break */
    LIN_ERR_PID    = 2U,                /*!< Error PID */
    LIN_ERR_CHKSUM = 3U,                /*!< Error checksum */
} en_lin_err_t;

/**
 * @brief LIN frame data structure definition
 */
typedef struct {
    __IO en_lin_frame_state_t enState;  /*!< LIN frame state
                                             This parameter can be a value of @ref en_lin_frame_state_t */
    uint8_t u8ID;                       /*!< ID-Number for Frame */
    uint8_t u8PID;                      /*!< Protected ID-Number for Frame */
    uint8_t u8Len;                      /*!< Data length */
    uint8_t au8Data[8];                 /*!< Data array */
    uint8_t u8Checksum;                 /*!< Data checksum */

    uint8_t u8XferCount;                /*!< Transfer data count */
    en_lin_err_t enError;               /*!< Error code
                                             This parameter can be a value of @ref en_lin_err_t */
} stc_lin_frame_t;

/**
 * @brief LIN interrupt number configure structure definition
 */
typedef struct {

    IRQn_Type RxFullIRQn;               /*!< USART RX full interrupt interrupt IRQ number
                                             This parameter can be a value of @ref IRQn_Type */
    IRQn_Type RxErrorIRQn;              /*!< USART RX error interrupt IRQ number
                                             This parameter can be a value of @ref IRQn_Type */
    IRQn_Type BreakWkupIRQn;            /*!< USART RX interrupt interrupt IRQ number
                                             This parameter can be a value of @ref IRQn_Type */
} stc_lin_irq_config_t;

/**
 * @brief LIN pin configure structure definition
 */
typedef struct {
    uint8_t  u8RxPort;
    uint16_t u16RxPin;
    uint8_t  u16RxPinFunc;
    uint8_t  u8TxPort;
    uint16_t u16TxPin;
    uint8_t  u16TxPinFunc;
} stc_lin_pin_config_t;

/**
 * @brief LIN slave configure structure definition
 */
typedef struct {
    uint32_t u32AdjustBaudrate;  /*!< Adjust by calculating sync-byte-field */
} stc_lin_slave_config_t;

/**
 * @brief LIN mode initialization structure definition
 */
typedef struct {
    CM_USART_TypeDef       *USARTx;         /*!< USART instance
                                                 This parameter can be CM_USARTx(x = 5,10) */
    uint32_t               u32LinMode;      /*!< LIN mode @ref LIN_Mode */
    en_lin_state_t         enLinState;      /*!< LIN state @ref en_lin_state_t */
    stc_usart_lin_init_t   stcLinInit;      /*!< LIN function initialization @ref stc_usart_lin_init_t */
    stc_lin_pin_config_t   stcPinConfig;    /*!< LIN pin configure @ref stc_lin_pin_config_t */
    stc_lin_irq_config_t   stcIrqConfig;    /*!< LIN function IRQ number configure structure @ref stc_lin_irq_config_t */
    stc_lin_slave_config_t stcSlaveConfig;  /*!< The configure is valid when slave mode. */
    stc_lin_frame_t        *pstcFrame;      /*!< LIN frame @ref stc_lin_frame_t */
} stc_lin_handle_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup LIN_Global_Macros LIN Global Macros
 * @{
 */

/**
 * @defgroup LIN_Mode LIN Mode
 * @{
 */
#define LIN_MASTER                      (0UL)       /*!< LIN master */
#define LIN_SLAVE                       (1UL)       /*!< LIN slave */
/**
 * @}
 */

/**
 * @defgroup LIN_Fram_Data_Length LIN Fram Data Length
 * @{
 */
#define LIN_FRAME_DATA_2BYTE          (1U << 4)   /*!< Frame data length:2 bytes */
#define LIN_FRAME_DATA_4BYTE          (2U << 4)   /*!< Frame data length:4 bytes */
#define LIN_FRAME_DATA_8BYTE          (3U << 4)   /*!< Frame data length:8 bytes */
/**
 * @}
 */

/**
 * @defgroup LIN_Receive_Waiting_Time LIN Receive Waiting Time
 * @{
 */
#define LIN_REC_WAITING_FOREVER         (-1)
/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup LIN_Global_Functions
 * @{
 */
int32_t LIN_Init(stc_lin_handle_t *pstcLinHandle);

int32_t LIN_SetState(stc_lin_handle_t *pstcLinHandle, en_lin_state_t enState);
en_lin_state_t LIN_GetState(const stc_lin_handle_t *pstcLinHandle);
int32_t LIN_Sleep(stc_lin_handle_t *pstcLinHandle);
int32_t LIN_SendWakeupSignal(const stc_lin_handle_t *pstcLinHandle);

/* LIN master polling transfer */
int32_t LIN_MASTER_SendFrame(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame);
int32_t LIN_MASTER_ReceiveFrame(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame, int32_t i32Timeout);

/* LIN slave polling transfer */
int32_t LIN_SLAVE_ReceiveFrame(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame, int32_t i32Timeout);
int32_t LIN_SLAVE_SendFrame(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame, int32_t i32Timeout);
int32_t LIN_SLAVE_ReceiveFrameHeader(stc_lin_handle_t *pstcLinHandle, stc_lin_frame_t *pstcFrame,
                                     int32_t i32Timeout);
int32_t LIN_SLAVE_ReceiveFrameResponse(const stc_lin_handle_t *pstcLinHandle, int32_t i32Timeout);
int32_t LIN_SLAVE_SendFrameResponse(stc_lin_handle_t *pstcLinHandle);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __LIN_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
