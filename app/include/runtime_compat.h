#ifndef DAMIAO_RUNTIME_COMPAT_H
#define DAMIAO_RUNTIME_COMPAT_H

#include <stddef.h>
#include <stdint.h>

void dm4310_runtime_initialize(void);
#if defined(DAMIAO_DM4310)
void dm4310_runtime_halt(void) __attribute__((noreturn));
void dm4310_runtime_exit(uint32_t status, uint32_t context)
    __attribute__((noreturn));
int32_t dm4310_runtime_round_to_int(float value);
uint32_t dm4310_runtime_double_to_uint(double value);
void dm4310_runtime_copy_bytes(void *destination, const void *source, size_t size);
#if defined(__arm__) || defined(__thumb__)
float dm4310_runtime_uint64_to_float(uint64_t value) __attribute__((pcs("aapcs")));
float __aeabi_d2f(double value) __attribute__((pcs("aapcs")));
double __wrap___aeabi_dadd(double left, double right) __attribute__((pcs("aapcs")));
double __wrap___aeabi_dsub(double left, double right) __attribute__((pcs("aapcs")));
double __wrap___aeabi_dmul(double left, double right) __attribute__((pcs("aapcs")));
double __wrap___aeabi_ddiv(double left, double right) __attribute__((pcs("aapcs")));
double __wrap___aeabi_f2d(float value) __attribute__((pcs("aapcs")));
double __wrap___aeabi_ui2d(unsigned value) __attribute__((pcs("aapcs")));
unsigned __wrap___aeabi_d2uiz(double value) __attribute__((pcs("aapcs")));
#else
float dm4310_runtime_uint64_to_float(uint64_t value);
#endif
void *dm4310_runtime_alloc(size_t size);
void dm4310_runtime_free(void *pointer);
float dm4310_runtime_force_underflow(void);
uint32_t dm4310_runtime_stream_error(const volatile void *stream);
char dm4310_runtime_decimal_separator(void);
#endif

#endif
