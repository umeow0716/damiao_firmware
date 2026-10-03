#ifndef DAMIAO_RUNTIME_COMPAT_H
#define DAMIAO_RUNTIME_COMPAT_H

#include <stddef.h>
#include <stdint.h>

void runtime_initialize(void);
void runtime_halt(void) __attribute__((noreturn));
void runtime_exit(uint32_t status, uint32_t context) __attribute__((noreturn));
int32_t runtime_round_to_int(float value);
uint32_t runtime_double_to_uint(double value);
void runtime_copy_bytes(void *destination, const void *source, size_t size);
#if defined(__arm__) || defined(__thumb__)
float runtime_uint64_to_float(uint64_t value) __attribute__((pcs("aapcs")));
float __aeabi_d2f(double value) __attribute__((pcs("aapcs")));
double __wrap___aeabi_dadd(double left, double right) __attribute__((pcs("aapcs")));
double __wrap___aeabi_dsub(double left, double right) __attribute__((pcs("aapcs")));
double __wrap___aeabi_dmul(double left, double right) __attribute__((pcs("aapcs")));
double __wrap___aeabi_ddiv(double left, double right) __attribute__((pcs("aapcs")));
double __wrap___aeabi_f2d(float value) __attribute__((pcs("aapcs")));
double __wrap___aeabi_ui2d(unsigned value) __attribute__((pcs("aapcs")));
unsigned __wrap___aeabi_d2uiz(double value) __attribute__((pcs("aapcs")));
#else
float runtime_uint64_to_float(uint64_t value);
#endif
void *runtime_alloc(size_t size);
void runtime_free(void *pointer);
float runtime_force_underflow(void);
uint32_t runtime_stream_error(const volatile void *stream);
char runtime_decimal_separator(void);

#endif
