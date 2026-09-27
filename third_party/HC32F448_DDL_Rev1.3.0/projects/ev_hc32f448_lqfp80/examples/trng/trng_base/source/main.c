/**
 *******************************************************************************
 * @file  trng/trng_base/source/main.c
 * @brief Main program TRNG base for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             TRNG_Handler add __DSB for Arm Errata 838869
                                    Add TRNG_Cmd function
                                    Set XTAL as system clock source
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
 * @addtogroup TRNG_Base
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* Set to non-zero to use TRNG interrupt mode, zero to use polling mode. */
#define TRNG_USE_INTERRUPT              (1U)

#if (TRNG_USE_INTERRUPT > 0U)
/* TRNG interrupt source and number define */
#define TRNG_INT_IRQn                   (TRNG_IRQn)
#endif

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void SystemClockConfig(void);

static void TrngConfig(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static uint32_t m_au32Random[2U];

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Main function of template project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Unlock peripherals or registers */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);
    /* System clock config */
    SystemClockConfig();
    /* Initializes UART for debug printing. Baudrate is 115200. */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
    /* Config TRNG */
    TrngConfig();
    /* Lock peripherals or registers */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);

    for (;;) {
#if (TRNG_USE_INTERRUPT > 0U)
        TRNG_Start();
        /* Get random number in TRNG_IrqCallback */
#else
        (void)TRNG_GenerateRandom(m_au32Random, 2U);
#endif
        DDL_DelayMS(500U);
        DDL_Printf("Random numbers: 0x%08x, 0x%08x\r\n", (unsigned int)m_au32Random[0U], (unsigned int)m_au32Random[1U]);
    }
}

/**
 * @brief  Set XTAL as system clock source.
 * @param  None
 * @retval None
 */
static void SystemClockConfig(void)
{
    stc_clock_xtal_init_t stcXtalInit;

    /* XTAL config */
    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    /* Config XTAL and Enable XTAL */
    stcXtalInit.u8State = CLK_XTAL_ON;
    stcXtalInit.u8Mode = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_XTAL);
}

/**
 * @brief  TRNG initialization configuration.
 * @param  None
 * @retval None
 */
static void TrngConfig(void)
{
    /* Enable TRNG. */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_TRNG, ENABLE);
    /* TRNG initialization configuration. */
    TRNG_Init(TRNG_SHIFT_CNT64, TRNG_RELOAD_INIT_VAL_ENABLE);
    TRNG_Cmd(ENABLE);

#if (TRNG_USE_INTERRUPT > 0U)
    NVIC_ClearPendingIRQ(TRNG_INT_IRQn);
    NVIC_SetPriority(TRNG_INT_IRQn, DDL_IRQ_PRIO_15);
    NVIC_EnableIRQ(TRNG_INT_IRQn);
#endif
}

#if (TRNG_USE_INTERRUPT > 0U)
/**
 * @brief  TRNG end IRQ callback
 * @param  None
 * @retval None
 */
void TRNG_Handler(void)
{
    (void)TRNG_GetRandom(m_au32Random, 2U);

    __DSB();  /* Arm Errata 838869 */
}
#endif

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
