/**
 *******************************************************************************
 * @file  emb/emb_sram_brake_timer4/source/main.c
 * @brief This example demonstrates how to use SRAM error brake function of
 *        EMB function.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Modify TMR4_PwmConfig: enable main output following PWM initialization
                                    Optimize the 2nd data in SRAM_GenerateError()
   2025-11-03       CDT             Fix Timer PWM init channel typo
                                    Optimize EMB IRQ callback register
                                    Change SAMPLE_FUNC value to SAMPLE_FUNC_SRAM_ECC_CHECK
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
 * @addtogroup EMB_SRAM_Brake_TMR4
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/* Peripheral register WE/WP selection */
#define LL_PERIPH_SEL                   (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                                         LL_PERIPH_EFM | LL_PERIPH_SRAM)

/* Key pin definition */
#define KEY_PORT                        (GPIO_PORT_B)
#define KEY_PIN                         (GPIO_PIN_06)

/* Function of SRAM checking definitions. */
#define SAMPLE_FUNC_SRAM_PARITY_CHECK   (1U)
#define SAMPLE_FUNC_SRAM_ECC_CHECK      (2U)

/* Select a function of SRAM checking */
#define SAMPLE_FUNC                     (SAMPLE_FUNC_SRAM_ECC_CHECK)
#define SRAM_EXP_TYPE                   (SRAM_EXP_TYPE_NMI)

/* Definitions according to the function that just specified. */
#if (SAMPLE_FUNC == SAMPLE_FUNC_SRAM_PARITY_CHECK)
#define SRAM_CHECK_SRAM                 (SRAM_CHECK_SRAMH)
#define SRAM_CHECK_ADDR                 (0x1FFFFFF0UL)
#else /* (SAMPLE_FUNC == SAMPLE_FUNC_SRAM_ECC_CHECK) */
#define SRAM_ECC_MD                     (SRAM_SRAM0_ECC_MD3)
#define SRAM_ECC_SRAM                   (SRAM_ECC_SRAM0)
#define SRAM_CHECK_SRAM                 (SRAM_CHECK_SRAM0)
#define SRAM_CHECK_ADDR                 (0x20004000UL)
#endif

/* TMR4 PWM pin definition */
#define TIM4_OH_PORT                    (GPIO_PORT_D)
#define TIM4_OH_PIN                     (GPIO_PIN_08)
#define TIM4_OH_GPIO_FUNC               (GPIO_FUNC_11)

#define TIM4_OL_PORT                    (GPIO_PORT_D)
#define TIM4_OL_PIN                     (GPIO_PIN_09)
#define TIM4_OL_GPIO_FUNC               (GPIO_FUNC_11)

/* TMR4 unit definition */
#define TMR4_UNIT                       (CM_TMR4_1)
#define TMR4_FCG_ENABLE()               (FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR4_1, ENABLE))

/* TMR4 count period value */
#define TMR4_CNT_PERIOD_VALUE(div)      ((uint16_t)(TMR4_ClockFreq() / (1UL << (uint32_t)(div)) / 64UL) - 1U)

/* EMB unit definition */
#define EMB_GROUP                       (CM_EMB1)
#define EMB_FCG_ENABLE()                (FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_EMB, ENABLE))

/* EMB interrupt definition */
#define EMB_INT_IRQn                    (INT000_IRQn)
#define EMB_INT_SRC                     (INT_SRC_EMB_GR1)

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
 * @brief  Get key state
 * @param  None
 * @retval An @ref en_flag_status_t enumeration type value.
 */
static en_flag_status_t KEY_State(void)
{
    en_flag_status_t enKeyState = RESET;

    if (PIN_RESET == GPIO_ReadInputPins(KEY_PORT, KEY_PIN)) {
        DDL_DelayMS(50UL);

        if (PIN_RESET == GPIO_ReadInputPins(KEY_PORT, KEY_PIN)) {
            while (PIN_RESET == GPIO_ReadInputPins(KEY_PORT, KEY_PIN)) {
            }
            enKeyState = SET;
        }
    }

    return enKeyState;
}

/**
 * @brief  Generate an error of SRAM.
 * @param  None
 * @retval None
 */
static void SRAM_GenerateError(void)
{
    __UNUSED uint32_t u32Tmp;

#if (SAMPLE_FUNC == SAMPLE_FUNC_SRAM_PARITY_CHECK)
    /* Read a SRAM address that uninitialized and the parity check error will occur after the reading operation. */
    u32Tmp = RW_MEM32(SRAM_CHECK_ADDR);
#else
    /* 1. Specifies an ECC mode. */
    SRAM_SetEccMode(SRAM_ECC_SRAM, SRAM_ECC_MD);

    /* 2. Write and read while an ECC is enabled. */
    RW_MEM32(SRAM_CHECK_ADDR) = 0x12345678UL;
    u32Tmp = RW_MEM32(SRAM_CHECK_ADDR);

    /* 3. Disable ECC mode and write a different value to the same address. */
    SRAM_SetEccMode(SRAM_ECC_SRAM, SRAM_ECC_MD_INVD);
    RW_MEM32(SRAM_CHECK_ADDR) = 0x12345670UL;

    /* 4. Enable the ECC mode again. */
    SRAM_SetEccMode(SRAM_ECC_SRAM, SRAM_ECC_MD);

    /* 5. Read the address that was just written and the ECC check error will occur after the reading operation. */
    u32Tmp = RW_MEM32(SRAM_CHECK_ADDR);
#endif
}

/**
 * @brief  Configures SRAM.
 * @param  None
 * @retval None
 */
static void SRAM_Config(void)
{
    SRAM_Init();

    SRAM_SetExceptionType(SRAM_CHECK_SRAM, SRAM_EXP_TYPE);

#if (SAMPLE_FUNC == SAMPLE_FUNC_SRAM_ECC_CHECK)
    SRAM_SetEccMode(SRAM_ECC_SRAM, SRAM_ECC_MD);
#endif
}

/**
 * @brief  Get TMR4 clock frequency.
 * @param  None
 * @retval TMR4 clock frequency
 */
static uint32_t TMR4_ClockFreq(void)
{
    return CLK_GetBusClockFreq(CLK_BUS_PCLK0);
}

/**
 * @brief  Configure TMR4 PWM
 * @param  None
 * @retval None
 */
static void TMR4_PwmConfig(void)
{
    stc_tmr4_init_t stcTmr4Init;
    stc_tmr4_oc_init_t stcTmr4OcInit;
    stc_tmr4_pwm_init_t stcTmr4PwmInit;
    un_tmr4_oc_ocmrh_t unTmr4OcOcmrh;
    un_tmr4_oc_ocmrl_t unTmr4OcOcmrl;

    /* Initialize PWM I/O */
    GPIO_SetFunc(TIM4_OH_PORT, TIM4_OH_PIN, TIM4_OH_GPIO_FUNC);
    GPIO_SetFunc(TIM4_OL_PORT, TIM4_OL_PIN, TIM4_OL_GPIO_FUNC);

    /* Enable TMR4 peripheral clock */
    TMR4_FCG_ENABLE();

    /************************* Configure TMR4 counter *************************/
    /* TMR4 counter: initialize */
    (void)TMR4_StructInit(&stcTmr4Init);
    stcTmr4Init.u16ClockDiv = TMR4_CLK_DIV1024;
    stcTmr4Init.u16PeriodValue = TMR4_CNT_PERIOD_VALUE(stcTmr4Init.u16ClockDiv);
    (void)TMR4_Init(TMR4_UNIT, &stcTmr4Init);

    /************************* Configure TMR4 output-compare ******************/
    /* TMR4 OC channel: initialize TMR4 structure */
    (void)TMR4_OC_StructInit(&stcTmr4OcInit);
    stcTmr4OcInit.u16CompareValue = (stcTmr4Init.u16PeriodValue / 2U);

    /* TMR4 OC channel: initialize channel */
    (void)TMR4_OC_Init(TMR4_UNIT, TMR4_OC_CH_XH, &stcTmr4OcInit);
    (void)TMR4_OC_Init(TMR4_UNIT, TMR4_OC_CH_XL, &stcTmr4OcInit);

    /* TMR4 OC high channel: compare mode OCMR[15:0] = 0x0FFF = b 0000 1111 1111 1111 */
    unTmr4OcOcmrh.OCMRx_f.OCFDCH = TMR4_OC_OCF_SET; /* bit[0]     1  */
    unTmr4OcOcmrh.OCMRx_f.OCFPKH = TMR4_OC_OCF_SET; /* bit[1]     1  */
    unTmr4OcOcmrh.OCMRx_f.OCFUCH = TMR4_OC_OCF_SET; /* bit[2]     1  */
    unTmr4OcOcmrh.OCMRx_f.OCFZRH = TMR4_OC_OCF_SET; /* bit[3]     1  */
    unTmr4OcOcmrh.OCMRx_f.OPDCH  = TMR4_OC_INVT;    /* Bit[5:4]   11 */
    unTmr4OcOcmrh.OCMRx_f.OPPKH  = TMR4_OC_INVT;    /* Bit[7:6]   11 */
    unTmr4OcOcmrh.OCMRx_f.OPUCH  = TMR4_OC_INVT;    /* Bit[9:8]   11 */
    unTmr4OcOcmrh.OCMRx_f.OPZRH  = TMR4_OC_INVT;    /* Bit[11:10] 11 */
    unTmr4OcOcmrh.OCMRx_f.OPNPKH = TMR4_OC_HOLD;    /* Bit[13:12] 00 */
    unTmr4OcOcmrh.OCMRx_f.OPNZRH = TMR4_OC_HOLD;    /* Bit[15:14] 00 */
    TMR4_OC_SetHighChCompareMode(TMR4_UNIT, TMR4_OC_CH_XH, unTmr4OcOcmrh);

    /* TMR4 OC low channel: compare mode OCMR[31:0] 0x0FF0 0FFF = b 0000 1111 1111 0000   0000 1111 1111 1111 */
    unTmr4OcOcmrl.OCMRx_f.OCFDCL  = TMR4_OC_OCF_SET; /* bit[0]     1  */
    unTmr4OcOcmrl.OCMRx_f.OCFPKL  = TMR4_OC_OCF_SET; /* bit[1]     1  */
    unTmr4OcOcmrl.OCMRx_f.OCFUCL  = TMR4_OC_OCF_SET; /* bit[2]     1  */
    unTmr4OcOcmrl.OCMRx_f.OCFZRL  = TMR4_OC_OCF_SET; /* bit[3]     1  */
    unTmr4OcOcmrl.OCMRx_f.OPDCL   = TMR4_OC_INVT;    /* bit[5:4]   11 */
    unTmr4OcOcmrl.OCMRx_f.OPPKL   = TMR4_OC_INVT;    /* bit[7:6]   11 */
    unTmr4OcOcmrl.OCMRx_f.OPUCL   = TMR4_OC_INVT;    /* bit[9:8]   11 */
    unTmr4OcOcmrl.OCMRx_f.OPZRL   = TMR4_OC_INVT;    /* bit[11:10] 11 */
    unTmr4OcOcmrl.OCMRx_f.OPNPKL  = TMR4_OC_HOLD;    /* bit[13:12] 00 */
    unTmr4OcOcmrl.OCMRx_f.OPNZRL  = TMR4_OC_HOLD;    /* bit[15:14] 00 */
    unTmr4OcOcmrl.OCMRx_f.EOPNDCL = TMR4_OC_HOLD;    /* bit[17:16] 00 */
    unTmr4OcOcmrl.OCMRx_f.EOPNUCL = TMR4_OC_HOLD;    /* bit[19:18] 00 */
    unTmr4OcOcmrl.OCMRx_f.EOPDCL  = TMR4_OC_INVT;    /* bit[21:20] 11 */
    unTmr4OcOcmrl.OCMRx_f.EOPPKL  = TMR4_OC_INVT;    /* bit[23:22] 11 */
    unTmr4OcOcmrl.OCMRx_f.EOPUCL  = TMR4_OC_INVT;    /* bit[25:24] 11 */
    unTmr4OcOcmrl.OCMRx_f.EOPZRL  = TMR4_OC_INVT;    /* bit[27:26] 11 */
    unTmr4OcOcmrl.OCMRx_f.EOPNPKL = TMR4_OC_HOLD;    /* bit[29:28] 00 */
    unTmr4OcOcmrl.OCMRx_f.EOPNZRL = TMR4_OC_HOLD;    /* bit[31:30] 00 */
    TMR4_OC_SetLowChCompareMode(TMR4_UNIT, TMR4_OC_CH_XL, unTmr4OcOcmrl);

    /* TMR4 OC: enable */
    TMR4_OC_Cmd(TMR4_UNIT, TMR4_OC_CH_XH, ENABLE);
    TMR4_OC_Cmd(TMR4_UNIT, TMR4_OC_CH_XL, ENABLE);

    /************************* Configure TMR4 PWM *****************************/
    /* TMR4 PWM: initialize */
    (void)TMR4_PWM_StructInit(&stcTmr4PwmInit);
    stcTmr4PwmInit.u16Polarity = TMR4_PWM_OXH_HOLD_OXL_INVT;
    (void)TMR4_PWM_Init(TMR4_UNIT, TMR4_PWM_CH_X, &stcTmr4PwmInit);

    /* TMR4 PWM: enable main output  */
    TMR4_PWM_MainOutputCmd(TMR4_UNIT, ENABLE);

    /* TMR4 PWM: set port output normal */
    TMR4_PWM_SetPortOutputMode(TMR4_UNIT, TMR4_PWM_PIN_OXH, TMR4_PWM_PIN_OUTPUT_NORMAL);
    TMR4_PWM_SetPortOutputMode(TMR4_UNIT, TMR4_PWM_PIN_OXL, TMR4_PWM_PIN_OUTPUT_NORMAL);

    /* TMR4 PWM: set PWM pin output when EMB event occur. */
    TMR4_PWM_SetAbnormalPinStatus(TMR4_UNIT, TMR4_PWM_PIN_OXH, TMR4_PWM_ABNORMAL_PIN_LOW);
    TMR4_PWM_SetAbnormalPinStatus(TMR4_UNIT, TMR4_PWM_PIN_OXL, TMR4_PWM_ABNORMAL_PIN_LOW);

    /* TMR4 PWM: enable the TMR4 PWM main output by hardware after clear EMB event. */
    TMR4_PWM_EmbHWMainOutputCmd(TMR4_UNIT, ENABLE);

    /* Start TMR4 count. */
    TMR4_Start(TMR4_UNIT);
}

/**
 * @brief  EMB IRQ Callback.
 * @param  None
 * @retval None
 */
void EMB_IrqCallback(void)
{
    if (SET == EMB_GetStatus(EMB_GROUP, EMB_FLAG_SYS)) {
        /* SRAM parity error */
        if (SET == SRAM_GetStatus(SRAM_FLAG_SRAMH_PYERR)) {
            BSP_LED_On(LED_RED);
        }

        /* SRAM ECC error */
        if (SET == SRAM_GetStatus(SRAM_FLAG_SRAM0_1ERR)) {
            BSP_LED_On(LED_YELLOW);
        }

        /* Wait key pressed */
        while (RESET == KEY_State()) {
        }

        /* Clear the SRAM error status. */
        SRAM_ClearStatus(SRAM_FLAG_ALL);;

        /* Clear the EMB OSC status. */
        EMB_ClearStatus(EMB_GROUP, EMB_FLAG_SYS);

        BSP_LED_Off(LED_RED | LED_YELLOW);
    }
}

/**
 * @brief  Main function of EMB SRAM error brake
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    stc_emb_tmr4_init_t stcEmbInit;

    /* MCU Peripheral registers write unprotected */
    LL_PERIPH_WE(LL_PERIPH_SEL);

    /* BSP IO */
    BSP_IO_Init();

    /* BSP Led */
    BSP_LED_Init();
    BSP_LED_Off(LED_RED | LED_YELLOW);

    /* Configure SRAM. */
    SRAM_Config();

    /* Configure TMR4 PWM. */
    TMR4_PwmConfig();

    /* Enable EMB peripheral clock */
    EMB_FCG_ENABLE();

    /* EMB: initialize */
    (void)EMB_TMR4_StructInit(&stcEmbInit);
    stcEmbInit.stcSys.u32SramEccError = EMB_SRAM_ECC_ERR_ENABLE;
    stcEmbInit.stcSys.u32SramParityError = EMB_SRAM_PARITY_ERR_ENABLE;
    (void)EMB_TMR4_Init(EMB_GROUP, &stcEmbInit);

    /* EMB: enable interrupt */
    EMB_IntCmd(EMB_GROUP, EMB_INT_SYS, ENABLE);

    /* EMB: set release PWM condition */
    EMB_SetReleasePwmCond(EMB_GROUP, EMB_EVT_SYS, EMB_RELEASE_PWM_COND_FLAG_ZERO);

    /* EMB: register IRQ callback. */
    (void)INTC_IrqInstallHandle(EMB_INT_IRQn, EMB_INT_SRC, DDL_IRQ_PRIO_DEFAULT, EMB_IrqCallback);

    for (;;) {
        /* Wait key pressed */
        while (RESET == KEY_State()) {
        }

        /* Generate SRAM error */
        SRAM_GenerateError();
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
