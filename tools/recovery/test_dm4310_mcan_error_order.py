#!/usr/bin/env python3
"""Isolated host test of the production platform MCAN error orchestration.

Board MMIO is mocked: this checks call/state order, not hardware equivalence.
Run explicitly; deliberately absent from the normal build graph.
"""
from pathlib import Path
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[2]
    source = (root / "app/src/platform.c").read_text()
    start = source.index("uint8_t platform_ack_mcan_irq(void)")
    end = source.index("\nvoid platform_ack_debug_uart_irq", start)
    function = source[start:end]
    harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#define DAMIAO_DM4310 1
static struct { unsigned can_error; } events;
#define APP_DEFERRED_EVENTS events
static uint16_t record[74];
static unsigned ir, after_init, calls;
static char trace[8];
static void note(char c) { trace[calls++] = c; }
static uint32_t *app_config_staging_record(void) {
    return (uint32_t *)(void *)record;
}
static bool board_mcan_reinitialization_requested(void) {
    note('R'); return (ir & 0x800000U) != 0;
}
static bool board_mcan_init(uint16_t node, uint8_t selector) {
    note('I'); assert(node == 0x123U); assert(selector == 255U);
    ir = after_init; return true;
}
static uint8_t board_mcan_recover_bus_off(void) {
    note('B');
    if (calls == 3) assert(events.can_error == 1U);
    return (ir & 0x2000000U) ? 2U : 0U;
}
static uint8_t board_mcan_ack_interrupt(void) {
    note('A');
    assert(events.can_error == ((ir & 0x2000000U) ? 2U :
                                (calls == 4 ? 1U : 7U)));
    return 0;
}
'''
    cases = r'''
int main(void) {
    record[0x20/2] = 0x123; record[0x8c/2] = 0x100;
    const unsigned inputs[] = {0, 0x2000000, 0x2800000, 0x800000};
    const unsigned outputs[] = {0, 0, 0, 0x2000000};
    for (unsigned i = 0; i < 4; ++i) {
        ir = inputs[i]; after_init = outputs[i]; calls = 0;
        events.can_error = 7;
        assert(platform_ack_mcan_irq() == 0);
        assert(trace[0] == 'R');
        if (inputs[i] & 0x800000) {
            assert(calls == 4 && trace[1] == 'I' && trace[2] == 'B');
        } else assert(calls == 3 && trace[1] == 'B');
        assert(trace[calls-1] == 'A');
    }
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="dm4310-mcan-error-") as tmp:
        path = Path(tmp)
        (path / "test.c").write_text(harness + function + cases)
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        str(path / "test.c"), "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True)
    print("MCAN error orchestration: 4 state/order cases passed (mock board).")


if __name__ == "__main__":
    main()
