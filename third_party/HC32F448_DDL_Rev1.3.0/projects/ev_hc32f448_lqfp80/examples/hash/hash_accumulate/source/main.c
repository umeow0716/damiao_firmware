/**
 *******************************************************************************
 * @file  hash/hash_accumulate/source/main.c
 * @brief Main program HASH Accumulate for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2025-11-03       CDT             First version
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
#include <string.h>
#include "main.h"

/**
 * @addtogroup HC32F448_DDL_Examples
 * @{
 */

/**
 * @addtogroup HASH_Accumulate
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define HASH_MSG_DIGEST_SIZE        (32U)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void SystemClockConfig(void);
static void HashConfig(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
const static char *m_s8SrcData1 = "abcdeasddfsgfdgdhrghdabcdeasddfsgfdgdhrghdhdfhdhdhdb";
const static char *m_s8SrcData2 = "abcdeasddfsgfdgdhrghdabcdeasddfsgfdgdhrghdhdfhdhdhdbabcdeasddfsgfdgdhrghdhdfhdhdhdbabcdeasddf";
const static char *m_s8SrcData3 = "abcderghdhdfhdhdhdbabcdeasddfsgfdgdhrghdhdfhdhdhdbabcdeasddfsgfdgdhrghdhdfhrghdhdfhdhdhdbabcdeasddfsgfdgdhrghdhdfhdhdhdbabcdeasddfsgfdgdhrghdhdfh";

const static uint8_t m_au8MsgDigest[HASH_MSG_DIGEST_SIZE] = {
    0x0d, 0x93, 0x93, 0x06, 0xb2, 0x46, 0xd8, 0xd7,
    0x96, 0x6d, 0x05, 0x5c, 0x6f, 0xe3, 0x0b, 0xac,
    0x2c, 0x78, 0x84, 0x54, 0x50, 0x6f, 0xf0, 0x15,
    0xf8, 0xde, 0xb4, 0xda, 0xf4, 0x62, 0x94, 0x4b,
};
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Main function of HASH base project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint8_t au8MsgDigest[HASH_MSG_DIGEST_SIZE];

    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);
    /* System clock config */
    SystemClockConfig();
    /* Initializes UART for debug printing. Baudrate is 115200. */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
    /* HASH configuration. */
    HashConfig();
    /* MCU Peripheral registers write protected. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);

    /***************** Configuration end, application start **************/

    for (;;) {
        /* input data */
        HASH_InputData((uint8_t *)m_s8SrcData1, strlen(m_s8SrcData1));
        HASH_InputData((uint8_t *)m_s8SrcData2, strlen(m_s8SrcData2));
        HASH_InputData((uint8_t *)m_s8SrcData3, strlen(m_s8SrcData3));
        /* get digest */
        HASH_GetMsgDigest(au8MsgDigest);
        /* compare result */
        if (memcmp(m_au8MsgDigest, au8MsgDigest, HASH_MSG_DIGEST_SIZE) == 0U) {
            DDL_Printf("HASH basic calculation OK.\r\n");
        } else {
            DDL_Printf("HASH basic calculation FAIL.\r\n");
            for (;;) {
                /* rsvd */
            }
        }

        DDL_DelayMS(500U);
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
 * @brief  HASH configuration.
 * @param  None
 * @retval None
 */
static void HashConfig(void)
{
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_HASH, ENABLE);
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
