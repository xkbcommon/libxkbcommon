#!/usr/bin/env python3

# Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
# SPDX-License-Identifier: MIT

"""
Transform keysyms comments into docstring
"""

import argparse
import html
import itertools
import sys
from pathlib import Path

import tomllib

# Root of the project
SCRIPT = Path(__file__)
sys.path.append(str(SCRIPT.parent))
ROOT = SCRIPT.parent.parent
DEFAULT_HEADER = ROOT / "include/xkbcommon/xkbcommon-keysyms.h"
DEFAULT_AGE = ROOT / "data/keysyms/age.toml"

from keysyms import (  # noqa: E402
    DeprecationReason,
    Keysym,
    KeysymCategory,
    Keysyms,
    Semantics,
)

# Parse commands
parser = argparse.ArgumentParser(
    description="Transform keysyms comments into docstring"
)
parser.add_argument(
    "c_header",
    type=Path,
    default=DEFAULT_HEADER,
    help="Path to the libxkbcommon keysym header",
)
parser.add_argument(
    "--age",
    type=Path,
    default=DEFAULT_AGE,
    help="Path to the TOML file with keysyms age",
)
args = parser.parse_args()


def semantics(s: Semantics) -> str:
    match s:
        case Semantics.Default:
            return ""
        case Semantics.ComputerNumpad:
            return "computer numpad"
        case Semantics.OtherKeypad:
            return "phone, remote controls and other keypads"
        case _:
            raise ValueError(s)


def escape(raw: str):
    return (
        html.escape(raw, quote=False)
        .replace("\\", "\\\\")
        .replace("`", "\\`")
        .replace("@", "\\@")
    )


def serialize(keysym: Keysym, age: str) -> str:
    prefix = "\n * "
    comment = f"{prefix}Keysym **{keysym.name}**"
    properties = f"{prefix}<dl>{prefix}<dt>Value</dt><dd>`0x{keysym.value:04x}`</dd>"
    deprecated = ""

    category_ref = keysym.category.casefold().replace(" ", "-") + "-keysyms"
    properties += (
        f"{prefix}<dt>Category</dt><dd>[{keysym.category}](@ref {category_ref})</dd>"
    )

    properties += f"{prefix}<dt>Preferred name</dt><dd>"
    if keysym.preferred is keysym:
        properties += "✅"
    elif keysym.deprecated:
        properties += "⚠️"
    else:
        properties += "ℹ️"
    properties += f" [{keysym.preferred.name}](@ref {keysym.preferred.macro}) ("
    if keysym.preferred is keysym:
        properties += "current name"
    elif keysym.deprecated:
        properties += "replacement"
    else:
        properties += "alternative"
    properties += ")</dd>"

    aliases: list[Keysym] = sorted(
        (
            k
            for k in itertools.chain((keysym.canonical,), keysym.canonical.aliases)
            if k is not keysym and k is not keysym.preferred
        ),
        key=lambda k: k.name,
    )
    if aliases:
        if keysym is keysym.preferred:
            properties += f"{prefix}<dt>Aliases</dt>"
        else:
            properties += f"{prefix}<dt>Other aliases</dt>"
        for alias in aliases:
            properties += "<dd>"
            properties += "🚫" if alias.deprecated else "ℹ️"
            properties += f" [{alias.name}](@ref {alias.macro})"
            if alias.deprecated:
                properties += " (deprecated)"
            properties += "</dd>"

    if (char := keysym.canonical.char) is not None:
        comment += prefix
        if keysym.canonical.deprecation is DeprecationReason.UNICODE_MISMATCH:
            deprecated += (
                f"{prefix}@deprecated Unclear correspondance in Unicode; closest "
                f"is: {char.markdown_cp}"
            )
            if c := char.some_char(printable=True):
                deprecated += f" “{escape(c)}”"
            approximation = "**⚠️ approximation:** "
        else:
            assert keysym.canonical.deprecation is None
            approximation = ""

        properties += f"{prefix}<dt>Unicode code point</dt><dd>{approximation}{char.markdown_cp}</dd>"

        if c := char.some_char(printable=True):
            properties += (
                f"{prefix}<dt>Character</dt><dd>{approximation}{escape(c)}</dd>"
            )

        if s := semantics(keysym.canonical.char_semantics):
            properties += f"{prefix}<dt>Special semantics</dt><dd>{s}</dd>"

        if keysym.canonical.char_aliases or keysym.category is KeysymCategory.Legacy:
            properties += f"{prefix}<dt>Keysyms with the same character</dt>"
            if keysym.category is KeysymCategory.Legacy:
                properties += (
                    f"<dd>`U{char.cp:04X}`: [Unicode keysym](@ref unicode-keysyms)</dd>"
                )
            for char_alias in keysym.canonical.char_aliases:
                properties += f"<dd>[{char_alias.name}](@ref {char_alias.macro})"
                if s := semantics(char_alias.char_semantics):
                    properties += f": {s}"
                properties += "</dd>"
        # TODO: Unicode keysym range
    elif keysym.deprecated:
        comment += prefix
        deprecated = f"{prefix}@deprecated "
        match keysym.deprecation:
            case DeprecationReason.TYPO:
                deprecated += "*Typo*"
            case DeprecationReason.IMPLICIT_ALIAS | DeprecationReason.LEGACY_ALIAS:
                deprecated += "*Legacy alias*"
            case DeprecationReason.UNKNOWN:
                deprecated += "*Legacy keysym*"
            case _:
                raise ValueError(keysym)

        if keysym.deprecated_keysym:
            if keysym.aliases:
                deprecated += (
                    f"{prefix}@deprecated All the names of this keysym are deprecated"
                )
        elif keysym.preferred is not keysym:
            deprecated += f". Use `::{keysym.preferred.macro}` instead."
        else:
            raise ValueError(keysym)
    elif keysym.comment:
        comment += f": {keysym.comment.strip()}{prefix}"
    else:
        comment += prefix
        pass  # TODO

    comment += deprecated
    if deprecated:
        comment += prefix

    properties += f"{prefix}</dl>"
    comment += properties

    if age:
        comment += f"{prefix}@since {age}"

    comment += f"{prefix}@addindex {keysym.name}"

    value = keysym.pretty_value
    return f"/**{comment}\n */\n#define {keysym.macro}\t{value}"


if __name__ == "__main__":
    with args.age.open("rb") as f:
        ages = {
            name: entry["version"]
            for entry in tomllib.load(f).values()
            for name in entry["names"]
        }

    print("""\
    /**
    * @defgroup predefined-keysyms Predefined keysyms
    * List of *predefined* [keysyms](@ref xkb_keysym_t) names
    *
    * @ingroup keysyms
    * @{
    */

    """)

    for x in Keysyms.parse_iter_file(args.c_header):
        if isinstance(x, Keysym):
            print(serialize(x, ages.get(x.name, "")))
        else:
            print(x, end="")

    print("/** @} */")
