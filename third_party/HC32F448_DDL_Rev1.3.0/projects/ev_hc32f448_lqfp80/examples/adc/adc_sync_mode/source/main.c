/**
 *******************************************************************************
 * @file  adc/adc_sync_mode/source/main.c
 * @brief Main program ADC sync mode for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Removed SRAM wait cycle relevant code
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
 * @addtogroup ADC_Sync_Mode
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* 'ADC_SYNC_ADC1_ADC2' and 'ADC_SYNC_ADC1_ADC2_ADC3' can be used. */
#define ADC_SYNC_UNITS                  (ADC_SYNC_ADC1_ADC2)

/* ADC synchronous mode. @ref ADC_Sync_Mode */
#define ADC_SYNC_MD                     (ADC_SYNC_SINGLE_DELAY_TRIG)

#if (ADC_SYNC_MD == ADC_SYNC_SINGLE_DELAY_TRIG)
#define ADC_SYNC_SAMPLE_TIME            (11U)
#define ADC_SYNC_TRIG_DELAY             (12U)
#define ADC_MD                          (ADC_MD_SEQA_CONT)
#elif (ADC_SYNC_MD == ADC_SYNC_SINGLE_PARALLEL_TRIG)
#define ADC_SYNC_SAMPLE_TIME            (11U)
#define ADC_SYNC_TRIG_DELAY             (0U)
#define ADC_MD                          (ADC_MD_SEQA_CONT)
#elif (ADC_SYNC_MD == ADC_SYNC_CYCLIC_DELAY_TRIG)
#define ADC_SYNC_SAMPLE_TIME            (11U)
#define ADC_SYNC_TRIG_DELAY             (60U)
#define ADC_MD                          (ADC_MD_SEQA_SINGLESHOT)
#elif (ADC_SYNC_MD == ADC_SYNC_CYCLIC_PARALLEL_TRIG)
#define ADC_SYNC_SAMPLE_TIME            (11U)
#define ADC_SYNC_TRIG_DELAY             (120U)
#define ADC_MD                          (ADC_MD_SEQA_SINGLESHOT)
#else
#error "This sync mode is NOT supported."
#endif

/* ADC peripheral clock for this example. */
#if (ADC_SYNC_UNITS == ADC_SYNC_ADC1_ADC2)
#define ADC_PERIPH_CLK                  (FCG3_PERIPH_ADC1 | FCG3_PERIPH_ADC2)
#elif (ADC_SYNC_UNITS == ADC_SYNC_ADC1_ADC2_ADC3)
#define ADC_PERIPH_CLK                  (FCG3_PERIPH_ADC1 | FCG3_PERIPH_ADC2 | FCG3_PERIPH_ADC3)
#endif

/**
 * ADC channels definition for this example.
 * NOTE!!! DO NOT enable sequence B, it will disturb the synchronization timing.
 */
#if (ADC_SYNC_UNITS == ADC_SYNC_ADC1_ADC2) && (ADC_SYNC_MD == ADC_SYNC_SINGLE_DELAY_TRIG)
/* These definitions just for a sampling rate of 5Msps. */
#define ADC1_CH                         (ADC_CH10)
#define ADC2_CH                         (ADC_CH6)
#define ADC_CH_PORT                     (GPIO_PORT_C)
#define ADC_CH_PIN                      (GPIO_PIN_00)
#else
#define ADC1_CHX                        (ADC_CH1)
#define ADC1_CHX_PORT                   (GPIO_PORT_A)
#define ADC1_CHX_PIN                    (GPIO_PIN_01)
#define ADC1_CHY                        (ADC_CH2)
#define ADC1_CHY_PORT                   (GPIO_PORT_A)
#define ADC1_CHY_PIN                    (GPIO_PIN_02)
#define ADC1_CHZ                        (ADC_CH4)
#define ADC1_CHZ_PORT                   (GPIO_PORT_A)
#define ADC1_CHZ_PIN                    (GPIO_PIN_04)

#define ADC2_CHX                        (ADC_CH1)
#define ADC2_CHX_PORT                   (GPIO_PORT_A)
#define ADC2_CHX_PIN                    (GPIO_PIN_05)
#define ADC2_CHY                        (ADC_CH2)
#define ADC2_CHY_PORT                   (GPIO_PORT_A)
#define ADC2_CHY_PIN                    (GPIO_PIN_06)
#define ADC2_CHZ                        (ADC_CH3)
#define ADC2_CHZ_PORT                   (GPIO_PORT_A)
#define ADC2_CHZ_PIN                    (GPIO_PIN_07)

#define ADC3_CHX                        (ADC_CH1)
#define ADC3_CHX_PORT                   (GPIO_PORT_E)
#define ADC3_CHX_PIN                    (GPIO_PIN_03)
#define ADC3_CHY                        (ADC_CH3)
#define ADC3_CHY_PORT                   (GPIO_PORT_E)
#define ADC3_CHY_PIN                    (GPIO_PIN_12)
#define ADC3_CHZ                        (ADC_CH10)
#define ADC3_CHZ_PORT                   (GPIO_PORT_E)
#define ADC3_CHZ_PIN                    (GPIO_PIN_15)
#endif

/* ADC channel sampling time */
#define ADC_SAMPLE_TIME                 ADC_SYNC_SAMPLE_TIME

/* Hard trigger */
#define ADC_SEQ_HARDTRIG                (ADC_HARDTRIG_ADTRG_PIN)
#define ADC_SEQ_TRIG_PORT               (GPIO_PORT_E)
#define ADC_SEQ_TRIG_PIN                (GPIO_PIN_14)
#define ADC_SEQ_TRIG_PIN_FUNC           (GPIO_FUNC_1)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void AdcConfig(void);
static void AdcInitConfig(void);
static void AdcSetPinAnalogMode(void);
static void AdcSyncConfig(void);
static void AdcHardTriggerConfig(void);
static void IndicateConfig(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  BSP clock initialize.
 *         SET board system clock to PLLH@200MHz
 * @param  None
 * @retval None
 */
void BSP_CLK_Init(void)
{
    stc_clock_xtal_init_t stcXtalInit;
    stc_clock_pll_init_t stcPLLHInit;

    /* PCLK0, HCLK Max 200MHz */
    /* PCLK1, PCLK4, EX BUS Max 100MHz */
    /* PCLK2 Max 60MHz */
    /* PCLK3 Max 50MHz */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL,
                    (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV4 |
                     CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2 | CLK_EXCLK_DIV2 |
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
    /* VCO = (8/1)*75 = 600MHz*/
    stcPLLHInit.u8PLLState = CLK_PLL_ON;
    stcPLLHInit.PLLCFGR = 0UL;
    stcPLLHInit.PLLCFGR_f.PLLM = (1UL  - 1UL);
    stcPLLHInit.PLLCFGR_f.PLLN = (75UL - 1UL);
    stcPLLHInit.PLLCFGR_f.PLLP = (3UL  - 1UL);
    stcPLLHInit.PLLCFGR_f.PLLQ = (10UL - 1UL);
    stcPLLHInit.PLLCFGR_f.PLLR = (10UL - 1UL);
    stcPLLHInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcPLLHInit);

    /* 3 cycles for 150 ~ 200MHz */
    (void)EFM_SetWaitCycle(EFM_WAIT_CYCLE3);
    /* 3 cycles for 150 ~ 200MHz */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT3);
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);
}

/**
 * @brief  Main function of adc_sync_mode project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                 LL_PERIPH_EFM);
    /* Configures the system clock */
    BSP_CLK_Init();
    /* Configures ADC. */
    AdcConfig();
    /* Use the PWM of TimerA to indicate the scan timing of ADC synchronous mode. */
    IndicateConfig();
    /* MCU Peripheral registers write protected. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                 LL_PERIPH_EFM);

    /***************** Configuration end, application start **************/
    for (;;) {
    }
}

/**
 * @brief  ADC configuration.
 * @param  None
 * @retval None
 */
static void AdcConfig(void)
{
    AdcInitConfig();
    AdcSyncConfig();
    AdcHardTriggerConfig();
}

/**
 * @brief  Initializes ADC.
 * @param  None
 * @retval None
 */
static void AdcInitConfig(void)
{
    stc_adc_init_t stcAdcInit;

    /* 1. Enable ADC peripheral clock. */
    FCG_Fcg3PeriphClockCmd(ADC_PERIPH_CLK, ENABLE);
    /* 2. Modify the default value depends on the application. */
    (void)ADC_StructInit(&stcAdcInit);
    stcAdcInit.u16ScanMode = ADC_MD;
    /* 3. Initializes ADC. */
    (void)ADC_Init(CM_ADC1, &stcAdcInit);
    (void)ADC_Init(CM_ADC2, &stcAdcInit);
#if (ADC_SYNC_UNITS == ADC_SYNC_ADC1_ADC2_ADC3)
    (void)ADC_Init(CM_ADC3, &stcAdcInit);
#endif
    /* 4. ADC channel configuration. */
    /* 4.1 Set the ADC pin to analog input mode. */
    AdcSetPinAnalogMode();
    /* 4.2 Enable ADC channels and set sample time. */
#if (ADC_SYNC_UNITS == ADC_SYNC_ADC1_ADC2) && (ADC_SYNC_MD == ADC_SYNC_SINGLE_DELAY_TRIG)
    ADC_ChCmd(CM_ADC1, ADC_SEQ_A, ADC1_CH, ENABLE);
    ADC_ChCmd(CM_ADC2, ADC_SEQ_A, ADC2_CH, ENABLE);
    ADC_SetSampleTime(CM_ADC1, ADC1_CH, ADC_SAMPLE_TIME);
    ADC_SetSampleTime(CM_ADC2, ADC2_CH, ADC_SAMPLE_TIME);
#else
    ADC_ChCmd(CM_ADC1, ADC_SEQ_A, ADC1_CHX, ENABLE);
    ADC_ChCmd(CM_ADC1, ADC_SEQ_A, ADC1_CHY, ENABLE);
    ADC_ChCmd(CM_ADC1, ADC_SEQ_A, ADC1_CHZ, ENABLE);

    ADC_ChCmd(CM_ADC2, ADC_SEQ_A, ADC2_CHX, ENABLE);
    ADC_ChCmd(CM_ADC2, ADC_SEQ_A, ADC2_CHY, ENABLE);
    ADC_ChCmd(CM_ADC2, ADC_SEQ_A, ADC2_CHZ, ENABLE);

    ADC_SetSampleTime(CM_ADC1, ADC1_CHX, ADC_SAMPLE_TIME);
    ADC_SetSampleTime(CM_ADC1, ADC1_CHY, ADC_SAMPLE_TIME);
    ADC_SetSampleTime(CM_ADC1, ADC1_CHZ, ADC_SAMPLE_TIME);

    ADC_SetSampleTime(CM_ADC2, ADC2_CHX, ADC_SAMPLE_TIME);
    ADC_SetSampleTime(CM_ADC2, ADC2_CHY, ADC_SAMPLE_TIME);
    ADC_SetSampleTime(CM_ADC2, ADC2_CHZ, ADC_SAMPLE_TIME);
#if (ADC_SYNC_UNITS == ADC_SYNC_ADC1_ADC2_ADC3)
    ADC_ChCmd(CM_ADC3, ADC_SEQ_A, ADC3_CHX, ENABLE);
    ADC_ChCmd(CM_ADC3, ADC_SEQ_A, ADC3_CHY, ENABLE);
    ADC_ChCmd(CM_ADC3, ADC_SEQ_A, ADC3_CHZ, ENABLE);

    ADC_SetSampleTime(CM_ADC3, ADC3_CHX, ADC_SAMPLE_TIME);
    ADC_SetSampleTime(CM_ADC3, ADC3_CHY, ADC_SAMPLE_TIME);
    ADC_SetSampleTime(CM_ADC3, ADC3_CHZ, ADC_SAMPLE_TIME);
#endif
#endif
    /* 5. Specifies the clock source of ADC. */
    CLK_SetPeriClockSrc(CLK_PERIPHCLK_PLLQ);
}

/**
 * @brief  Set specified ADC pin to analog mode.
 * @param  None
 * @retval None
 */
static void AdcSetPinAnalogMode(void)
{
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinAttr = PIN_ATTR_ANALOG;
#if (ADC_SYNC_UNITS == ADC_SYNC_ADC1_ADC2) && (ADC_SYNC_MD == ADC_SYNC_SINGLE_DELAY_TRIG)
    (void)GPIO_Init(ADC_CH_PORT, ADC_CH_PIN, &stcGpioInit);
#else
    (void)GPIO_Init(ADC1_CHX_PORT, ADC1_CHX_PIN, &stcGpioInit);
    (void)GPIO_Init(ADC1_CHY_PORT, ADC1_CHY_PIN, &stcGpioInit);
    (void)GPIO_Init(ADC1_CHZ_PORT, ADC1_CHZ_PIN, &stcGpioInit);

    (void)GPIO_Init(ADC2_CHX_PORT, ADC2_CHX_PIN, &stcGpioInit);
    (void)GPIO_Init(ADC2_CHY_PORT, ADC2_CHY_PIN, &stcGpioInit);
    (void)GPIO_Init(ADC2_CHZ_PORT, ADC2_CHZ_PIN, &stcGpioInit);
#if (ADC_SYNC_UNITS == ADC_SYNC_ADC1_ADC2_ADC3)
    (void)GPIO_Init(ADC3_CHX_PORT, ADC3_CHX_PIN, &stcGpioInit);
    (void)GPIO_Init(ADC3_CHY_PORT, ADC3_CHY_PIN, &stcGpioInit);
    (void)GPIO_Init(ADC3_CHZ_PORT, ADC3_CHZ_PIN, &stcGpioInit);
#endif
#endif
}

/**
 * @brief  Synchronous mode configuration.
 * @param  None
 * @retval None
 */
static void AdcSyncConfig(void)
{
    ADC_SyncModeConfig(ADC_SYNC_UNITS, ADC_SYNC_MD, ADC_SYNC_TRIG_DELAY);
    ADC_SyncModeCmd(ENABLE);
}

/**
 * @brief  ADC hard trigger configuration.
 * @param  None
 * @retval None
 */
static void AdcHardTriggerConfig(void)
{
    GPIO_SetFunc(ADC_SEQ_TRIG_PORT, ADC_SEQ_TRIG_PIN, ADC_SEQ_TRIG_PIN_FUNC);
    ADC_TriggerConfig(CM_ADC1, ADC_SEQ_A, ADC_SEQ_HARDTRIG);
    ADC_TriggerCmd(CM_ADC1, ADC_SEQ_A, ENABLE);
}

/**
 * @brief  Use PWM of TimerA to indicate the scan timing of ADC synchronous mode.
 *         The inversion of PWM level indicates the end of ADC scanning.
 *         ADC1: TMRA1, PMW2, PA9,  Func4
 *         ADC2: TMRA2, PMW2, PC13, Func5
 *         ADC3: TMRA3, PMW2, PB14, Func6
 * @param  None
 * @retval None
 */
static void IndicateConfig(void)
{
    stc_tmra_init_t stcTmraInit;
    stc_tmra_pwm_init_t stcPwmInit;

    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);
    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMRA_1 | FCG2_PERIPH_TMRA_2 | FCG2_PERIPH_TMRA_3, ENABLE);

    (void)TMRA_StructInit(&stcTmraInit);
    stcTmraInit.u8CountSrc = TMRA_CNT_SRC_HW;
    stcTmraInit.hw_count.u16CountUpCond   = TMRA_CNT_UP_COND_EVT;
    stcTmraInit.hw_count.u16CountDownCond = TMRA_CNT_DOWN_COND_INVD;
    stcTmraInit.u32PeriodValue = 0U;
    (void)TMRA_Init(CM_TMRA_1, &stcTmraInit);
    (void)TMRA_Init(CM_TMRA_2, &stcTmraInit);
    (void)TMRA_Init(CM_TMRA_3, &stcTmraInit);

    AOS_SetTriggerEventSrc(AOS_TMRA_0, EVT_SRC_ADC1_EOCA);
    AOS_SetTriggerEventSrc(AOS_TMRA_1, EVT_SRC_ADC2_EOCA);
    AOS_SetTriggerEventSrc(AOS_TMRA_2, EVT_SRC_ADC3_EOCA);

    (void)TMRA_PWM_StructInit(&stcPwmInit);
    stcPwmInit.u32CompareValue = 0U;
    (void)TMRA_PWM_Init(CM_TMRA_1, TMRA_CH2, &stcPwmInit);
    (void)TMRA_PWM_Init(CM_TMRA_2, TMRA_CH2, &stcPwmInit);
    (void)TMRA_PWM_Init(CM_TMRA_3, TMRA_CH2, &stcPwmInit);
    TMRA_PWM_OutputCmd(CM_TMRA_1, TMRA_CH2, ENABLE);
    TMRA_PWM_OutputCmd(CM_TMRA_2, TMRA_CH2, ENABLE);
    TMRA_PWM_OutputCmd(CM_TMRA_3, TMRA_CH2, ENABLE);

    GPIO_SetFunc(GPIO_PORT_A, GPIO_PIN_09, GPIO_FUNC_4);
    GPIO_SetFunc(GPIO_PORT_C, GPIO_PIN_13, GPIO_FUNC_5);
    GPIO_SetFunc(GPIO_PORT_B, GPIO_PIN_14, GPIO_FUNC_6);
    TMRA_Start(CM_TMRA_1);
    TMRA_Start(CM_TMRA_2);
    TMRA_Start(CM_TMRA_3);
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
