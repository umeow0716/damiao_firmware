/**
 *******************************************************************************
 * @file  efm/efm_int/source/main.c
 * @brief Main program of EFM for the Device Driver Library.
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
 * @addtogroup EFM_Int
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define EFM_SECTOR30_NUM            (30U)
#define EFM_IRQn                    (EFM_PEERR_RDCOL_IRQn)


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
 * @brief  EFM Program/Erase Error IRQ callback
 * @param  None
 * @retval None
 */
void EFM_PEError_ReadCol_Handler(void)
{
    BSP_LED_Off(LED_BLUE);
    BSP_LED_On(LED_RED);

    /* Clear Flag */
    EFM_ClearStatus(EFM_FLAG_PEPRTERR);

    __DSB();  /* Arm Errata 838869 */
}

/**
 * @brief  Configure IRQ handler && NVIC.
 * @param  None
 * @retval None
 */
static void EFM_IntInit(void)
{
    NVIC_ClearPendingIRQ(EFM_IRQn);
    NVIC_SetPriority(EFM_IRQn, DDL_IRQ_PRIO_15);
    NVIC_EnableIRQ(EFM_IRQn);

    /* Enable Interrupt function */
    EFM_IntCmd(EFM_INT_PEERR, ENABLE);
}

/**
 * @brief  Main function of EFM project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint32_t u32TestData = 0x5A5A5A5AUL;
    uint32_t u32Addr;

    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_EFM);

    /* System clock init */
    BSP_CLK_Init();
    /* Expand IO init */
    BSP_IO_Init();
    /* LED init */
    BSP_LED_Init();
    /* KEY Init */
    BSP_KEY_Init();

    /* Turn on LED blue */
    BSP_LED_On(LED_BLUE);

    /*Configure EFM interrupt */
    EFM_IntInit();

    /* Wait flash ready. */
    while (SET != EFM_GetStatus(EFM_FLAG_RDY)) {
        ;
    }

    /* EFM_FWMC write enable */
    EFM_FWMC_Cmd(ENABLE);
    /* Sector 30~31 disable write protection */
    (void)EFM_SequenceSectorOperateCmd(EFM_SECTOR30_NUM, 2U, ENABLE);
    /* Erase sector 30. */
    u32Addr = EFM_SECTOR_ADDR(EFM_SECTOR30_NUM);
    (void)EFM_SectorErase(u32Addr);

    /* Program within the allowed address. Program in sector 30*/
    (void)EFM_ProgramWord(u32Addr, u32TestData);

    /* Press KEY1 */
    while (RESET == BSP_KEY_GetStatus(BSP_KEY_1)) {
        ;
    }

    /* Program outside the allowed address. Program in sector 29*/
    (void)EFM_ProgramWord((u32Addr - 32U), u32TestData);
    /* Sector 30~31 enable write protection */
    (void)EFM_SequenceSectorOperateCmd(EFM_SECTOR30_NUM, 2U, DISABLE);

    EFM_FWMC_Cmd(DISABLE);
    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_EFM);

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
