/**
 *******************************************************************************
 * @file  event_port/ep_inout/source/main.c
 * @brief Main program Event Port Input/Output for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Comment revise
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
 * @addtogroup EVENT_PORT_INOUT
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define LED_G_PORT          (EVT_PORT_1)
#define LED_G_PIN           (EVT_PIN_02)
#define KEY5_PORT           (EVT_PORT_2)
#define KEY5_PIN            (EVT_PIN_06)
#define EP_TRIG_SEL         (AOS_EVTPORT12)
#define EP_TRIG_SRC         (EVT_SRC_EVENT_PORT2)
#define EP_FUNC             (GPIO_FUNC_14)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void EventPort_Init(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Event port initialize
 * @param  None
 * @retval None
 */
static void EventPort_Init(void)
{
    stc_ep_init_t stcEPInit;
    /* Set PB6 as Event Port 2.6 */
    GPIO_SetFunc(KEY5_PORT, KEY5_PIN, EP_FUNC);
    /* Set PA2 Event Port 1.2 */
    GPIO_SetFunc(LED_G_PORT, LED_G_PIN, EP_FUNC);
    /* Enable Event port operation clock */
    FCG_Fcg0PeriphClockCmd(PWC_FCG0_AOS, ENABLE);
    /* Set Event Port 2 event as the trigger source for Event Port 1 */
    AOS_SetTriggerEventSrc(EP_TRIG_SEL, EP_TRIG_SRC);

    /* Set Event Port 2.6 falling edge detect enable */
    (void)EP_StructInit(&stcEPInit);
    stcEPInit.u32Edge           = EP_TRIG_FALLING;
    stcEPInit.u32Filter         = EP_FILTER_ON;
    stcEPInit.u32FilterClock    = EP_FCLK_DIV8;
    (void)EP_Init(KEY5_PORT, KEY5_PIN, &stcEPInit);

    /* Set Event Port 1.2 as output function */
    (void)EP_StructInit(&stcEPInit);
    stcEPInit.enPinState        = EVT_PIN_RESET;
    stcEPInit.u32PinDir         = EP_DIR_OUT;
    stcEPInit.u32PinTriggerOps  = EP_OPS_TOGGLE;
    (void)EP_Init(LED_G_PORT, LED_G_PIN, &stcEPInit);
}

/**
 * @brief  Main function of EventPort input/output project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU);
    /* BSP Clock initialize */
    BSP_CLK_Init();
    /* Event Port init */
    EventPort_Init();
    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU);
    /* wait KEY5 key pressed */
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
