/**
 *******************************************************************************
 * @file  exmc/exmc_smc_lcd_nt35510/source/main.c
 * @brief This example demonstrates LCD function.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Re-implement BSP_CLK_Init()
   2025-11-03       CDT             Use BSP_XTAL_PIN to replace BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN
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
 * @addtogroup EXMC_SMC_LCD_NT35510
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/**
 * @brief point
 */
typedef struct {
    uint16_t u16X;
    uint16_t u16Y;
} stc_touchpad_point_t;

/**
 * @brief window
 */
typedef struct {
    uint16_t u16X1;
    uint16_t u16Y1;
    uint16_t u16X2;
    uint16_t u16Y2;
} stc_touchpad_window_t;

/**
 * @brief touch data definition
 */
typedef struct {
    stc_touchpad_point_t stcPoint;
    en_flag_status_t enPointPress;
} stc_touchpad_data_t;

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static stc_touchpad_data_t m_stcTouchData;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @brief  BSP clock initialize.
 *         SET board system clock to PLLH@160MHz
 *         Flash: 3 wait
 *         SRAM_H: 0 wait
 *         SRAM_B: 1 wait
 *         PCLK0: 160MHz
 *         PCLK1: 80MHz
 *         PCLK2: 40MHz
 *         PCLK3: 40MHz
 *         PCLK4: 80MHz
 *         EXCLK: 40MHz
 *         HCLK:  160MHz
 * @param  None
 * @retval None
 */
void BSP_CLK_Init(void)
{
    stc_clock_xtal_init_t stcXtalInit;
    stc_clock_pll_init_t stcPLLHInit;

    /* PCLK0, HCLK Max 200MHz */
    /* PCLK1, PCLK4 Max 100MHz */
    /* PCLK2, EXCLK Max 60MHz */
    /* PCLK3 Max 50MHz */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL,
                    (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV4 |
                     CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2 | CLK_EXCLK_DIV4 |
                     CLK_HCLK_DIV1));

    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    /* Config Xtal and enable Xtal */
    stcXtalInit.u8Mode   = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv    = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8State  = CLK_XTAL_ON;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);

    (void)CLK_PLLStructInit(&stcPLLHInit);
    /* VCO = (8/1)*80 = 640MHz*/
    stcPLLHInit.u8PLLState = CLK_PLL_ON;
    stcPLLHInit.PLLCFGR = 0UL;
    stcPLLHInit.PLLCFGR_f.PLLM = 1UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLN = 80UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLP = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLQ = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLR = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcPLLHInit);

    /* 3 cycles for 150 ~ 200MHz */
    (void)EFM_SetWaitCycle(EFM_WAIT_CYCLE3);
    /* 3 cycles for 150 ~ 200MHz */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT3);
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);
}

/**
 * @brief  Return true is the touchpad is pressed
 * @param  None
 * @retval Press state
 */
static bool TOUCHPAD_IsPressed(void)
{
    uint8_t u8Tmp;
    uint8_t u8Status;
    bool bPressed = false;

    BSP_GT9XX_REG_Read(GT9XX_TOUCH_STATUS, &u8Status, 1UL);
    if ((u8Status & 0x80U) != 0U) {
        u8Tmp = 0U;
        BSP_GT9XX_REG_Write(GT9XX_TOUCH_STATUS, &u8Tmp, 1U);

        if ((0U < (u8Status & 0x0FU)) && ((u8Status & 0x0FU) < 6U)) {
            bPressed = true;
        }
    }

    return bPressed;
}

/**
 * @brief  Get touch data.
 * @param  [out] pstcData               Pointer to a @ref stc_touchpad_data_t structure.
 * @retval None
 */
static void TOUCHPAD_Read(stc_touchpad_data_t *pstcData)
{
    static uint16_t u16LastX = 0U;
    static uint16_t u16LastY = 0U;

    /*Save the pressed coordinates and the state*/
    if (TOUCHPAD_IsPressed()) {
        BSP_GT9XX_GetXY(GT9XX_POINT1, &u16LastX, &u16LastY);
        pstcData->enPointPress = SET;
    } else {
        pstcData->enPointPress = RESET;
    }

    /*Set the last pressed coordinates*/
    pstcData->stcPoint.u16X = u16LastX;
    pstcData->stcPoint.u16Y = u16LastY;
}

/**
 * @brief  Check if a point is on an window
 * @param  [in] pstcWin                 Pointer to a @ref stc_touchpad_window_t structure.
 * @param  [in] pstcPoint               Pointer to a @ref stc_touchpad_point_t structure.
 * @retval bool:
 *           - true:                    The point is in the area.
 *           - false:                   The point is out the area.
 */
bool TOUCHPAD_IsPointOn(const stc_touchpad_window_t *pstcWin, const stc_touchpad_point_t *pstcPoint)
{
    bool bIsPointOnWin = false;

    if ((pstcPoint->u16X >= pstcWin->u16X1 && pstcPoint->u16X <= pstcWin->u16X2) && \
        (pstcPoint->u16Y >= pstcWin->u16Y1 && pstcPoint->u16Y <= pstcWin->u16Y2)) {
        bIsPointOnWin = true;
    }

    return bIsPointOnWin;
}

/**
 * @brief  Main function of LCD project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint8_t i;
    uint16_t u16Width;
    uint16_t u16Height;
    bool bIsPointOnWin;
    const uint8_t u8RectangleCount = 3U;
    uint16_t u16WinSize;
    stc_touchpad_window_t stcWin;

    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_EFM);

    /* Initialize system clock: */
    BSP_CLK_Init();

    BSP_IO_Init();
    BSP_LCD_IO_Init();

    /* Initialize LCD touch pad */
    BSP_GT9XX_Init();

    /* HW Reset LCD */
    BSP_LCD_RSTCmd(EIO_PIN_RESET); /* RST# to low */
    DDL_DelayMS(50UL);
    BSP_LCD_RSTCmd(EIO_PIN_SET);   /* RST# to high */
    DDL_DelayMS(50UL);

    /* Initialize NT35510 LCD */
    BSP_NT35510_Init();

    /* Clear LCD screen */
    BSP_NT35510_Clear(LCD_COLOR_BLACK);

    /* Turn on LCD backlight */
    BSP_LCD_BKLCmd(EIO_PIN_SET);

    /* Set LCD cursor */
    BSP_NT35510_SetCursor(0U, 0U);

    /* MCU Peripheral registers write protected */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_EFM);

    u16Width = BSP_NT35510_GetPixelWidth();
    u16Height = BSP_NT35510_GetPixelHeight();

    for (;;) {
        for (i = 0U; i < u8RectangleCount; i++) {
            bIsPointOnWin = false;
            u16WinSize = u16Width / u8RectangleCount;

            stcWin.u16X1 = (i * u16WinSize);
            stcWin.u16Y1 = (u16Height / 2U) - (u16WinSize / 2U);
            stcWin.u16X2 = stcWin.u16X1 + u16WinSize - 1U;
            stcWin.u16Y2 = (u16Height / 2U) + (u16WinSize / 2U) - 1U;
            BSP_NT35510_DrawRectangle(stcWin.u16X1, stcWin.u16Y1, stcWin.u16X2, stcWin.u16Y2, LCD_COLOR_GREEN);

            do {
                (void)memset(&m_stcTouchData, 0, sizeof(m_stcTouchData));
                TOUCHPAD_Read(&m_stcTouchData);
                if (m_stcTouchData.enPointPress == SET) {
                    bIsPointOnWin = TOUCHPAD_IsPointOn(&stcWin, &m_stcTouchData.stcPoint);
                }
            } while (false == bIsPointOnWin);

            BSP_NT35510_Clear(LCD_COLOR_BLACK);
        }

        BSP_NT35510_DrawCircle(u16Width / 2U, u16Height / 2U, u16Width / 4U, LCD_COLOR_RED);
        DDL_DelayMS(1000UL);

        BSP_NT35510_Clear(LCD_COLOR_RED);
        DDL_DelayMS(1000UL);

        BSP_NT35510_Clear(LCD_COLOR_GREEN);
        DDL_DelayMS(1000UL);

        BSP_NT35510_Clear(LCD_COLOR_BLUE);
        DDL_DelayMS(1000UL);

        /* Clear LCD screen */
        BSP_NT35510_Clear(LCD_COLOR_BLACK);
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
