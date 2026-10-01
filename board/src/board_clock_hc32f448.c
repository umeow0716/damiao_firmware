#include "board_clock.h"

#include "hc32f448.h"
#include "system_hc32f448.h"

#define BOARD_CLOCK_SOURCE_PLLH (5U)

#define FCG0_TRANSITION_VALUE (0xFFFFFA0EUL)
#define FCGX_TRANSITION_VALUE (0xFFFFFFFFUL)
#define FCG0_PROTECT_UNLOCK   (0xA5A50001UL)
#define FCG0_PROTECT_LOCK     (0xA5A50000UL)
#define CLOCK_PROTECT_UNLOCK  (0xA501U)
#define CLOCK_PROTECT_LOCK    (0xA500U)

/* Rev1.3.0 removed the old wait-state register names from CM_SRAMC_TypeDef,
 * but the recovered image writes these addresses before selecting 200 MHz.
 * Keep the raw addresses explicit so the compatibility operation is visible. */
#define SRAMC_LEGACY_WAIT_CONFIG (*(volatile uint32_t *)0x40050800UL)
#define SRAMC_LEGACY_WP0         (*(volatile uint32_t *)0x40050804UL)
#define SRAMC_LEGACY_WP1         (*(volatile uint32_t *)0x4005080CUL)

typedef struct {
    uint32_t fcg0;
    uint32_t fcg1;
    uint32_t fcg2;
    uint32_t fcg3;
} PeripheralClockGates;

#if !defined(DAMIAO_DM4310)
static bool clock_ready;
#endif

static void transition_delay(void)
{
#if defined(DAMIAO_DM4310)
    for (uint16_t count = 0U; count < 512U; ++count) {
        /* Keep the factory's register-only 16-bit increment/compare loop. */
        __asm volatile ("" ::: "memory");
    }
#else
    for (volatile uint32_t count = 0U; count < 512U; ++count) {
        __NOP();
    }
#endif
}

static void clock_registers_unlock(void)
{
    CM_PWC->FPRC = (uint16_t)(CM_PWC->FPRC | CLOCK_PROTECT_UNLOCK);
}

static void clock_registers_lock(void)
{
    CM_PWC->FPRC = (uint16_t)(CLOCK_PROTECT_LOCK |
                              (CM_PWC->FPRC & (uint16_t)~1U));
}

static PeripheralClockGates peripheral_clocks_capture(void)
{
    PeripheralClockGates saved = {
        CM_PWC->FCG0, CM_PWC->FCG1, CM_PWC->FCG2, CM_PWC->FCG3
    };

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
    while ((CM_CMU->OSCSTBSR & mask) == 0U) {
        /* The official APP has no timeout or alternate clock path here. */
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

    /* Exact EFM_REG_Unlock()/FRMC sequence at 0x23314..0x23332.  The factory
     * APP intentionally leaves FAPRT at 0x3210 and writes that value twice. */
    CM_EFM->FAPRT = 0x0123UL;
    CM_EFM->FAPRT = 0x3210UL;
    CM_EFM->FRMC = config->flash_read_config;
    CM_EFM->FAPRT = 0x3210UL;
    CM_EFM->FAPRT = 0x3210UL;
}

#if defined(DAMIAO_DM4310)
static void set_system_source(
    uint8_t source, const PeripheralClockGates *saved)
{
    /* The caller already captured FCG0..3 at routine entry.  The factory
     * does not reread them here before masking every peripheral clock. */
    peripheral_clocks_gate_for_transition();
    clock_registers_unlock();
    CM_CMU->CKSWR = source;
    clock_registers_lock();
    transition_delay();
    peripheral_clocks_restore(saved);
}
#else
static void set_system_source(uint8_t source)
{
    const bool pll_transition =
        ((CM_CMU->CKSWR & CMU_CKSWR_CKSW) == BOARD_CLOCK_SOURCE_PLLH) ||
        (source == BOARD_CLOCK_SOURCE_PLLH);
    PeripheralClockGates saved = {0U, 0U, 0U, 0U};

    if (pll_transition) {
        saved = peripheral_clocks_capture();
        peripheral_clocks_gate_for_transition();
    }
    clock_registers_unlock();
    CM_CMU->CKSWR = source;
    clock_registers_lock();
    transition_delay();
    if (pll_transition) {
        peripheral_clocks_restore(&saved);
    }
}
#endif

bool board_clock_init(void)
{
    BoardClockConfig config;
    board_clock_build_config(&config);
#if !defined(DAMIAO_DM4310)
    clock_ready = false;
#endif

    /* The recovered routine captures all four gates before masking IRQs. */
    const PeripheralClockGates saved = peripheral_clocks_capture();

    /* system_clock_and_systick_init@0x23208 first masks the three peripheral
     * IRQs that can be inherited across a loader handoff. */
    NVIC_DisableIRQ(INT001_IRQn);
    NVIC_DisableIRQ(INT002_IRQn);
    NVIC_DisableIRQ(INT003_IRQn);

    CM_PWC->FCG0PC = FCG0_PROTECT_UNLOCK;
    /* Factory 0x2322c writes the gate-protection key before sampling CKSWR. */
#if defined(DAMIAO_DM4310)
    const bool already_on_pll = CM_CMU->CKSWR == BOARD_CLOCK_SOURCE_PLLH;
#else
    const bool already_on_pll =
        (CM_CMU->CKSWR & CMU_CKSWR_CKSW) == BOARD_CLOCK_SOURCE_PLLH;
#endif
    if (already_on_pll) {
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
    /* The original restores and locks FCG0..3 even when the entry source was
     * not PLLH; in that case these are harmless same-value writes. */
    peripheral_clocks_restore(&saved);

    clock_registers_unlock();
    CM_CMU->XTALCFGR = 0xA0U;
#if defined(DAMIAO_DM4310)
    clock_registers_lock();
    clock_registers_unlock();
#endif
    CM_CMU->XTALCR = 0U;
#if !defined(DAMIAO_DM4310)
    clock_registers_lock();
    transition_delay();
#endif
    wait_for_clock(CMU_OSCSTBSR_XTALSTBF);
#if defined(DAMIAO_DM4310)
    clock_registers_lock();
#endif

    configure_memory_wait_states(&config);

    /* The factory image clears PLLSRC, writes the complete PLLH image, then
     * enables PLLH.  It does not take the source-only HRC detour. */
    /* 0x23334 writes 0xa501 directly for this first PLLH operation; unlike
     * the surrounding protected writes there is no FPRC read beforehand. */
#if defined(DAMIAO_DM4310)
    CM_PWC->FPRC = CLOCK_PROTECT_UNLOCK;
#else
    clock_registers_unlock();
#endif
    CM_CMU->PLLHCFGR &= ~CMU_PLLHCFGR_PLLSRC;
    clock_registers_lock();
    clock_registers_unlock();
    CM_CMU->PLLHCFGR = config.pll_config;
    clock_registers_lock();
    clock_registers_unlock();
    CM_CMU->PLLHCR = 0U;
#if !defined(DAMIAO_DM4310)
    clock_registers_lock();
#endif
    wait_for_clock(CMU_OSCSTBSR_PLLHSTBF);
#if defined(DAMIAO_DM4310)
    clock_registers_lock();
#endif

#if defined(DAMIAO_DM4310)
    set_system_source(BOARD_CLOCK_SOURCE_PLLH, &saved);
#else
    set_system_source(BOARD_CLOCK_SOURCE_PLLH);
#endif

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0x00FFFFFFUL;
    SysTick->VAL = 0U;
    /* Factory 0x23410 reloads CTRL and sets CLKSOURCE with an RMW. */
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;

    CM_PWC->FCG2 &= ~PWC_FCG2_TMRA_1;
    CM_TMRA_1->CNTER = 0U;
    CM_TMRA_1->BCSTRL = 0x10U;
    CM_TMRA_1->BCSTRH = 0x01U;

#if !defined(DAMIAO_DM4310)
    SystemCoreClock = config.system_hz;
#endif
#if !defined(DAMIAO_DM4310)
    clock_ready = true;
#endif
    return true;
}

bool board_clock_is_ready(void)
{
#if defined(DAMIAO_DM4310)
    return true;
#else
    return clock_ready;
#endif
}
