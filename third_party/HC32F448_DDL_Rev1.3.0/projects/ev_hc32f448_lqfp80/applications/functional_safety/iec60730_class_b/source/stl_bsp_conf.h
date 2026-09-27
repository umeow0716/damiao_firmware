/**
 *******************************************************************************
 * @file  functional_safety/iec60730_class_b/source/stl_bsp_conf.h
 * @brief This file contains STL BSP resource configure.
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

#ifndef __STL_BSP_CONF_H__
#define __STL_BSP_CONF_H__

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h>
#include <math.h>
#include "hc32_ll.h"
#include "stl_common.h"
#include "ev_hc32f448_lqfp80_bsp.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup STL_IEC60730_Application
 * @{
 */

/**
 * @addtogroup IEC60730_STL_BSP_Configure
 * @{
 */

/*******************************************************************************
 * Global type definitions ('typedef')
*******************************************************************************/

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/**
 * @defgroup STL_Function_Configure STL Function Configure
 * @brief This is the list of function to be used in the STL Library.
 * Select the functions you need to use to STL_ON.
 * @{
 */
#define STL_PRINT_ENABLE                STL_ON
#define STL_RESET_AT_FAILURE            STL_OFF
/**
 * @}
 */

/**
 * @defgroup STL_Print_Configure STL Print Configure
 * @{
 */
#if (STL_PRINT_ENABLE == STL_ON)

#define STL_PRINTF_DEVICE               (CM_USART2)
#define STL_PRINTF_DEVICE_FCG_ENALBE()  FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_USART2, ENABLE)

#define STL_PRINTF_BAUDRATE             (115200UL)
#define STL_PRINTF_BAUDRATE_ERR_MAX     (0.025F)

#define STL_PRINTF_PORT                 (GPIO_PORT_C)
#define STL_PRINTF_PIN                  (GPIO_PIN_10)
#define STL_PRINTF_PORT_FUNC            (GPIO_FUNC_36)


#endif
/**
 * @}
 */

/**
 * @defgroup STL_RAM_Configure STL RAM Configure
 * @{
 */
#define STL_RAM1_START                  (0x1FFF8000UL)
#define STL_RAM1_END                    (0x1FFFFFFFUL)

#define STL_RAM2_START                  (0x20000000UL)
#define STL_RAM2_END                    (0x20007FFFUL)

/* Memory address must be coherent with MARCH_RAM in linker *.sct */
#define STL_MARCH_RAM_START             (0x1FFF8000UL)
#define STL_MARCH_RAM_END               (0x1FFF801FUL)
/**
 * @}
 */

/**
 * @defgroup STL_ROM_Configure STL ROM Configure
 * @{
 */
#define STL_ROM_START                   (0x00000000UL)
#define STL_ROM_END                     (0x0003FFFFUL)
#define STL_ROM_SIZE                    (ROM_END - ROM_START + 1UL)
/**
 * @}
 */

/**
 * @defgroup STL_Systick_Configure STL Systick Configure
 * @{
 */
#define STL_SYSTICK_TICK_FREQ           (100UL)                                     /* Frequency: 100Hz */
#define STL_SYSTICK_TICK_VALUE          (HCLK_VALUE / STL_SYSTICK_TICK_FREQ)        /* Period value: 10ms */
/**
 * @}
 */

/**
 * @defgroup STL_Interrupt_Resource_Configure STL Interrupt Resource Configure definition
 * @{
 */
/* Clock test (Timer0 and TimerA) */
#define STL_TMR0_OVF_INT_SRC            (INT_SRC_TMR0_2_CMP_A)
#define STL_TMR0_OVF_INT_IRQn           (INT000_IRQn)
#define STL_TMR0_OVF_IRQ_PRIO           (DDL_IRQ_PRIO_DEFAULT)

#define STL_TMRA_OVF_INT_SRC            (INT_SRC_TMRA_1_OVF)
#define STL_TMRA_OVF_INT_IRQn           (INT001_IRQn)
#define STL_TMRA_OVF_IRQ_PRIO           (DDL_IRQ_PRIO_DEFAULT)

/* Clock test (FCM) */
#define STL_FCM_ERR_INT_IRQn            (INT002_IRQn)
#define STL_FCM_ERR_IRQ_PRIO            (DDL_IRQ_PRIO_DEFAULT)

#define STL_FCM_OVF_INT_IRQn            (INT001_IRQn)
#define STL_FCM_OVF_IRQ_PRIO            (DDL_IRQ_PRIO_DEFAULT)

/* Interrupt test (four timer-x,y,z,w) */
#define STL_TMRx_OVF_INT_SRC            (INT_SRC_TMRA_2_OVF)
#define STL_TMRx_OVF_INT_IRQn           (INT010_IRQn)
#define STL_TMRx_OVF_IRQ_PRIO           (DDL_IRQ_PRIO_05)

#define STL_TMRy_OVF_INT_SRC            (INT_SRC_TMRA_3_OVF)
#define STL_TMRy_OVF_INT_IRQn           (INT011_IRQn)
#define STL_TMRy_OVF_IRQ_PRIO           (DDL_IRQ_PRIO_04)

#define STL_TMRz_OVF_INT_SRC            (INT_SRC_TMRA_4_OVF)
#define STL_TMRz_OVF_INT_IRQn           (INT012_IRQn)
#define STL_TMRz_OVF_IRQ_PRIO           (DDL_IRQ_PRIO_03)

#define STL_TMRw_OVF_INT_SRC            (INT_SRC_TMRA_5_OVF)
#define STL_TMRw_OVF_INT_IRQn           (INT013_IRQn)
#define STL_TMRw_OVF_IRQ_PRIO           (DDL_IRQ_PRIO_02)

/* ADC test */
#define STL_ADC_AWD_INT_SRC             (INT_SRC_ADC1_CMP0)
#define STL_ADC_AWD_INT_IRQn            (INT014_IRQn)
#define STL_ADC_AWD_IRQ_PRIO            (DDL_IRQ_PRIO_DEFAULT)
/**
 * @}
 */

/**
 * @defgroup STL_Clock_Test_RefClk_Resource_Configure STL Clock Test RefClk Resource Configure
 * @{
 */
#define STL_TMR0_UNIT                   (CM_TMR0_2)
#define STL_TMR0_CH                     (TMR0_CH_A)
#define STL_TMR0_FCG_ENABLE()           FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_2, ENABLE)

#define STL_TMR0_INT                    (TMR0_INT_CMP_A)
#define STL_TMR0_FLAG                   (TMR0_FLAG_CMP_A)

#define STL_TMR0_CLK_FREQ               (LRC_VALUE)
#define STL_TMR0_CLK_DIV                (TMR0_CLK_DIV1)
#define STL_TMR0_FREQ                   (10UL)   /* Frequency: 10Hz(100ms) */
#define STL_TMR0_PERIOD_VALUE           (uint16_t)((STL_TMR0_CLK_FREQ / STL_TMR0_FREQ) / (pow(2,STL_TMR0_CLK_DIV>>TMR0_BCONR_CKDIVA_POS)))
/**
 * @}
 */

/**
 * @defgroup STL_Clock_Test_SysClk_Resource_Configure STL Clock Test SysClk Resource Configure
 * @{
 */
#define STL_TMRA_UNIT                   (CM_TMRA_1)
#define STL_TMRA_FCG_ENABLE()           FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMRA_1, ENABLE)

#define STL_TMRA_MD                     (TMRA_MD_SAWTOOTH)
#define STL_TMRA_DIR                    (TMRA_DIR_UP)
#define STL_TMRA_INT                    (TMRA_INT_OVF)
#define STL_TMRA_FLAG                   (TMRA_FLAG_OVF)
#define STL_TMRA_CLK_FREQ               (CLK_GetBusClockFreq(CLK_BUS_PCLK0))
#define STL_TMRA_CLK_DIV                (TMRA_CLK_DIV8)
#define STL_TMRA_FREQ                   (1000UL)     /* Frequency: 1000Hz(1ms) */
#define STL_TMRA_PERIOD_VALUE           (uint16_t)((STL_TMRA_CLK_FREQ / STL_TMRA_FREQ) / (pow(2, STL_TMRA_CLK_DIV >> TMRA_BCSTRL_CKDIV_POS)))

/* TMRA interrupt count range: acceptable error 20% */
#define STL_TMRA_COUNT_LOW              ((uint16_t)((STL_TMRA_FREQ / STL_TMR0_FREQ) * 0.8F))
#define STL_TMRA_COUNT_HIGH             ((uint16_t)((STL_TMRA_FREQ / STL_TMR0_FREQ) * 1.2F))
/**
 * @}
 */

/**
 * @defgroup EV_Board_VCC_And_ADRef_Voltage EV_Board VCC And ADRef Voltage
 * @{
 */
#define STL_EV_BOARD_VCC                (3.3F)
#define STL_INNER_VREF                  (1.15F)
/**
 * @}
 */

/**
 * @defgroup STL_ADC_Test_Resource_Configure STL ADC Test Resource Configure
 * @{
 */
#define STL_ADC_FCG_ENABLE()            FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_ADC1, ENABLE)

#define STL_ADC_UNIT                    (CM_ADC1)
#define STL_ADC_CH                      (ADC_CH10)

#define STL_ADC_PORT                    (GPIO_PORT_C)
#define STL_ADC_PIN                     (GPIO_PIN_00)

#define STL_ADC_VREF                    (3.3F)
#define STL_ADC_AWD_LOW_THRESHOLD_VOL   (1.5F)
#define STL_ADC_AWD_HIGH_THRESHOLD_VOL  (1.7F)
#define STL_ADC_ACCURACY                (1UL << 12U)

#define STL_ADC_AWD_UNIT                (ADC_AWD0)
#define STL_ADC_AWD_FLAG                (ADC_AWD_FLAG_AWD0)
#define STL_ADC_AWD_TYPE                (ADC_AWD_INT_AWD0)
#define STL_ADC_CAL_VOL(adcVal)         ((float32_t)(adcVal) * STL_ADC_VREF) / ((float32_t)STL_ADC_ACCURACY)
#define STL_ADC_CAL_VAL(vol)            ((uint16_t)(((float32_t)(vol) * (float32_t)STL_ADC_ACCURACY) / (float32_t)STL_ADC_VREF))
#define STL_ADC_WINDOW_LOW              (STL_ADC_CAL_VAL(STL_ADC_AWD_LOW_THRESHOLD_VOL) + 8U)
#define STL_ADC_WINDOW_HIGH             (STL_ADC_CAL_VAL(STL_ADC_AWD_HIGH_THRESHOLD_VOL) - 8U)
/**
 * @}
 */

/**
 * @defgroup STL_Interrupt_Test_Resource_Configure STL Interrupt Test Resource Configure
 * @{
 */
#define STL_TMR_FCG                     (FCG2_PERIPH_TMRA_2 | FCG2_PERIPH_TMRA_3 | \
                                         FCG2_PERIPH_TMRA_4 | FCG2_PERIPH_TMRA_5)
#define STL_TMR_FCG_ENABLE()            FCG_Fcg2PeriphClockCmd(STL_TMR_FCG, ENABLE)

#define STL_TMRx_UNIT                   (CM_TMRA_2)
#define STL_TMRx_INT_OVF                (TMRA_INT_OVF)
#define STL_TMRx_FLAG_OVF               (TMRA_FLAG_OVF)
#define STL_TMRx_FREQ                   (STL_SYSTICK_TICK_FREQ / 4UL)
#define STL_TMRx_FREQ_OFFSET            (5UL)

#define STL_TMRy_UNIT                   (CM_TMRA_3)
#define STL_TMRy_INT_OVF                (TMRA_INT_OVF)
#define STL_TMRy_FLAG_OVF               (TMRA_FLAG_OVF)
#define STL_TMRy_FREQ                   (STL_SYSTICK_TICK_FREQ / 2UL)
#define STL_TMRy_FREQ_OFFSET            (5UL)

#define STL_TMRz_UNIT                   (CM_TMRA_4)
#define STL_TMRz_INT_OVF                (TMRA_INT_OVF)
#define STL_TMRz_FLAG_OVF               (TMRA_FLAG_OVF)
#define STL_TMRz_FREQ                   (STL_SYSTICK_TICK_FREQ + STL_SYSTICK_TICK_FREQ / 4UL)
#define STL_TMRz_FREQ_OFFSET            (5UL)

#define STL_TMRw_UNIT                   (CM_TMRA_5)
#define STL_TMRw_INT_OVF                (TMRA_INT_OVF)
#define STL_TMRw_FLAG_OVF               (TMRA_FLAG_OVF)
#define STL_TMRw_FREQ                   (STL_SYSTICK_TICK_FREQ + STL_SYSTICK_TICK_FREQ / 2UL)
#define STL_TMRw_FREQ_OFFSET            (5UL)
/**
 * @}
 */

/**
 * @defgroup STL_IO_Test_Resource_Configure STL IO Test Resource Configure
 * @{
 */
#define STL_INPUT_PORTx                 (GPIO_PORT_B)
#define STL_INPUT_PORTx_ALL_PINS        (GPIO_PIN_B_ALL)
#define STL_INPUT_PORTx_TEST_PINS       (GPIO_PIN_06)
#define STL_INPUT_PORTx_EXPECT_VAL      (STL_INPUT_PORTx_TEST_PINS)

#define STL_OUTPUT_PORTx                (GPIO_PORT_A)
#define STL_OUTPUT_PORTx_ALL_PINS       (GPIO_PIN_A_ALL)
#define STL_OUTPUT_PORTx_TEST_PINS      (GPIO_PIN_02)
#define STL_OUTPUT_PORTx_OUT_VAL        (STL_OUTPUT_PORTx_TEST_PINS)
#define STL_OUTPUT_PORTx_EXPECT_VAL     (STL_OUTPUT_PORTx_TEST_PINS)
/**
 * @}
 */

/**
 * @defgroup STL_XTAL32_CONFIG STL XTAL32 Configure definition
 * @{
 */
#define STL_XTAL32_PORT                  (GPIO_PORT_C)
#define STL_XTAL32_IN_PIN                (GPIO_PIN_14)
#define STL_XTAL32_OUT_PIN               (GPIO_PIN_15)
/**
 * @}
 */

/**
 * @defgroup STL_RED_LED_DEF STL RED_LED definition
 * @{
 */
#define STL_RED_LED_INIT()              do {                                   \
    BSP_IO_Init();                                                             \
    BSP_LED_Init();                                                            \
} while (0)

#define STL_RED_LED_OFF()               (BSP_LED_Off(LED_RED))
#define STL_RED_LED_ON()                (BSP_LED_On(LED_RED))

#define STL_CLK_TEST_PORT              (GPIO_PORT_A)
#define STL_CLK_TEST_PIN               (GPIO_PIN_01)

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @brief STL test safety failure handle
 * @param None
 * @retval None
 */
__STATIC_INLINE void STL_SafetyFailure(void)
{
    STL_RED_LED_ON();
#if (STL_RESET_AT_FAILURE == STL_ON)
    NVIC_SystemReset(); /* Generate system reset */
#endif
}

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __STL_BSP_CONF_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
