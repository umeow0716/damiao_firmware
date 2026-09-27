/**
 *******************************************************************************
 * @file  intc/intc_extint_key/source/main.c
 * @brief Main program EXTINT_KEY for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Integrate global, independent interrupt in one project
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
 * @addtogroup EXTINT_KEY
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define KEY5_PORT               (GPIO_PORT_B)
#define KEY5_PIN                (GPIO_PIN_06)
#define KEY5_EXTINT_CH          (EXTINT_CH06)
#define KEY5_INT_SRC            (INT_SRC_PORT_EIRQ6)
#define KEY5_GLB_INT_IRQn       (INT001_IRQn)
#define KEY5_INT_PRIO           (DDL_IRQ_PRIO_DEFAULT)
#define KEY5_INDEP_INT_IRQn     (EXTINT_PORT_EIRQ6_IRQn)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void EXTINT_KEY5_Global_IrqCallback(void);
static void KEY5_Global_Init(void);
static void KEY5_Indep_Init(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  KEY5 External interrupt Ch.6 callback function
 *         IRQ No.1 in Global IRQ entry No.0~31 is used for EXTINT6
 * @param  None
 * @retval None
 */
static void EXTINT_KEY5_Global_IrqCallback(void)
{
    if (SET == EXTINT_GetExtIntStatus(KEY5_EXTINT_CH)) {
        BSP_LED_Toggle(LED_BLUE);
        while (PIN_RESET == GPIO_ReadInputPins(KEY5_PORT, KEY5_PIN)) {
            ;
        }
        EXTINT_ClearExtIntStatus(KEY5_EXTINT_CH);
        NVIC_DisableIRQ(KEY5_GLB_INT_IRQn);
        NVIC_EnableIRQ(KEY5_INDEP_INT_IRQn);
    }
}

/**
 * @brief  KEY5 global IT initialize
 * @param  None
 * @retval None
 */
static void KEY5_Global_Init(void)
{
    stc_extint_init_t stcExtIntInit;
    stc_irq_signin_config_t stcIrqSignConfig;
    stc_gpio_init_t stcGpioInit;

    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
    stcGpioInit.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(KEY5_PORT, KEY5_PIN, &stcGpioInit);

    /* ExtInt config */
    (void)EXTINT_StructInit(&stcExtIntInit);
    stcExtIntInit.u32Filter      = EXTINT_FILTER_ON;
    stcExtIntInit.u32FilterClock = EXTINT_FCLK_DIV8;
    stcExtIntInit.u32Edge = EXTINT_TRIG_FALLING;
    (void)EXTINT_Init(KEY5_EXTINT_CH, &stcExtIntInit);

    /* IRQ sign-in */
    stcIrqSignConfig.enIntSrc = KEY5_INT_SRC;
    stcIrqSignConfig.enIRQn   = KEY5_GLB_INT_IRQn;
    stcIrqSignConfig.pfnCallback = &EXTINT_KEY5_Global_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, KEY5_INT_PRIO);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);
}

/**
 * @brief  KEY5 External interrupt Ch.6 callback function
 * @param  None
 * @retval None
 */
void EXTINT06_SWINT22_Handler(void)
{
    if (SET == EXTINT_GetExtIntStatus(KEY5_EXTINT_CH)) {
        BSP_LED_Toggle(LED_YELLOW);
        while (PIN_RESET == GPIO_ReadInputPins(KEY5_PORT, KEY5_PIN)) {
            ;
        }
        EXTINT_ClearExtIntStatus(KEY5_EXTINT_CH);
        NVIC_DisableIRQ(KEY5_INDEP_INT_IRQn);
        NVIC_EnableIRQ(KEY5_GLB_INT_IRQn);
    }

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  KEY5 independent IT initialize
 * @param  None
 * @retval None
 */
static void KEY5_Indep_Init(void)
{
    stc_extint_init_t stcExtIntInit;
    stc_gpio_init_t stcGpioInit;

    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
    stcGpioInit.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(KEY5_PORT, KEY5_PIN, &stcGpioInit);

    /* ExtInt config */
    (void)EXTINT_StructInit(&stcExtIntInit);
    stcExtIntInit.u32Filter      = EXTINT_FILTER_ON;
    stcExtIntInit.u32FilterClock = EXTINT_FCLK_DIV8;
    stcExtIntInit.u32Edge = EXTINT_TRIG_FALLING;
    (void)EXTINT_Init(KEY5_EXTINT_CH, &stcExtIntInit);

    /* NVIC config */
    NVIC_ClearPendingIRQ(KEY5_INDEP_INT_IRQn);
    NVIC_SetPriority(KEY5_INDEP_INT_IRQn, KEY5_INT_PRIO);
    NVIC_EnableIRQ(KEY5_INDEP_INT_IRQn);
}

/**
 * @brief  Main function of EXTINT project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU);
    /* BSP Clock initialize */
    BSP_CLK_Init();
    /* Expand IO initialize */
    BSP_IO_Init();
    /* BSP LED initialize */
    BSP_LED_Init();
    /* KEY5 initialize */
    KEY5_Global_Init();
    KEY5_Indep_Init();
    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU);
    /* wait KEY5 pressed */
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
