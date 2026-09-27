/**
 *******************************************************************************
 * @file  timera/timera_capture/source/main.c
 * @brief Main program TimerA capture for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Set XTAL as system clock source
   2025-11-03       CDT             Example optimized.
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
 * @addtogroup TIMERA_Capture
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* TimerA unit and channel definitions for this example. */
#define TMRA_UNIT3                          (CM_TMRA_3)
#define TMRA_UNIT5                          (CM_TMRA_5)
#define TMRA_PERIPH_ENABLE()                FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMRA_5 | FCG2_PERIPH_TMRA_3, ENABLE)
#define TMRA_AOS_CAPT_REG                   (AOS_TMRA_1)

/* Definitions for capture PWM pin edge */
#define TMRA3_CH1_CAPT_COND                 (TMRA_CAPT_COND_PWM_RISING)
#define TMRA3_CH1_CAPT_PWM_PORT             (GPIO_PORT_A)
#define TMRA3_CH1_CAPT_PWM_PIN              (GPIO_PIN_06)
#define TMRA3_CH1_CAPT_PWM_PIN_FUNC         (GPIO_FUNC_5)
#define TMRA3_CH1_CAPT_INT                  (TMRA_INT_CMP_CH1)
#define TMRA3_CH1_CAPT_FLAG                 (TMRA_FLAG_CMP_CH1)
#define TMRA3_CH1_CAPT_LOG                  "TimerA3 channel 1 captured the rising edge of the PWM pin"

/* Definitions for capture peripheral event */
#define TMRA5_CH2_CAPT_COND                 TMRA_CAPT_COND_EVT
#define TMRA5_CH2_CAPT_EVT                  (BSP_KEY1_EVT)
#define TMRA5_CH2_CAPT_INT                  (TMRA_INT_CMP_CH2)
#define TMRA5_CH2_CAPT_FLAG                 (TMRA_FLAG_CMP_CH2)
#define TMRA5_CH2_CAPT_LOG                  "TimerA5 channel 2 captured the specified event"

/* Definitions for capture TRIG pin edge */
/* Only channel 3 of TimerA supports capturing the edge of TRIG pin. */
#define TMRA5_CH3_CAPT_COND                 TMRA_CAPT_COND_TRIG_FALLING
#define TMRA5_CH3_CAPT_TRIG_PORT            (GPIO_PORT_A)
#define TMRA5_CH3_CAPT_TRIG_PIN             (GPIO_PIN_10)
#define TMRA5_CH3_CAPT_TRIG_PIN_FUNC        (GPIO_FUNC_5)
#define TMRA5_CH3_CAPT_INT                  (TMRA_INT_CMP_CH3)
#define TMRA5_CH3_CAPT_FLAG                 (TMRA_FLAG_CMP_CH3)
#define TMRA5_CH3_CAPT_LOG                  "TimerA5 channel 3 captured the falling edge of the TRIG pin"

/* Definitions for capture XOR edge */
/* Only channel 4 of TimerA supports capturing the XOR signal. */
#define TMRA5_CH4_CAPT_COND                 TMRA_CAPT_COND_XOR_RISING
#define TMRA5_CH4_CAPT_CLKA_PORT            (GPIO_PORT_A)
#define TMRA5_CH4_CAPT_CLKA_PIN             (GPIO_PIN_02)
#define TMRA5_CH4_CAPT_CLKA_PIN_FUNC        (GPIO_FUNC_5)
#define TMRA5_CH4_CAPT_CLKB_PORT            (GPIO_PORT_A)
#define TMRA5_CH4_CAPT_CLKB_PIN             (GPIO_PIN_03)
#define TMRA5_CH4_CAPT_CLKB_PIN_FUNC        (GPIO_FUNC_5)
#define TMRA5_CH4_CAPT_INT                  (TMRA_INT_CMP_CH4)
#define TMRA5_CH4_CAPT_FLAG                 (TMRA_FLAG_CMP_CH4)
#define TMRA5_CH4_CAPT_LOG                  "TimerA5 channel 4 captured the rising edge of XOR signal"

/* Specifies clock divider that you need. @ref TMRA_Clock_Divider */
#define TMRA_CLK_DIV                        (TMRA_CLK_DIV4)

/* Definitions of interrupt. */
#define TMRA3_INT_IRQn                      (TMRA_3_CMP_IRQn)
#define TMRA3_CMP_IRQ_HANDLER               TMRA_3_Cmp_Handler
#define TMRA5_INT_IRQn                      (TMRA_5_CMP_IRQn)
#define TMRA5_CMP_IRQ_HANDLER               TMRA_5_Cmp_Handler

#define TMRA_INT_PRIO                       (DDL_IRQ_PRIO_DEFAULT)

#define TMRA3_INT_TYPE                      (TMRA3_CH1_CAPT_INT)

#define TMRA5_INT_TYPE                      (TMRA5_CH2_CAPT_INT | \
                                             TMRA5_CH3_CAPT_INT | \
                                             TMRA5_CH4_CAPT_INT)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void TmrAConfig(void);
static void TmrACaptureCondConfig(void);
static void TmrAIrqConfig(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  Main function of timera_capture project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_INTC);
    /* Initializes UART for debug printing. Baudrate is 115200. */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
    /* Configures TimerA. */
    TmrAConfig();
    /* MCU Peripheral registers write protected. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_INTC);

    /* Starts TimerA. */
    TMRA_Start(TMRA_UNIT3);
    TMRA_Start(TMRA_UNIT5);

    /***************** Configuration end, application start **************/

    for (;;) {
        /* See IRQ handler */
    }
}

/**
 * @brief  TimerA configuration.
 * @param  None
 * @retval None
 */
static void TmrAConfig(void)
{
    stc_tmra_init_t stcTmraInit;

    /* 1. Enable TimerA peripheral clock. */
    TMRA_PERIPH_ENABLE();

    /* 2. Set a default initialization value for stcTmraInit. */
    (void)TMRA_StructInit(&stcTmraInit);

    /* 3. Modifies the initialization values depends on the application. */
    stcTmraInit.sw_count.u8ClockDiv = TMRA_CLK_DIV;
    (void)TMRA_Init(TMRA_UNIT3, &stcTmraInit);
    (void)TMRA_Init(TMRA_UNIT5, &stcTmraInit);

    /* 4. Set function mode as capturing mode. */
    TMRA_SetFunc(TMRA_UNIT3, TMRA_CH1, TMRA_FUNC_CAPT);
    TMRA_SetFunc(TMRA_UNIT5, TMRA_CH2, TMRA_FUNC_CAPT);
    TMRA_SetFunc(TMRA_UNIT5, TMRA_CH3, TMRA_FUNC_CAPT);
    TMRA_SetFunc(TMRA_UNIT5, TMRA_CH4, TMRA_FUNC_CAPT);

    /* 5. Configures the capture condition. */
    TmrACaptureCondConfig();

    /* 6. Configures IRQ if needed. */
    TMRA_IntCmd(TMRA_UNIT3, TMRA3_INT_TYPE, ENABLE);
    TMRA_IntCmd(TMRA_UNIT5, TMRA5_INT_TYPE, ENABLE);
    TmrAIrqConfig();
}

/**
 * @brief  Capture condition configuration.
 * @param  None
 * @retval None
 */
static void TmrACaptureCondConfig(void)
{
    /* Channel 1 capture */
    GPIO_SetFunc(TMRA3_CH1_CAPT_PWM_PORT, TMRA3_CH1_CAPT_PWM_PIN, TMRA3_CH1_CAPT_PWM_PIN_FUNC);
    TMRA_HWCaptureCondCmd(TMRA_UNIT3, TMRA_CH1, TMRA3_CH1_CAPT_COND, ENABLE);

    /* Channel 2 capture */
    BSP_KEY_Init();
    /* Enable AOS function. */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);
    AOS_SetTriggerEventSrc(TMRA_AOS_CAPT_REG, TMRA5_CH2_CAPT_EVT);
    TMRA_HWCaptureCondCmd(TMRA_UNIT5, TMRA_CH2, TMRA5_CH2_CAPT_COND, ENABLE);

    /* Channel 3 capture */
    GPIO_SetFunc(TMRA5_CH3_CAPT_TRIG_PORT, TMRA5_CH3_CAPT_TRIG_PIN, TMRA5_CH3_CAPT_TRIG_PIN_FUNC);
    TMRA_HWCaptureCondCmd(TMRA_UNIT5, TMRA_CH3, TMRA5_CH3_CAPT_COND, ENABLE);

    /* Channel 4 capture */
    GPIO_SetFunc(TMRA5_CH4_CAPT_CLKA_PORT, TMRA5_CH4_CAPT_CLKA_PIN, TMRA5_CH4_CAPT_CLKA_PIN_FUNC);
    GPIO_SetFunc(TMRA5_CH4_CAPT_CLKB_PORT, TMRA5_CH4_CAPT_CLKB_PIN, TMRA5_CH4_CAPT_CLKB_PIN_FUNC);
    TMRA_HWCaptureCondCmd(TMRA_UNIT5, TMRA_CH4, TMRA5_CH4_CAPT_COND, ENABLE);
}

/**
 * @brief  TimerA interrupt configuration.
 * @param  None
 * @retval None
 */
static void TmrAIrqConfig(void)
{
    NVIC_ClearPendingIRQ(TMRA3_INT_IRQn);
    NVIC_SetPriority(TMRA3_INT_IRQn, TMRA_INT_PRIO);
    NVIC_EnableIRQ(TMRA3_INT_IRQn);

    NVIC_ClearPendingIRQ(TMRA5_INT_IRQn);
    NVIC_SetPriority(TMRA5_INT_IRQn, TMRA_INT_PRIO);
    NVIC_EnableIRQ(TMRA5_INT_IRQn);
}

/**
 * @brief  TimerA3 capture IRQ handler.
 * @param  None
 * @retval None
 */
void TMRA3_CMP_IRQ_HANDLER(void)
{
    /* A capture occurred */
    /* Get capture value by calling function TMRA_GetCompareValue. */
    if (TMRA_GetStatus(TMRA_UNIT3, TMRA3_CH1_CAPT_FLAG) == SET) {
        TMRA_ClearStatus(TMRA_UNIT3, TMRA3_CH1_CAPT_FLAG);
        DDL_Printf("%s. Captured value is %u\r\n", TMRA3_CH1_CAPT_LOG,
                   (unsigned int)TMRA_GetCompareValue(TMRA_UNIT3, TMRA_CH1));
    }
    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  TimerA5 capture IRQ handler.
 * @param  None
 * @retval None
 */
void TMRA5_CMP_IRQ_HANDLER(void)
{
    /* A capture occurred */
    /* Get capture value by calling function TMRA_GetCompareValue. */
    if (TMRA_GetStatus(TMRA_UNIT5, TMRA5_CH2_CAPT_FLAG) == SET) {
        TMRA_ClearStatus(TMRA_UNIT5, TMRA5_CH2_CAPT_FLAG);
        DDL_Printf("%s. Captured value is %u\r\n", TMRA5_CH2_CAPT_LOG,
                   (unsigned int)TMRA_GetCompareValue(TMRA_UNIT5, TMRA_CH2));
    }

    if (TMRA_GetStatus(TMRA_UNIT5, TMRA5_CH3_CAPT_FLAG) == SET) {
        TMRA_ClearStatus(TMRA_UNIT5, TMRA5_CH3_CAPT_FLAG);
        DDL_Printf("%s. Captured value is %u\r\n", TMRA5_CH3_CAPT_LOG,
                   (unsigned int)TMRA_GetCompareValue(TMRA_UNIT5, TMRA_CH3));
    }

    if (TMRA_GetStatus(TMRA_UNIT5, TMRA5_CH4_CAPT_FLAG) == SET) {
        TMRA_ClearStatus(TMRA_UNIT5, TMRA5_CH4_CAPT_FLAG);
        DDL_Printf("%s. Captured value is %u\r\n", TMRA5_CH4_CAPT_LOG,
                   (unsigned int)TMRA_GetCompareValue(TMRA_UNIT5, TMRA_CH4));
    }

    __DSB();  /* Arm Errata 838869 */
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
