#!/usr/bin/env python3
"""Test production formatter classification only; not decimal equivalence."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / "app/src/debug_console.c").read_text()
start = source.index("typedef struct FactoryFormatterStream")
end = source.index("/* Factory 20328", start)
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#define DAMIAO_DM4310 1
#include "dm4310_formatter_arithmetic.h"
static char output[16];
static size_t length;
static unsigned finite_calls;
static void platform_debug_write(const char *s, size_t n) {
    memcpy(output+length,s,n); length+=n; output[length]=0;
}
static char dm4310_runtime_decimal_separator(void) { return '.'; }
void dm4310_factory_fixed_digits(uint64_t bits, unsigned precision,
                                 Dm4310FactoryDecimal *decimal) {
    assert(bits == 0 && precision == 6);
    decimal->count=0; decimal->exponent=-7; decimal->digit[0]=0;
    ++finite_calls;
}
'''
cases = r'''
int main(void) {
    const uint64_t inputs[] = {0, UINT64_C(0x8000000000000000),
        UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8000000000001), UINT64_C(0xfff8000000000001)};
    const char *expected[] = {"0.000000","-0.000000","inf","-inf","nan","-nan"};
    for (unsigned i=0;i<6;++i) {
        FactoryFormatterStream stream = {.write = factory_stream_uart};
        double value; memcpy(&value,&inputs[i],8); length=0;
        write_factory_float(value,6,&stream); assert(strcmp(output,expected[i])==0);
    }
    assert(finite_calls==2); return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="dm4310-float-class-") as tmp:
    path = Path(tmp)
    (path / "test.c").write_text(harness + source[start:end] + cases)
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "app/include"),
                    str(path / "test.c"),
                    str(root / "app/src/dm4310_binary64_extended.c"),
                    "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
print("Float classification: 6 cases passed; finite conversion mocked.")
