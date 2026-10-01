#!/usr/bin/env python3
"""Compose real Reset startup with main's first deferred-loop entry.

Reset, SystemInit, scatter loading and the C runtime execute in one Unicorn
machine.  Main then executes with only the already-closed board/peripheral
workers replaced at their call boundaries.  The test therefore checks the
parent call graph and ABI without duplicating the workers' exhaustive MMIO
and calibration regressions.
"""

import os
import struct

from unicorn import UC_HOOK_CODE
import unicorn.arm_const as arm

from dm4310_unicorn import A, load_images, make_machine
from regressions.dm4310_model_layout import F, FACTORY_STACK_TOP


FACTORY, SYMBOLS, SEGMENTS = load_images()
MODEL = os.environ.get("DAMIAO_RECOVERY_MODEL", "dm4310").lower()
SOFTWARE_VERSION = {
    "dm4310": 0x37313035,
    "dm4340": 0x37313135,
    "dm8009": 0x37313436,
}[MODEL]
FACTORY_RUNTIME_ENTRY = F(0x20368)
FACTORY_MAIN = F(0x252F4)
FACTORY_LOOP = F(0x2540E)
SOURCE_RUNTIME_ENTRY = SYMBOLS["dm4310_runtime_main_entry"]
SOURCE_MAIN = SYMBOLS["main"]
SOURCE_LOOP = SOURCE_MAIN + 0x60

BOOT_RECORD = 0x0001E000
HARDWARE_VARIANT = A(0x1FFFA5BC)
STARTUP_BUS_VOLTAGE = A(0x1FFFF16C)
POSITION_STATE = A(0x1FFFF190)
CONSOLE_STATE = A(0x1FFFF0C0)
MCAN_DISPATCH = A(0x1FFFF29C)
MCAN_INITIALIZE_STUB = 0x0002C000
CONFIGURATION = A(0x1FFFA5C8)
APP_STATE = SYMBOLS["g_app"]

FACTORY_CALLS = {
    F(0x23208): "platform_early_init",
    F(0x26DE8): "platform_confirm_application_boot",
    F(0x26E90): "platform_update_application_identity",
    F(0x21F28): "board_delay_500",
    F(0x221F4): "board_identity",
    F(0x2359C): "uart_init",
    F(0x21A00): "adc_init",
    F(0x256C8): "power_stage_self_test",
    F(0x21EB0): "crc_init",
    F(0x22C10): "position_init",
    F(0x22658): "calibration_load",
    F(0x21B00): "adc_offset_calibration",
    F(0x228C0): "configuration_load_and_derive",
    F(0x24EC0): "current_sensor_startup",
    F(0x26758): "debug_status",
    F(0x22FAC): "sampling_timer_init",
}

SOURCE_CALLS = {
    SYMBOLS["platform_early_init"]: "platform_early_init",
    SYMBOLS["platform_confirm_application_boot"]:
        "platform_confirm_application_boot",
    SYMBOLS["platform_update_application_identity"]:
        "platform_update_application_identity",
    SYMBOLS["app_state_init"]: "app_state_init",
    SYMBOLS["debug_console_reset"]: "debug_console_reset",
    SYMBOLS["platform_prepare_board_startup"]:
        "platform_prepare_board_startup",
    SYMBOLS["platform_initialize_peripherals"]:
        "platform_initialize_peripherals",
    SYMBOLS["platform_load_parameters"]: "platform_load_parameters",
    SYMBOLS["platform_prepare_runtime_configuration"]:
        "platform_prepare_runtime_configuration",
    SYMBOLS["platform_initialize_runtime"]: "platform_initialize_runtime",
    SYMBOLS["debug_console_print_status"]: "debug_console_print_status",
    SYMBOLS["platform_check_startup_bus_voltage"]:
        "platform_check_startup_bus_voltage",
    SYMBOLS["platform_start_control_loop"]:
        "platform_start_control_loop",
}

EXPECTED_SOURCE_CALLS = (
    "platform_early_init",
    "platform_confirm_application_boot",
    "platform_update_application_identity",
    "app_state_init",
    "debug_console_reset",
    "platform_prepare_board_startup",
    "platform_initialize_peripherals",
    "platform_load_parameters",
    "platform_prepare_runtime_configuration",
    "platform_initialize_runtime",
    "debug_console_print_status",
    "platform_check_startup_bus_voltage",
    "platform_start_control_loop",
)


def word(machine, address):
    return struct.unpack("<I", bytes(machine.mem_read(address, 4)))[0]


def write_word(machine, address, value):
    machine.mem_write(address, struct.pack("<I", value))


def return_from_stub(machine, value=1):
    machine.reg_write(arm.UC_ARM_REG_R0, value)
    machine.reg_write(arm.UC_ARM_REG_PC,
                      machine.reg_read(arm.UC_ARM_REG_LR))


def boot_to_main(original, clock_source, incoming_primask):
    machine = make_machine(original, FACTORY, SEGMENTS)
    if original:
        # Actual scatter decompression reads the copy-down payload after the
        # executable part of the factory image.
        machine.mem_write(0x20000, FACTORY)
    machine.mem_map(0x00010000, 0x00010000)
    machine.mem_map(0x40000000, 0x00100000)
    machine.mem_map(0xE0000000, 0x00100000)
    machine.mem_write(0x1FFF0000, bytes([0xA5]) * 0x10000)
    machine.mem_write(0x20000000, bytes([0xA5]) * 0x10000)
    machine.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xF00000)
    machine.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    machine.reg_write(arm.UC_ARM_REG_SP, 0x2000F000)
    machine.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    machine.reg_write(arm.UC_ARM_REG_PRIMASK, incoming_primask)
    machine.mem_write(0x40010684,
                      struct.pack("<I", 1 if clock_source & 2 else 0))
    machine.mem_write(0x40054026, bytes([clock_source]))
    machine.mem_write(0x40054100,
                      struct.pack("<I", 0x39306300 |
                                  (0x80 if clock_source & 4 else 0)))

    if original:
        machine.emu_start(0x20389, FACTORY_RUNTIME_ENTRY, count=2_000_000)
        assert machine.reg_read(arm.UC_ARM_REG_PC) == FACTORY_RUNTIME_ENTRY
        machine.emu_start(FACTORY_RUNTIME_ENTRY | 1, FACTORY_MAIN,
                          count=100_000)
        expected_main = FACTORY_MAIN
    else:
        machine.emu_start(SYMBOLS["Reset_Handler"] | 1,
                          SOURCE_RUNTIME_ENTRY, count=2_000_000)
        assert machine.reg_read(arm.UC_ARM_REG_PC) == SOURCE_RUNTIME_ENTRY
        machine.emu_start(SOURCE_RUNTIME_ENTRY | 1, SOURCE_MAIN,
                          count=100_000)
        expected_main = SOURCE_MAIN
    assert machine.reg_read(arm.UC_ARM_REG_PC) == expected_main
    assert machine.reg_read(arm.UC_ARM_REG_SP) == FACTORY_STACK_TOP
    assert machine.reg_read(arm.UC_ARM_REG_PRIMASK) == incoming_primask
    return machine


def run(original, clock_source, incoming_primask, confirm_boot,
        hardware_variant):
    machine = boot_to_main(original, clock_source, incoming_primask)
    record_word_1 = 0 if confirm_boot else 1
    machine.mem_write(BOOT_RECORD,
                      struct.pack("<IIIII", 0x10203040, record_word_1,
                                  0x50607080, 0x90A0B0C0, 0xD0E0F000))
    write_word(machine, STARTUP_BUS_VOLTAGE, 0)

    normalized = []
    source_calls = []
    factory_calls = []
    saw_factory_bus_check = [False]

    def factory_stub(emu, address):
        name = FACTORY_CALLS[address]
        factory_calls.append(name)
        if name == "platform_early_init":
            normalized.append("clock")
        elif name == "platform_confirm_application_boot":
            normalized.append("confirm_boot")
        elif name == "platform_update_application_identity":
            assert emu.reg_read(arm.UC_ARM_REG_R0) == SOFTWARE_VERSION
            normalized.append("application_identity")
        elif name == "board_delay_500":
            assert emu.reg_read(arm.UC_ARM_REG_R0) == 500
            normalized.append("board_delay_500")
        elif name == "board_identity":
            normalized.append("board_identity")
            return_from_stub(emu, hardware_variant)
            return
        elif name == "uart_init":
            assert emu.reg_read(arm.UC_ARM_REG_R0) == 921600
            normalized.append("uart")
        elif name == "adc_init":
            normalized.append("adc")
        elif name == "power_stage_self_test":
            normalized.append("power_stage_self_test")
        elif name == "crc_init":
            normalized.append("crc")
        elif name == "position_init":
            normalized.append("position")
        elif name == "calibration_load":
            normalized.append("calibration")
        elif name == "adc_offset_calibration":
            assert emu.reg_read(arm.UC_ARM_REG_R0) == A(0x1FFFF144)
            normalized.append("adc_offsets")
        elif name == "configuration_load_and_derive":
            normalized.extend(("configuration_load", "configuration_derive"))
            # The closed configuration worker selects and publishes the
            # startup MCAN callback consumed indirectly by main@0x2539e.
            write_word(emu, MCAN_DISPATCH, MCAN_INITIALIZE_STUB | 1)
        elif name == "current_sensor_startup":
            normalized.append("current_sensors")
        elif name == "debug_status":
            normalized.append("debug_status")
        elif name == "sampling_timer_init":
            normalized.append("sampling_timer")
        return_from_stub(emu)

    def source_stub(emu, address):
        name = SOURCE_CALLS[address]
        source_calls.append(name)
        if name == "platform_early_init":
            normalized.append("clock")
        elif name == "platform_confirm_application_boot":
            if word(emu, BOOT_RECORD + 4) != 1:
                normalized.append("confirm_boot")
        elif name == "platform_update_application_identity":
            normalized.append("application_identity")
        elif name == "platform_prepare_board_startup":
            normalized.extend(("board_delay_500", "board_identity"))
            emu.mem_write(HARDWARE_VARIANT, bytes([hardware_variant]))
        elif name == "platform_initialize_peripherals":
            normalized.extend(("uart", "adc", "power_stage_self_test",
                               "crc", "position", "calibration",
                               "adc_offsets"))
        elif name == "platform_load_parameters":
            assert emu.reg_read(arm.UC_ARM_REG_R0) == APP_STATE
            normalized.append("configuration_load")
        elif name == "platform_prepare_runtime_configuration":
            normalized.append("configuration_derive")
        elif name == "platform_initialize_runtime":
            normalized.extend(("current_sensors", "mcan"))
            write_word(emu, POSITION_STATE + 4, 0)
            write_word(emu, POSITION_STATE + 0x10, 0)
            write_word(emu, CONSOLE_STATE, 0)
            write_word(emu, CONSOLE_STATE + 4, 1)
        elif name == "debug_console_print_status":
            normalized.append("debug_status")
        elif name == "platform_check_startup_bus_voltage":
            normalized.append("bus_check")
        elif name == "platform_start_control_loop":
            normalized.extend(("sampling_timer", "adc_irq_enable"))
        return_from_stub(emu)

    def code(emu, address, size, unused):
        if address == (FACTORY_LOOP if original else SOURCE_LOOP):
            if original:
                normalized.append("adc_irq_enable")
            emu.emu_stop()
            return
        if original:
            if address in FACTORY_CALLS:
                factory_stub(emu, address)
                return
            if address == F(0x253A4):
                assert not saw_factory_bus_check[0]
                saw_factory_bus_check[0] = True
                normalized.append("bus_check")
                return
            if address == MCAN_INITIALIZE_STUB:
                assert emu.reg_read(arm.UC_ARM_REG_R0) == (
                    int.from_bytes(emu.mem_read(CONFIGURATION + 0x8C, 2),
                                   "little"))
                assert emu.reg_read(arm.UC_ARM_REG_R1) == (
                    int.from_bytes(emu.mem_read(CONFIGURATION + 0x20, 2),
                                   "little"))
                factory_calls.append("mcan_initialize")
                normalized.append("mcan")
                return_from_stub(emu)
                return
        elif address in SOURCE_CALLS:
            source_stub(emu, address)

    machine.hook_add(UC_HOOK_CODE, code)
    main_entry = FACTORY_MAIN if original else SOURCE_MAIN
    machine.emu_start(main_entry | 1, 0x30000, count=100_000)
    assert machine.reg_read(arm.UC_ARM_REG_PC) == (
        FACTORY_LOOP if original else SOURCE_LOOP)
    # The shared Unicorn ARM/Thumb machine does not model CPSIE's M-profile
    # PRIMASK mutation.  Preserve and compare that model explicitly; the
    # factory/source instruction paths both execute past their CPSIE.
    assert machine.reg_read(arm.UC_ARM_REG_PRIMASK) == incoming_primask
    assert word(machine, 0xE000ED08) == 0x20000
    assert bytes(machine.mem_read(HARDWARE_VARIANT, 1)) == bytes(
        [hardware_variant])
    assert word(machine, POSITION_STATE + 4) == 0
    assert word(machine, POSITION_STATE + 0x10) == 0
    assert word(machine, CONSOLE_STATE) == 0
    assert word(machine, CONSOLE_STATE + 4) == 1
    if original:
        assert saw_factory_bus_check[0]
        assert machine.reg_read(arm.UC_ARM_REG_SP) == FACTORY_STACK_TOP - 8
    else:
        assert tuple(source_calls) == EXPECTED_SOURCE_CALLS, source_calls
        assert machine.reg_read(arm.UC_ARM_REG_SP) == FACTORY_STACK_TOP - 16
        assert bytes(machine.mem_read(APP_STATE + 0x1C8, 24)) == bytes(24)
    return (tuple(normalized), tuple(factory_calls), tuple(source_calls),
            machine.reg_read(arm.UC_ARM_REG_FPSCR),
            machine.reg_read(arm.UC_ARM_REG_PRIMASK))


def main():
    expected_base = (
        "clock", "application_identity", "board_delay_500",
        "board_identity", "uart", "adc", "power_stage_self_test", "crc",
        "position", "calibration", "adc_offsets", "configuration_load",
        "configuration_derive", "current_sensors", "mcan", "debug_status",
        "bus_check", "sampling_timer", "adc_irq_enable",
    )
    for case in range(8):
        incoming_primask = case & 1
        confirm_boot = bool(case & 2)
        hardware_variant = (case >> 1) & 3
        factory_result = run(True, case, incoming_primask, confirm_boot,
                             hardware_variant)
        source_result = run(False, case, incoming_primask, confirm_boot,
                            hardware_variant)
        expected = list(expected_base)
        if confirm_boot:
            expected.insert(1, "confirm_boot")
        assert factory_result[0] == tuple(expected), (case, factory_result[0])
        assert source_result[0] == tuple(expected), (case, source_result[0])
        assert factory_result[0] == source_result[0]
        assert factory_result[3] == source_result[3]
        assert factory_result[4] == source_result[4] == incoming_primask
    print("PASS: 8 continuous Reset/SystemInit/scatter/runtime/main cases; "
          "startup call order/ABI, conditional boot confirmation, fixed "
          "state, VTOR, emulated PRIMASK parity, FPSCR and first "
          "deferred-loop entry")


if __name__ == "__main__":
    main()
