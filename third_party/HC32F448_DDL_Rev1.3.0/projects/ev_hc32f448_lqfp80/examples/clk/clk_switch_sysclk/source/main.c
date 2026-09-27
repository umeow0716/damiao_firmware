/**
 *******************************************************************************
 * @file  clk/clk_switch_sysclk/source/main.c
 * @brief Main program of CLK for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Modify XTAL32 initialize process
                                    Removed SRAM wait cycle relevant code
   2025-11-03       CDT             Use BSP_XTAL_Init to replace XtalInit
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
 * @addtogroup CLK_Switch_sysclk
 * @{
 */
/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define MCO_PORT                        GPIO_PORT_A
#define MCO_PIN                         GPIO_PIN_08
#define MCO_GPIO_FUNC                   GPIO_FUNC_1

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
static uint8_t au8SysClockTbl[] = {
    CLK_SYSCLK_SRC_HRC,
    CLK_SYSCLK_SRC_MRC,
    CLK_SYSCLK_SRC_LRC,
    CLK_SYSCLK_SRC_XTAL,
    CLK_SYSCLK_SRC_XTAL32,
    CLK_SYSCLK_SRC_PLL,
};

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void MCOInit(void);
static void PLLHInit(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  MCO pin initialize
 * @param  None
 * @retval None
 */
static void MCOInit(void)
{
    /* Configure clock output pin */
    GPIO_SetFunc(MCO_PORT, MCO_PIN, MCO_GPIO_FUNC);
    /* Configure clock output system clock */
    CLK_MCOConfig(CLK_MCO1, CLK_MCO_SRC_HCLK, CLK_MCO_DIV8);
    /* MCO1 output enable */
    CLK_MCOCmd(CLK_MCO1, ENABLE);
}

/**
 * @brief  PLLH initialize
 * @param  None
 * @retval None
 */
static void PLLHInit(void)
{
    stc_clock_pll_init_t stcPLLHInit;

    (void)CLK_PLLStructInit(&stcPLLHInit);
    /* PLLH config */
    /* 8MHz/M*N = 8/1*100/4 = 200MHz */
    stcPLLHInit.PLLCFGR = 0UL;
    stcPLLHInit.PLLCFGR_f.PLLM = (1UL  - 1UL);
    stcPLLHInit.PLLCFGR_f.PLLN = (100UL - 1UL);
    stcPLLHInit.PLLCFGR_f.PLLR = (4UL  - 1UL);
    stcPLLHInit.PLLCFGR_f.PLLQ = (4UL  - 1UL);
    stcPLLHInit.PLLCFGR_f.PLLP = (4UL  - 1UL);
    stcPLLHInit.u8PLLState = CLK_PLL_ON;
    stcPLLHInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcPLLHInit);
}

/**
 * @brief  Main function of CLK switch project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint8_t i = 0U;

    /* Register write unprotected for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU);
    /* Set bus clock div. */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL, (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV4 | CLK_PCLK3_DIV4 |
                                      CLK_PCLK4_DIV2 | CLK_EXCLK_DIV2 | CLK_HCLK_DIV1));
    /* BSP key initialize */
    BSP_KEY_Init();
    /* flash read wait cycle setting */
    EFM_SetWaitCycle(EFM_WAIT_CYCLE3);
    /* GPIO read wait cycle setting */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT3);
    /* output system clock */
    MCOInit();
    /* Xtal initialize */
    BSP_XTAL_Init();
    /* Xtal32 initialize*/
    BSP_XTAL32_Init();
    /* MPLL initialize */
    PLLHInit();
    /* enable LRC */
    (void)CLK_LrcCmd(ENABLE);
    /* enable HRC */
    (void)CLK_HrcCmd(ENABLE);
    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_GPIO);

    for (;;) {
        if (SET == BSP_KEY_GetStatus(BSP_KEY_5)) {
            CLK_SetSysClockSrc(au8SysClockTbl[i]);
            while (RESET != BSP_KEY_GetStatus(BSP_KEY_5));
            i++;
            if (i >= sizeof(au8SysClockTbl) / sizeof(au8SysClockTbl[0U])) {
                i = 0U;
            }
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
