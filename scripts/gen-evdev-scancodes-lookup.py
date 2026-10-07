#!/usr/bin/env python3

# Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
#
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import math
import re
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).parent.parent
KEY_ENTRY_PATTERN = re.compile(
    r"""^
    \#define\s+
    KEY_(?P<name>\w+)\s+
    (?P<code>0x[0-9a-fA-F]+|[0-9]+)
    """,
    re.VERBOSE,
)


def convert_line(line: str) -> str:
    if m := KEY_ENTRY_PATTERN.match(line):
        code = int(m.group("code"), base=0)
        return code, f"""\t{{{m.group("code")}, "{m.group("name")}"}},"""
    else:
        return 0, ""


def generate(lines: Iterable[str]):
    print('#include "config.h"')
    print("#include <stdint.h>")
    print('#include "utils.h"')
    print("static const struct {")
    print("\tuint32_t code;")
    print("\tconst char *name;")
    print("}  evdev_scancodes[] = {")
    code_pre = -math.inf
    for code, line in filter(lambda x: bool(x[1]), map(convert_line, lines)):
        if code > code_pre:
            print(line)
        else:
            raise ValueError(f"not sorted: {code_pre} >= {code}")
    print("};")
    print("")
    print("static inline const char *")
    # Binary search
    print("evdev_scancode_name(uint32_t code) {")
    print("\tuint32_t low = 0;")
    print("\tuint32_t high = (uint32_t)ARRAY_SIZE(evdev_scancodes) - 1;")
    print("\twhile (low <= high) {")
    print("\t\tconst uint32_t mid = low + (high - low) / 2;")
    print("\t\tif (evdev_scancodes[mid].code < code) low = mid + 1;")
    print("\t\telse if (evdev_scancodes[mid].code > code) high = mid - 1;")
    print("\t\telse return evdev_scancodes[mid].name;")
    print("\t}")
    print("\treturn NULL;")
    print("}")


def main():
    # Parse commands
    parser = argparse.ArgumentParser(
        description="Generate lookup table for evdev scancodes"
    )
    parser.add_argument(
        "input",
        type=argparse.FileType("rt", encoding="utf-8"),
        help="Path to the evdev scancodes header",
    )

    args = parser.parse_args()
    generate(args.input)


if __name__ == "__main__":
    main()
