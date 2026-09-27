/**
 *******************************************************************************
 * @file  keyscan/keyscan_matrix/source/main.c
 * @brief Main program keyscan for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Modify external interrupt filter B macros
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
 * @addtogroup Keyscan
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define KEYSCAN_ROW0_EXTINT     (EXTINT_CH00)
#define KEYSCAN_ROW1_EXTINT     (EXTINT_CH01)

#define KEYSCAN_ROW0_IRQn       (INT001_IRQn)
#define KEYSCAN_ROW1_IRQn       (INT002_IRQn)

#define KEYSCAN_ROW0_INT_SRC    (INT_SRC_PORT_EIRQ0)
#define KEYSCAN_ROW1_INT_SRC    (INT_SRC_PORT_EIRQ1)

/* KEYOUT port, pin definition */
#define KEYOUT0_PORT            (GPIO_PORT_C)
#define KEYOUT0_PIN             (GPIO_PIN_06)
#define KEYOUT1_PORT            (GPIO_PORT_D)
#define KEYOUT1_PIN             (GPIO_PIN_08)

/* KEYIN port, pin definition */
#define KEYIN0_PORT             (GPIO_PORT_A)
#define KEYIN0_PIN              (GPIO_PIN_00)
#define KEYIN0_SEL              (KEYSCAN_IN_0)
#define KEYIN1_PORT             (GPIO_PORT_A)
#define KEYIN1_PIN              (GPIO_PIN_01)
#define KEYIN1_SEL              (KEYSCAN_IN_1)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void KEYSCAN_ROW0_IrqCallback(void);
static void KEYSCAN_ROW1_IrqCallback(void);
static void KEYSCAN_ROW0_Init(void);
static void KEYSCAN_ROW1_Init(void);
static void KEYSCAN_COL_Init(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  EXTINT Ch.0 as KEYSCAN row 0 callback function
 * @param  None
 * @retval None
 */
static void KEYSCAN_ROW0_IrqCallback(void)
{
    if (SET == EXTINT_GetExtIntStatus(KEYSCAN_ROW0_EXTINT)) {
        BSP_LED_Toggle(LED_BLUE);
        DDL_Printf("Key row 0, col %ld is pressed.\r\n", KEYSCAN_GetKeyoutIdx());
        /* wait key released */
        while (PIN_RESET == GPIO_ReadInputPins(KEYIN0_PORT, KEYIN0_PIN)) {
            ;
        }
        /* clear int request flag */
        EXTINT_ClearExtIntStatus(KEYSCAN_ROW0_EXTINT);
    }
}

/**
 * @brief  EXTINT Ch.1 as KEYSCAN row 1 callback function
 * @param  None
 * @retval None
 */
static void KEYSCAN_ROW1_IrqCallback(void)
{
    if (SET == EXTINT_GetExtIntStatus(KEYSCAN_ROW1_EXTINT)) {
        BSP_LED_Toggle(LED_YELLOW);
        DDL_Printf("Key row 1, col %ld is pressed.\r\n", KEYSCAN_GetKeyoutIdx());
        /* wait key released */
        while (PIN_RESET == GPIO_ReadInputPins(KEYIN1_PORT, KEYIN1_PIN)) {
            ;
        }
        /* clear int request flag */
        EXTINT_ClearExtIntStatus(KEYSCAN_ROW1_EXTINT);
    }
}

/**
 * @brief  KEYSCAN row 0 initialize
 * @param  None
 * @retval None
 */
static void KEYSCAN_ROW0_Init(void)
{
    stc_extint_init_t stcExtIntInit;
    stc_irq_signin_config_t stcIrqSignConfig;
    stc_gpio_init_t stcGpioInit;

    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
    stcGpioInit.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(KEYIN0_PORT, KEYIN0_PIN, &stcGpioInit);

    /* ExtInt config */
    (void)EXTINT_StructInit(&stcExtIntInit);
    stcExtIntInit.u32Edge = EXTINT_TRIG_FALLING;
    stcExtIntInit.u32FilterB = EXTINT_FILTER_B_ON;
    stcExtIntInit.u32FilterBClock = EXTINT_FILTER_B_LVL4;
    (void)EXTINT_Init(KEYSCAN_ROW0_EXTINT, &stcExtIntInit);

    /* IRQ sign-in */
    stcIrqSignConfig.enIntSrc = KEYSCAN_ROW0_INT_SRC;
    stcIrqSignConfig.enIRQn   = KEYSCAN_ROW0_IRQn;
    stcIrqSignConfig.pfnCallback = &KEYSCAN_ROW0_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(KEYSCAN_ROW0_IRQn);
    NVIC_SetPriority(KEYSCAN_ROW0_IRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(KEYSCAN_ROW0_IRQn);
}

/**
 * @brief  KEYSCAN row 1 initialize
 * @param  None
 * @retval None
 */
static void KEYSCAN_ROW1_Init(void)
{
    stc_extint_init_t stcExtIntInit;
    stc_irq_signin_config_t stcIrqSignConfig;
    stc_gpio_init_t stcGpioInit;

    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
    stcGpioInit.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(KEYIN1_PORT, KEYIN1_PIN, &stcGpioInit);

    /* ExtInt config */
    (void)EXTINT_StructInit(&stcExtIntInit);
    stcExtIntInit.u32Edge = EXTINT_TRIG_FALLING;
    stcExtIntInit.u32FilterB = EXTINT_FILTER_B_ON;
    stcExtIntInit.u32FilterBClock = EXTINT_FILTER_B_LVL4;
    (void)EXTINT_Init(KEYSCAN_ROW1_EXTINT, &stcExtIntInit);

    /* IRQ sign-in */
    stcIrqSignConfig.enIntSrc = KEYSCAN_ROW1_INT_SRC;
    stcIrqSignConfig.enIRQn   = KEYSCAN_ROW1_IRQn;
    stcIrqSignConfig.pfnCallback = &KEYSCAN_ROW1_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(KEYSCAN_ROW1_IRQn);
    NVIC_SetPriority(KEYSCAN_ROW1_IRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(KEYSCAN_ROW1_IRQn);
}

/**
 * @brief  KEYSCAN column initialize
 * @param  None
 * @retval None
 */
static void KEYSCAN_COL_Init(void)
{
    stc_gpio_init_t stcGpioInit;
    stc_keyscan_init_t stcKeyscanInit;

    (void)GPIO_StructInit(&stcGpioInit);
    (void)KEYSCAN_StructInit(&stcKeyscanInit);

    stcGpioInit.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(KEYOUT0_PORT, KEYOUT0_PIN, &stcGpioInit);
    GPIO_SetFunc(KEYOUT0_PORT, KEYOUT0_PIN, GPIO_FUNC_8);

    (void)GPIO_Init(KEYOUT1_PORT, KEYOUT1_PIN, &stcGpioInit);
    GPIO_SetFunc(KEYOUT1_PORT, KEYOUT1_PIN, GPIO_FUNC_8);

    FCG_Fcg0PeriphClockCmd(PWC_FCG0_KEY, ENABLE);

    (void)CLK_LrcCmd(ENABLE);

    stcKeyscanInit.u32HizCycle = KEYSCAN_HIZ_CYCLE_4;
    stcKeyscanInit.u32LowCycle = KEYSCAN_LOW_CYCLE_512;
    stcKeyscanInit.u32KeyClock = KEYSCAN_CLK_LRC;
    stcKeyscanInit.u32KeyOut   = KEYSCAN_OUT_0T7;
    stcKeyscanInit.u32KeyIn    = (KEYIN0_SEL | KEYIN1_SEL);

    (void)KEYSCAN_Init(&stcKeyscanInit);
}

/**
 * @brief  Main function of keyscan project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU);
    /* BSP Clock initialize */
    BSP_CLK_Init();
    /* Expand IO init */
    BSP_IO_Init();
    /* BSP LED initialize */
    BSP_LED_Init();
    /* Printf init */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
    /* Matrix KEY row init */
    KEYSCAN_ROW0_Init();
    KEYSCAN_ROW1_Init();
    /* Matrix KEY column init */
    KEYSCAN_COL_Init();
    /* Clear all KEYIN interrupt flag before enable */
    EXTINT_ClearExtIntStatus(KEYSCAN_ROW0_EXTINT);
    EXTINT_ClearExtIntStatus(KEYSCAN_ROW1_EXTINT);
    /* KEYSCAN enable */
    KEYSCAN_Cmd(ENABLE);
    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU);
    /* Add your code here */
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
