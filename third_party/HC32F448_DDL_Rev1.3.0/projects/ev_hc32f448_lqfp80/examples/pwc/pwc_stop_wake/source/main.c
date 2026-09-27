/**
 *******************************************************************************
 * @file  pwc/pwc_stop_wake/source/main.c
 * @brief Main program of PWC for the Device Driver Library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2023-05-31       CDT             First version
   2023-12-15       CDT             Delete redundant code
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
 * @addtogroup PWC_Stop_wake
 * @{
 */
/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define KEY5_PORT               (GPIO_PORT_B)
#define KEY5_PIN                (GPIO_PIN_06)
#define KEY5_EXTINT_CH          (EXTINT_CH06)
#define KEY5_INT_SRC            (INT_SRC_PORT_EIRQ6)
#define KEY5_INT_IRQn           (INT001_IRQn)
#define KEY5_INT                (INTC_INT1)
#define KEY5_EVT                (INTC_EVT1)

#define DLY_MS                  (500UL)

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void STOP_Config(void);
static int32_t STOP_IsReady(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static char *m_StopTypeTbl1[] = {"PWC_STOP_WFI", "PWC_STOP_WFE_INT", "PWC_STOP_WFE_EVT"};
static char *m_StopTypeTbl2[] = {"interrupt handle", "interrupt request", "event"};
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  KEY5 External interrupt callback function
 * @param  None
 * @retval None
 */
static void KEY5_IrqHandle()
{
    DDL_Printf("KEY5 handle occurs!\r\n");
    DDL_DelayMS(DLY_MS);
    /* Clear flag */
    EXTINT_ClearExtIntStatus(KEY5_EXTINT_CH);
    /* Clear pending */
    NVIC_ClearPendingIRQ(KEY5_INT_IRQn);
    __DSB();
}

/**
 * @brief  Whether ready to entry stop mode.
 * @param  None
 * @retval int32_t:
 * @note Ensure DMA stops transmit and no flash erase/program operation.
 */
static int32_t STOP_IsReady(void)
{
    int32_t i32Ret = LL_OK;
    uint8_t tmp1;
    uint8_t tmp2;
    uint8_t tmp3;

    tmp1 = (uint8_t)((READ_REG32(CM_EFM->FSR) & EFM_FSR_RDY) == EFM_FSR_RDY);

    tmp2 = (uint8_t)((READ_REG32(CM_DMA1->CHSTAT) & DMA_CHSTAT_DMAACT) == 0x00U);
    tmp3 = (uint8_t)((READ_REG32(CM_DMA2->CHSTAT) & DMA_CHSTAT_DMAACT) == 0x00U);

    if (0U == (tmp1 & tmp2 & tmp3)) {
        i32Ret = LL_ERR_NOT_RDY;
    }
    return i32Ret;
}

/**
 * @brief  MCU behavior config for stop mode.
 * @param  None
 * @retval None
 */
static void STOP_Config(void)
{
    stc_pwc_stop_mode_config_t stcStopConfig;

    (void)PWC_STOP_StructInit(&stcStopConfig);

    stcStopConfig.u16ExBusHold = PWC_STOP_EXBUS_HIZ;
    stcStopConfig.u16Clock = PWC_STOP_CLK_KEEP;
    stcStopConfig.u16FlashWait = PWC_STOP_FLASH_WAIT_ON;

    (void)PWC_STOP_Config(&stcStopConfig);

    /* Wake-up source config (EXTINT Ch.6 here) */
    INTC_WakeupSrcCmd(INTC_STOP_WKUP_EXTINT_CH6, ENABLE);
}

/**
 * @brief  Key5 init function
 * @param  None
 * @retval None
 */
static void Key5_Init(void)
{
    stc_extint_init_t stcExtIntInit;
    stc_irq_signin_config_t stcIrqSignConfig;
    stc_gpio_init_t stcGpioInit;

    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
    stcGpioInit.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(KEY5_PORT, KEY5_PIN, &stcGpioInit);

    /* ExtInt config */
    (void)EXTINT_StructInit(&stcExtIntInit);
    stcExtIntInit.u32Edge = EXTINT_TRIG_FALLING;
    (void)EXTINT_Init(KEY5_EXTINT_CH, &stcExtIntInit);

    /* IRQ sign-in */
    stcIrqSignConfig.enIntSrc    = KEY5_INT_SRC;
    stcIrqSignConfig.enIRQn      = KEY5_INT_IRQn;
    stcIrqSignConfig.pfnCallback = &KEY5_IrqHandle;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* Disable all */
    INTC_IntCmd(INTC_INT_ALL, DISABLE);
    /* NVIC config */
    NVIC_ClearPendingIRQ(KEY5_INT_IRQn);
    NVIC_SetPriority(KEY5_INT_IRQn, DDL_IRQ_PRIO_DEFAULT);
}

/**
 * @brief  Config before enter stop mode, and control after wake-up
 * @param  u8StopType       stop type
 *   @arg  PWC_STOP_WFI             Enter stop mode by WFI, and wake-up by interrupt handle.
 *   @arg  PWC_STOP_WFE_INT         Enter stop mode by WFE, and wake-up by interrupt request.
 *   @arg  PWC_STOP_WFE_EVT         Enter stop mode by WFE, and wake-up by event.
 * @retval None
 */
static void Enter_Stop_Control(uint8_t u8StopType)
{
    if (PWC_STOP_WFI == u8StopType) {
        /* Enable interrupt function */
        INTC_IntCmd(KEY5_INT, ENABLE);
        /* Disable event function */
        INTC_EventCmd(KEY5_EVT, DISABLE);
        /* NVIC config */
        NVIC_ClearPendingIRQ(KEY5_INT_IRQn);
        NVIC_SetPriority(KEY5_INT_IRQn, DDL_IRQ_PRIO_DEFAULT);
        NVIC_EnableIRQ(KEY5_INT_IRQn);
    } else if (PWC_STOP_WFE_INT == u8StopType) {
        /* Enable interrupt function */
        INTC_IntCmd(KEY5_INT, ENABLE);
        /* Disable event function */
        INTC_EventCmd(KEY5_EVT, DISABLE);
        /* Disable NVIC */
        NVIC_DisableIRQ(KEY5_INT_IRQn);
    } else if (PWC_STOP_WFE_EVT == u8StopType) {
        /* Disable interrupt function */
        INTC_IntCmd(KEY5_INT, DISABLE);
        /* Enable event function */
        INTC_EventCmd(KEY5_EVT, ENABLE);
    } else {
        /* rsvd */
    }
    DDL_Printf("MCU will enter stop mode by %s...\r\n", m_StopTypeTbl1[u8StopType]);
    DDL_DelayMS(DLY_MS);
    if (PWC_STOP_WFE_INT == u8StopType) {
        /* Clear pending */
        NVIC_ClearPendingIRQ(KEY5_INT_IRQn);
    }
    /* Enter stop */
    PWC_STOP_Enter(u8StopType);
    DDL_Printf("MCU wake up from stop mode by %s!\r\n\r\n", m_StopTypeTbl2[u8StopType]);
    DDL_DelayMS(DLY_MS);
    /* Clear flag */
    EXTINT_ClearExtIntStatus(KEY5_EXTINT_CH);
    /* Clear pending */
    NVIC_ClearPendingIRQ(KEY5_INT_IRQn);
}

/**
 * @brief  Main function of PWC Stop wakeup project
 * @param  None
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    /* Register write enable for some required peripherals. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_FCG | LL_PERIPH_EFM);

    /* System Clock init */
    BSP_CLK_Init();
    /* KEY init */
    Key5_Init();
    /* Print init */
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);

    STOP_Config();

    /* KEY5 */
    while (PIN_RESET != GPIO_ReadInputPins(KEY5_PORT, KEY5_PIN)) {
        ;
    }
    DDL_DelayMS(DLY_MS);

    /* Register write protected for some required peripherals. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_EFM);

    for (;;) {
        if (LL_OK == STOP_IsReady()) {
            Enter_Stop_Control(PWC_STOP_WFI);
            DDL_DelayMS(DLY_MS);
            Enter_Stop_Control(PWC_STOP_WFE_INT);
            DDL_DelayMS(DLY_MS);
            Enter_Stop_Control(PWC_STOP_WFE_EVT);
            DDL_DelayMS(DLY_MS);
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
