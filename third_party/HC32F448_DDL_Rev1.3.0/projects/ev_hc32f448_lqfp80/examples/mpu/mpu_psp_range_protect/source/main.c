/**
 *******************************************************************************
 * @file  mpu/mpu_psp_range_protect/source/main.c
 * @brief Main program of MPU PSP range protect for the Device Driver Library.
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
#include "main.h"

/**
 * @addtogroup HC32F448_DDL_Examples
 * @{
 */

/**
 * @addtogroup MPU_PSP_Range_Protect
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* unlock/lock peripheral */
#define EXAMPLE_PERIPH_WE               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | \
                                         LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_MPU)
#define EXAMPLE_PERIPH_WP               (LL_PERIPH_EFM | LL_PERIPH_FCG)

/* MPU define */
#define MPU_SP_UNIT                     (MPU_SP_UNIT_PSP)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  NMI MPU error IRQ handler.
 * @param  None
 * @retval None
 */
static void NMI_MpuError_IrqHandler(void)
{
    __set_PSP(0UL);
    if (SET == NMI_GetNmiStatus(NMI_SRC_BUS_ERR)) {
        NMI_ClearNmiStatus(NMI_SRC_BUS_ERR);
        if (SET == MPU_GetStatus(MPU_FLAG_PSP)) {
            MPU_ClearStatus(MPU_FLAG_PSP);
            BSP_LED_On(LED_RED);
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
    NMI_MpuError_IrqHandler();

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  MPU configuration.
 * @param  None
 * @retval None
 */
static void MPU_Config(void)
{
    stc_mpu_sp_init_t stcMpuSpInit;
    stc_nmi_init_t stcNmiInit;

    /* Set PSP value = 0UL */
    __set_PSP(0UL);

    /* Configure MPU */
    (void)MPU_SPStructInit(&stcMpuSpInit);
    stcMpuSpInit.u32ExceptionType   = MPU_SP_EXP_TYPE_NMI;
    stcMpuSpInit.u32Addr            = 0UL;
    stcMpuSpInit.u32Size            = 1024UL;
    MPU_SPInit(MPU_SP_UNIT, &stcMpuSpInit);
    MPU_SPCmd(MPU_SP_UNIT, ENABLE);

    /* Configure NMI interrupt */
    (void)NMI_StructInit(&stcNmiInit);
    stcNmiInit.u32Src = NMI_SRC_BUS_ERR;
    (void)NMI_Init(&stcNmiInit);
}

/**
 * @brief  Main function of MPU PSP Range protect.
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Peripheral registers write unprotected */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    /* Configure BSP */
    BSP_CLK_Init();
    BSP_IO_Init();
    BSP_LED_Init();
    BSP_KEY_Init();
    /* Configure UART */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
    /* Configure MPU */
    MPU_Config();
    /* Peripheral registers write protected */
    LL_PERIPH_WP(EXAMPLE_PERIPH_WP);
    BSP_LED_On(LED_BLUE);

    for (;;) {
        if (SET == BSP_KEY_GetStatus(BSP_KEY_2)) {
            __set_PSP(2048UL);
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
