#!/usr/bin/env python3
"""Project recovered DM4310 ELF sections onto the DM8009 V6417 SRAM ABI.

The input address map is generated from aligned factory instruction streams.
For every source section this report translates each referenced byte back to a
candidate section base.  A single candidate means the object can retain its
current internal layout; multiple candidates identify an object that V6417
split or reordered and therefore needs an explicit source/linker adjustment.
"""

from argparse import ArgumentParser
from collections import Counter
from io import BytesIO
from pathlib import Path
import csv

from elftools.elf.elffile import ELFFile


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ELF = ROOT / "build/dm4310.elf"
DEFAULT_MAP = (
    ROOT / "recovered/dm8009/tables/dm4310_sram_address_map.tsv"
)


def load_address_map(path):
    with path.open(newline="") as source:
        rows = csv.DictReader(source, delimiter="\t")
        return {
            int(row["dm4310_address"], 16): int(row["dm8009_address"], 16)
            for row in rows
        }


def load_sections(path):
    elf = ELFFile(BytesIO(path.read_bytes()))
    return [
        (section.name, section["sh_addr"], section["sh_size"])
        for section in elf.iter_sections()
        if section.name.startswith(".dm4310_")
        and 0x1FFF8000 <= section["sh_addr"] < 0x20000000
        and section["sh_size"]
    ]


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, default=DEFAULT_ELF)
    parser.add_argument("--address-map", type=Path, default=DEFAULT_MAP)
    args = parser.parse_args()

    address_map = load_address_map(args.address_map)
    print("section\tdm4310_base\tsize\treferences\tcandidate_bases")
    for name, base, size in load_sections(args.elf):
        candidates = Counter()
        for old_address, new_address in address_map.items():
            if base <= old_address < base + size:
                candidates[new_address - (old_address - base)] += 1
        rendered = ",".join(
            f"{candidate:#010x}:{count}"
            for candidate, count in candidates.most_common()
        )
        print(
            f"{name}\t{base:#010x}\t{size:#x}\t"
            f"{sum(candidates.values())}\t{rendered or '-'}"
        )


if __name__ == "__main__":
    main()
