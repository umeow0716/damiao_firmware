#!/usr/bin/env python3
"""Verify factory and raw CAN feedback data sources in source-built ELFs.

This recovery/development check is intentionally not part of the normal Make
workflow.  It executes the encoder from each ELF with deliberately different
filtered and unfiltered measurements, then checks the emitted eight-byte CAN
payload and the related 0x7FF LIVE selectors.
"""

from __future__ import annotations

import argparse
import struct
from io import BytesIO
from pathlib import Path

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB
import unicorn.arm_const as arm


ROOT = Path(__file__).resolve().parents[2]
FLASH_BASE = 0x00020000
FLASH_SIZE = 0x00020000
SRAM_BASE = 0x1FFF0000
SRAM_SIZE = 0x00010000
HIGH_SRAM_BASE = 0x20000000
HIGH_SRAM_SIZE = 0x00010000
STACK_TOP = 0x2000F000
RETURN_ADDRESS = 0x00030001
FRAME_ADDRESS = 0x20001000
CONTEXT_ADDRESS = 0x20002000
SCRATCH_ADDRESS = 0x20003000
DISPATCH_ADDRESS = 0x20004000
REQUEST_ADDRESS = 0x20005000
RESULT_ADDRESS = 0x20006000


def f32(machine: Uc, address: int, value: float) -> None:
    machine.mem_write(address, struct.pack("<f", value))


def symbol_addresses(elf: ELFFile) -> dict[str, int]:
    symbols = elf.get_section_by_name(".symtab")
    if symbols is None:
        raise ValueError("ELF has no symbol table")
    return {
        symbol.name: symbol["st_value"] & ~1
        for symbol in symbols.iter_symbols()
        if symbol.name
    }


def make_machine(elf: ELFFile) -> Uc:
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(FLASH_BASE, FLASH_SIZE)
    machine.mem_map(SRAM_BASE, SRAM_SIZE)
    machine.mem_map(HIGH_SRAM_BASE, HIGH_SRAM_SIZE)

    for segment in elf.iter_segments():
        if segment["p_type"] != "PT_LOAD" or segment["p_filesz"] == 0:
            continue
        address = segment["p_paddr"]
        if FLASH_BASE <= address < FLASH_BASE + FLASH_SIZE:
            machine.mem_write(address, segment.data())

    # Startup normally copies these initialized helper/code sections from
    # Flash to fixed SRAM before the CAN encoder can run.
    for section in elf.iter_sections():
        address = section["sh_addr"]
        if (SRAM_BASE <= address < SRAM_BASE + SRAM_SIZE and
                section["sh_type"] != "SHT_NOBITS" and section.data_size != 0):
            machine.mem_write(address, section.data())

    machine.reg_write(arm.UC_ARM_REG_SP, STACK_TOP)
    machine.reg_write(arm.UC_ARM_REG_LR, RETURN_ADDRESS)
    machine.reg_write(arm.UC_ARM_REG_C1_C0_2, 0x00F00000)
    machine.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    machine.reg_write(arm.UC_ARM_REG_FPSCR, 0)
    return machine


def protocol_uint(value: float, maximum: float, bits: int) -> int:
    return int(((value + maximum) * float((1 << bits) - 1)) /
               (2.0 * maximum))


def expected_payload(velocity: float, torque: float) -> bytes:
    position = protocol_uint(1.25, 12.5, 16)
    packed_velocity = protocol_uint(velocity, 30.0, 12)
    packed_torque = protocol_uint(torque, 10.0, 12)
    return bytes((
        0x25,
        position >> 8,
        position & 0xFF,
        packed_velocity >> 4,
        ((packed_velocity & 0x0F) << 4) | (packed_torque >> 8),
        packed_torque & 0xFF,
        42,
        35,
    ))


def configure_measurements(machine: Uc,
                           symbols: dict[str, int]) -> tuple[int, int, int]:
    config = symbols["config_staging_record"]
    motor = symbols["motor_runtime_state"]
    sample = symbols["sample_runtime_state"]

    machine.mem_write(config + 0x1C, struct.pack("<H", 0x456))
    machine.mem_write(config + 0x20, struct.pack("<I", 0x05))
    f32(machine, config + 0x54, 12.5)
    f32(machine, config + 0x58, 30.0)
    f32(machine, config + 0x5C, 10.0)

    f32(machine, motor + 0x18, 1.25)
    f32(machine, motor + 0x1C, 3.0)
    f32(machine, motor + 0x20, 70.0)
    f32(machine, motor + 0x30, 2.0)
    machine.mem_write(motor + 0x34, struct.pack("<I", 0x3F800000))
    f32(machine, motor + 0x40, 35.0)
    f32(machine, motor + 0x44, 2.0)
    f32(machine, motor + 0x5C, 0.1)

    f32(machine, sample + 0x64, -4.0)
    machine.mem_write(sample + 0x80, struct.pack("<I", 0x02))
    f32(machine, sample + 0x84, 42.0)
    return config, motor, sample


def write_context(machine: Uc, config: int, motor: int, sample: int,
                  dispatch: int, scratch: int) -> None:
    # McanIrqContext uses source-owned pointers, allowing the public IRQ APIs
    # to run without relying on a particular fixed-SRAM layout.
    context_words = (
        0,
        config,
        0,
        0,
        0,
        dispatch,
        scratch,
        scratch,
        sample,
        motor,
        0,
        0,
        0x05,
    )
    machine.mem_write(CONTEXT_ADDRESS, struct.pack("<13I", *context_words))


def load_variant(path: Path) -> tuple[Uc, dict[str, int]]:
    elf = ELFFile(BytesIO(path.read_bytes()))
    symbols = symbol_addresses(elf)
    required = (
        "can_protocol_encode_parameter_feedback_irq",
        "parameter_protocol_process_irq",
        "config_staging_record",
        "motor_runtime_state",
        "sample_runtime_state",
        "mcan_dispatch",
        "can_protocol_workspace",
    )
    missing = [name for name in required if name not in symbols]
    if missing:
        raise ValueError(f"{path}: missing symbols: {', '.join(missing)}")

    return make_machine(elf), symbols


def run_encoder(path: Path) -> tuple[bytes, int, int]:
    machine, symbols = load_variant(path)
    config, motor, sample = configure_measurements(machine, symbols)
    write_context(machine, config, motor, sample, DISPATCH_ADDRESS,
                  SCRATCH_ADDRESS)

    machine.reg_write(arm.UC_ARM_REG_R0, CONTEXT_ADDRESS)
    machine.reg_write(arm.UC_ARM_REG_R1, FRAME_ADDRESS)
    machine.emu_start(symbols["can_protocol_encode_parameter_feedback_irq"] | 1,
                      RETURN_ADDRESS & ~1, count=10000)

    if machine.reg_read(arm.UC_ARM_REG_PC) != (RETURN_ADDRESS & ~1):
        raise RuntimeError(f"{path}: feedback encoder did not return")
    payload = bytes(machine.mem_read(DISPATCH_ADDRESS + 8, 8))
    frame_id, = struct.unpack("<I", machine.mem_read(FRAME_ADDRESS, 4))
    frame_length = machine.mem_read(FRAME_ADDRESS + 4, 1)[0]
    return payload, frame_id, frame_length


def run_live_selector(path: Path, selector: int) -> bytes:
    machine, symbols = load_variant(path)
    config, motor, sample = configure_measurements(machine, symbols)
    dispatch = symbols["mcan_dispatch"]
    scratch = symbols["can_protocol_workspace"]
    write_context(machine, config, motor, sample, dispatch, scratch)

    parameter_header = 0x05 | (0xCC << 16) | (selector << 24)
    machine.mem_write(dispatch + 0x64, struct.pack("<I", parameter_header))
    machine.mem_write(REQUEST_ADDRESS, struct.pack("<IB", 0x7FF, 8))
    machine.reg_write(arm.UC_ARM_REG_R0, REQUEST_ADDRESS)
    machine.reg_write(arm.UC_ARM_REG_R1, RESULT_ADDRESS)
    machine.reg_write(arm.UC_ARM_REG_R2, CONTEXT_ADDRESS)
    machine.emu_start(symbols["parameter_protocol_process_irq"] | 1,
                      RETURN_ADDRESS & ~1, count=30000)

    if machine.reg_read(arm.UC_ARM_REG_PC) != (RETURN_ADDRESS & ~1):
        raise RuntimeError(f"{path}: LIVE selector {selector} did not return")
    if bytes(machine.mem_read(RESULT_ADDRESS, 3)) != b"\x01\x01\x01":
        raise RuntimeError(f"{path}: LIVE selector {selector} was not published")
    return bytes(machine.mem_read(dispatch + 8, 8))


def expected_live_measurement(value: float) -> bytes:
    return bytes((0x05, 0x02, 42, 35)) + struct.pack("<f", value)


def expected_live_combined(velocity: float) -> bytes:
    packed_velocity = protocol_uint(velocity, 30.0, 12)
    packed_current = (-40000) & 0xFFFF
    return (struct.pack("<f", 1.25) + struct.pack("<H", packed_velocity) +
            struct.pack("<H", packed_current))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", default="dm4310")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    args = parser.parse_args()

    factory_payload, factory_id, factory_length = run_encoder(
        args.build_dir / f"{args.model}.elf"
    )
    raw_payload, raw_id, raw_length = run_encoder(
        args.build_dir / f"{args.model}_raw.elf"
    )

    expected_factory = expected_payload(3.0, 2.0)
    expected_raw = expected_payload(7.0, -8.0)
    if factory_payload != expected_factory:
        raise SystemExit(
            f"factory payload mismatch: {factory_payload.hex()} != "
            f"{expected_factory.hex()}"
        )
    if raw_payload != expected_raw:
        raise SystemExit(
            f"raw payload mismatch: {raw_payload.hex()} != {expected_raw.hex()}"
        )
    if (factory_id, factory_length) != (0x456, 8):
        raise SystemExit("factory frame ID/length changed")
    if (raw_id, raw_length) != (0x456, 8):
        raise SystemExit("raw frame ID/length changed")

    live_expectations = {
        1: (expected_live_measurement(1.25), expected_live_measurement(1.25)),
        2: (expected_live_measurement(3.0), expected_live_measurement(7.0)),
        3: (expected_live_measurement(2.0), expected_live_measurement(-8.0)),
        4: (expected_live_combined(3.0), expected_live_combined(7.0)),
    }
    for selector, (factory_expected, raw_expected) in live_expectations.items():
        factory_live = run_live_selector(
            args.build_dir / f"{args.model}.elf", selector
        )
        raw_live = run_live_selector(
            args.build_dir / f"{args.model}_raw.elf", selector
        )
        if factory_live != factory_expected:
            raise SystemExit(
                f"factory LIVE selector {selector} mismatch: "
                f"{factory_live.hex()} != {factory_expected.hex()}"
            )
        if raw_live != raw_expected:
            raise SystemExit(
                f"raw LIVE selector {selector} mismatch: "
                f"{raw_live.hex()} != {raw_expected.hex()}"
            )

    print(
        f"PASS: {args.model} factory={factory_payload.hex()} uses filtered "
        f"velocity/torque; raw={raw_payload.hex()} uses unfiltered values; "
        "LIVE selectors 1-4 agree"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
