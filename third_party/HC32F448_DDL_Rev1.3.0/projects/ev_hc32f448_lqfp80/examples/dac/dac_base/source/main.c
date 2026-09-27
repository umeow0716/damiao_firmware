/**
 *******************************************************************************
 * @file  dac/dac_base/source/main.c
 * @brief Main program of DAC base for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2025-11-03       CDT             Delete DAC_DataRegAlignConfig() due to align configure in DAC_Init()
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
#include <math.h>

/**
 * @addtogroup HC32F448_DDL_Examples
 * @{
 */

/**
 * @addtogroup DAC_Base
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
typedef enum {
    E_KEY_NOT_PRESSED,
    E_KEY1_PRESSED,
    E_KEY2_PRESSED,
} en_key_event_t;

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define DAC_UNIT1_PORT                                      (GPIO_PORT_A)
#define DAC_UNIT1_CH1_PIN                                   (GPIO_PIN_04)
#define DAC_UNIT1_CH2_PIN                                   (GPIO_PIN_05)

/* config */
#define DAC_FUNCTION_CLK_GATE                               (PWC_FCG3_DAC)

#define DAC_UNIT                                            (CM_DAC)
#define DAC_CH                                              (DAC_CH1)
#define DAC_OUTPUT_PORT                                     (DAC_UNIT1_PORT)
#define DAC_OUTPUT_PIN                                      (DAC_UNIT1_CH1_PIN)

#define DAC_DATA_ALIGN                                      (DAC_DATA_ALIGN_LEFT)
#define DAC_AMP_ENABLE
#define DAC_ADP_ENABLE
#define DAC_ADP_SEL                                         (DAC_ADP_SEL_ALL)

#define SINE_DOT_NUMBER                                     (400U)
#define PI                                                  (3.1415926F)
#define DAC_AMP_MIN_VOL_0V2                                 (0x1000U/33U*2U)
#define DAC_MAX_VOL                                         (0x1000U/33U*30U)

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
 * @brief    Get key pressed event
 * @param    None
 * @retval   An en_key_event_t enumeration value
 */
static en_key_event_t Key_Event(void)
{
    en_key_event_t enEvent;

    if (SET == BSP_KEY_GetStatus(BSP_KEY_1)) {
        enEvent = E_KEY1_PRESSED;
    } else if (SET == BSP_KEY_GetStatus(BSP_KEY_2)) {
        enEvent = E_KEY2_PRESSED;
    } else {
        enEvent = E_KEY_NOT_PRESSED;
    }

    return enEvent;
}

/**
 * @brief    Sin table Initialization
 * @param    [in] pSinTable  sin table
 * @param    [in] u32count   number of pSinTable items
 * @retval   None
 */
static void SinTable_Init(uint32_t pSinTable[], uint32_t u32count)
{
    uint32_t i;
    uint32_t j;
    float32_t hd;       /* radian */
    float32_t fSin;

    j = u32count / 2U;
    hd = PI / j;
    for (i = 0U; i < u32count; i++) {
        fSin = j * sin(hd * i) + j;
        fSin *= ((float32_t)DAC_MAX_VOL / (float32_t)SINE_DOT_NUMBER);
#ifdef DAC_AMP_ENABLE
        fSin += (float32_t)DAC_AMP_MIN_VOL_0V2;
#endif
#if (DAC_DATA_ALIGN == DAC_DATA_ALIGN_LEFT)
        pSinTable[i] = (uint32_t)fSin << 4;
#else
        pSinTable[i] = (uint32_t)fSin;
#endif
    }
}

/**
 * @brief    Init DAC conversion
 * @param    None
 * @retval   None
 */
static void DAC_ConversionInit(void)
{
    stc_gpio_init_t stcGpioInit;
    stc_dac_init_t stcDacInit;

    /* Enable DAC peripheral clock. */
    FCG_Fcg3PeriphClockCmd(DAC_FUNCTION_CLK_GATE, ENABLE);

    /* Set GPIO pin for sine wave output */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinAttr = PIN_ATTR_ANALOG;
    (void)GPIO_Init(DAC_OUTPUT_PORT, DAC_OUTPUT_PIN, &stcGpioInit);

    /* Init DAC by default value: source from data register and output enabled */
    DAC_DeInit(DAC_UNIT);
    (void)DAC_StructInit(&stcDacInit);
    stcDacInit.u16Align = DAC_DATA_ALIGN;
    (void)DAC_Init(DAC_UNIT, DAC_CH, &stcDacInit);

#ifdef DAC_ADP_ENABLE
    /* Set ADC first */
    /* Enable ADC peripheral clock. */
    FCG_Fcg3PeriphClockCmd(PWC_FCG3_ADC1 | PWC_FCG3_ADC2 | PWC_FCG3_ADC3, ENABLE);
    if (CM_ADC1->STR == 0U) {
        if (CM_ADC2->STR == 0U) {
            if (CM_ADC3->STR == 0U) {
                DAC_ADCPrioConfig(DAC_UNIT, DAC_ADP_SEL, ENABLE);
                DAC_ADCPrioCmd(DAC_UNIT, ENABLE);
            }
        }
    }
#endif
}

/**
 * @brief    Set conversion data
 * @param    [in] u16Data The data table to be converted
 * @retval   None
 */
__STATIC_INLINE void DAC_SetConversionData(const uint16_t u16Data)
{
#ifdef DAC_ADP_ENABLE
    uint32_t u32TryCount;

    if (LL_ERR_BUSY == DAC_GetChConvertState(DAC_UNIT, DAC_CH)) {
        u32TryCount = 100U;
        while (u32TryCount != 0U) {
            u32TryCount--;
            if (LL_ERR_BUSY != DAC_GetChConvertState(DAC_UNIT, DAC_CH)) {
                break;
            }
        }
    }
#endif
    DAC_SetChData(DAC_UNIT, DAC_CH, u16Data);
}

/**
 * @brief    Start DAC conversion
 * @param    None
 * @retval   None
 */
static void DAC_StartConversion(void)
{
    /* Enable AMP */
#ifdef DAC_AMP_ENABLE
    DAC_SetChData(DAC_UNIT, DAC_CH, 0);
    (void)DAC_AMPCmd(DAC_UNIT, DAC_CH, ENABLE);
#endif

    (void)DAC_Start(DAC_UNIT, DAC_CH);

#ifdef DAC_AMP_ENABLE
    /* delay 3us before setting data*/
    DDL_DelayMS(1U);
#endif
}

/**
 * @brief    stop DAC conversion
 * @param    None
 * @retval   None
 */
static void DAC_StopConversion(void)
{
    (void)DAC_Stop(DAC_UNIT, DAC_CH);
}

/**
 * @brief  Main function of DAC base project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    uint16_t u16OutputCnt = 0U;
    en_key_event_t enEvent;
    uint8_t u8IsConversionStart = 0U;
    static uint32_t gu32SinTable[SINE_DOT_NUMBER];

    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                 LL_PERIPH_EFM);

    BSP_CLK_Init();
    BSP_IO_Init();
    BSP_LED_Init();
    BSP_KEY_Init();

    /* Init sine data table */
    SinTable_Init(gu32SinTable, SINE_DOT_NUMBER);

    /* Init DAC */
    DAC_ConversionInit();

    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                 LL_PERIPH_EFM);

    for (;;) {
        enEvent = Key_Event();
        switch (enEvent) {
            case E_KEY1_PRESSED:
                if (u8IsConversionStart == 1U) {
                    break;
                }
                u16OutputCnt = 0U;
                u8IsConversionStart = 1U;
                BSP_LED_On(LED_BLUE);
                DAC_StartConversion();
                break;
            case E_KEY2_PRESSED:
                if (u8IsConversionStart == 0U) {
                    break;
                }
                u8IsConversionStart = 0U;
                DAC_StopConversion();
                BSP_LED_Off(LED_BLUE);
                break;

            default:
                break;
        }

        if (u8IsConversionStart == 0U) {
            continue;
        }
        DAC_SetConversionData(gu32SinTable[u16OutputCnt]);
        /* Loop output */
        if (++u16OutputCnt >= SINE_DOT_NUMBER) {
            u16OutputCnt = 0U;
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
