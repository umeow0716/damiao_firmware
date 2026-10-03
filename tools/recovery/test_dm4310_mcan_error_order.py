#!/usr/bin/env python3
"""Isolated host test of production MCAN error orchestration.

Board MMIO is mocked: this checks call and state order, not hardware behavior.
The test is deliberately absent from the normal build graph.
"""

from pathlib import Path
import subprocess
import tempfile


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    source = (root / "app/src/platform.c").read_text(encoding="utf-8")
    start = source.index("uint8_t platform_ack_mcan_irq(")
    end = source.index("\nvoid platform_ack_debug_uart_irq", start)
    function = source[start:end]
    harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct FactoryMcanIrqReferences {
    volatile uint32_t *status;
} FactoryMcanIrqReferences;

static uint32_t interrupt_state;
static uint32_t state_after_reinitialization;
static unsigned call_count;
static char trace[4];

static void note(char event)
{
    trace[call_count++] = event;
}

static bool board_mcan_reinitialization_requested(
    const FactoryMcanIrqReferences *references)
{
    assert(references != 0);
    note('R');
    return (interrupt_state & UINT32_C(0x00800000)) != 0U;
}

static void board_mcan_reinitialize_live(
    bool after_parameter_write, const FactoryMcanIrqReferences *references)
{
    assert(!after_parameter_write && references != 0);
    note('I');
    interrupt_state = state_after_reinitialization;
}

static uint8_t board_mcan_recover_bus_off(
    const FactoryMcanIrqReferences *references)
{
    assert(references != 0);
    note('B');
    if (call_count == 3U) {
        assert(references->status[12] == 1U);
    }
    return (interrupt_state & UINT32_C(0x02000000)) != 0U ? 2U : 0U;
}

static uint8_t board_mcan_ack_interrupt(
    const FactoryMcanIrqReferences *references)
{
    assert(references != 0);
    note('A');
    return 0U;
}
'''
    cases = r'''
int main(void)
{
    static const struct {
        uint32_t initial;
        uint32_t after_reinitialization;
        uint32_t expected_status;
        char expected_trace[4];
        unsigned expected_calls;
    } cases[] = {
        {0U, 0U, 7U, {'R', 'B', 'A', 0}, 3U},
        {UINT32_C(0x02000000), 0U, 2U, {'R', 'B', 'A', 0}, 3U},
        {UINT32_C(0x02800000), 0U, 1U, {'R', 'I', 'B', 'A'}, 4U},
        {UINT32_C(0x00800000), UINT32_C(0x02000000), 2U,
         {'R', 'I', 'B', 'A'}, 4U},
    };
    uint32_t status[13] = {0U};
    const FactoryMcanIrqReferences references = {.status = status};
    for (unsigned index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        interrupt_state = cases[index].initial;
        state_after_reinitialization = cases[index].after_reinitialization;
        status[12] = 7U;
        call_count = 0U;
        assert(platform_ack_mcan_irq(&references) == 0U);
        assert(call_count == cases[index].expected_calls);
        assert(status[12] == cases[index].expected_status);
        for (unsigned event = 0U; event < call_count; ++event) {
            assert(trace[event] == cases[index].expected_trace[event]);
        }
    }
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="factory-mcan-error-") as temporary:
        directory = Path(temporary)
        test_source = directory / "test.c"
        executable = directory / "test"
        test_source.write_text(harness + function + cases, encoding="utf-8")
        subprocess.run(
            [
                "cc",
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                str(test_source),
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run([str(executable)], check=True)
    print("MCAN error orchestration: 4 state/order cases passed.")


if __name__ == "__main__":
    main()
