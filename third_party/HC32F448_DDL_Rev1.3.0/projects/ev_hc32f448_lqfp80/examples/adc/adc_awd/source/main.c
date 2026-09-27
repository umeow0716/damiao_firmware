/**
 *******************************************************************************
 * @file  adc/adc_awd/source/main.c
 * @brief Main program ADC analog watchdog for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Set XTAL as system clock source
   2025-11-03       CDT             Modify macro TMR0_CMP_VAL value
                                    Use BSP_PRINTF_BAUDRATE to replace immediate value 115200
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
 * @addtogroup ADC_Analog_Watchdog
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* ADC unit instance for this example. */
#define ADC_UNIT                        (CM_ADC1)
#define ADC_PERIPH_CLK                  (FCG3_PERIPH_ADC1)

/* AWD0 used in this example. */
#define AWD_UNIT                        (ADC_AWD0)

/* Selects ADC channels that needed. */
#define ADC_AWD_CH                      (ADC_CH10)
#define ADC_AWD_CH_PORT                 (GPIO_PORT_C)
#define ADC_AWD_CH_PIN                  (GPIO_PIN_00)

/* ADC sequence to be used. */
#define ADC_SEQ                         (ADC_SEQ_B)
/* Flag of conversion end. */
#define ADC_EOC_FLAG                    (ADC_FLAG_EOCB)

/* Hard trigger of the specified sequence. */
#define ADC_SEQ_HARDTRIG                (ADC_HARDTRIG_EVT0)
#define ADC_SEQ_AOS_TRIG_SEL            (AOS_ADC1_0)
#define ADC_SEQ_TRIG_EVT                (EVT_SRC_TMR0_1_CMP_B)

/* Timer0 for trigger */
#define TMR0_UNIT                       (CM_TMR0_1)
#define TMR0_CH                         (TMR0_CH_B)
#define TMR0_PERIPH_CLK                 (FCG2_PERIPH_TMR0_1)
#define TMR0_CLK_DIV                    (TMR0_CLK_DIV256)
/* Period 1 Second, TMR0_CMP_VAL = Timer0 clock / div - 1 */
#define TMR0_CMP_VAL                    (CLK_GetBusClockFreq(CLK_BUS_PCLK1) / (1UL << (TMR0_CLK_DIV >> TMR0_BCONR_CKDIVA_POS)) - 1UL)
#define TMR0_PERIPH_ENABLE()            FCG_Fcg2PeriphClockCmd(TMR0_PERIPH_CLK, ENABLE)

/* ADC reference voltage. The voltage of pin VREFH. */
#define ADC_VREF                        (3.3F)

/* ADC accuracy(according to the resolution of ADC). */
#define ADC_ACCURACY                    (1UL << 12U)

/* Calculate the voltage(mV). */
#define ADC_CAL_VOL(adcVal)             (uint16_t)((((float32_t)(adcVal) * ADC_VREF) / ((float32_t)ADC_ACCURACY)) * 1000.F)
#define ADC_CAL_VAL(vol)                ((uint16_t)(((float32_t)(vol) * (float32_t)ADC_ACCURACY) / (float32_t)ADC_VREF))

/**
 * AWD compare mode and compare windows.
 * AWD0_LOW_THRESHOLD_VOL <= The voltage of AWD0 pin <= AWD0_HIGH_THRESHOLD_VOL: The flag of AWD0 set.
 */
#define AWD_LOW_THRESHOLD_VOL           (1.5F)
#define AWD_HIGH_THRESHOLD_VOL          (2.5F)

#define ADC_AWD_MD                      (ADC_AWD_MD_CMP_IN)
#define ADC_AWD_LOW_THRESHOLD_VAL       (ADC_CAL_VAL(AWD_LOW_THRESHOLD_VOL))
#define ADC_AWD_HIGH_THRESHOLD_VAL      (ADC_CAL_VAL(AWD_HIGH_THRESHOLD_VOL))

/* Interrupt */
#define ADC_AWD_INT_IRQn                (ADC1_IRQn)
#define ADC_AWD_INT_PRIO                (DDL_IRQ_PRIO_03)
#define ADC_IRQ_HANDLER                 ADC1_Handler

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void SystemClockConfig(void);

static void AdcConfig(void);
static void AdcInitConfig(void);
static void AdcSetPinAnalogMode(void);
static void AdcAwdConfig(void);
static void AdcHardTriggerConfig(void);
static void AdcIrqConfig(void);
static void AdcHardTriggerStart(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
__IO static uint32_t m_u32AdcIrqFlag = 0UL;
static uint16_t m_u16AdcValueAwd;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Main function of ADC analog watchdog project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);
    /* System clock config */
    SystemClockConfig();
    /* Initializes UART for debug printing. Baudrate is 115200. */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
    /* Configures ADC. */
    AdcConfig();
    /* MCU Peripheral registers write protected. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);
    /* Start the peripheral which is used to trigger ADC. */
    AdcHardTriggerStart();

    /***************** Configuration end, application start **************/
    for (;;) {
        if (ADC_GetStatus(ADC_UNIT, ADC_EOC_FLAG) == SET) {
            ADC_ClearStatus(ADC_UNIT, ADC_EOC_FLAG);
            m_u16AdcValueAwd = ADC_GetValue(ADC_UNIT, ADC_AWD_CH);
            if ((m_u32AdcIrqFlag & ADC_AWD_FLAG_AWD0) != 0UL) {
                DDL_Printf("--->> AWD flag set, voltage is %u mV\r\n", ADC_CAL_VOL(m_u16AdcValueAwd));
                m_u32AdcIrqFlag &= (uint32_t)(~ADC_AWD_FLAG_AWD0);
            } else {
                DDL_Printf("AWD voltage is %u mV\r\n", ADC_CAL_VOL(m_u16AdcValueAwd));
            }
        }
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
 * @brief  ADC configuration.
 * @param  None
 * @retval None
 */
static void AdcConfig(void)
{
    AdcInitConfig();
    AdcAwdConfig();
    AdcIrqConfig();
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
    /* 2. Modify the default value depends on the application. Not needed here. */
    (void)ADC_StructInit(&stcAdcInit);
    stcAdcInit.u16ScanMode = ADC_MD_SEQA_SEQB_SINGLESHOT;
    /* 3. Initializes ADC. */
    (void)ADC_Init(ADC_UNIT, &stcAdcInit);
    /* 4. ADC channel configuration. */
    /* 4.1 Set the ADC pin to analog input mode. */
    AdcSetPinAnalogMode();
    /* 4.2 Enable ADC channels. */
    ADC_ChCmd(ADC_UNIT, ADC_SEQ, ADC_AWD_CH, ENABLE);
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
    (void)GPIO_Init(ADC_AWD_CH_PORT, ADC_AWD_CH_PIN, &stcGpioInit);
}

/**
 * @brief  AWD configuration.
 * @param  None
 * @retval None
 */
static void AdcAwdConfig(void)
{
    stc_adc_awd_config_t stcAwd;

    stcAwd.u16WatchdogMode  = ADC_AWD_MD;
    stcAwd.u16LowThreshold  = ADC_AWD_LOW_THRESHOLD_VAL;
    stcAwd.u16HighThreshold = ADC_AWD_HIGH_THRESHOLD_VAL;
    (void)ADC_AWD_Config(ADC_UNIT, AWD_UNIT, ADC_AWD_CH, &stcAwd);
    ADC_AWD_Cmd(ADC_UNIT, AWD_UNIT, ENABLE);
}

/**
 * @brief  ADC hard trigger configuration.
 * @param  None
 * @retval None
 */
static void AdcHardTriggerConfig(void)
{
    stc_tmr0_init_t stcTmr0Init;

    TMR0_PERIPH_ENABLE();
    /* Initials TIMER0. */
    (void)TMR0_StructInit(&stcTmr0Init);
    stcTmr0Init.u32ClockDiv     = TMR0_CLK_DIV;
    stcTmr0Init.u16CompareValue = (uint16_t)TMR0_CMP_VAL;
    (void)TMR0_Init(TMR0_UNIT, TMR0_CH, &stcTmr0Init);

    /* Specifies the event defined by 'ADC_SEQ_TRIG_EVT' as the hard trigger of the specified sequence. */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);
    ADC_TriggerConfig(ADC_UNIT, ADC_SEQ, ADC_SEQ_HARDTRIG);
    AOS_SetTriggerEventSrc(ADC_SEQ_AOS_TRIG_SEL, ADC_SEQ_TRIG_EVT);
    ADC_TriggerCmd(ADC_UNIT, ADC_SEQ, ENABLE);
}

/**
 * @brief  Start the peripheral which is used to trigger ADC.
 * @param  None
 * @retval None
 */
static void AdcHardTriggerStart(void)
{
    TMR0_Start(TMR0_UNIT, TMR0_CH);
}

/**
 * @brief  ADC IRQ configuration.
 * @param  None
 * @retval None
 */
static void AdcIrqConfig(void)
{
    NVIC_ClearPendingIRQ(ADC_AWD_INT_IRQn);
    NVIC_SetPriority(ADC_AWD_INT_IRQn, ADC_AWD_INT_PRIO);
    NVIC_EnableIRQ(ADC_AWD_INT_IRQn);
    ADC_AWD_IntCmd(ADC_UNIT, ADC_AWD_INT_AWD0, ENABLE);
}

/**
 * @brief  AWD0 IRQ callback.
 * @param  None
 * @retval None
 */
void ADC_IRQ_HANDLER(void)
{
    if (ADC_AWD_GetStatus(ADC_UNIT, ADC_AWD_FLAG_AWD0) == SET) {
        ADC_AWD_ClearStatus(ADC_UNIT, ADC_AWD_FLAG_AWD0);
        m_u32AdcIrqFlag |= ADC_AWD_FLAG_AWD0;
    }

    __DSB();  /* Arm Errata 838869 */
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
