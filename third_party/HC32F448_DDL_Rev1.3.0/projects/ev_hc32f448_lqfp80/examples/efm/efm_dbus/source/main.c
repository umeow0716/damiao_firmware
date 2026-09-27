/**
 *******************************************************************************
 * @file  efm/efm_dbus/source/main.c
 * @brief Main program of EFM for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Set API DBUS_Protect_test optimization level
   2025-11-03       CDT             Optimize code process
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
 * @addtogroup EFM_Dbus
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define ICG3_ADDRESS                    (0x0000040CUL)
#define IN_DBUS_SECTOR_NUM              (14U)
#define OUT_DBUS_SECTOR_NUM             (31U)

#define DBUS_Protect_test_ADDRESS_AC6   ".ARM.__at_0x20000"

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
static void DBUS_Protect_test(void) __attribute__((optnone, section(DBUS_Protect_test_ADDRESS_AC6)));
#elif defined (__GNUC__) && !defined (__CC_ARM)
static void DBUS_Protect_test(void) __attribute__((optimize("O0"), section(".dbusfun_sec")));
#elif defined (__CC_ARM)
#pragma O0
static void DBUS_Protect_test(void) __attribute__((section(DBUS_Protect_test_ADDRESS_AC6)));
#elif defined (__ICCARM__)
#pragma location="DBUS_FUNC"
static void DBUS_Protect_test(void);
#else
#error "unsupported compiler!!"
#endif

static const uint32_t u32WriteData[2U] = {0x1155U,  0xaa46U};
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  HardFault Handler
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
    DDL_Printf("BusFault occur!\r\n");
    BSP_LED_On(LED_RED);
    BSP_LED_Off(LED_BLUE);
    for (; ;) {
        ;
    }
}

/**
 * @brief  Dbus protect test function
 * @param  None
 * @retval None
 */
static void DBUS_Protect_test(void)
{
    uint32_t u32Address1 = EFM_SECTOR_ADDR(IN_DBUS_SECTOR_NUM);
    uint32_t u32Address2 = EFM_SECTOR_ADDR(OUT_DBUS_SECTOR_NUM);
    static  uint32_t u32ReadData1, u32ReadData2;

    BSP_LED_On(LED_BLUE);

    /* Mcu control flash */
    /* Read in dbus sector*/
    u32ReadData1 = *((uint32_t *)u32Address1);
    if (u32ReadData1 != u32WriteData[0]) {
        DDL_Printf("Data Read wrong in the DBUS protected sector!\r\n");
    }

    /* Read in out dbus sector*/
    u32ReadData2 = *((uint32_t *)u32Address2);
    if (u32ReadData2 != u32WriteData[1]) {
        DDL_Printf("Data Read wrong in unprotected sector!\r\n");
    }
}

/**
 * @brief  Main function of EFM project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint32_t u32Address1 = EFM_SECTOR_ADDR(IN_DBUS_SECTOR_NUM);
    uint32_t u32Address2 = EFM_SECTOR_ADDR(OUT_DBUS_SECTOR_NUM);
    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_EFM);
    /* Enable bus fault handler */
    SET_REG32_BIT(SCB->SHCSR, SCB_SHCSR_BUSFAULTENA_Msk);
    /* System clock init */
    BSP_CLK_Init();
    /* Expand IO init */
    BSP_IO_Init();
    /* KEY Init */
    BSP_KEY_Init();
    /* LED init */
    BSP_LED_Init();
    BSP_LED_Off(LED_RED | LED_BLUE);
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG);

    /* EFM_FWMC write enable */
    EFM_FWMC_Cmd(ENABLE);
    /* Wait for flash ready */
    while (SET != EFM_GetStatus(EFM_FLAG_RDY)) {
        ;
    }
    /* Sector 0 disables write protection */
    (void)EFM_SingleSectorOperateCmd(0U, ENABLE);

    if (ICG_FLASH_PROTECT_RST_DISABLE == READ_REG32(CM_ICG->ICG3)) {
        DDL_Printf("DBus protect not enable!\r\n");
        /* write data into flash */
        (void)EFM_SingleSectorOperateCmd(IN_DBUS_SECTOR_NUM, ENABLE);
        (void)EFM_SingleSectorOperateCmd(OUT_DBUS_SECTOR_NUM, ENABLE);
        (void)EFM_SectorErase(EFM_SECTOR_ADDR(IN_DBUS_SECTOR_NUM));
        (void)EFM_SectorErase(EFM_SECTOR_ADDR(OUT_DBUS_SECTOR_NUM));
        if (LL_OK != EFM_ProgramWord(u32Address1, u32WriteData[0]) || \
            (LL_OK != EFM_ProgramWord(u32Address2, u32WriteData[1]))) {
            DDL_Printf("EFM programming u32WriteData error!\r\n");
        }
    } else {
        DDL_Printf("DBus protect enable!\r\n");
    }
    /* DBus protect test */
    DBUS_Protect_test();
    DDL_Printf("Test finished!\r\n");
    /* Press KEY1 */
    while (RESET == BSP_KEY_GetStatus(BSP_KEY_1)) {
        ;
    }

    (void)EFM_ProgramWord(ICG3_ADDRESS, ICG_FLASH_PROTECT_RST_ENABLE);
    for (; ;) {
        DDL_DelayMS(500U);
        NVIC_SystemReset();
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
