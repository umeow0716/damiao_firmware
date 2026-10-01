#include "runtime_compat.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "app_profile.h"
#include "hc32f448.h"

#if defined(DAMIAO_DM4310)

void dm4310_runtime_errno_set_helper(uint32_t value);

uint32_t dm4310_runtime_stream_error(const volatile void *stream)
{
    /* Factory 0x21070 returns the mask, not a normalized boolean. */
    const volatile uint8_t *const bytes = stream;
    return bytes[12] & UINT32_C(0x80);
}

#if defined(__arm__) || defined(__thumb__)
/* Factory 0x242d4: preserve every register except the S0 result/FPSCR. */
__attribute__((naked, noinline, aligned(4)))
float dm4310_runtime_force_underflow(void)
{
    __asm volatile (
        "vldr s0, 1f\n"
        "vmul.f32 s0, s0, s0\n"
        "bx lr\n"
        ".balign 4\n"
        "1: .word 0x10000000\n");
}
#else
float dm4310_runtime_force_underflow(void)
{
    volatile float operand = 0x1p-95f;
    return operand * operand;
}
#endif

typedef struct __attribute__((packed, may_alias)) {
    uint32_t value;
} RuntimeCopyWord;

typedef struct __attribute__((packed, may_alias)) {
    uint16_t value;
} RuntimeCopyHalfword;

static void runtime_copy_words(volatile uint8_t **destination,
                               const volatile uint8_t **source,
                               unsigned count)
{
    uint32_t words[4];
    for (unsigned i = 0U; i < count; ++i) {
        words[i] = ((const volatile RuntimeCopyWord *)(*source + i * 4U))->value;
    }
    for (unsigned i = 0U; i < count; ++i) {
        ((volatile RuntimeCopyWord *)(*destination + i * 4U))->value = words[i];
    }
    *source += count * 4U;
    *destination += count * 4U;
}

void dm4310_runtime_copy_bytes(void *destination, const void *source, size_t size)
{
    /* 0x20734: preserve alignment-dependent read/write grouping, including
     * retained-byte/overlap behavior of malformed calibration uploads. */
    volatile uint8_t *out = destination;
    const volatile uint8_t *in = source;
    bool aligned_tail = false;
    if (size > 3U) {
        const unsigned misalignment = (uintptr_t)out & 3U;
        if (misalignment != 0U) {
            const uint8_t first = *in++;
            uint8_t second = 0U;
            uint8_t third = 0U;
            if (misalignment <= 2U) {
                second = *in++;
            }
            *out++ = first;
            if (misalignment == 1U) {
                third = *in++;
            }
            if (misalignment <= 2U) {
                *out++ = second;
            }
            if (misalignment == 1U) {
                *out++ = third;
            }
            size -= 4U - misalignment;
        }
        if (((uintptr_t)in & 3U) != 0U) {
            while (size >= 8U) {
                runtime_copy_words(&out, &in, 2U);
                size -= 8U;
            }
        } else {
            aligned_tail = true;
            while (size >= 32U) {
                runtime_copy_words(&out, &in, 4U);
                runtime_copy_words(&out, &in, 4U);
                size -= 32U;
            }
            if (size >= 16U) {
                runtime_copy_words(&out, &in, 4U);
                size -= 16U;
            }
            if (size >= 8U) {
                runtime_copy_words(&out, &in, 2U);
                size -= 8U;
            }
        }
        if (size >= 4U) {
            runtime_copy_words(&out, &in, 1U);
            size -= 4U;
        }
    }
    if (aligned_tail) {
        uint16_t halfword = 0U;
        uint8_t byte = 0U;
        if (size >= 2U) {
            halfword = ((const volatile RuntimeCopyHalfword *)in)->value;
            in += 2U;
        }
        if ((size & 1U) != 0U) {
            byte = *in;
        }
        if (size >= 2U) {
            ((volatile RuntimeCopyHalfword *)out)->value = halfword;
            out += 2U;
        }
        if ((size & 1U) != 0U) {
            *out = byte;
        }
        return;
    }
    uint8_t first = 0U;
    uint8_t second = 0U;
    uint8_t third = 0U;
    if (size >= 2U) {
        first = *in++;
        second = *in++;
    }
    if ((size & 1U) != 0U) {
        third = *in;
    }
    if (size >= 2U) {
        *out++ = first;
        *out++ = second;
    }
    if ((size & 1U) != 0U) {
        *out = third;
    }
}

/* Softdouble arithmetic uses the shared factory instruction cores in
 * dm4310_extended_core.S, including their raw flag and stack contract. */

/* Factory 0x27450: reciprocal seeds for normalized high bytes 128..255. */
static const uint8_t seeds[128]
    __attribute__((section(".dm4310_division_seeds"), aligned(1), used)) = {
    255, 253, 251, 249, 247, 245, 244, 242, 240, 238, 237, 235, 233, 232, 230, 228,
    227, 225, 224, 222, 221, 219, 218, 216, 215, 213, 212, 211, 209, 208, 207, 205,
    204, 203, 202, 200, 199, 198, 197, 196, 194, 193, 192, 191, 190, 189, 188, 187,
    186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, 175, 174, 173, 172, 171,
    170, 169, 168, 168, 167, 166, 165, 164, 163, 163, 162, 161, 160, 159, 159, 158,
    157, 156, 156, 155, 154, 153, 153, 152, 151, 151, 150, 149, 149, 148, 147, 147,
    146, 145, 145, 144, 143, 143, 142, 142, 141, 140, 140, 139, 139, 138, 137, 137,
    136, 136, 135, 135, 134, 133, 133, 132, 132, 131, 131, 130, 130, 129, 129, 128
};

uint32_t dm4310_runtime_double_to_uint(double value)
{
    /* 0x274d8 uses the high word including its sign in exponent extraction.
     * ARM register shifts >=32 yield zero; never express those as C shifts. */
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    const uint32_t high = (uint32_t)(bits >> 32U);
    const uint32_t low = (uint32_t)bits;
    const int32_t shift = 1054 - (int32_t)(high >> 20U);
    if (shift >= 0) {
        const uint32_t mantissa = UINT32_C(0x80000000) |
                                  (high << 11U) | (low >> 21U);
        return shift < 32 ? mantissa >> (unsigned)shift : 0U;
    }
    if ((high & UINT32_C(0x80000000)) != 0U) {
        return 0U;
    }
    /* NaNs select action 4 of unary descriptor 0x24802492 at 0x27528.
     * 0x27720 writes canonical double NaN (low word zero); the uint caller
     * returns that low word. This path performs no errno/FPSCR/MMIO writes.
     * Positive infinity selects action 0, returning UINT32_MAX at 0x27530. */
    if ((high & UINT32_C(0x7fffffff)) > UINT32_C(0x7ff00000) ||
        (high == UINT32_C(0x7ff00000) && low != 0U)) {
        return 0U;
    }
    return UINT32_MAX;
}

unsigned __wrap___aeabi_d2uiz(double value)
{
    return dm4310_runtime_double_to_uint(value);
}

/* Float rounding and its integer conversion caller share original ASM. */

double __wrap___aeabi_ui2d(unsigned value)
{
    /* 0x27532 is unsigned conversion, despite the old signed ledger name.
     * Guard zero before CLZ/shift: C must not shift a word by 32. */
    uint64_t bits = 0U;
    if (value != 0U) {
        const unsigned leading = (unsigned)__builtin_clz(value);
        const uint32_t normalized = value << leading;
        bits = ((uint64_t)(1054U - leading) << 52U) |
               ((uint64_t)(normalized & UINT32_C(0x7fffffff)) << 21U);
    }
    double converted;
    memcpy(&converted, &bits, sizeof(converted));
    return converted;
}

__attribute__((naked, noipa))
float dm4310_runtime_uint64_to_float(uint64_t value __attribute__((unused)))
{
    /* 0x27b3c/0x27b56: arithmetic-only leaf. Preserve the factory scratch
     * registers, flags and absence of stack traffic, not just rounded bits. */
    __asm volatile (
        "mov.w r2, #0x3f800000\n"
        "b 1f\n"
        "1: movs r3, r1\n"
        "add.w r2, r2, #0x0f000000\n"
        "itee ne\n"
        "movne r3, #32\n"
        "movseq.w r1, r0\n"
        "bxeq lr\n"
        "clz ip, r1\n"
        "sub.w r3, r3, ip\n"
        "lsl.w r1, r1, ip\n"
        "lsr.w ip, r0, r3\n"
        "add.w r2, r2, r3, lsl #23\n"
        "rsb.w r3, r3, #32\n"
        "orr.w r1, r1, ip\n"
        "lsls.w ip, r0, r3\n"
        "orrs.w ip, ip, r1, lsl #25\n"
        "adc.w r0, r2, r1, lsr #8\n"
        "it ne\n"
        "movsne.w ip, #0x04000000\n"
        "it cs\n"
        "biccs.w r0, r0, #1\n"
        "bx lr\n");
}


/* Float-to-double uses the original raw conversion/exception core in ASM. */

float __aeabi_d2f(double value)
{
    /* Factory 0x27074: integer-only rounding and exception dispatch. The
     * AAPCS declaration keeps the compiler helper's R0/R1 -> R0 ABI even
     * though normal application functions use hard-float argument passing. */
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    const uint32_t low = (uint32_t)bits;
    const uint32_t high = (uint32_t)(bits >> 32U);
    const uint32_t sign = high & UINT32_C(0x80000000);
    const uint32_t magnitude = high & UINT32_C(0x7fffffff);
    const uint32_t adjusted = magnitude - 0x38000000U;
    uint32_t result;
    if ((adjusted >= 0x100000U) && (adjusted < 0x0ff00000U)) {
        const uint32_t carry = (low >> 28U) & 1U;
        result = (sign | (adjusted << 3U)) + (low >> 29U) + carry;
        if (((low << 4U) == 0U) && (carry != 0U)) {
            result &= ~UINT32_C(1);
        }
    } else if ((int32_t)adjusted < 0x100000) {
        result = sign;
    } else if ((magnitude | (uint32_t)(low != 0U)) > 0x7ff00000U) {
        result = UINT32_C(0x7fc00000);
    } else {
        result = sign | UINT32_C(0x7f800000);
    }
    float converted;
    memcpy(&converted, &result, sizeof(converted));
    return converted;
}

#define DM4310_RUNTIME_WORD_COUNT (24U)
#if defined(DAMIAO_DM8009_V3)
#define DM4310_HEAP_BASE          (0x1FFFF530UL)
#define DM4310_HEAP_LIMIT         (0x1FFFF930UL)
#else
#define DM4310_HEAP_BASE          (0x1FFFF4F8UL)
#define DM4310_HEAP_LIMIT         (0x1FFFF8F8UL)
#endif
#define DM4310_HEAP_FREE_HEADER   (DM4310_HEAP_BASE + 0x14UL)

typedef struct {
    uint32_t decimal_offset;
    uint32_t thousands_offset;
    uint32_t grouping_offset;
    char strings[4];
} Dm4310Locale;

__attribute__((used, section(".dm4310_c_locale")))
static const Dm4310Locale dm4310_c_locale = {
    .decimal_offset = 12U,
    .thousands_offset = 14U,
    .grouping_offset = 15U,
    .strings = {'.', '\0', '\0', '\0'},
};

_Static_assert(sizeof(Dm4310Locale) == 16U,
               "DM4310 locale object size changed");
_Static_assert(offsetof(Dm4310Locale, strings) == 12U,
               "DM4310 locale strings moved");

__attribute__((used, section(".dm4310_c_locale_name")))
static const char dm4310_c_locale_name[4] = {'C', '\0', '\0', '\0'};

__attribute__((noipa))
static int runtime_locale_compare(const char *left, const char *right)
{
    const uint32_t ones = UINT32_C(0x01010101);
    for (;;) {
        for (unsigned word = 0U; word < 3U; ++word) {
            uint32_t a, b;
            __asm__ volatile ("ldr %0, [%2], #4\n\tldr %1, [%3], #4"
                              : "=&r" (a), "=&r" (b),
                                "+r" (left), "+r" (right)
                              : : "memory");
            if (a != b) {
                uint32_t shift, prefix;
                const uint32_t difference = a - b;
                __asm__ volatile ("rev %0, %2\n\tclz %0, %0\n\t"
                                  "and %0, %0, #24\n\t"
                                  "rsb %1, %0, #32\n\t"
                                  "lsr %1, %3, %1"
                                  : "=&r" (shift), "=&r" (prefix)
                                  : "r" (difference), "r" (ones));
                if (((a - prefix) & ~a & (prefix << 7U)) != 0U) {
                    return 0;
                }
                return (int)((a >> shift) & 0xffU) -
                       (int)((b >> shift) & 0xffU);
            }
            if (((a - ones) & ~a & UINT32_C(0x80808080)) != 0U) {
                return 0;
            }
        }
    }
}

__attribute__((noipa))
static const Dm4310Locale *runtime_select_locale(uint32_t category,
                                                const char *name)
{
    (void)category;
    if ((name != NULL) && (*(volatile const uint8_t *)name != 0U) &&
        (runtime_locale_compare(dm4310_c_locale_name, name) != 0)) {
        return NULL;
    }
    return &dm4310_c_locale;
}

__attribute__((used, section(".dm4310_c_runtime_state")))
uint32_t dm4310_c_runtime_state[DM4310_RUNTIME_WORD_COUNT];

extern uint32_t dm4310_runtime_errno
    __attribute__((alias("dm4310_c_runtime_state")));

__attribute__((noipa))
static void runtime_semihost_character(uint32_t character)
{
    /* Factory's eight-byte frame places the byte at its base. */
    volatile uint8_t byte __attribute__((aligned(8))) = (uint8_t)character;
    register uint32_t operation __asm__("r0") = 3U;
    register const volatile uint8_t *argument __asm__("r1") = &byte;
    __asm__ volatile ("bkpt #0xab"
                      : "+r" (operation), "+r" (argument) : : "memory");
}

__attribute__((noipa))
static void runtime_semihost_diagnostic(const char *message, const char *detail)
{
    runtime_semihost_character(10U);
    if (message != NULL) {
        for (const char *p = message; *p != '\0'; ++p) {
            runtime_semihost_character((uint8_t)*p);
        }
    }
    if (detail != NULL) {
        for (const char *p = detail; *p != '\0'; ++p) {
            runtime_semihost_character((uint8_t)*p);
        }
    }
    runtime_semihost_character(10U);
}

__attribute__((noipa))
static uint32_t runtime_heap_report(uint32_t reason)
{
    runtime_semihost_diagnostic("SIGRTMEM: Out of heap memory",
        reason == 1U ? ": Heap memory corrupted" : NULL);
    return 1U;
}

__attribute__((naked, noipa, noreturn))
void dm4310_runtime_exit(uint32_t status, uint32_t context)
{
    (void)status;
    (void)context;
    /* Factory 210d2 -> 2037a: preserve the real finalizer frame even
     * though its body is empty. The halt entry never returns. */
    __asm__ volatile (
        "push {r4, lr}\n"
        "mov r4, r0\n"
        "nop.w\n"
        "mov r0, r4\n"
        "pop {r4, lr}\n"
        "b .Ldm4310_exit_finish\n"
        ".Ldm4310_exit_finalize:\n"
        "push {r4, lr}\n"
        "pop {r4, pc}\n"
        ".Ldm4310_exit_finish:\n"
        "push {r0, r1}\n"
        "bl .Ldm4310_exit_finalize\n"
        "pop {r0, r1}\n"
        "bl dm4310_runtime_halt\n");
}

__attribute__((noipa))
__attribute__((noreturn))
void dm4310_runtime_halt(void)
{
    register uint32_t operation __asm__("r0") = 0x18U;
    register uint32_t reason __asm__("r1") = UINT32_C(0x20026);
    __asm__ volatile ("bkpt #0xab"
                      : "+r" (operation), "+r" (reason) : : "memory");
    for (;;) {
        __asm__ volatile ("b .");
    }
}

__attribute__((noipa))
static void runtime_halt_if_error(uint32_t error)
{
    if (error != 0U) {
        dm4310_runtime_halt();
    }
}

__attribute__((noipa))
static void runtime_heap_fatal(uint32_t reason)
{
    runtime_halt_if_error(runtime_heap_report(reason));
}

__attribute__((noipa))
static void runtime_heap_check_region(uint32_t base, uint32_t limit)
{
    if (base + 16U > limit) {
        runtime_heap_fatal(0U);
    }
}

/* Factory 0x20e1c: insert a supplied region, then use normal coalescing. */
static void runtime_heap_insert(volatile uint32_t *root, uint32_t address,
                                uint32_t size)
{
    volatile uint32_t *previous = root;
    volatile uint32_t *next =
        (volatile uint32_t *)(uintptr_t)root[1];
    while (next != NULL && (uint32_t)(uintptr_t)next < address) {
        previous = next;
        next = (volatile uint32_t *)(uintptr_t)next[1];
    }
    if (previous[0] + (uint32_t)(uintptr_t)previous != address) {
        const uint32_t aligned = ((address + 3U) & ~UINT32_C(7)) + 4U;
        size -= aligned - address;
        address = aligned;
    }
    *(volatile uint32_t *)(uintptr_t)address = size;
    dm4310_runtime_free((void *)(uintptr_t)(address + 4U));
}

void dm4310_runtime_initialize(void)
{
    __set_FPSCR(0x03000000UL);

    /* Reset's runtime-context entry already performed the temporary-stack
     * phase and sixteen-word clear before this heap/locale setup. */

    runtime_heap_check_region(DM4310_HEAP_BASE, DM4310_HEAP_LIMIT);
    dm4310_c_runtime_state[2] = DM4310_HEAP_BASE;
    volatile uint32_t *const sentinel =
        (volatile uint32_t *)DM4310_HEAP_BASE;
    sentinel[0] = 0U;
    /* 0x208c2 initializes next/self as one STRD before insertion. */
    __asm volatile ("strd %0, %1, [%2, #4]"
                    : : "r" (0U), "r" ((uint32_t)DM4310_HEAP_BASE),
                      "r" (sentinel) : "memory");
    /* 0x208b4 reloads the root before the generic insertion tail-call. */
    volatile uint32_t *const root = (volatile uint32_t *)(uintptr_t)
        *(volatile const uint32_t *)&dm4310_c_runtime_state[2];
    runtime_heap_insert(root, DM4310_HEAP_BASE + 16U,
                        DM4310_HEAP_LIMIT - (DM4310_HEAP_BASE + 16U));
    /* Factory initializes the free list before selecting/storing locale at
     * auxiliary context +0x0c (0x1ffff4bc). */
    dm4310_c_runtime_state[11] =
        (uint32_t)(uintptr_t)runtime_select_locale(0U, NULL);
}

char dm4310_runtime_decimal_separator(void)
{
    const uint8_t *const locale =
        (const uint8_t *)(uintptr_t)dm4310_c_runtime_state[11];
    uint32_t offset;
    memcpy(&offset, locale, sizeof(offset));
    return (char)locale[offset];
}

void *dm4310_runtime_alloc(size_t size)
{
    /* 0x203cc..3d0 reads the root before checking size overflow. */
    volatile uint32_t *const root = (volatile uint32_t *)(uintptr_t)
        *(volatile const uint32_t *)&dm4310_c_runtime_state[2];
    const uint32_t requested = (uint32_t)size;
    const uint32_t block_size = (requested + 11U) & ~UINT32_C(7);
    if (block_size <= requested) {
        return NULL;
    }

    volatile uint32_t *previous = root;
    volatile uint32_t *block =
        (volatile uint32_t *)(uintptr_t)root[1];
    while (block != NULL) {
        const uint32_t available = block[0];
        if (available >= block_size) {
            if (available < block_size + 8U) {
                previous[1] = block[1];
            } else {
                volatile uint32_t *const remainder =
                    (volatile uint32_t *)((uintptr_t)block + block_size);
                remainder[1] = block[1];
                /* Factory reloads size after publishing remainder->next. */
                remainder[0] = block[0] - block_size;
                previous[1] = (uint32_t)(uintptr_t)remainder;
                block[0] = block_size;
            }
            return (void *)((uintptr_t)block + 4U);
        }
        previous = block;
        block = (volatile uint32_t *)(uintptr_t)block[1];
    }

    /* heap_extend@0x20838 reaches a no-op runtime hook and returns zero in
     * this fixed 0x400-byte region. */
    return NULL;
}

void dm4310_runtime_free(void *pointer)
{
    /* Factory 0x2042c..430 fetches the heap root even for free(NULL). */
    volatile uint32_t *const root = (volatile uint32_t *)(uintptr_t)
        *(volatile const uint32_t *)&dm4310_c_runtime_state[2];
    if (pointer == NULL) {
        return;
    }

    volatile uint32_t *block =
        (volatile uint32_t *)((uintptr_t)pointer - 4U);
    volatile uint32_t *previous = root;
    volatile uint32_t *next =
        (volatile uint32_t *)(uintptr_t)root[1];
    while ((next != NULL) && ((uintptr_t)next < (uintptr_t)block)) {
        previous = next;
        next = (volatile uint32_t *)(uintptr_t)next[1];
    }

    const uint32_t previous_size = previous[0];
    if ((uintptr_t)previous + previous_size == (uintptr_t)block) {
        const uint32_t released_size = block[0];
        previous[0] = previous_size + released_size;
        block = previous;
    } else {
        previous[1] = (uint32_t)(uintptr_t)block;
    }

    const uint32_t block_size = block[0];
    if ((uintptr_t)block + block_size == (uintptr_t)next) {
        block[1] = next[1];
        block[0] = block_size + next[0];
    } else {
        block[1] = (uint32_t)(uintptr_t)next;
    }
}

#else

void dm4310_runtime_initialize(void)
{
}

#endif
