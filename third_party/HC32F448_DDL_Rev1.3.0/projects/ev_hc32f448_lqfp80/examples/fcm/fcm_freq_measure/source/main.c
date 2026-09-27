/**
 *******************************************************************************
 * @file  fcm/fcm_freq_measure/source/main.c
 * @brief FCM main program example for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             FCM interface add instance
                                    Use BSP_XTAL_PIN to replace BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN
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
 * @addtogroup FCM
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
typedef struct {
    char *pi8TargetClock;
    uint32_t u32TargetClock;
    uint32_t u32TargetClockFreq;
} stcFmcTargetTbl_t;

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define REF_FREQ            (XTAL_VALUE)
#define REF_DIV             (8192U)
#define REF_FREQ_L          (XTAL32_VALUE)
#define REF_DIV_L           (8192U)
#define TAR_DIV             (1U)
#define FMC_DEMO_BAUDRATE   (9600U)
#define XTAL32_FREQ         (XTAL32_VALUE)
#define HRC_FREQ            (16UL * 1000UL * 1000UL)
#define LRC_FREQ            (LRC_VALUE)
#define SWDTLRC_FREQ        (SWDTLRC_VALUE)
#define PCLK1_FREQ          (4UL * 1000UL * 1000UL)
#define MRC_FREQ            (MRC_VALUE)
#define PLLHP_FREQ          (50UL * 1000UL * 1000UL)

#define FCM_ERR_IRQn        (INT000_IRQn)
#define FCM_OVF_IRQn        (INT001_IRQn)
#define FCM_END_IRQn        (INT002_IRQn)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void FCM_Error_IrqCallback(void);
static void FCM_Ovf_IrqCallback(void);
static void FCM_End_IrqCallback(void);
static void FcmErrorIntInit(void);
static void FcmEndIntInit(void);
static void FcmOvfIntInit(void);
static void FcmInit(uint32_t u32TargetClock, uint32_t u32TargetClockFreq);
static void RefClockInit(void);
static void TargetClockInit(void);
static void SwdtInit(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static char *m_pi8TargetClock;
static stcFmcTargetTbl_t m_stcFcmTargetTbl[] = {
    {"1.XTAL32", FCM_TARGET_CLK_XTAL32, XTAL32_FREQ},
    {"2.HRC", FCM_TARGET_CLK_HRC, HRC_FREQ},
    {"3.LRC", FCM_TARGET_CLK_LRC, LRC_FREQ},
    {"4.SWDTLRC", FCM_TARGET_CLK_SWDTLRC, SWDTLRC_FREQ},
    {"5.PCLK1(SYSCLK=8MHz div2)", FCM_TARGET_CLK_PCLK1, PCLK1_FREQ},
    {"6.MRC", FCM_TARGET_CLK_MRC, MRC_VALUE},
    {"7.PLLHP(VCO=800MHz div16)", FCM_TARGET_CLK_PLLHP, PLLHP_FREQ},

};

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  FCM frequency error IRQ callback
 * @param  None
 * @retval None
 */
static void FCM_Error_IrqCallback(void)
{
    FCM_Cmd(CM_FCM, DISABLE);
    FCM_ClearStatus(CM_FCM, FCM_FLAG_ERR);
    DDL_Printf("Error: Freq. out of range!\r\n");
}

/**
 * @brief  FCM measure counter overflow IRQ callback
 * @param  None
 * @retval None
 */
static void FCM_Ovf_IrqCallback(void)
{
    FCM_Cmd(CM_FCM, DISABLE);
    FCM_ClearStatus(CM_FCM, FCM_FLAG_OVF);
    DDL_Printf("Error: Count Overflow!\r\n");
}

/**
 * @brief  FCM measure end IRQ callback
 * @param  None
 * @retval None
 */
static void FCM_End_IrqCallback(void)
{
    uint16_t u16FcmCnt;
    uint32_t u32TargetClock;
    uint32_t u32RefFreq;
    uint32_t u32RefDiv;

    u16FcmCnt = FCM_GetCountValue(CM_FCM);
    FCM_Cmd(CM_FCM, DISABLE);
    FCM_ClearStatus(CM_FCM, FCM_FLAG_END);
    u32TargetClock = READ_REG32_BIT(CM_FCM->MCCR, FCM_MCCR_MCKS);
    if ((FCM_TARGET_CLK_XTAL32 == u32TargetClock) || (FCM_TARGET_CLK_LRC == u32TargetClock) || \
        (FCM_TARGET_CLK_SWDTLRC == u32TargetClock)) {
        u32RefFreq = REF_FREQ_L;
        u32RefDiv = REF_DIV_L;
    } else {
        u32RefFreq = REF_FREQ;
        u32RefDiv = REF_DIV;
    }
    if ((FCM_TARGET_CLK_XTAL32 == u32TargetClock) || (FCM_TARGET_CLK_LRC == u32TargetClock) || \
        (FCM_TARGET_CLK_SWDTLRC == u32TargetClock)) {
        DDL_Printf("%s freq. is %lu Hz\r\n", m_pi8TargetClock, (u32RefFreq * u16FcmCnt / u32RefDiv) * TAR_DIV);
    } else {
        DDL_Printf("%s freq. is %lu KHz\r\n", m_pi8TargetClock, (u32RefFreq / 1000UL * u16FcmCnt / u32RefDiv) * TAR_DIV);
    }
}

/**
 * @brief  FCM frequency error interrupt init
 * @param  None
 * @retval None
 */
static void FcmErrorIntInit(void)
{
    stc_irq_signin_config_t stcIrqSignConfig;

    stcIrqSignConfig.enIntSrc = INT_SRC_FCMFERRI;
    stcIrqSignConfig.enIRQn   = FCM_ERR_IRQn;
    stcIrqSignConfig.pfnCallback = &FCM_Error_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(FCM_ERR_IRQn);
    NVIC_SetPriority(FCM_ERR_IRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(FCM_ERR_IRQn);
}

/**
 * @brief  FCM measure end interrupt init
 * @param  None
 * @retval None
 */
static void FcmEndIntInit(void)
{
    stc_irq_signin_config_t stcIrqSignConfig;

    stcIrqSignConfig.enIntSrc = INT_SRC_FCMMENDI;
    stcIrqSignConfig.enIRQn   = FCM_END_IRQn;
    stcIrqSignConfig.pfnCallback = &FCM_End_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(FCM_END_IRQn);
    NVIC_SetPriority(FCM_END_IRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(FCM_END_IRQn);
}

/**
 * @brief  FCM counter overflow interrupt init
 * @param  None
 * @retval None
 */
static void FcmOvfIntInit(void)
{
    stc_irq_signin_config_t stcIrqSignConfig;

    stcIrqSignConfig.enIntSrc = INT_SRC_FCMCOVFI;
    stcIrqSignConfig.enIRQn   = FCM_OVF_IRQn;
    stcIrqSignConfig.pfnCallback = &FCM_Ovf_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(FCM_OVF_IRQn);
    NVIC_SetPriority(FCM_OVF_IRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(FCM_OVF_IRQn);
}

/**
 * @brief  FCM init
 * @param  [in] u32TargetClock Target clock type selection. @ref FCM_Target_Clock_Src
 * @param  [in] u32TargetClockFreq Target clock frequency.
 * @retval None
 */
static void FcmInit(uint32_t u32TargetClock, uint32_t u32TargetClockFreq)
{
    stc_fcm_init_t stcFcmInit;
    uint32_t u32RefFreq;
    uint32_t u32RefDiv;

    (void)FCM_StructInit(&stcFcmInit);
    if ((FCM_TARGET_CLK_XTAL32 == u32TargetClock) || (FCM_TARGET_CLK_LRC == u32TargetClock) || \
        (FCM_TARGET_CLK_SWDTLRC == u32TargetClock)) {
        u32RefFreq = REF_FREQ_L;
        u32RefDiv = REF_DIV_L;
        stcFcmInit.u32RefClock     = FCM_REF_CLK_XTAL32;
        stcFcmInit.u32RefClockDiv  = FCM_REF_CLK_DIV8192;
    } else {
        u32RefFreq = REF_FREQ;
        u32RefDiv = REF_DIV;
        stcFcmInit.u32RefClock     = FCM_REF_CLK_XTAL;
        stcFcmInit.u32RefClockDiv  = FCM_REF_CLK_DIV8192;
    }

    stcFcmInit.u32RefClockEdge = FCM_REF_CLK_RISING;
    stcFcmInit.u32TargetClock  = u32TargetClock;
    stcFcmInit.u32ExceptionType = FCM_EXP_TYPE_INT;
    stcFcmInit.u32TargetClockDiv = FCM_TARGET_CLK_DIV1;

    /* idea count value = (tar_freq/tar_div)/(ref_freq/ref_div) */
    stcFcmInit.u16LowerLimit = (uint16_t)((((u32TargetClockFreq / TAR_DIV) / (u32RefFreq / u32RefDiv)) * 97UL) / 100UL);
    stcFcmInit.u16UpperLimit = (uint16_t)((((u32TargetClockFreq / TAR_DIV) / (u32RefFreq / u32RefDiv)) * 103UL) / 100UL);
    if (1U == (stcFcmInit.u16UpperLimit - stcFcmInit.u16LowerLimit)) {
        stcFcmInit.u16LowerLimit -= 1U;
        stcFcmInit.u16UpperLimit += 1U;
    }

    (void)FCM_Init(CM_FCM, &stcFcmInit);
    FCM_IntCmd(CM_FCM, (FCM_INT_OVF | FCM_INT_END | FCM_INT_ERR), ENABLE);
}

/**
 * @brief  FCM reference clock init
 * @param  None
 * @retval None
 */
static void RefClockInit(void)
{
    stc_clock_xtal_init_t stcXtalInit;

    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    stcXtalInit.u8State = CLK_XTAL_ON;
    stcXtalInit.u8Mode  = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv   = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_XTAL);
}

/**
 * @brief  FCM target clock init, including HRC, LRC, PLLH, XTAL32
 * @param  None
 * @retval None
 */
static void TargetClockInit(void)
{
    stc_clock_xtal32_init_t stcXtal32Init;
    stc_clock_pll_init_t stcPLLInit;

    /* Xtal32 config */
    GPIO_AnalogCmd(BSP_XTAL32_PORT, BSP_XTAL32_PIN, ENABLE);
    (void)CLK_Xtal32StructInit(&stcXtal32Init);
    stcXtal32Init.u8State = CLK_XTAL32_ON;
    stcXtal32Init.u8Drv   = CLK_XTAL32_DRV_MID;
    stcXtal32Init.u8Filter = CLK_XTAL32_FILTER_ALL_MD;
    (void)CLK_Xtal32Init(&stcXtal32Init);

    (void)CLK_HrcCmd(ENABLE);
    (void)CLK_MrcCmd(ENABLE);
    (void)CLK_LrcCmd(ENABLE);

    /* PCLK0, HCLK 8MHz */
    /* PCLK1, PCLK4, EX BUS 4MHz */
    /* PCLK2, PCLK3 2MHz */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL,                                                \
                    (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV4 |             \
                     CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2 | CLK_EXCLK_DIV2 |             \
                     CLK_HCLK_DIV1));

    /* PLL config */
    (void)CLK_PLLStructInit(&stcPLLInit);
    /**
        VCO = 8/1*100 = 800MHz
        8MHz/M*N = 8/1*100/16 = 50MHz
    */
    stcPLLInit.u8PLLState = CLK_PLL_ON;
    stcPLLInit.PLLCFGR = 0UL;
    stcPLLInit.PLLCFGR_f.PLLM = 1UL - 1UL;
    stcPLLInit.PLLCFGR_f.PLLN = 100UL - 1UL;
    stcPLLInit.PLLCFGR_f.PLLP = 16UL - 1UL;
    stcPLLInit.PLLCFGR_f.PLLQ = 16UL - 1UL;
    stcPLLInit.PLLCFGR_f.PLLR = 16UL - 1UL;
    stcPLLInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcPLLInit);
}

/**
 * @brief  SWDT initialize, SWDTLRC will work after SWDT works.
 * @param  None
 * @retval None
 */
static void SwdtInit(void)
{
    stc_swdt_init_t stcSwdtInit;

    /* SWDT configure */
    stcSwdtInit.u32CountPeriod   = SWDT_CNT_PERIOD256;
    stcSwdtInit.u32ClockDiv      = SWDT_CLK_DIV32;
    stcSwdtInit.u32RefreshRange  = SWDT_RANGE_0TO100PCT;
    stcSwdtInit.u32LPMCount      = SWDT_LPM_CNT_CONT;
    stcSwdtInit.u32ExceptionType = SWDT_EXP_TYPE_INT;
    (void)SWDT_Init(&stcSwdtInit);
}

/**
 * @brief  Wait KEY5 press
 * @param  None
 * @retval None
 */
static void WaitKeyPress(void)
{
    while (SET != BSP_KEY_GetStatus(BSP_KEY_5)) {
        DDL_DelayMS(10UL);
        SWDT_FeedDog();
    }
    while (RESET != BSP_KEY_GetStatus(BSP_KEY_5)) {
        DDL_DelayMS(10UL);
        SWDT_FeedDog();
    }
}

/**
 * @brief  Wait FCM process end
 * @param  None
 * @retval None
 */
static void WaitFcmEnd(void)
{
    while (0UL != READ_REG32(bCM_FCM->STR_b.START)) {
        DDL_DelayMS(10UL);
        SWDT_FeedDog();
    }
}

/**
 * @brief  Main function of FCM project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint8_t i = 0U;
    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU);
    /* BSP key init */
    BSP_KEY_Init();

    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_FCM, ENABLE);

    RefClockInit();
    TargetClockInit();
    FcmErrorIntInit();
    FcmEndIntInit();
    FcmOvfIntInit();
    SwdtInit();

    DDL_PrintfInit(BSP_PRINTF_DEVICE, FMC_DEMO_BAUDRATE, BSP_PRINTF_Preinit);

    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_FCG | LL_PERIPH_GPIO);

    DDL_Printf("XTAL=8MHz and XTAL32=32768Hz divided by 8192 are used as the reference clock for this demo.\r\n");

    for (;;) {
        WaitKeyPress();
        m_pi8TargetClock = m_stcFcmTargetTbl[i].pi8TargetClock;
        FcmInit(m_stcFcmTargetTbl[i].u32TargetClock, m_stcFcmTargetTbl[i].u32TargetClockFreq);
        FCM_Cmd(CM_FCM, ENABLE);
        WaitFcmEnd();
        i++;
        if (i >= sizeof(m_stcFcmTargetTbl) / sizeof(m_stcFcmTargetTbl[0])) {
            i = 0;
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
