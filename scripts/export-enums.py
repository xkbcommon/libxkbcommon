#!/usr/bin/env python3

# Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
#
# SPDX-License-Identifier: MIT

import argparse
import difflib
import json
import sys
from pathlib import Path
from typing import Any, Iterable, Optional, Union

import jinja2
from clang.cindex import Config, CursorKind, Index

# Define the structure for extracted enum data
EnumConstant = dict[str, Union[int, str]]
EnumData = dict[str, list[EnumConstant]]

SCRIPT = Path(__file__)
ROOT = SCRIPT.parent.parent
ENUMS_PATH = ROOT / "data" / "enums"
HEADERS_PATH = ROOT / "include" / "xkbcommon"

LIBXKBCOMMON_HEADERS = (
    HEADERS_PATH / "xkbcommon.h",
    HEADERS_PATH / "xkbcommon-compose.h",
    HEADERS_PATH / "xkbcommon-features.h",
)
ALL_HEADERS = LIBXKBCOMMON_HEADERS
# TODO: other headers
# (
#     HEADERS_PATH / "xkbcommon-x11.h",
#     HEADERS_PATH / "xkbregistry.h",
# )


def get_enum_data(header_path: Path) -> Optional[EnumData]:
    """
    Parses a C header file and extracts all enums and their constants into a
    structured Python dictionary.

    Returns:
        dict: A dictionary where keys are enum names (or typedef names) and
              values are lists of constant dictionaries. Returns None on failure.
    """

    try:
        # Initialize Clang Index. Config.set_library_path must be called
        # before this if needed.
        index: Index = Index.create()
    except Exception as e:
        print(f"Error initializing libclang: {e}", file=sys.stderr)
        print(
            "Please ensure libclang is correctly installed and accessible.",
            file=sys.stderr,
        )
        return None

    # Parse arguments: standard C header mode and C11 standard.
    args: list[str] = ["-x", "c-header", "-std=c11"]

    if not header_path.exists():
        print(f"Error: Header file not found at '{header_path}'", file=sys.stderr)
        return None

    # Create the translation unit (TU)
    # libclang expects a string path, so we cast the Path object back to str
    tu: Any = index.parse(str(header_path), args=args)
    if not tu:
        print(
            f"Error: Failed to parse translation unit for '{header_path}'",
            file=sys.stderr,
        )
        return None

    enum_data: EnumData = {}

    def visit_node(cursor: Any) -> None:
        """Recursively visits the nodes in the AST."""

        if cursor.kind == CursorKind.ENUM_DECL:
            enum_name: str = cursor.displayname

            # Handle anonymous enums (try to find typedef parent)
            if not enum_name:
                parent: Any = cursor.semantic_parent
                if parent and parent.kind == CursorKind.TYPEDEF_DECL:
                    enum_name = parent.displayname
                else:
                    # Skip truly anonymous enums that aren’t typedef’d
                    return

            constants: list[EnumConstant] = []

            # Extract constants
            for child in cursor.get_children():
                if child.kind == CursorKind.ENUM_CONSTANT_DECL:
                    value: Union[int, Any] = child.enum_value

                    constants.append(
                        {
                            "name": child.displayname,
                            # Store value as its native type (int/str)
                            "value": value if isinstance(value, int) else str(value),
                        }
                    )

            if constants:
                enum_data[enum_name] = constants

        # Recurse into children of the current node
        for child in cursor.get_children():
            visit_node(child)

    # Start the traversal from the root
    visit_node(tu.cursor)

    return enum_data


def format_to_yaml(header_path: Path, enum_data: EnumData) -> str:
    """
    Formats the structured enum data into a YAML-like string.
    """
    output: list[str] = [
        f"# Extracted Enums from: {header_path.resolve().relative_to(ROOT)}\n"
    ]

    for enum_name, constants in enum_data.items():
        output.append(f"{enum_name}:")
        for constant in constants:
            output.append(f"  - name: {constant['name']}")

            value_str: str = json.dumps(constant["value"])
            output.append(f"    value: {value_str}")

    return "\n".join(output)


def generate_c(env: jinja2.Environment, root: Path, file: Path, **data) -> None:
    """Generate a file from its Jinja2 template"""
    template_path = file.with_suffix(f"{file.suffix}.jinja")
    template = env.get_template(str(template_path))
    path = root / file
    with path.open("wt", encoding="utf-8") as fd:
        fd.writelines(template.generate(**data))


def enum_name_from_feature(feature: str) -> str | None:
    if "_FEATURE_ENUM_" in feature:
        return feature.replace("_FEATURE_ENUM_", "_").lower()
    else:
        return None


def update_command(args: argparse.Namespace) -> int:
    """
    Handles the 'update' subcommand: parses header files, update YAML and C files.
    """
    # Update YAML files
    enums: dict[Path, EnumData] = {}
    for header_path in ALL_HEADERS:
        yaml_path = ENUMS_PATH / header_path.with_suffix(".yaml").name
        if (enum_data := get_enum_data(header_path)) is not None:
            enums[header_path] = enum_data
            with yaml_path.open("wt", encoding="utf-8") as fd:
                fd.write(format_to_yaml(header_path, enum_data))
                fd.write("\n")
        else:
            return 1

    # Update C files
    template_loader = jinja2.FileSystemLoader(ROOT, encoding="utf-8")
    jinja_env = jinja2.Environment(
        loader=template_loader,
        keep_trailing_newline=True,
        trim_blocks=True,
        lstrip_blocks=True,
        extensions=["jinja2.ext.do"],
    )

    def is_flag_like(values: Iterable[EnumConstant]) -> bool:
        return all(v["value"] >= 0 and v["value"].bit_count() <= 1 for v in values)

    IMPLICIT_FLAGS = {
        "xkb_state_component",
        "xkb_keyboard_controls",
        "xkb_state_match",
    }

    def is_flag_name(enum: str) -> bool:
        return enum.endswith("_flags") or enum in IMPLICIT_FLAGS

    def is_flag(enum: str, values: Iterable[EnumConstant]) -> bool:
        return is_flag_like(values) and is_flag_name(enum)

    jinja_env.globals["enum_name_from_feature"] = enum_name_from_feature
    jinja_env.globals["is_flag"] = is_flag
    jinja_env.globals["has_zero"] = lambda es: any(e["value"] == 0 for e in es)
    jinja_env.globals["has_values_mask"] = lambda es: all(
        e["value"] >= 0 and e["value"] < 16 for e in es
    )
    enum_data: EnumData = {}
    for header_path in LIBXKBCOMMON_HEADERS:
        enum_data.update(enums[header_path])
    generate_c(
        env=jinja_env,
        root=ROOT,
        script=SCRIPT.relative_to(ROOT),
        file=Path("src/features.c"),
        enum_data=enum_data,
    )
    generate_c(
        env=jinja_env,
        root=ROOT,
        script=SCRIPT.relative_to(ROOT),
        file=Path("src/features/enums.h"),
        enum_data=enum_data,
    )
    return 0


def export_command(args: argparse.Namespace) -> int:
    """Handles the 'export' subcommand: parses file and prints YAML to stdout."""
    if (enum_data := get_enum_data(args.header_file)) is not None:
        print(format_to_yaml(args.header_file, enum_data))
        return 0
    else:
        return 1


def check_xkb_enum(
    ref_enum: str, ref_enum_header_path: Path | None, header_path: Path, data: EnumData
) -> int:
    if ref_enum_header_path is not None:
        if (ref_data := get_enum_data(ref_enum_header_path)) is None:
            return 1
    else:
        ref_data = data

    # Enum xkb_feature should contain all other enums
    enums = set(enum for enum in data)
    for entry in ref_data[ref_enum]:
        if (enum := enum_name_from_feature(entry["name"])) is not None:
            enums.discard(enum)
    if enums:
        print(
            f"Error: missing entries in {ref_enum} for header {header_path}: {enums}",
            file=sys.stderr,
        )
        return 1
    return 0


def check_header(header_path: Path, yaml_path: Path) -> int:
    extracted_data: Optional[EnumData] = get_enum_data(header_path)
    if extracted_data is None:
        return 1

    # Enum xkb_feature should contain all enums from libxkbcommon
    if header_path.name == "xkbcommon-features.h":
        ret = check_xkb_enum("xkb_feature", None, header_path, extracted_data)
    else:
        ret = check_xkb_enum(
            "xkb_feature",
            header_path.with_name("xkbcommon-features.h"),
            header_path,
            extracted_data,
        )

    if ret:
        return ret

    # Generate the YAML output string
    extracted_yaml: str = format_to_yaml(header_path, extracted_data).strip()

    try:
        expected_yaml: str = yaml_path.read_text().strip()
    except Exception as e:
        print(f"Error reading YAML file: {e}", file=sys.stderr)
        return 1

    # Comparison
    if extracted_yaml == expected_yaml:
        print(f"Check SUCCESS: Extracted enum structure matches '{yaml_path}'.")
        return 0
    else:
        print(
            f"Check FAILED: Extracted enum structure does NOT match '{yaml_path}'.",
            file=sys.stderr,
        )
        # Split strings into lines for difflib, ensuring a trailing newline for the last line
        expected = expected_yaml.splitlines(keepends=True)
        got = extracted_yaml.splitlines(keepends=True)

        # Generate the unified diff
        diff_lines = difflib.unified_diff(
            expected,
            got,
            fromfile=str(yaml_path),
            tofile=f"(result from parsing header: {args.header_file})",
        )

        print("\n--- Unified Difference ---", file=sys.stderr)
        for line in diff_lines:
            print(line.rstrip("\n"), file=sys.stderr)

        return 1


def check_command(args: argparse.Namespace) -> int:
    """
    Handles the 'check' subcommand: parses file and compares generated YAML
    against a target YAML file.

    Returns:
        int: The exit code (0 for success, 1 for failure).
    """
    header_path: Path
    if (header_path := args.header_file) is not None:
        if (yaml_path := args.yaml_file) is None:
            yaml_path = ENUMS_PATH / header_path.with_suffix(".yaml").name
        return check_header(header_path, yaml_path)
    else:
        for header_path in ALL_HEADERS:
            yaml_path = ENUMS_PATH / header_path.with_suffix(".yaml").name
            if ret := check_header(header_path, yaml_path):
                return ret
        return 0


if __name__ == "__main__":
    parser: argparse.ArgumentParser = argparse.ArgumentParser(
        description="A tool to extract and check C enums using libclang"
    )

    parser.add_argument(
        "--libclang-path",
        type=Path,
        default=None,
        help="Path to the directory containing the libclang library.",
    )

    # Setup subparsers for commands
    subparsers: Any = parser.add_subparsers(
        dest="command", required=True, help="Available subcommands"
    )

    # UPDATE Command
    parser_update: argparse.ArgumentParser = subparsers.add_parser(
        "update", help="Parse headers, update YAML and C files."
    )
    parser_update.set_defaults(func=update_command)

    # EXPORT Command
    parser_export: argparse.ArgumentParser = subparsers.add_parser(
        "export", help="Parse header and print the YAML output."
    )
    parser_export.add_argument(
        "header_file", type=Path, help="Path to the C header file (.h) to be parsed."
    )
    parser_export.set_defaults(func=export_command)

    # CHECK Command
    parser_check: argparse.ArgumentParser = subparsers.add_parser(
        "check", help="Parse header and compare generated YAML against a target file."
    )
    parser_check.add_argument(
        "--header-file",
        type=Path,
        required=False,
        help="Path to the C header file (.h) to be parsed.",
    )
    parser_check.add_argument(
        "--yaml-file",
        type=Path,
        required=False,
        help="Path to the expected YAML file to compare against.",
    )
    parser_check.set_defaults(func=check_command)

    args: argparse.Namespace = parser.parse_args()

    # 1. Handle global configuration (libclang path) before running the command function
    if args.libclang_path:
        print(
            f"Setting libclang library path to: {args.libclang_path}", file=sys.stderr
        )
        try:
            Config.set_library_path(str(args.libclang_path))
        except Exception as e:
            print(f"Warning: Failed to set libclang path: {e}", file=sys.stderr)

    # 2. Run the specific command function
    exit_code: int = args.func(args)
    sys.exit(exit_code)
