#!/usr/bin/env python3
"""Isolated host test for the production binary64 classifier."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include <assert.h>
#include <stdint.h>

#include "formatter_arithmetic.h"

int main(void)
{
    static const struct {
        uint64_t bits;
        uint32_t classification;
    } cases[] = {
        {UINT64_C(0x0000000000000000), 0U},
        {UINT64_C(0x8000000000000000), 0U},
        {UINT64_C(0x0000000000000001), 4U},
        {UINT64_C(0x8000000000000001), 4U},
        {UINT64_C(0x3ff0000000000000), 5U},
        {UINT64_C(0xbff0000000000000), 5U},
        {UINT64_C(0x7ff0000000000000), 3U},
        {UINT64_C(0xfff0000000000000), 3U},
        {UINT64_C(0x7ff8000000000001), 7U},
        {UINT64_C(0xfff8000000000001), 7U},
    };
    for (unsigned index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        assert(factory_binary64_classify(cases[index].bits) ==
               cases[index].classification);
    }
    return 0;
}
'''


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="factory-float-class-") as temporary:
        directory = Path(temporary)
        source = directory / "test.c"
        executable = directory / "test"
        source.write_text(HARNESS, encoding="utf-8")
        subprocess.run(
            [
                "cc",
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(ROOT / "app/include"),
                str(source),
                str(ROOT / "app/src/softfloat_binary64.c"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run([str(executable)], check=True)
    print("Binary64 classification: 10 edge cases passed.")


if __name__ == "__main__":
    main()
