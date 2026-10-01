#!/usr/bin/env python3
"""Compare SRAM C routines with factory state/MMIO traces outside make."""
import hashlib
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ
from unicorn import arm_const as arm


ROOT = Path(__file__).resolve().parents[2]
EFM = 0x40010400
SAMPLE = 0x1ffff104
DRIVE = 0x1fffa510


def compiled_sections():
    elf = ROOT / "build/dm4310.elf"
    listing = subprocess.check_output(["arm-none-eabi-objdump", "-h", str(elf)], text=True)
    names = (".text", ".rodata", ".dm4310_flash_writer",
             ".dm4310_flash_erase_only", ".dm4310_helper_clear_runtime_loop_states")
    result = []
    with tempfile.TemporaryDirectory(prefix="dm4310-state-arm-") as temporary:
        for name in names:
            path = Path(temporary) / name
            subprocess.run(["arm-none-eabi-objcopy", "--dump-section",
                            f"{name}={path}", str(elf)], check=True)
            address = next(int(line.split()[3], 16) for line in listing.splitlines()
                           if len(line.split()) > 3 and line.split()[1] == name)
            result.append((address, path.read_bytes()))
    return result


def execute(sections, entry, sector, words, delay, primask, seed):
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(0, 0x40000)
    machine.mem_map(0x1fff0000, 0x10000)
    machine.mem_map(0x20000000, 0x10000)
    machine.mem_map(0x40010000, 0x1000)
    for address, image in sections:
        machine.mem_write(address, image)
    rng = random.Random(seed)
    machine.mem_write(SAMPLE, rng.randbytes(0xa4))
    machine.mem_write(DRIVE, rng.randbytes(0xa8))
    payload = b"".join(struct.pack("<I", 0xdead0000 + i) for i in range(words))
    machine.mem_write(0x20001000, payload)
    machine.mem_write(EFM + 0x18, struct.pack("<I", 0xaabb1234))
    machine.mem_write(EFM + 0x1c, struct.pack("<I", 0x99887766))
    machine.mem_write(0x40010590, struct.pack("<I", 0xaa55aa55))
    trace = []
    polls = 0
    operation_started = False
    cache_injected = False

    def hook(uc, access, address, size, value, unused):
        nonlocal polls, operation_started, cache_injected
        register = EFM <= address < EFM + 0x28 or address == 0x40010590
        state = SAMPLE <= address < SAMPLE + 0xa4 or DRIVE <= address < DRIVE + 0xa8
        data = 0x20001000 <= address < 0x20001000 + len(payload)
        flash = sector <= address < sector + max(4, len(payload))
        if access == UC_MEM_READ:
            if address == EFM + 0x20:
                polls += 1
                uc.mem_write(address, struct.pack("<I", 0 if polls <= delay else 0x110))
            if address == EFM + 0x18 and operation_started and not cache_injected:
                current = struct.unpack("<I", uc.mem_read(address, 4))[0]
                uc.mem_write(address, struct.pack("<I", current | 0x40000))
                cache_injected = True
            if register or data:
                actual = int.from_bytes(uc.mem_read(address, size), "little")
                trace.append(("read", address, size, actual))
        else:
            if register or state or flash:
                trace.append(("write", address, size, value))
            if address == EFM + 4 and value == 0xfedcba98:
                # Distinguish capture-before-unlock from the original order.
                current = struct.unpack("<I", uc.mem_read(EFM + 0x18, 4))[0]
                uc.mem_write(EFM + 0x18, struct.pack("<I", current ^ 0x30000))
            if flash:
                operation_started = True
                polls = 0

    machine.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, hook)
    machine.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    machine.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    machine.reg_write(arm.UC_ARM_REG_FPSCR, 0x3000000)
    machine.reg_write(arm.UC_ARM_REG_PRIMASK, primask)
    machine.reg_write(arm.UC_ARM_REG_R0, sector)
    machine.reg_write(arm.UC_ARM_REG_R1, 0x20001000)
    machine.reg_write(arm.UC_ARM_REG_R2, words)
    machine.reg_write(arm.UC_ARM_REG_SP, 0x200004f8)
    machine.reg_write(arm.UC_ARM_REG_LR, 0x3f101)
    machine.emu_start(entry | 1, 0x3f100, count=100000)
    assert machine.reg_read(arm.UC_ARM_REG_PC) == 0x3f100
    assert machine.reg_read(arm.UC_ARM_REG_SP) == 0x200004f8
    assert machine.reg_read(arm.UC_ARM_REG_PRIMASK) == primask
    state = bytes(machine.mem_read(SAMPLE, 0xa4)) + bytes(machine.mem_read(DRIVE, 0xa8))
    return trace, state, bytes(machine.mem_read(EFM, 0x28)), bytes(machine.mem_read(0x40010590, 4))


def main():
    factory = (ROOT / "recovered/binaries/dm4310/"
               "dm4310_v3_v5017_app_flash_00020000_memory.bin").read_bytes()
    assert hashlib.sha256(factory).hexdigest() == (
        "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4")
    original = [(0x20000, factory), (0x1fff8000, factory[0x8680:0xab90])]
    compiled = compiled_sections()
    count = 0
    for entry in (0x1fff9950, 0x1fff9a38):
        for sector in (0x10000, 0x1e000, 0x3e000):
            for words in ((0, 1, 2, 5, 37, 257) if entry == 0x1fff9950 else (0,)):
                for delay in (0, 3):
                    for primask in (0, 1):
                        arguments = entry, sector, words, delay, primask, count
                        expected = execute(original, *arguments)
                        actual = execute(compiled, *arguments)
                        assert actual == expected, (arguments, actual, expected)
                        count += 1
    print(f"Flash: {count} factory/compiled MMIO and data-access traces match")
    for seed in range(64):
        arguments = 0x1fff9d5c, 0x10000, 0, 0, seed & 1, seed
        expected = execute(original, *arguments)
        actual = execute(compiled, *arguments)
        assert actual == expected, (seed, actual, expected)
        assert len(actual[0]) == 23
    print("Loop clear: 64 factory/compiled 23-store traces and final states match")


if __name__ == "__main__":
    main()
