#include "board_clock.h"

#include "hc32f448.h"
#include "system_hc32f448.h"

#define BOARD_CLOCK_SOURCE_PLLH (5U)

#define FCG0_TRANSITION_VALUE (0xFFFFFA0EUL)
#define FCGX_TRANSITION_VALUE (0xFFFFFFFFUL)
#define FCG0_PROTECT_UNLOCK (0xA5A50001UL)
#define FCG0_PROTECT_LOCK (0xA5A50000UL)
#define CLOCK_PROTECT_UNLOCK (0xA501U)
#define CLOCK_PROTECT_LOCK (0xA500U)

/* Rev1.3.0 removed the old wait-state register names from CM_SRAMC_TypeDef,
 * but the fixed-layout image writes these addresses before selecting 200 MHz.
 * Keep the raw addresses explicit so the compatibility operation is visible. */
#define SRAMC_LEGACY_WAIT_CONFIG (*(volatile uint32_t *)0x40050800UL)
#define SRAMC_LEGACY_WP0 (*(volatile uint32_t *)0x40050804UL)
#define SRAMC_LEGACY_WP1 (*(volatile uint32_t *)0x4005080CUL)

typedef struct
{
    uint32_t fcg0;
    uint32_t fcg1;
    uint32_t fcg2;
    uint32_t fcg3;
} PeripheralClockGates;

static void transition_delay(void)
{
    for (uint16_t count = 0U; count < 512U; ++count)
    {
        /* Keep the firmware's register-only 16-bit increment/compare loop. */
        __asm volatile("" ::: "memory");
    }
}

static void clock_registers_unlock(void)
{
    CM_PWC->FPRC = (uint16_t)(CM_PWC->FPRC | CLOCK_PROTECT_UNLOCK);
}

static void clock_registers_lock(void)
{
    CM_PWC->FPRC = (uint16_t)(CLOCK_PROTECT_LOCK | (CM_PWC->FPRC & (uint16_t)~1U));
}

static PeripheralClockGates peripheral_clocks_capture(void)
{
    PeripheralClockGates saved = {CM_PWC->FCG0, CM_PWC->FCG1, CM_PWC->FCG2, CM_PWC->FCG3};

    return saved;
}

static void peripheral_clocks_gate_for_transition(void)
{
    CM_PWC->FCG0PC = FCG0_PROTECT_UNLOCK;
    CM_PWC->FCG0 = FCG0_TRANSITION_VALUE;
    CM_PWC->FCG1 = FCGX_TRANSITION_VALUE;
    CM_PWC->FCG2 = FCGX_TRANSITION_VALUE;
    CM_PWC->FCG3 = FCGX_TRANSITION_VALUE;
    transition_delay();
}

static void peripheral_clocks_restore(const PeripheralClockGates *saved)
{
    CM_PWC->FCG0 = saved->fcg0;
    CM_PWC->FCG1 = saved->fcg1;
    CM_PWC->FCG2 = saved->fcg2;
    CM_PWC->FCG3 = saved->fcg3;
    CM_PWC->FCG0PC = FCG0_PROTECT_LOCK;
    transition_delay();
}

static void wait_for_clock(uint8_t mask)
{
    while ((CM_CMU->OSCSTBSR & mask) == 0U)
    {
        /* Clock stabilization is mandatory; there is no alternate source. */
    }
}

static void configure_memory_wait_states(const BoardClockConfig *config)
{
    SRAMC_LEGACY_WP0 = 0x77UL;
    SRAMC_LEGACY_WP1 = 0x77UL;
    SRAMC_LEGACY_WAIT_CONFIG = config->sram_wait_config;
    SRAMC_LEGACY_WP0 = 0x76UL;
    SRAMC_LEGACY_WP1 = 0x76UL;
    CM_PWC->RAMOPM = 0x8043UL;

    /* Preserve the EFM unlock and FRMC write sequence.  FAPRT intentionally
     * remains at 0x3210, with the closing key written twice. */
    CM_EFM->FAPRT = 0x0123UL;
    CM_EFM->FAPRT = 0x3210UL;
    CM_EFM->FRMC = config->flash_read_config;
    CM_EFM->FAPRT = 0x3210UL;
    CM_EFM->FAPRT = 0x3210UL;
}

static void set_system_source(uint8_t source, const PeripheralClockGates *saved)
{
    /* The caller captured FCG0..3 at routine entry.  Do not reread them here
     * before masking every peripheral clock. */
    peripheral_clocks_gate_for_transition();
    clock_registers_unlock();
    CM_CMU->CKSWR = source;
    clock_registers_lock();
    transition_delay();
    peripheral_clocks_restore(saved);
}

bool board_clock_init(void)
{
    BoardClockConfig config;
    board_clock_build_config(&config);

    /* The fixed-layout routine captures all four gates before masking IRQs. */
    const PeripheralClockGates saved = peripheral_clocks_capture();

    /* system_clock_and_systick_init first masks the three peripheral
     * IRQs that can be inherited across a loader handoff. */
    NVIC_DisableIRQ(INT001_IRQn);
    NVIC_DisableIRQ(INT002_IRQn);
    NVIC_DisableIRQ(INT003_IRQn);

    CM_PWC->FCG0PC = FCG0_PROTECT_UNLOCK;
    /* This path writes the gate-protection key before sampling CKSWR. */
    const bool already_on_pll = CM_CMU->CKSWR == BOARD_CLOCK_SOURCE_PLLH;
    if (already_on_pll)
    {
        CM_PWC->FCG0 = FCG0_TRANSITION_VALUE;
        CM_PWC->FCG1 = FCGX_TRANSITION_VALUE;
        CM_PWC->FCG2 = FCGX_TRANSITION_VALUE;
        CM_PWC->FCG3 = FCGX_TRANSITION_VALUE;
        transition_delay();
    }

    clock_registers_unlock();
    CM_CMU->SCFGR = config.system_clock_config;
    clock_registers_lock();
    transition_delay();
    /* Restore and lock FCG0..3 even when the entry source was
     * not PLLH; in that case these are harmless same-value writes. */
    peripheral_clocks_restore(&saved);

    clock_registers_unlock();
    CM_CMU->XTALCFGR = 0xA0U;
    clock_registers_lock();
    clock_registers_unlock();
    CM_CMU->XTALCR = 0U;
    wait_for_clock(CMU_OSCSTBSR_XTALSTBF);
    clock_registers_lock();

    configure_memory_wait_states(&config);

    /* The firmware image clears PLLSRC, writes the complete PLLH image, then
     * enables PLLH.  It does not take the source-only HRC detour. */
    /* The first PLLH operation writes the unlock value directly; unlike the
     * surrounding protected writes, it does not read FPRC beforehand. */
    CM_PWC->FPRC = CLOCK_PROTECT_UNLOCK;
    CM_CMU->PLLHCFGR &= ~CMU_PLLHCFGR_PLLSRC;
    clock_registers_lock();
    clock_registers_unlock();
    CM_CMU->PLLHCFGR = config.pll_config;
    clock_registers_lock();
    clock_registers_unlock();
    CM_CMU->PLLHCR = 0U;
    wait_for_clock(CMU_OSCSTBSR_PLLHSTBF);
    clock_registers_lock();

    set_system_source(BOARD_CLOCK_SOURCE_PLLH, &saved);

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0x00FFFFFFUL;
    SysTick->VAL = 0U;
    /* This path reloads CTRL and sets CLKSOURCE with an RMW. */
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;

    CM_PWC->FCG2 &= ~PWC_FCG2_TMRA_1;
    CM_TMRA_1->CNTER = 0U;
    CM_TMRA_1->BCSTRL = 0x10U;
    CM_TMRA_1->BCSTRH = 0x01U;

    return true;
}

bool board_clock_is_ready(void)
{
    return true;
}
