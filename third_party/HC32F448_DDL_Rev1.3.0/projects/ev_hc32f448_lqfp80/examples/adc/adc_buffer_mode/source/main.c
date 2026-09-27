/**
 *******************************************************************************
 * @file  adc/adc_buffer_mode/source/main.c
 * @brief Main program ADC data buffer mode for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Set XTAL as system clock source
   2025-11-03       CDT             Use BSP_PRINTF_BAUDRATE to replace immediate value 115200
                                    Use BSP_XTAL_PIN to replace BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN
                                    Use ADC_EOC_INT to replace ADC_EOS_INT
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
 * @addtogroup ADC_Buffer_Mode
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

/* Selects ADC channels that needed. */
#define ADC_CH_POTENTIOMETER            (ADC_CH10)
#define ADC_PIN_POTENTIOMETER           (ADC1_PIN_PC0)
#define ADC_CH_PORT                     (GPIO_PORT_C)
#define ADC_CH_PIN                      (GPIO_PIN_00)

/* Potentiometer pin PC0, remap to ADC_CH2 and ADC_CH3. */
#define ADC_REMAP_PIN                   (ADC_PIN_POTENTIOMETER)
#define ADC_REMAP_PIN_CH2               (ADC_CH2)
#define ADC_REMAP_PIN_CH3               (ADC_CH3)

/* ADC sequence to be used. */
#define ADC_SEQ                         (ADC_SEQ_A)
/* Flag of conversion end. */
#define ADC_EOC_FLAG                    (ADC_FLAG_EOCA)
#define ADC_EOC_INT                     (ADC_INT_EOCA)

/* Hard trigger of the specified sequence. */
#define ADC_SEQ_HARDTRIG                (ADC_HARDTRIG_ADTRG_PIN)
#define ADC_SEQ_TRIG_PORT               (GPIO_PORT_E)
#define ADC_SEQ_TRIG_PIN                (GPIO_PIN_14)
#define ADC_SEQ_TRIG_PIN_FUNC           (GPIO_FUNC_1)

/* ADC reference voltage. The voltage of pin VREFH. */
#define ADC_VREF                        (3.3F)

/* ADC accuracy(according to the resolution of ADC). */
#define ADC_ACCURACY                    (1UL << 12U)

/* Calculate the voltage(mV). */
#define ADC_CAL_VOL(adcVal)             (uint16_t)((((float32_t)(adcVal) * ADC_VREF) / ((float32_t)ADC_ACCURACY)) * 1000.F)

/* Timeout value. */
#define ADC_TIMEOUT_VAL                 (1000U)

/* Interrupt */
#define ADC_INT_IRQn                    (ADC1_IRQn)
#define ADC_INT_PRIO                    (DDL_IRQ_PRIO_03)
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
static void AdcHardTriggerConfig(void);
static void AdcIrqConfig(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
__IO static uint32_t m_u32AdcIrqFlag = 0UL;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Main function of ADC buffer mode project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint16_t u16AdcValA, u16AdcValB;
    uint16_t u16AdcAverage;

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

    /***************** Configuration end, application start **************/
    for (;;) {
        if (m_u32AdcIrqFlag != 0UL) {
            m_u32AdcIrqFlag = 0U;
            u16AdcValA = ADC_GetValue(ADC_UNIT, ADC_REMAP_PIN_CH2);
            DDL_Printf("First:   The ADC value of potentiometer is %u, voltage is %u mV\r\n",
                       u16AdcValA, ADC_CAL_VOL(u16AdcValA));
            u16AdcValB = ADC_GetValue(ADC_UNIT, ADC_REMAP_PIN_CH3);
            DDL_Printf("Second:  The ADC value of potentiometer is %u, voltage is %u mV\r\n",
                       u16AdcValB, ADC_CAL_VOL(u16AdcValB));
            u16AdcAverage = (u16AdcValA + u16AdcValB) / 2U;
            DDL_Printf("Average: The ADC value of potentiometer is %u, voltage is %u mV\r\n",
                       u16AdcAverage, ADC_CAL_VOL(u16AdcAverage));
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
    stcAdcInit.u16ScanMode = ADC_MD_SEQA_BUF;
    /* 3. Initializes ADC. */
    (void)ADC_Init(ADC_UNIT, &stcAdcInit);
    /* 4. ADC channel configuration. */
    /* 4.1 Set the ADC pin to analog input mode. */
    AdcSetPinAnalogMode();
    /* 4.2 Channel remapping */
    ADC_ChRemap(ADC_UNIT, ADC_REMAP_PIN_CH2, ADC_REMAP_PIN);
    ADC_ChRemap(ADC_UNIT, ADC_REMAP_PIN_CH3, ADC_REMAP_PIN);
    /* 4.2 Enable ADC channels. */
    ADC_ChCmd(ADC_UNIT, ADC_SEQ, ADC_REMAP_PIN_CH2, ENABLE);
    ADC_ChCmd(ADC_UNIT, ADC_SEQ, ADC_REMAP_PIN_CH3, ENABLE);
    /* 5. Conversion data average calculation function, if needed.
          Call ADC_ConvDataAverageChCmd() again to enable more average channels if needed. */
    ADC_ConvDataAverageConfig(ADC_UNIT, ADC_AVG_CNT8);
    ADC_ConvDataAverageChCmd(ADC_UNIT, ADC_REMAP_PIN_CH2, ENABLE);
    ADC_ConvDataAverageChCmd(ADC_UNIT, ADC_REMAP_PIN_CH3, ENABLE);
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
    (void)GPIO_Init(ADC_CH_PORT, ADC_CH_PIN, &stcGpioInit);
}

/**
 * @brief  ADC hard trigger configuration.
 * @param  None
 * @retval None
 */
static void AdcHardTriggerConfig(void)
{
    GPIO_SetFunc(ADC_SEQ_TRIG_PORT, ADC_SEQ_TRIG_PIN, ADC_SEQ_TRIG_PIN_FUNC);
    ADC_TriggerConfig(ADC_UNIT, ADC_SEQ, ADC_SEQ_HARDTRIG);
    ADC_TriggerCmd(ADC_UNIT, ADC_SEQ, ENABLE);
}

/**
 * @brief  ADC IRQ configuration.
 * @param  None
 * @retval None
 */
static void AdcIrqConfig(void)
{
    NVIC_ClearPendingIRQ(ADC_INT_IRQn);
    NVIC_SetPriority(ADC_INT_IRQn, ADC_INT_PRIO);
    NVIC_EnableIRQ(ADC_INT_IRQn);
    ADC_IntCmd(ADC_UNIT, ADC_EOC_INT, ENABLE);
}

/**
 * @brief  AWD0 IRQ callback.
 * @param  None
 * @retval None
 */
void ADC_IRQ_HANDLER(void)
{
    if (ADC_GetStatus(ADC_UNIT, ADC_EOC_FLAG) == SET) {
        ADC_ClearStatus(ADC_UNIT, ADC_EOC_FLAG);
        m_u32AdcIrqFlag = 1U;
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
