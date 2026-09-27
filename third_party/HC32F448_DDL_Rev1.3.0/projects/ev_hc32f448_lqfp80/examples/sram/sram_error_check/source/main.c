/**
 *******************************************************************************
 * @file  sram/sram_error_check/source/main.c
 * @brief Main program of SRAM for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             sample code changed according to driver change
   2025-11-03       CDT             Select SRAM ECC/parity check via key
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
 * @addtogroup SRAM_Error_Check
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define SRAM0_CHECK_ADDR        (0x20001000UL)
#define SRAMH_CHECK_ADDR        (0x1FFFFFF0UL)

#define SRAM0_ECC_EI_BIT        ((1U << 5U) | (1U << 10U))
#define SRAM0_ECC_MD            (SRAM_SRAM0_ECC_MD1)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void SramConfig(void);
static void NMI_SramError_IrqHandler(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Main function of SRAM error check project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    __UNUSED uint32_t u32Tmp;

    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_SRAM);
    /* Expand IO init */
    BSP_IO_Init();
    /* BSP Led */
    BSP_LED_Init();
    /* BSP Key */
    BSP_KEY_Init();
    /* BSP print */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);

    /* Print sram ecc reset */
    if ((SET == RMU_GetStatus(RMU_FLAG_RAM_ECC))) {
        BSP_LED_On(LED_RED);
        DDL_Printf("SRAM0 ECC error occurs and reset!\r\n");
        RMU_ClearStatus();
        DDL_DelayMS(500U);
        BSP_LED_Off(LED_RED);
    }
    /* SRAM configuration. */
    SramConfig();
    /* MCU Peripheral registers write protected. */
    LL_PERIPH_WP(LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_GPIO);

    for (;;) {
        if (SET == BSP_KEY_GetStatus(BSP_KEY_1)) {
            /* Make SRAM0 ecc error */
            SRAM_ErrorInjectBitCmd(SRAM_ECC_SRAM0, SRAM0_ECC_EI_BIT, ENABLE);
            SRAM_ErrorInjectCmd(SRAM_ECC_SRAM0, ENABLE);
            /* Read data */
            u32Tmp = RW_MEM32(SRAM0_CHECK_ADDR);
            DDL_DelayMS(500U);
            BSP_LED_Off(LED_RED);
        } else if (SET == BSP_KEY_GetStatus(BSP_KEY_2)) {
            /* Make SRAMH parity error */
            /* Read a SRAM address that uninitialized and the parity check error will occur after the reading operation. */
            u32Tmp = RW_MEM32(SRAMH_CHECK_ADDR);
            DDL_DelayMS(500U);
            BSP_LED_Off(LED_BLUE);
        }
    }
}

/**
 * @brief  NMI IRQ Handler.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
    NMI_SramError_IrqHandler();

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  Configures SRAM.
 * @param  None
 * @retval None
 */
static void SramConfig(void)
{
    stc_nmi_init_t stcNmiInit;
    /* Sram init */
    SRAM_Init();
    /* Set sram exception type */
    SRAM_SetExceptionType(SRAM_CHECK_SRAM0, SRAM_EXP_TYPE_RST);
    SRAM_SetExceptionType(SRAM_CHECK_SRAMH, SRAM_EXP_TYPE_NMI);
    /* Set sram ECC mode */
    SRAM_SetEccMode(SRAM_ECC_SRAM0, SRAM0_ECC_MD);
    /* Reset SRAM0 */
    RW_MEM32(SRAM0_CHECK_ADDR) = 0xFFFFFFFFUL;

    /* NMI interrupt configuration. */
    stcNmiInit.u32Src = NMI_SRC_SRAM_PARITY;
    (void)NMI_Init(&stcNmiInit);
}

/**
 * @brief  NMI SRAM error IRQ handler.
 * @param  None
 * @retval None
 */
static void NMI_SramError_IrqHandler(void)
{
    NMI_ClearNmiStatus(NMI_SRC_SRAM_PARITY);
    if (SET == SRAM_GetStatus(SRAM_FLAG_SRAMH_PYERR)) {
        BSP_LED_On(LED_BLUE);
        DDL_Printf("SRAMH parity error occurs!\r\n");
        /* Clear flag */
        SRAM_ClearStatus(SRAM_FLAG_SRAMH_PYERR);
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
