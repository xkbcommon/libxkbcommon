#!/usr/bin/env python3

import argparse
import itertools
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any, TypeAlias

import jinja2

KEYSYM_PATTERN = re.compile(
    r"^#define\s+XKB_KEY_(?P<name>\w+)\s+(?P<value>0x[0-9a-fA-F]+)\s"
)
MAX_AMBIGUOUS_NAMES = 3

KeysymsBounds: TypeAlias = dict[str, int | str]
KeysymsCaseFoldedNames: TypeAlias = dict[str, list[str]]


def load_keysyms(path: Path) -> tuple[KeysymsBounds, KeysymsCaseFoldedNames]:
    # Load the keysyms header
    keysym_min = sys.maxsize
    keysym_max = 0
    min_unicode_keysym = 0x01000100
    max_unicode_keysym = 0x0110FFFF
    canonical_names: dict[int, str] = {}
    casefolded_names: dict[str, list[str]] = defaultdict(list)
    max_unicode_name = "U10FFFF"
    max_keysym_name = "0x1fffffff"  # XKB_KEYSYM_MAX

    with path.open("rt", encoding="utf-8") as fd:
        for line in fd:
            if m := KEYSYM_PATTERN.match(line):
                value = int(m.group("value"), 16)
                keysym_min = min(keysym_min, value)
                keysym_max = max(keysym_max, value)
                name = m.group("name")
                casefolded_names[name.casefold()].append(name)
                if value not in canonical_names:
                    canonical_names[value] = name

    XKB_KEYSYM_LONGEST_CANONICAL_NAME = max(
        max(canonical_names.values(), key=len),
        max_unicode_name,
        max_keysym_name,
        key=len,
    )
    XKB_KEYSYM_LONGEST_NAME = max(
        max(itertools.chain.from_iterable(casefolded_names.values()), key=len),
        max_unicode_name,
        max_keysym_name,
        key=len,
    )

    # Keep only ambiguous case-insensitive names and sort them
    for name in tuple(casefolded_names.keys()):
        count = len(casefolded_names[name])
        if count < 2:
            del casefolded_names[name]
        elif count > MAX_AMBIGUOUS_NAMES:
            raise ValueError(
                f"""Expected max {MAX_AMBIGUOUS_NAMES} keysyms for "{name}", got: {count}"""
            )
        else:
            casefolded_names[name].sort()

    return (
        {
            "XKB_KEYSYM_MIN_ASSIGNED": min(keysym_min, min_unicode_keysym),
            "XKB_KEYSYM_MAX_ASSIGNED": max(keysym_max, max_unicode_keysym),
            "XKB_KEYSYM_MIN_EXPLICIT": keysym_min,
            "XKB_KEYSYM_MAX_EXPLICIT": keysym_max,
            "XKB_KEYSYM_COUNT_EXPLICIT": len(canonical_names),
            # Extra byte for terminating NULL
            "XKB_KEYSYM_NAME_MAX_SIZE": len(XKB_KEYSYM_LONGEST_CANONICAL_NAME) + 1,
            "XKB_KEYSYM_LONGEST_CANONICAL_NAME": XKB_KEYSYM_LONGEST_CANONICAL_NAME,
            "XKB_KEYSYM_LONGEST_NAME": XKB_KEYSYM_LONGEST_NAME,
        },
        casefolded_names,
    )


def generate(
    env: jinja2.Environment,
    data: dict[str, Any],
    root: Path,
    file: Path,
):
    """Generate a file from its Jinja2 template"""
    template_path = file.with_suffix(f"{file.suffix}.jinja")
    template = env.get_template(str(template_path))
    path = root / file
    with path.open("wt", encoding="utf-8") as fd:
        fd.writelines(template.generate(**data))


# Root of the project
ROOT = Path(__file__).parent.parent

# Parse commands
parser = argparse.ArgumentParser(
    description="Generate C header files related to keysyms bounds"
)
parser.add_argument(
    "--root",
    type=Path,
    default=ROOT,
    help="Path to the root of the project (default: %(default)s)",
)

args = parser.parse_args()

# Configure Jinja
template_loader = jinja2.FileSystemLoader(args.root, encoding="utf-8")
jinja_env = jinja2.Environment(
    loader=template_loader,
    keep_trailing_newline=True,
    trim_blocks=True,
    lstrip_blocks=True,
)

jinja_env.filters["keysym"] = lambda ks: f"0x{ks:0>8x}"

# Load keysyms
keysyms_bounds, keysyms_ambiguous_case_insensitive_names = load_keysyms(
    args.root / "include/xkbcommon/xkbcommon-keysyms.h"
)

# Generate the files
generate(
    jinja_env,
    keysyms_bounds,
    args.root,
    Path("src/keysym.h"),
)

generate(
    jinja_env,
    dict(
        keysyms_bounds,
        ambiguous_case_insensitive_names=keysyms_ambiguous_case_insensitive_names,
        MAX_AMBIGUOUS_NAMES=MAX_AMBIGUOUS_NAMES,
    ),
    args.root,
    Path("test/keysym.h"),
)
