#!/usr/bin/env python3
import argparse
import re
import sys

# Matches outer <a ... href="#..."> tag + non-tag text, stripping any nested <a ...>...</a>
NESTED_LINK_REGEX = re.compile(
    r'(<a\b[^>]*?\bhref=["\']?#[^"\'\s>]*["\']?[^>]*>[^<]*)<a\b[^>]*?>(.*?)</a>',
    re.IGNORECASE | re.DOTALL,
)


def fix_html_file(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Replaces <a href="#outer">text <a href="http...">inner</a> with <a href="#outer">text inner
    fixed_content = NESTED_LINK_REGEX.sub(r"\1\2", content)

    if fixed_content != content:
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(fixed_content)
        print(f"Fixed nested links in: {file_path}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(
        description="Unwrap nested HTML anchor tags in Doxygen-generated documentation."
    )
    parser.add_argument(
        "files",
        nargs="+",
        metavar="FILE",
        help="Path to HTML file(s) to process",
    )

    args = parser.parse_args()

    for filepath in args.files:
        fix_html_file(filepath)


if __name__ == "__main__":
    main()
