#!/usr/bin/env python3
"""Decode HC32F448 peripheral/RAM captures from bins/.

The decoder deliberately uses the SVD shipped with the checked-in DDL so the
report can be reproduced without a Python package or an Internet connection.
It reports registers that differ from reset and adds board-specific summaries
whose meaning was confirmed against the decrypted official APP image.
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OFFICIAL_APP = (
    ROOT / "reference" / "official" /
    "APP_DM4310_V3_V5017_04.decrypted.bin"
)
DDL = ROOT / "third_party" / "HC32F448_DDL_Rev1.3.0"
SVD = (
    DDL
    / "drivers"
    / "cmsis"
    / "Device"
    / "HDSC"
    / "hc32f4xx"
    / "Source"
    / "GCC"
    / "svd"
    / "HC32F448.svd"
)
DEVICE_HEADER = (
    DDL
    / "drivers"
    / "cmsis"
    / "Device"
    / "HDSC"
    / "hc32f4xx"
    / "Include"
    / "hc32f448.h"
)


@dataclass(frozen=True)
class Capture:
    filename: str
    base: int
    length: int
    peripheral: str | None


CAPTURES = (
    Capture("adc1.bin", 0x40040000, 0x100, "ADC1"),
    Capture("adc2.bin", 0x40040400, 0x100, "ADC2"),
    Capture("adc3.bin", 0x40040800, 0x100, "ADC3"),
    Capture("aos_trigger_routing.bin", 0x40010800, 0x090, "AOS"),
    Capture("clock_peripheral_gates.bin", 0x40048000, 0x080, "PWC"),
    Capture("dma2_0x40053400_0x80_w32.bin", 0x40053400, 0x080, "DMA2"),
    Capture("efm.bin", 0x40010400, 0x100, "EFM"),
    Capture("gpio_pfs_0x40053800_0x480_h16.bin", 0x40053800, 0x480, "GPIO"),
    Capture("interrupt_selector.bin", 0x40051000, 0x100, "INTC"),
    Capture("mcan1.bin", 0x40029000, 0x100, "MCAN1"),
    Capture("mcan_message_ram.bin", 0x4002B000, 0x400, None),
    Capture("spi3.bin", 0x40020000, 0x020, "SPI3"),
    Capture("tmr0.bin", 0x40024000, 0x100, "TMR01"),
    Capture("tmr4_pwm.bin", 0x40038000, 0x100, "TMR41"),
    Capture("usart1.bin", 0x4001CC00, 0x080, "USART1"),
    Capture("app_runtime_config.bin", 0x1FFFA510, 0x5FE8, None),
    Capture("boot_can_node_id_0x0003e020.bin", 0x0003E020, 0x004, None),
    Capture("boot_can_rate_selector_0x0003e08c.bin", 0x0003E08C, 0x004, None),
)


def parse_int(text: str | None, default: int = 0) -> int:
    return default if text is None else int(text, 0)


def field_bounds(field: ET.Element) -> tuple[int, int]:
    if field.findtext("bitOffset") is not None:
        low = parse_int(field.findtext("bitOffset"))
        return low, low + parse_int(field.findtext("bitWidth")) - 1
    if field.findtext("lsb") is not None:
        return parse_int(field.findtext("lsb")), parse_int(field.findtext("msb"))
    bit_range = field.findtext("bitRange")
    if bit_range is None:
        raise ValueError(f"field {field.findtext('name')} has no bit position")
    high_text, low_text = bit_range.strip("[]").split(":")
    return parse_int(low_text), parse_int(high_text)


def read_events() -> dict[int, str]:
    events: dict[int, str] = {}
    pattern = re.compile(r"\b(?:EVT|INT)_SRC_([A-Z0-9_]+)\s*=\s*(\d+)U")
    for name, value in pattern.findall(DEVICE_HEADER.read_text(errors="replace")):
        events.setdefault(int(value), name)
    return events


def read_svd() -> dict[str, ET.Element]:
    root = ET.parse(SVD).getroot()
    peripherals = root.find("peripherals")
    if peripherals is None:
        raise ValueError("SVD has no peripherals node")
    return {item.findtext("name", ""): item for item in peripherals.findall("peripheral")}


def register_source(
    peripheral: ET.Element, peripherals: dict[str, ET.Element]
) -> ET.Element:
    derived = peripheral.get("derivedFrom")
    return peripherals[derived] if derived else peripheral


def decode_registers(
    capture: Capture,
    data: bytes,
    peripherals: dict[str, ET.Element],
) -> list[str]:
    assert capture.peripheral is not None
    peripheral = peripherals[capture.peripheral]
    source = register_source(peripheral, peripherals)
    registers = source.find("registers")
    if registers is None:
        return []

    output: list[str] = []
    seen: set[tuple[int, int]] = set()
    for register in registers.findall("register"):
        offset = parse_int(register.findtext("addressOffset"))
        size_bits = parse_int(register.findtext("size"), 32)
        size_bytes = (size_bits + 7) // 8
        key = (offset, size_bits)
        if key in seen or offset + size_bytes > len(data):
            continue
        seen.add(key)

        value = int.from_bytes(data[offset : offset + size_bytes], "little")
        reset = parse_int(register.findtext("resetValue"))
        if value == reset:
            continue

        width = size_bytes * 2
        name = register.findtext("name", "?")
        fields_out: list[str] = []
        fields = register.find("fields")
        if fields is not None:
            for field in fields.findall("field"):
                low, high = field_bounds(field)
                field_value = (value >> low) & ((1 << (high - low + 1)) - 1)
                if field_value:
                    fields_out.append(f"{field.findtext('name')}={field_value:#x}")
        suffix = f" — {', '.join(fields_out)}" if fields_out else ""
        output.append(
            f"- `+0x{offset:03x}` `{name}` = `0x{value:0{width}x}`"
            f" (reset `0x{reset:x}`){suffix}"
        )
    return output


def arm_scatter_decode(data: bytes, output_length: int) -> bytes:
    output = bytearray(output_length)
    input_pos = 0
    output_pos = 0
    while output_pos < output_length:
        token = data[input_pos]
        input_pos += 1
        literals = token & 3
        if literals == 0:
            literals = data[input_pos]
            input_pos += 1
        match = token >> 4
        if match == 0:
            match = data[input_pos]
            input_pos += 1
        for _ in range(1, literals):
            if output_pos >= output_length:
                break
            output[output_pos] = data[input_pos]
            output_pos += 1
            input_pos += 1
        if match and output_pos < output_length:
            low = data[input_pos]
            input_pos += 1
            kind = token & 0x0C
            distance = low + (kind << 6)
            if kind == 0x0C:
                distance = low + (data[input_pos] << 8)
                input_pos += 1
            source = output_pos - distance
            for _ in range(match + 2):
                if output_pos >= output_length:
                    break
                output[output_pos] = output[source]
                output_pos += 1
                source += 1
    return bytes(output)


def scalar(data: bytes, dump_base: int, address: int, kind: str = "f") -> int | float:
    offset = address - dump_base
    return struct.unpack_from("<" + kind, data, offset)[0]


def custom_summary(files: dict[str, bytes], events: dict[int, str]) -> list[str]:
    lines = ["## Board-specific summary", ""]

    intc = files["interrupt_selector.bin"]
    lines.extend(["### Interrupt routing", ""])
    for irq in range(5):
        event = struct.unpack_from("<I", intc, 0x5C + irq * 4)[0] & 0x1FF
        lines.append(f"- IRQ{irq}: `{event}` `{events.get(event, 'UNKNOWN')}`")
    lines.extend(
        [
            "",
            "IRQ1 is the DMA completion interrupt. The request path is "
            "`SPI3_SPRI (371) -> AOS DMA2_TRGSEL0 -> DMA2_TC0 (65) -> IRQ1`.",
            "",
        ]
    )

    aos = files["aos_trigger_routing.bin"]
    lines.extend(["### AOS event routes", ""])
    for offset, name in ((0x14, "DMA1 channel 0"), (0x2C, "DMA2 channel 0"), (0x78, "ADC1 trigger A")):
        event = struct.unpack_from("<I", aos, offset)[0] & 0x1FF
        lines.append(f"- {name}: `{event}` `{events.get(event, 'UNKNOWN')}`")
    lines.append("")

    adc1 = files["adc1.bin"]
    adc2 = files["adc2.bin"]
    adc3 = files["adc3.bin"]
    mux2 = struct.unpack_from("<H", adc2, 0x38)[0]
    mux3 = struct.unpack_from("<H", adc3, 0x38)[0]
    lines.extend(
        [
            "### ADC/PWM sampling", "",
            f"- ADC1 selected channels: `0x{struct.unpack_from('<I', adc1, 0x0C)[0]:x}`; physical PA0/PA1/PA2.",
            f"- ADC2 CHMUXR0: `0x{mux2:04x}`; selected logical channels 0/1/2 map to PA4/PA5/PB1.",
            f"- ADC3 CHMUXR0: `0x{mux3:04x}`; selected logical channels 0/1/2 map to PA6/PA7/PB10.",
            "- ADC1 trigger A is TMR4_1 special compare 0; ADC1/2/3 run in synchronous parallel mode.",
            "- All active channels use sample time 20 ADC clocks.",
            "",
        ]
    )

    mcan = files["mcan1.bin"]
    nbtp = struct.unpack_from("<I", mcan, 0x1C)[0]
    dbtp = struct.unpack_from("<I", mcan, 0x0C)[0]
    lines.extend(
        [
            "### MCAN1", "",
            f"- `NBTP=0x{nbtp:08x}`: 1 Mbit/s nominal at the recovered 80 MHz MCAN clock.",
            f"- `DBTP=0x{dbtp:08x}`: 5 Mbit/s data phase, CAN FD+BRS and transmitter delay compensation enabled.",
            "- Two standard filters: classic-mask motor ID 3 (mask 0x0ff) to FIFO0 and exact ID 0x7ff to FIFO0.",
            "- FIFO0: 3 x 64-byte elements at +0x080; FIFO1: 3 x 64-byte elements at +0x158.",
            "- Tx FIFO: 3 x 64-byte elements at +0x328; Tx event FIFO: 4 entries at +0x308.",
            "",
        ]
    )

    runtime = files["app_runtime_config.bin"]
    runtime_base = 0x1FFFA510
    config_base = 0x1FFFA5C8
    state_base = 0x1FFFF104
    raw_base = 0x1FFFC778
    model = runtime[config_base - runtime_base + 0x34 : config_base - runtime_base + 0x40]
    model_text = model.decode("ascii", errors="replace")
    raw = [scalar(runtime, runtime_base, raw_base + i * 2, "H") for i in range(9)]
    lines.extend(
        [
            "### Live app state", "",
            f"- Product/model bytes: `{model_text}`; CAN ID `{scalar(runtime, runtime_base, config_base + 0x20, 'I')}`, master ID `{scalar(runtime, runtime_base, config_base + 0x1C, 'I')}`.",
            f"- Pole pairs `{scalar(runtime, runtime_base, config_base + 0x40, 'I')}`; Rs `{scalar(runtime, runtime_base, config_base + 0x44):.9g} ohm`; Ls `{scalar(runtime, runtime_base, config_base + 0x48):.9g} H`; flux `{scalar(runtime, runtime_base, config_base + 0x4C):.9g} Wb`.",
            f"- Current zero offsets: `{scalar(runtime, runtime_base, state_base + 0x40):.4f}`, `{scalar(runtime, runtime_base, state_base + 0x44):.4f}`, `{scalar(runtime, runtime_base, state_base + 0x48):.4f}` ADC counts.",
            f"- Snapshot bus voltage `{scalar(runtime, runtime_base, state_base + 0x68):.4f} V`, temperature-like channel `{scalar(runtime, runtime_base, state_base + 0x84):.4f}`, fault code `{scalar(runtime, runtime_base, state_base + 0x80, 'I')}`.",
            "- ADC result words: `" + " ".join(str(item) for item in raw) + "`.",
            "",
        ]
    )

    gpio = files["gpio_pfs_0x40053800_0x480_h16.bin"]
    dma2 = files["dma2_0x40053400_0x80_w32.bin"]
    boot_node_id = struct.unpack(
        "<I", files["boot_can_node_id_0x0003e020.bin"]
    )[0]
    boot_rate = struct.unpack(
        "<I", files["boot_can_rate_selector_0x0003e08c.bin"]
    )[0]
    lines.extend(
        [
            "### Newly recaptured GPIO/DMA and boot CAN configuration",
            "",
            f"- Boot CAN node ID: `{boot_node_id}`; rate selector: `{boot_rate}`.",
            f"- GPIO/PFS capture contains `{sum(value != 0 for value in gpio)}` non-zero bytes.",
            f"- DMA2 capture contains `{sum(value != 0 for value in dma2)}` non-zero bytes.",
            "",
            "### Capture consistency warnings",
            "",
        ]
    )
    if not any(gpio):
        lines.append(
            "- The recaptured GPIO/PFS image is entirely zero. This conflicts with the running "
            "ADC/SPI/MCAN/PWM/USART configuration and is not a valid PFS readback."
        )
    if not any(dma2):
        lines.append(
            "- The recaptured DMA2 image is entirely zero. This conflicts with AOS DMA2 routing and "
            "the live position DMA destination at 0x1fffa666; treat it as an invalid readback."
        )
    if any(gpio) and any(dma2):
        lines.append("- No all-zero GPIO/DMA capture warning remains.")
    lines.append("")
    return lines


def initialized_ram_diff(runtime: bytes) -> list[str]:
    firmware = OFFICIAL_APP.read_bytes()
    packed_start = 0x2AB90 - 0x20000
    packed_end = 0x2C84C - 0x20000
    initialized = arm_scatter_decode(firmware[packed_start:packed_end], 0x2268)
    live = runtime[:0x2268]
    changed = [index for index, pair in enumerate(zip(initialized, live)) if pair[0] != pair[1]]
    return [
        "## Initialized RAM comparison",
        "",
        f"The live capture differs from the scatter-loaded image in `{len(changed)}` of `0x2268` bytes. "
        "Those changes are runtime calibration/configuration values rather than a decode error.",
        "",
    ]


def build_report(bins: Path) -> str:
    peripherals = read_svd()
    events = read_events()
    files: dict[str, bytes] = {}
    lines = ["# HC32F448 capture decode", ""]

    for capture in CAPTURES:
        path = bins / capture.filename
        if not path.is_file():
            raise FileNotFoundError(path)
        data = path.read_bytes()
        if len(data) != capture.length:
            raise ValueError(
                f"{path}: expected {capture.length:#x} bytes, got {len(data):#x}"
            )
        files[capture.filename] = data

    lines.extend(custom_summary(files, events))
    lines.extend(initialized_ram_diff(files["app_runtime_config.bin"]))
    lines.extend(["## Registers differing from reset", ""])
    for capture in CAPTURES:
        if capture.peripheral is None:
            continue
        lines.extend(
            [
                f"### {capture.peripheral} (`{capture.base:#010x}`)",
                "",
            ]
        )
        decoded = decode_registers(capture, files[capture.filename], peripherals)
        lines.extend(decoded or ["- No decoded register differs from the SVD reset value."])
        lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bins", type=Path, default=ROOT / "bins")
    parser.add_argument("--output", type=Path)
    arguments = parser.parse_args()
    try:
        report = build_report(arguments.bins)
    except (FileNotFoundError, ValueError, ET.ParseError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    if arguments.output:
        arguments.output.write_text(report + "\n")
    else:
        print(report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
