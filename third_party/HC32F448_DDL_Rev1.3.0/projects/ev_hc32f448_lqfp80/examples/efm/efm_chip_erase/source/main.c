/**
 *******************************************************************************
 * @file  efm/efm_chip_erase/source/main.c
 * @brief Main program of EFM for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-09-30       CDT             Fixed bug # release write protect before sector erase
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
 * @addtogroup EFM_Chip_Erase
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define EFM_SECTOR0_NUM         (0U)
#define EFM_SECTOR13_NUM        (13U)

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
 * @brief  Main function of EFM project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint32_t u32Addr0, u32Addr1;
    uint8_t u8TestBuf[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    uint8_t u8ExpectBuf[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t u8ReadBuf1[20] = {0};
    uint8_t u8ReadBuf2[20] = {0};
    int32_t i32Ret1, i32Ret2;

    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_EFM);

    /* System clock init */
    BSP_CLK_Init();
    /* Expand IO init */
    BSP_IO_Init();
    /* LED init */
    BSP_LED_Init();

    /* Wait flash ready. */
    while (SET != EFM_GetStatus(EFM_FLAG_RDY)) {
        ;
    }

    /* EFM_FWMC write enable */
    EFM_FWMC_Cmd(ENABLE);
    /* Release bus while erase & program */
    EFM_SetBusStatus(EFM_BUS_RELEASE);

    u32Addr0 = EFM_SECTOR_ADDR(EFM_SECTOR0_NUM);
    u32Addr1 = EFM_SECTOR_ADDR(EFM_SECTOR13_NUM);
    /* Release write protect */
    EFM_SingleSectorOperateCmd(EFM_SECTOR0_NUM, ENABLE);
    EFM_SingleSectorOperateCmd(EFM_SECTOR13_NUM, ENABLE);
    /* Erase sector */
    (void)EFM_SectorErase(u32Addr0);
    (void)EFM_SectorErase(u32Addr1);
    /* Sequence program. */
    (void)EFM_SequenceProgram(u32Addr0, u8TestBuf, sizeof(u8TestBuf));
    (void)EFM_Program(u32Addr1, u8TestBuf, sizeof(u8TestBuf));

    (void)EFM_ReadByte(u32Addr0, u8ReadBuf1, sizeof(u8TestBuf));
    (void)EFM_ReadByte(u32Addr1, u8ReadBuf2, sizeof(u8TestBuf));

    i32Ret1 = memcmp(u8ReadBuf1, u8TestBuf, sizeof(u8TestBuf));
    i32Ret2 = memcmp(u8ReadBuf2, u8TestBuf, sizeof(u8TestBuf));
    if ((0 == i32Ret1) && (0 == i32Ret2)) {
        /* LED blue, as expected */
        BSP_LED_On(LED_BLUE);
    } else {
        /* LED red */
        BSP_LED_On(LED_RED);
        for (;;) {
            ;
        }
    }

    /* FLASH disable write protection (sector 0~31) */
    EFM_SequenceSectorOperateCmd(EFM_SECTOR0_NUM, 32U, ENABLE);
    /* Chip Erase: flash All erased */
    (void)EFM_ChipErase(EFM_CHIP_ALL);
    (void)EFM_ReadByte(u32Addr0, u8ReadBuf1, sizeof(u8ExpectBuf));
    (void)EFM_ReadByte(u32Addr1, u8ReadBuf2, sizeof(u8ExpectBuf));

    i32Ret1 = memcmp(u8ReadBuf1, u8ExpectBuf, sizeof(u8ExpectBuf));
    i32Ret2 = memcmp(u8ReadBuf2, u8ExpectBuf, sizeof(u8ExpectBuf));
    if ((0 == i32Ret1) && (0 == i32Ret2)) {
        /* LED blue, as expected */
        BSP_LED_On(LED_BLUE);
    } else {
        /* LED red */
        BSP_LED_On(LED_RED);
    }

    /* FLASH enable write protection (sector 0~31) */
    (void)EFM_SequenceSectorOperateCmd(EFM_SECTOR0_NUM, 32U, DISABLE);

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
