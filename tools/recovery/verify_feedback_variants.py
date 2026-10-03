#!/usr/bin/env python3
"""Verify the CAN feedback/response variant matrix in source-built ELFs.

This recovery/development check is intentionally not part of the normal Make
workflow.  It executes the encoder from each ELF with deliberately different
filtered and unfiltered measurements, then checks the emitted eight-byte CAN
payload, related 0x7FF LIVE selectors, command-response policy and banner.
"""

from __future__ import annotations

import argparse
import struct
import subprocess
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
DEFAULT_OBJDUMP = ROOT / "tools/arm-gnu-toolchain/bin/arm-none-eabi-objdump"


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


def verify_command_response_policy(path: Path, sends_response: bool,
                                   objdump: Path) -> None:
    completed = subprocess.run(
        [str(objdump), "-d", "--disassemble=mcan1_receive_irq", str(path)],
        check=True,
        capture_output=True,
        text=True,
    )
    disassembly = completed.stdout
    has_feedback_encoder = "<can_protocol_encode_feedback_irq>" in disassembly
    if has_feedback_encoder != sends_response:
        raise SystemExit(
            f"{path}: command feedback call policy mismatch; "
            f"expected sends_response={sends_response}"
        )
    for required in ("<can_protocol_decode_command_irq>",
                     "<app_apply_can_command_irq>"):
        if required not in disassembly:
            raise SystemExit(f"{path}: missing retained command path {required}")
    # Parameter replies must remain available in every variant.
    for required in ("<parameter_protocol_process_irq>",
                     "<platform_mcan_send_prebuilt_irq>"):
        if required not in disassembly:
            raise SystemExit(f"{path}: missing retained parameter path {required}")


def verify_banner(path: Path, expected: str, all_banners: tuple[str, ...]) -> None:
    image = path.read_bytes()
    expected_bytes = expected.encode("ascii") + b"\0"
    if expected_bytes not in image:
        raise SystemExit(f"{path}: missing expected banner {expected!r}")
    for banner in all_banners:
        if banner != expected and banner.encode("ascii") + b"\0" in image:
            raise SystemExit(f"{path}: unexpected banner {banner!r}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", default="dm4310")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--objdump", type=Path, default=DEFAULT_OBJDUMP)
    args = parser.parse_args()

    variants = (
        ("factory", "", False, True, "DMBOT Motor Driver"),
        ("raw", "_raw", True, True,
         "DMBOT Motor Driver(with raw result)"),
        ("no_response", "_no_response", False, False,
         "DMBOT Motor Driver(no response)"),
        ("raw_no_response", "_raw_no_response", True, False,
         "DMBOT Motor Driver(with raw result and no response)"),
    )
    banners = tuple(variant[4] for variant in variants)
    for name, suffix, uses_raw, sends_response, banner in variants:
        path = args.build_dir / f"{args.model}{suffix}.elf"
        payload, frame_id, frame_length = run_encoder(path)
        expected = (expected_payload(7.0, -8.0) if uses_raw else
                    expected_payload(3.0, 2.0))
        if payload != expected:
            raise SystemExit(
                f"{name} payload mismatch: {payload.hex()} != {expected.hex()}"
            )
        if (frame_id, frame_length) != (0x456, 8):
            raise SystemExit(f"{name} frame ID/length changed")

        live_expectations = {
            1: expected_live_measurement(1.25),
            2: expected_live_measurement(7.0 if uses_raw else 3.0),
            3: expected_live_measurement(-8.0 if uses_raw else 2.0),
            4: expected_live_combined(7.0 if uses_raw else 3.0),
        }
        for selector, live_expected in live_expectations.items():
            live = run_live_selector(path, selector)
            if live != live_expected:
                raise SystemExit(
                    f"{name} LIVE selector {selector} mismatch: "
                    f"{live.hex()} != {live_expected.hex()}"
                )
        verify_command_response_policy(path, sends_response, args.objdump)
        verify_banner(path, banner, banners)

    print(f"PASS: {args.model} four-variant feedback/response matrix and banners")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
