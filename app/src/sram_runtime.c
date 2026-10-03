/*
 * Fixed-address SRAM runtime for interrupt entry points, timing-sensitive
 * arithmetic kernels, and their literal pools. Application behavior belongs
 * in the owning app/ or board/ module; add code here only when it must execute
 * from SRAM or preserve an address consumed by the low-level ABI.
 */
#include "app_profile.h"

#include "commissioning.h"
#include "board_delay.h"
#include "board_mcan.h"
#include "board_sampling_timer.h"
#include "interrupts.h"
#include "motor_math.h"
#include "output_sensor.h"
#include "platform.h"
#include "temperature_table.h"

static float wrap_angle_helper(float angle);
void sram_runtime_initialize_literals(void);
void delay_ms_helper(uint32_t milliseconds);
float checked_sqrtf(float value);
uint32_t *runtime_context_helper(void);
void runtime_errno_set_helper(uint32_t value);

extern float motor_sine_table[2049];

/* Shared firmware SRAM literal pools, decoded as data words (not code). */
__attribute__((used, section(".fault_monitor_constants"))) static const uint32_t
    fault_monitor_constants[
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
        27
#else
        26
#endif
] = {MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff1f0), UINT32_C(0x1ffff17c)),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558)),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff104), UINT32_C(0x1ffff090)),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014)),
     UINT32_C(0x42f00000),
     UINT32_C(0x40040000),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffc778), UINT32_C(0x1ffff348)),
     UINT32_C(0x40040450),
     UINT32_C(0x40040850),
     UINT32_C(0x40040452),
     UINT32_C(0x40040852),
     UINT32_C(0x40040454),
     UINT32_C(0x40040854),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa560), UINT32_C(0x1fffa648)),
     (uintptr_t)temperature_celsius_table,
     APP_PROFILE_RUNTIME_CACHE_0_BITS,
     UINT32_C(0x3f13cd3a),
     UINT32_C(0x3a000000),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff1a8), UINT32_C(0x1ffff134)),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff190), UINT32_C(0x1ffff11c)),
     UINT32_C(0x40c90fdb),
     UINT32_C(0x40490fdb),
     UINT32_C(0xc0490fdb),
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
     UINT32_C(0xbf13cd3a),
#endif
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa568), UINT32_C(0x1fffa650)),
     UINT32_C(0x00000000),
     UINT32_C(0x447a0000)};
__attribute__((
    used,
    section(".current_control_constants"))) static const uint32_t current_control_constants[4] = {
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff380), UINT32_C(0x1ffff30c)),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffc78c), UINT32_C(0x1ffff35c)), UINT32_C(0x3f7ae148),
    UINT32_C(0x40040000)};
__attribute__((
    used,
    section(".position_timer_constants"))) static const uint32_t position_timer_constants[8] = {
    UINT32_C(0x40010400), UINT32_C(0x01234567),
    UINT32_C(0x40010418), UINT32_C(0x40010424),
    UINT32_C(0x4001041c), UINT32_C(0x40010590),
    UINT32_C(0x0001e000), MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffc824), UINT32_C(0x1fffc7b0))};
__attribute__((used, section(".output_sensor_constants"))) static const uint32_t
    output_sensor_constants[
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM) || defined(DAMIAO_LAYOUT_SHIFTED_SRAM)
        20
#else
        19
#endif
] = {UINT32_C(0x03544000),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff23c), UINT32_C(0x1ffff1c8)),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa568), UINT32_C(0x1fffa650)),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa590), UINT32_C(0x1fffa678)),
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM) || defined(DAMIAO_LAYOUT_SHIFTED_SRAM)
     UINT32_C(0x42820000),
     UINT32_C(0x43fa0000),
     UINT32_C(0x447a0000),
     UINT32_C(0x461c4000),
#else
     UINT32_C(0x43fa0000),
     UINT32_C(0x447a0000),
     UINT32_C(0x461c4000),
#endif
     (uintptr_t)board_mcan_init_classic,
     (uintptr_t)board_mcan_init_fd,
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff9acd), UINT32_C(0x1fff9b05)),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fff9ba1), UINT32_C(0x1fff9bd9)),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffcc4c), UINT32_C(0x1fffcbd8)),
     UINT32_C(0xaa000055),
     UINT32_C(0x880000ff),
     UINT32_C(0x4002b000),
     UINT32_C(0xaa020155),
     UINT32_C(0xe000ed0c),
     UINT32_C(0x05fa0004),
     MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff29c), UINT32_C(0x1ffff228))};
__attribute__((used, section(".mcan_receive_constants_primary"))) static const uint32_t
    mcan_receive_constants_primary[27] = {
        UINT32_C(0x40038000),
        UINT32_C(0x40020000),
        UINT32_C(0x00000000),
        UINT32_C(0x40053408),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa666), UINT32_C(0x1fffa6a0)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff190), UINT32_C(0x1ffff11c)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffc84c), UINT32_C(0x1fffc7d8)),
        UINT32_C(0x3c800000),
        UINT32_C(0x39c90fdb),
        UINT32_C(0x40c90fdb),
        UINT32_C(0x40b00000),
        UINT32_C(0xc0b00000),
        UINT32_C(0x40053448),
        UINT32_C(0x4005341c),
        UINT32_C(0x40053418),
        UINT32_C(0x40029000),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff1f0), UINT32_C(0x1ffff17c)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff29c), UINT32_C(0x1ffff228)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffcc4c), UINT32_C(0x1fffcbd8)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff104), UINT32_C(0x1ffff090)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff1a8), UINT32_C(0x1ffff134)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa5c0), UINT32_C(0x1fffa550)),
        UINT32_C(0x00036000),
        UINT32_C(0x40040000),
        UINT32_C(0x40040446)};
__attribute__((used, section(".mcan_receive_constants_secondary"))) static const uint32_t
    mcan_receive_constants_secondary[5] = {
        UINT32_C(0x43fa0000), MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff29c), UINT32_C(0x1ffff228)),
        UINT32_C(0x3c23d70a), UINT32_C(0x38d1b717),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffcc4c), UINT32_C(0x1fffcbd8))};
__attribute__((used, section(".mcan_receive_constants_tertiary"))) static const uint32_t
    mcan_receive_constants_tertiary[9] = {
        UINT32_C(0xc61c4000),
        UINT32_C(0x461c4000),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff078), UINT32_C(0x1ffff004)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa5c0), UINT32_C(0x1fffa550)),
        MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff1a8), UINT32_C(0x1ffff134)),
        UINT32_C(0x41200000),
        APP_PROFILE_CURRENT_FULL_SCALE_BITS,
        UINT32_C(0x880000ff),
        UINT32_C(0x4002b000)};
__attribute__((
    used,
    section(".output_scaling_constants"))) static const uint32_t output_scaling_constants[7] = {
    UINT32_C(0x4422f983),
    UINT32_C(0x45000000),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffd078), UINT32_C(0x1fffd004)),
    UINT32_C(0x3ac90fdb),
    UINT32_C(0x40b00000),
    UINT32_C(0xc0b00000),
    UINT32_C(0x40c90fdb)};
__attribute__((
    used,
    section(".flash_operation_constants"))) static const uint32_t flash_operation_constants[3] = {
    UINT32_C(0x40010400), UINT32_C(0x01234567), UINT32_C(0x40010590)};
__attribute__((
    used, section(".mcan_transmit_constants"))) static const uint32_t mcan_transmit_constants[3] = {
    UINT32_C(0x40029000), UINT32_C(0x4002b000), UINT32_C(0x4002b330)};
__attribute__((used, section(".pwm_constants"))) static const uint32_t pwm_constants[4] = {
    UINT32_C(0x3f5db3d7), UINT32_C(0x451c4000), UINT32_C(0x40038000), UINT32_C(0x00000000)};
__attribute__((
    used, section(".control_loop_constants"))) static const uint32_t control_loop_constants[8] = {
    UINT32_C(0x00000000),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff104), UINT32_C(0x1ffff090)),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa510), UINT32_C(0x1fffa5f8)),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa538), UINT32_C(0x1fffa620)),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa568), UINT32_C(0x1fffa650)),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa590), UINT32_C(0x1fffa678)),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffc78c), UINT32_C(0x1ffff35c)),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffc7d8), UINT32_C(0x1ffff3a8))};
__attribute__((
    used, section(".commissioning_constants"))) static const uint32_t commissioning_constants[8] = {
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1fffa5c8), UINT32_C(0x1fffa558)),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff23c), UINT32_C(0x1ffff1c8)),
    UINT32_C(0x3fddb3d7),
    UINT32_C(0x3f13cd3a),
    UINT32_C(0x44fa0000),
    UINT32_C(0x457a0000),
    UINT32_C(0x4a742400),
    MEMORY_LAYOUT_ADDRESS(UINT32_C(0x1ffff088), UINT32_C(0x1ffff014))};

extern const uint32_t __image_literal_copies_start__[];
extern const uint32_t __image_literal_copies_end__[];

void sram_runtime_initialize_literals(void)
{
    /* Firmware scatter also copies these zero alignment halfwords. The
     * a4ba/a50e padding is already owned by the conversion/sqrt sections. */
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
    *(volatile uint16_t *)0x1FFF91EEUL = 0U;
    *(volatile uint16_t *)0x1FFF92F6UL = 0U;
    *(volatile uint16_t *)0x1FFF9C6AUL = 0U;
    *(volatile uint16_t *)0x1FFFA336UL = 0U;
#else
    *(volatile uint16_t *)0x1FFF982EUL = 0U;
    *(volatile uint16_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9932UL, 0UL) = 0U;
    *(volatile uint16_t *)MEMORY_LAYOUT_ADDRESS(0x1FFF9C32UL, 0UL) = 0U;
    *(volatile uint16_t *)MEMORY_LAYOUT_ADDRESS(0x1FFFA2FEUL, 0UL) = 0U;
#endif
    const uint32_t *descriptor = __image_literal_copies_start__;
    while (descriptor != __image_literal_copies_end__)
    {
        const volatile uint32_t *source = (const volatile uint32_t *)(uintptr_t)*descriptor++;
        volatile uint32_t *destination = (volatile uint32_t *)(uintptr_t)*descriptor++;
        uint32_t words = *descriptor++;
        while (words-- != 0U)
        {
            *destination++ = *source++;
        }
    }
}
/* Alias to word zero of the complete firmware runtime block. */
extern uint32_t runtime_errno;

/* Runtime-context helpers retain the errno path's stack and register side effects. */
__attribute__((naked, used, aligned(4))) uint32_t *runtime_context_helper(void)
{
    __asm volatile("ldr.n r0, [pc, #0]\n"
                   "bx lr\n"
#if defined(DAMIAO_LAYOUT_RELOCATED_SRAM)
                   ".word 0x1ffff4c8");
#elif defined(DAMIAO_LAYOUT_SHIFTED_SRAM)
                   ".word 0x1ffff494");
#else
                   ".word 0x1ffff490");
#endif
}

__attribute__((naked, used, aligned(2))) void runtime_errno_set_helper(uint32_t value)
{
    (void)value;
    __asm volatile("push {r4, lr}\n"
                   "mov r4, r0\n"
                   "bl runtime_context_helper\n"
                   "str r4, [r0]\n"
                   "pop {r4, pc}");
}

/* This path: keep VSQRT, integer classification, callee-saved d8 and
 * the conditional errno call rather than letting C choose a new sequence. */
__attribute__((naked, used, aligned(2))) float checked_sqrtf(float value)
{
    (void)value;
    __asm volatile("push {r4, lr}\n"
                   "vpush {d8}\n"
                   "vsqrt.f32 s16, s0\n"
                   "vmov r0, s16\n"
                   "bic.w r0, r0, #0x80000000\n"
                   "rsb.w r0, r0, #0x7f800000\n"
                   "lsrs r0, r0, #31\n"
                   "beq.n 1f\n"
                   "vmov r0, s0\n"
                   "bic.w r0, r0, #0x80000000\n"
                   "rsb.w r0, r0, #0x7f800000\n"
                   "lsrs r0, r0, #31\n"
                   "itt eq\n"
                   "moveq r0, #1\n"
                   "bleq runtime_errno_set_helper\n"
                   "1: vmov.f32 s0, s16\n"
                   "vpop {d8}\n"
                   "pop {r4, pc}");
}

float fast_sin(float angle)
{
    float sine;
    float cosine;
    angle = motor_wrapf(angle, -0x1.921fb6p+1f, 0x1.921fb6p+1f);
    sincos_helper(angle, &sine, &cosine);
    return sine;
}

/* Stable SRAM entries branch to source-owned implementations in ordinary
 * Flash. Grow the target function, not its fixed-address veneer. */
#define SRAM_ENTRY_VENEER(name, section_name, target)                                              \
    __attribute__((naked, used, section(section_name), aligned(2))) void name(void)                \
    {                                                                                              \
        __asm volatile("b.w " #target);                                                            \
    }

__attribute__((
    used,
    section(".identification_filter_constants"))) static const uint32_t filter_back_calculation =
    0x3E4CCCCDUL;

__attribute__((naked, used, section(".helper_identification_filter"), aligned(2))) void
identification_filter_helper(CommissioningIdentificationFilter *filter)
{
    (void)filter;
    /* The back-calculation literal occupies its own fixed SRAM pool. */
    __asm volatile("vldr s0, [r0]\n"
                   "vldr s1, [r0, #8]\n"
                   "vmul.f32 s0, s0, s1\n"
                   "vstr s0, [r0, #12]\n"
                   "vldr s1, [r0, #16]\n"
                   "vldr s2, [r0, #4]\n"
                   "vmla.f32 s1, s2, s0\n"
                   "vldr s2, [r0, #28]\n"
                   "vldr s3, [pc, #720]\n"
                   "vmla.f32 s1, s2, s3\n"
                   "vstr s1, [r0, #16]\n"
                   "vadd.f32 s1, s0, s1\n"
                   "vstr s1, [r0, #20]\n"
                   "vldr s0, [r0, #32]\n"
                   "vcmpe.f32 s1, s0\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bcc.n 1f\n"
                   "vldr s0, [r0, #36]\n"
                   "vcmpe.f32 s1, s0\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bgt.n 1f\n"
                   "vmov.f32 s0, s1\n"
                   "1: vstr s0, [r0, #24]\n"
                   "vsub.f32 s0, s0, s1\n"
                   "vstr s0, [r0, #28]\n"
                   "bx lr");
}

SRAM_ENTRY_VENEER(fault_monitor_helper, ".helper_fault_monitor", fault_monitor_state_step)
SRAM_ENTRY_VENEER(reset_control_state_helper, ".helper_reset_control_state",
                  reset_control_state_step)
SRAM_ENTRY_VENEER(clear_runtime_loop_states_helper, ".helper_clear_runtime_loop_states",
                  commissioning_clear_runtime_loop_states)

__attribute__((naked, used, section(".helper_derive_runtime_controller_states"), aligned(2))) void
derive_runtime_controller_states_helper(void)
{
    __asm volatile("movw ip, #:lower16:commissioning_derive_runtime_controller_states\n"
                   "movt ip, #:upper16:commissioning_derive_runtime_controller_states\n"
                   "bx ip");
}

__attribute__((naked, used, section(".helper_mcan_send_classic"), aligned(2))) uint32_t
mcan_send_classic_helper(const uint8_t *data, uint16_t id, uint8_t length)
{
    (void)data;
    (void)id;
    (void)length;
    __asm volatile("b.w board_mcan_send_classic_payload");
}

__attribute__((naked, used, section(".helper_mcan_send_fd"), aligned(2))) uint32_t
mcan_send_fd_helper(const uint8_t *data, uint16_t id, uint8_t length)
{
    (void)data;
    (void)id;
    (void)length;
    __asm volatile("b.w board_mcan_send_fd_payload");
}

__attribute__((naked, used, section(".helper_mcan_send_variable_fd"), aligned(2))) uint32_t
mcan_send_variable_fd_helper(const uint8_t *data, uint16_t id, uint8_t length, uint32_t initial_dlc)
{
    (void)data;
    (void)id;
    (void)length;
    (void)initial_dlc;
    __asm volatile("movw ip, #:lower16:board_mcan_send_variable_fd_payload\n"
                   "movt ip, #:upper16:board_mcan_send_variable_fd_payload\n"
                   "bx ip");
}

__attribute__((naked, used, section(".helper_motion_observer"), aligned(2))) void
motion_observer_helper(MotionObserver *observer)
{
    (void)observer;
    __asm volatile("vldr s0, [r0]\n"
                   "vldr s3, [r0, #20]\n"
                   "vsub.f32 s0, s0, s3\n"
                   "vstr s0, [r0, #8]\n"
                   "vldr s1, [r0, #12]\n"
                   "vsub.f32 s1, s0, s1\n"
                   "vldr s2, [pc, #356]\n"
                   "vmul.f32 s2, s1, s2\n"
                   "vstr s2, [r0, #16]\n"
                   "vstr s0, [r0, #12]\n"
                   "vldr s1, [r0, #24]\n"
                   "vldr s4, [r0, #32]\n"
                   "vldr s5, [r0, #40]\n"
                   "vmov.f32 s6, s1\n"
                   "vldr s7, [r0, #4]\n"
                   "vmla.f32 s6, s4, s0\n"
                   "vmla.f32 s2, s4, s0\n"
                   "vmla.f32 s6, s5, s7\n"
                   "vldr s5, [r0, #28]\n"
                   "vmla.f32 s3, s5, s6\n"
                   "vstr s3, [r0, #20]\n"
                   "vldr s3, [r0, #36]\n"
                   "vmul.f32 s3, s5, s3\n"
                   "vmla.f32 s1, s3, s2\n"
                   "vstr s1, [r0, #24]\n"
                   "vldr s0, [r0, #44]\n"
                   "vmul.f32 s0, s1, s0\n"
                   "vstr s0, [r0, #48]\n"
                   "vldr s1, [r0, #56]\n"
                   "vcmpe.f32 s0, s1\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bgt.n 1f\n"
                   "vmov.f32 s1, s0\n"
                   "b.n 2f\n"
                   "1: vstr s1, [r0, #48]\n"
                   "2: vldr s0, [r0, #52]\n"
                   "vcmpe.f32 s1, s0\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bcs.n 3f\n"
                   "vstr s0, [r0, #48]\n"
                   "3: bx lr");
}

__attribute__((
    used, section(".motion_observer_constants"))) static const uint32_t motion_derivative_scale =
    0x447A0000UL;

__attribute__((naked, used, section(".helper_current_controller"), aligned(2))) float
current_controller_helper(CurrentController *axis)
{
    (void)axis;
    __asm volatile("vldr s0, [r0, #8]\n"
                   "vldr s1, [r0, #20]\n"
                   "vsub.f32 s0, s0, s1\n"
                   "vstr s0, [r0, #12]\n"
                   "vldr s3, [r0, #16]\n"
                   "vldr s2, [r0, #24]\n"
                   "vldr s4, [r0, #60]\n"
                   "vmla.f32 s1, s3, s2\n"
                   "vldr s3, [r0, #44]\n"
                   "vmla.f32 s1, s3, s0\n"
                   "vldr s3, [r0, #40]\n"
                   "vmla.f32 s1, s3, s4\n"
                   "vstr s1, [r0, #20]\n"
                   "vldr s3, [r0, #48]\n"
                   "vmla.f32 s2, s3, s0\n"
                   "vstr s2, [r0, #24]\n"
                   "vldr s0, [r0, #4]\n"
                   "vsub.f32 s0, s0, s1\n"
                   "vstr s0, [r0, #52]\n"
                   "vldr s1, [r0]\n"
                   "vmul.f32 s0, s1, s0\n"
                   "vstr s0, [r0, #56]\n"
                   "vsub.f32 s0, s0, s2\n"
                   "vldr s1, [r0, #36]\n"
                   "vmul.f32 s0, s0, s1\n"
                   "vstr s0, [r0, #60]\n"
                   "vldr s1, [r0, #68]\n"
                   "vcmpe.f32 s0, s1\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bcc.n 1f\n"
                   "vmov.f32 s1, s0\n"
                   "b.n 2f\n"
                   "1: vstr s1, [r0, #60]\n"
                   "2: vldr s0, [r0, #72]\n"
                   "vcmpe.f32 s1, s0\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bgt.n 3f\n"
                   "vmov.f32 s0, s1\n"
                   "b.n 4f\n"
                   "3: vstr s0, [r0, #60]\n"
                   "4: vstr s0, [r0, #64]\n"
                   "bx lr");
}

__attribute__((naked, used, section(".helper_output_sensor"), aligned(4))) float
output_sensor_helper(OutputSensorState *state)
{
    (void)state;
    /* This path is the complete lookup/unwrap implementation, not
     * a veneer.  Keeping the body here also avoids an observable data read
     * from a fixed-SRAM veneer literal before the state accesses. */
    __asm volatile("push {r4, lr}\n"
                   "vldr s0, [r0, #24]\n"
                   "vldr s1, [r0, #16]\n"
                   "vldr s2, [r0, #48]\n"
                   "mov r4, r0\n"
                   "vmls.f32 s0, s1, s2\n"
                   "vldr s2, [r0, #44]\n"
                   "vmul.f32 s1, s1, s2\n"
                   "bl atan2_helper\n"
                   "vldr s3, [pc, #152]\n"
                   "vldr s2, [pc, #144]\n"
                   "vmov.f32 s1, s3\n"
                   "vmla.f32 s1, s0, s2\n"
                   "vcvt.s32.f32 s0, s1\n"
                   "vmov r0, s0\n"
                   "cmp r0, #0\n"
                   "it lt\n"
                   "addlt.w r0, r0, #4096\n"
                   "cmp.w r0, #4096\n"
                   "it ge\n"
                   "subge.w r0, r0, #4096\n"
                   "ldr r1, [pc, #116]\n"
                   "vmov.f32 s1, s3\n"
                   "ldrh.w r0, [r1, r0, lsl #1]\n"
                   "ldr r1, [pc, #112]\n"
                   "vmov s0, r0\n"
                   "vcvt.f32.u32 s0, s0\n"
                   "vsub.f32 s0, s0, s1\n"
                   "vldr s1, [pc, #96]\n"
                   "vmul.f32 s0, s0, s1\n"
                   "vstr s0, [r4, #52]\n"
                   "vldr s1, [r4, #64]\n"
                   "vsub.f32 s1, s0, s1\n"
                   "vmov r0, s1\n"
                   "cmp r0, r1\n"
                   "ble.n 1f\n"
                   "ldr r0, [r4, #68]\n"
                   ".inst.n 0x1e40\n" /* subs r0, r0, #1 */
                   "str r0, [r4, #68]\n"
                   "1: vmov r0, s1\n"
                   "ldr r1, [pc, #64]\n"
                   "cmp r0, r1\n"
                   "bls.n 2f\n"
                   "ldr r0, [r4, #68]\n"
                   ".inst.n 0x1c40\n" /* adds r0, r0, #1 */
                   "str r0, [r4, #68]\n"
                   "2: vstr s0, [r4, #64]\n"
                   "vldr s1, [r4, #68]\n"
                   "vldr s2, [pc, #48]\n"
                   "vcvt.f32.s32 s1, s1\n"
                   "vmla.f32 s0, s1, s2\n"
                   "vldr s1, [r4, #56]\n"
                   "vsub.f32 s0, s0, s1\n"
                   "vstr s0, [r4, #60]\n"
                   "pop {r4, pc}\n"
                   ".short 0");
}

__attribute__((naked, used, section(".helper_svpwm"), aligned(2))) void svpwm_helper(float alpha,
                                                                                     float beta)
{
    (void)alpha;
    (void)beta;
    __asm volatile("b.w board_sampling_timer_write_space_vector");
}

/* This firmware entry has only ten bytes before the adjacent sqrt veneer.
 * Materialize the absolute C target with a fixed movw/movt/bx
 * veneer rather than asking the linker for a range-extension thunk. */
__attribute__((naked, used, section(".helper_atan2"), aligned(2))) float atan2_helper(float y,
                                                                                      float x)
{
    (void)y;
    (void)x;
    __asm volatile("movw ip, #:lower16: output_atan2f\n"
                   "movt ip, #:upper16: output_atan2f\n"
                   "bx ip");
}

__attribute__((naked, used, section(".helper_delay_ms"), aligned(2))) void
delay_ms_helper(uint32_t milliseconds)
{
    (void)milliseconds;
    __asm volatile("movw ip, #:lower16:board_delay_ms\n"
                   "movt ip, #:upper16:board_delay_ms\n"
                   "bx ip");
}

__attribute__((naked, used, section(".helper_derive_control_parameters"), aligned(2))) void
derive_control_parameters_helper(void)
{
    __asm volatile("movw ip, #:lower16:platform_derive_control_parameters\n"
                   "movt ip, #:upper16:platform_derive_control_parameters\n"
                   "bx ip");
}

__attribute__((naked, used, section(".helper_select_configuration_bank_b"), aligned(2))) void
select_configuration_bank_b_helper(void)
{
    __asm volatile("movw ip, #:lower16:platform_select_configuration_bank_b\n"
                   "movt ip, #:upper16:platform_select_configuration_bank_b\n"
                   "bx ip");
}

__attribute__((naked, used, section(".helper_sqrt"), aligned(2))) float sqrt_helper(float value)
{
    (void)value;
    __asm volatile("movw ip, #:lower16: checked_sqrtf\n"
                   "movt ip, #:upper16: checked_sqrtf\n"
                   "bx ip");
}
__attribute__((used, section(".rls_constants"))) static const uint32_t rls_constants[3] = {
    0x3F7D70A4UL, 0x3F814AFDUL, 0xBF814AFDUL};

__attribute__((naked, used, section(".helper_rls2"), aligned(2))) void
rls2_helper(CommissioningRls2 *estimator)
{
    (void)estimator;
    __asm volatile("vpush {d8}\n"
                   "vldr s1, [r0, #4]\n"
                   "vldr s7, [r0, #20]\n"
                   "vldr s0, [r0, #8]\n"
                   "vldr s6, [r0, #24]\n"
                   "vmul.f32 s2, s7, s1\n"
                   "vldr s5, [r0, #28]\n"
                   "vldr s8, [r0]\n"
                   "vldr s11, [r0, #12]\n"
                   "vmla.f32 s2, s6, s0\n"
                   "vmul.f32 s3, s5, s1\n"
                   "vldr s4, [r0, #32]\n"
                   "vmov.f32 s14, s8\n"
                   "vmls.f32 s8, s1, s11\n"
                   "vmla.f32 s3, s4, s0\n"
                   "vmul.f32 s9, s1, s2\n"
                   "vldr s12, [r0, #16]\n"
                   "vmls.f32 s8, s0, s12\n"
                   "vmla.f32 s9, s0, s3\n"
                   "vldr s10, [pc, #336]\n"
                   "vmov.f32 s13, #1.0\n"
                   "vadd.f32 s10, s9, s10\n"
                   "vdiv.f32 s9, s13, s10\n"
                   "vmul.f32 s2, s2, s9\n"
                   "vmul.f32 s3, s3, s9\n"
                   "vmov.f32 s9, s13\n"
                   "vmls.f32 s9, s2, s1\n"
                   "vldr s15, [pc, #308]\n"
                   "vmul.f32 s10, s9, s15\n"
                   "vldr s16, [pc, #304]\n"
                   "vmls.f32 s13, s3, s0\n"
                   "vmla.f32 s11, s2, s8\n"
                   "vmul.f32 s9, s2, s16\n"
                   "vmul.f32 s16, s3, s16\n"
                   "vmla.f32 s12, s3, s8\n"
                   "vmul.f32 s9, s9, s0\n"
                   "vmul.f32 s1, s16, s1\n"
                   "vmul.f32 s0, s13, s15\n"
                   "vmul.f32 s13, s10, s7\n"
                   "vmul.f32 s10, s10, s6\n"
                   "vmul.f32 s7, s1, s7\n"
                   "vmul.f32 s1, s1, s6\n"
                   "vmla.f32 s13, s9, s5\n"
                   "vmla.f32 s10, s9, s4\n"
                   "vmla.f32 s7, s0, s5\n"
                   "vmla.f32 s1, s0, s4\n"
                   "vstr s10, [r0, #24]\n"
                   "vstr s7, [r0, #28]\n"
                   "vstr s1, [r0, #32]\n"
                   "vstr s14, [r0, #4]\n"
                   "adds r0, #12\n"
                   "vstmia r0, {s11-s13}\n"
                   "vpop {d8}\n"
                   "bx lr");
}
__attribute__((naked, used, section(".helper_flux_observer"), aligned(2))) void
flux_observer_helper(CommissioningFluxObserver *observer)
{
    (void)observer;
    __asm volatile("vldr s3, [r0, #64]\n"
                   "vldr s0, [r0, #20]\n"
                   "vldr s2, [r0, #28]\n"
                   "vldr s5, [r0, #36]\n"
                   "vmul.f32 s1, s3, s0\n"
                   "vldr s0, [r0, #80]\n"
                   "vldr s6, [r0, #56]\n"
                   "vmov.f32 s4, #1.0\n"
                   "vmla.f32 s1, s2, s5\n"
                   "vmls.f32 s4, s6, s0\n"
                   "vldr s6, [r0, #32]\n"
                   "vmul.f32 s1, s1, s0\n"
                   "vmla.f32 s1, s4, s6\n"
                   "vstr s1, [r0, #32]\n"
                   "vldr s6, [r0, #24]\n"
                   "vmul.f32 s3, s3, s6\n"
                   "vldr s6, [r0, #60]\n"
                   "vmls.f32 s3, s2, s1\n"
                   "vmls.f32 s3, s6, s2\n"
                   "vmul.f32 s3, s3, s0\n"
                   "vmla.f32 s3, s4, s5\n"
                   "vstr s3, [r0, #36]\n"
                   "vldr s4, [r0, #12]\n"
                   "vsub.f32 s5, s4, s1\n"
                   "vstr s5, [r0, #40]\n"
                   "vldr s4, [r0, #16]\n"
                   "vmul.f32 s5, s1, s5\n"
                   "vsub.f32 s4, s4, s3\n"
                   "vstr s4, [r0, #44]\n"
                   "vmla.f32 s5, s3, s4\n"
                   "vldr s1, [r0, #48]\n"
                   "vmul.f32 s3, s2, s4\n"
                   "vmla.f32 s1, s0, s5\n"
                   "vstr s1, [r0, #48]\n"
                   "vldr s2, [r0, #52]\n"
                   "vmla.f32 s2, s0, s3\n"
                   "vstr s2, [r0, #52]\n"
                   "vldr s0, [r0, #68]\n"
                   "vldr s3, [r0, #4]\n"
                   "vmls.f32 s0, s3, s1\n"
                   "vstr s0, [r0, #56]\n"
                   "vldr s1, [r0, #72]\n"
                   "vldr s3, [r0, #8]\n"
                   "vmls.f32 s1, s3, s2\n"
                   "vstr s1, [r0, #60]\n"
                   "vldr s2, [r0]\n"
                   "vmul.f32 s0, s2, s0\n"
                   "vstr s0, [r0, #84]\n"
                   "vmul.f32 s0, s2, s1\n"
                   "vstr s0, [r0, #88]\n"
                   "bx lr");
}
__attribute__((naked, used, section(".helper_sincos"), aligned(2))) void
sincos_helper(float angle, volatile float *sine, volatile float *cosine)
{
    (void)angle;
    (void)sine;
    (void)cosine;
    /* Literal loads use the shared SRAM pool.  Keep the narrow loads and
     * branches together with their VFP register ownership. */
    __asm volatile("push {r4, lr}\n"
                   "vldr s2, [pc, #428]\n"
                   "vldr s1, [pc, #428]\n"
                   "vmla.f32 s1, s0, s2\n"
                   "vmov r2, s1\n"
                   "cmp.w r2, #0x45000000\n"
                   "beq.n 1f\n"
                   "vcmpe.f32 s1, #0.0\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bne.n 2f\n"
                   "1: vldr s0, [pc, #404]\n"
                   "vstr s0, [r0]\n"
                   "vmov.f32 s0, #-1.0\n"
                   "vstr s0, [r1]\n"
                   "pop {r4, pc}\n"
                   "2: vcvt.u32.f32 s0, s1\n"
                   ".inst.n 0x4c60\n" /* ldr r4, [pc, #384]; fixed aligned VMA. */
                   "vmov r2, s0\n"
                   "uxth r2, r2\n"
                   "vmov s0, r2\n"
                   "add.w r3, r4, r2, lsl #2\n"
                   "add.w r2, r2, #512\n"
                   "vcvt.f32.u32 s0, s0\n"
                   "vsub.f32 s0, s1, s0\n"
                   "vldmia r3, {s1-s2}\n"
                   "vsub.f32 s2, s2, s1\n"
                   "vmla.f32 s1, s0, s2\n"
                   "vstr s1, [r0]\n"
                   "ubfx r0, r2, #0, #11\n"
                   "add.w r0, r4, r0, lsl #2\n"
                   "vldmia r0, {s1-s2}\n"
                   "vsub.f32 s2, s2, s1\n"
                   "vmla.f32 s1, s0, s2\n"
                   "vstr s1, [r1]\n"
                   "pop {r4, pc}");
}
/* This path; branch ordering also defines unordered inputs. */
__attribute__((naked, used, section(".helper_clamp"), aligned(2))) float
clamp_helper(float value __attribute__((unused)), float minimum __attribute__((unused)),
             float maximum __attribute__((unused)))
{
    __asm volatile("vcmpe.f32 s0, s2\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "ble.n 2f\n"
                   "vmov.f32 s0, s2\n"
                   "1: bx lr\n"
                   "2: vcmpe.f32 s0, s1\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bcs.n 1b\n"
                   "vmov.f32 s0, s1\n"
                   "bx lr");
}
/* This path: preserve the entire SRAM instruction body, including
 * VCMPE exception behavior and the intermediate conditional store. */
__attribute__((naked, used, section(".helper_wrap"), aligned(2))) void
wrap_helper(volatile float *value __attribute__((unused)), float minimum __attribute__((unused)),
            float maximum __attribute__((unused)))
{
    __asm volatile("vldr s3, [r0]\n"
                   "vsub.f32 s2, s1, s0\n"
                   "vcmpe.f32 s3, s1\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bgt.n 1f\n"
                   "vmov.f32 s1, s3\n"
                   "b.n 2f\n"
                   "1: vsub.f32 s1, s3, s2\n"
                   "vstr s1, [r0]\n"
                   "2: vcmpe.f32 s1, s0\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "bcs.n 3f\n"
                   "vadd.f32 s0, s1, s2\n"
                   "vstr s0, [r0]\n"
                   "3: bx lr");
}
/* This path is an unreferenced but valid arithmetic body between
 * wrap-once and vector limit. Preserve it as well as the referenced entries. */
__attribute__((naked, used, section(".helper_wrap"), aligned(2))) static float
wrap_angle_helper(float angle)
{
    (void)angle;
    __asm volatile("vldr s1, [pc, #236]\n"
                   ".inst.n 0x493c\n" /* ldr r1, [pc, #240], positive pi. */
                   "vmul.f32 s1, s0, s1\n"
                   "vcvt.s32.f32 s1, s1\n"
                   "vcvt.f32.s32 s2, s1\n"
                   "vldr s1, [pc, #220]\n"
                   "vmls.f32 s0, s2, s1\n"
                   "vmov r0, s0\n"
                   "cmp r0, r1\n"
                   "ble.n 1f\n"
                   "vsub.f32 s0, s0, s1\n"
                   "1: .inst.n 0x4934\n" /* ldr r1, [pc, #208], negative pi. */
                   "vmov r0, s0\n"
                   "cmp r0, r1\n"
                   "bls.n 2f\n"
                   "vadd.f32 s0, s0, s1\n"
                   "2: bx lr");
}
__attribute__((naked, used, section(".helper_limit_vector"), aligned(2))) void
limit_vector_helper(float limit, volatile float *x, volatile float *y)
{
    (void)limit;
    (void)x;
    (void)y;
    __asm volatile("push {r4, r5, r6, lr}\n"
                   "mov r4, r1\n"
                   "vpush {d8}\n"
                   "vmov.f32 s16, s0\n"
                   "vldr s0, [r0]\n"
                   "mov r5, r0\n"
                   "vmul.f32 s1, s0, s0\n"
                   "vldr s0, [r1]\n"
                   "vmla.f32 s1, s0, s0\n"
                   "vmov.f32 s0, s1\n"
                   "bl sqrt_helper\n"
                   "vcmpe.f32 s0, s16\n"
                   "vmrs APSR_nzcv, fpscr\n"
                   "ble.n 1f\n"
                   "vldr s1, [r5]\n"
                   "vmul.f32 s1, s1, s16\n"
                   "vdiv.f32 s2, s1, s0\n"
                   "vstr s2, [r5]\n"
                   "vldr s1, [r4]\n"
                   "vmul.f32 s1, s1, s16\n"
                   "vdiv.f32 s2, s1, s0\n"
                   "vstr s2, [r4]\n"
                   "1: vpop {d8}\n"
                   "pop {r4, r5, r6, pc}");
}

__attribute__((naked, used, section(".helper_float_to_uint"), aligned(2))) uint32_t
float_to_uint_helper(float value, float minimum, float maximum, uint8_t bits)
{
    (void)value;
    (void)minimum;
    (void)maximum;
    (void)bits;
    __asm volatile("movs r1, #1\n"
                   "vsub.f32 s2, s2, s1\n"
                   "lsls r1, r0\n"
                   "vsub.f32 s1, s0, s1\n"
                   ".inst.n 0x1e49\n" /* subs r1, r1, #1 using the required T1 encoding. */
                   "vmov s0, r1\n"
                   "vcvt.f32.s32 s0, s0\n"
                   "vmul.f32 s0, s1, s0\n"
                   "vdiv.f32 s1, s0, s2\n"
                   "vcvt.s32.f32 s0, s1\n"
                   "vmov r0, s0\n"
                   "bx lr");
}

__attribute__((naked, used, section(".helper_uint_to_float"), aligned(2))) float
uint_to_float_helper(uint32_t value, float minimum, float maximum, uint8_t bits)
{
    (void)value;
    (void)minimum;
    (void)maximum;
    (void)bits;
    __asm volatile("vmov s2, r0\n"
                   "vsub.f32 s1, s1, s0\n"
                   "movs r0, #1\n"
                   "vcvt.f32.s32 s2, s2\n"
                   "lsls r0, r1\n"
                   ".inst.n 0x1e40\n" /* subs r0, r0, #1 using the required T1 encoding. */
                   "vmul.f32 s2, s2, s1\n"
                   "vmov s1, r0\n"
                   "vcvt.f32.s32 s3, s1\n"
                   "vdiv.f32 s1, s2, s3\n"
                   "vadd.f32 s0, s1, s0\n"
                   "bx lr\n"
                   /* Shared arithmetic constants follow the alignment padding. */
                   ".hword 0\n"
                   ".word 0x43a2f983\n" /* Sine table index scale. */
                   ".word 0x44800000\n" /* Index origin: 1024. */
                   ".word 0x00000000\n"
                   ".word motor_sine_table\n"
                   ".word 0x3e22f983\n" /* Reciprocal two pi. */
                   ".word 0x40c90fdb\n" /* Two pi. */
                   ".word 0x40490fdb\n" /* Pi. */
                   ".word 0xc0490fdb"); /* Negative pi. */
}
