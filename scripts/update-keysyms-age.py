#!/usr/bin/env python3

# Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
# SPDX-License-Identifier: MIT


"""

Walk a range of commits in a git repository, find every line that was
*added* matching:

    #define XKB_KEY_<something>

and record, for each distinct symbol, the first commit that introduced it
and the next git tag that contains that commit. Results are written to a
TOML file.

Usage:
    python3 update-keysyms-age.py --repo /path/to/repo \
        --range v1.0.0..v1.5.0 \
        --path include/xkbcommon/xkbcommon-keysyms.h \
        --output xkb_key_defines.csv

If --range is omitted, the full history (all commits reachable from HEAD)
is scanned. If --path is omitted, the whole repository is scanned.

Incremental / resume mode:
    If --output already points to an existing TOML file *and* --range was
    NOT given, the script resumes instead of rescanning everything: it
    reads the entry of the existing TOML, starts scanning from the
    commit right after that row's first_commit through HEAD, and appends
    only the newly found symbols to the file (no header rewritten, no
    existing rows touched). This makes it cheap to re-run periodically to
    pick up newly added symbols.

    To force a full rescan even if the output file exists, either delete
    the output file first or pass an explicit --range.
"""

import argparse
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any

import tomllib
from packaging.version import Version

SCRIPT = Path(__file__)
ROOT = SCRIPT.parent.parent
HEADER = ROOT / "include/xkbcommon/xkbcommon-keysyms.h"
OUTPUT = ROOT / "data/keysyms/age.toml"
TAG_PREFIX = "xkbcommon-"

# Matches a diff "added line" of the form: +#define XKB_KEY_foo ...
MACRO_PATTERN = re.compile(r"^\+\s*#define\s+XKB_KEY_(\w+)")

# Matches a commit header line in `git log -p` output: "commit <hash>"
COMMIT_PATTERN = re.compile(r"^commit ([0-9a-f]{7,40})")


def run_git(args: list[str], cwd: Path):
    try:
        result = subprocess.run(["git"] + args, cwd=cwd, capture_output=True, text=True)
    except OSError as e:
        raise RuntimeError(f"could not run git in '{cwd}': {e}")
    if result.returncode != 0:
        raise RuntimeError(
            "git {} failed:\n{}".format(" ".join(args), result.stderr.strip())
        )
    return result.stdout


def get_branch_root_commit(repo: Path) -> str:
    return run_git(["hash-object", "-t", "tree", "/dev/null"], repo).strip()


def get_log_text(repo: Path, rev_range: str, path: Path):
    """
    Fetch full patches (in chronological order, oldest first) for every
    commit that touched a line matching '#define XKB_KEY_'. We filter to
    only *added* lines ourselves afterwards, since -G matches any change
    (add or remove) to a matching line.
    """
    args = [
        "log",
        "--reverse",
        "-p",
        "--no-color",
        "--no-decorate",
        "-G",
        r"#define\s+XKB_KEY_",
    ]
    if rev_range:
        args.append(rev_range)
    if path:
        args += ["--", str(path)]
    return run_git(args, repo)


def parse_log(log_text):
    """
    Returns an ordered dict: macro_name -> first_commit_hash
    (first = earliest commit in the scanned range that added the line).
    """
    results = {}
    current_commit = None
    for line in log_text.splitlines():
        m = COMMIT_PATTERN.match(line)
        if m:
            current_commit = m.group(1)
            continue
        m = MACRO_PATTERN.match(line)
        if m and current_commit:
            name = m.group(1)
            if name not in results:
                results[name] = current_commit
    return results


def get_lib_version(repo: Path, commit: str, rev_range: str | None) -> Version | None:
    """
    Returns the nearest tag that contains `commit` (i.e. the first tag
    reachable from `commit`), or '' if no tag contains it (e.g. it hasn't
    been released yet).
    """

    candidates = set(
        t
        for t in run_git(["tag", "--contains", commit], repo).splitlines()
        if t.startswith(TAG_PREFIX)
    )

    if not candidates:
        return None

    for r in (rev_range, f"{commit}..HEAD"):
        if not r:
            continue
        _, end = r.split("..")
        backup = set(candidates)
        for tag in backup:
            try:
                run_git(["merge-base", "--is-ancestor", tag, end], repo)
            except RuntimeError:
                candidates.remove(tag)
        if candidates:
            break
        else:
            candidates = backup

    results: list[tuple[int, Version]] = sorted(
        (
            int(run_git(["rev-list", "--count", f"{commit}..{tag}"], repo).strip()),
            Version(tag.removeprefix(TAG_PREFIX)),
        )
        for tag in candidates
    )

    if not results:
        return None
    elif results[0][1].is_prerelease:
        # Pre-release: check for next non-prerelease tag
        for _, version in results[1:]:
            if not version.is_prerelease:
                return version

    return results[0][1]


def read_existing_output(path: Path) -> tuple[str, set[str]]:
    """
    Read a previously-generated TOML, if any.

    Returns (last_commit, existing_symbols):
      - last_commit: first_commit value of the LAST data row in the file,
        or None if the file has no data rows.
      - existing_symbols: set of symbol names already present, so we never
        write a duplicate row.
    """
    existing_symbols: set[str] = set()
    last_commit: str = ""

    with path.open("rb") as f:
        commits = tomllib.load(f)

    if not commits:
        return "", existing_symbols

    for last_commit, entry in commits.items():
        existing_symbols.update(entry["names"])

    return last_commit, existing_symbols


def main():
    parser = argparse.ArgumentParser(
        description=(
            'Scan a git commit range for added "#define XKB_KEY_*" lines '
            "and record the first commit + next tag for each symbol."
        )
    )
    parser.add_argument(
        "--repo",
        type=Path,
        default=Path("."),
        help="Path to the git repo (default: current dir)",
    )
    parser.add_argument(
        "--range",
        default=None,
        help='Commit range, e.g. "xkbcommon-1.0.0..xkbcommon-1.5.0" or "abc123..HEAD". '
        "Omit to scan the whole history reachable from HEAD (or, if "
        "--output already exists, to resume from where it left off).",
    )
    parser.add_argument(
        "--path",
        type=Path,
        default=HEADER,
        help="Restrict the scan to a specific file or path within the repo. (default: %(default)s)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=OUTPUT,
        help="Output TOML file path (default: %(default)s)",
    )
    args = parser.parse_args()

    # Decide whether we're resuming an existing CSV or doing a fresh scan.
    appending = False
    existing_symbols: set[str] = set()
    effective_range = args.range

    if (
        args.range is None
        and os.path.exists(args.output)
        and os.path.getsize(args.output) > 0
    ):
        try:
            last_commit, existing_symbols = read_existing_output(args.output)
        except OSError as e:
            print(
                f"Error reading existing output '{args.output}': {e}", file=sys.stderr
            )
            sys.exit(1)

        if last_commit:
            effective_range = f"{last_commit}..HEAD"
            appending = True
            print(
                f"Resuming: found existing '{args.output}', scanning {effective_range}",
                file=sys.stderr,
            )
        else:
            print(
                f"'{args.output}' exists but has no data; doing a full scan.",
                file=sys.stderr,
            )
    elif effective_range is not None:
        if effective_range.startswith(".."):
            effective_range = get_branch_root_commit(args.repo) + effective_range
        if effective_range.endswith(".."):
            effective_range += "HEAD"

    try:
        log_text = get_log_text(args.repo, effective_range, args.path)
    except RuntimeError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    results = parse_log(log_text)
    new_results: dict[str, list[str]] = defaultdict(list)
    for name, commit in results.items():
        if name not in existing_symbols:
            new_results[commit].append(name)

    if not new_results:
        print("No new '#define XKB_KEY_*' found.", file=sys.stderr)

    entries: dict[str, dict[str, Any]] = {}
    for commit, names in new_results.items():
        version = get_lib_version(args.repo, commit, effective_range)
        if not version:
            print(
                f"WARNING: cannot find tag for commit {commit}. Skip names: {','.join(names)}",
                file=sys.stderr,
            )
            continue
        entries[commit] = {"version": version, "names": names}

    if entries:
        file_mode = "at" if appending else "wt"
        with args.output.open(file_mode, encoding="utf-8") as f:
            for k, (commit, entry) in enumerate(entries.items()):
                if k > 0 or (k == 0 and existing_symbols):
                    f.write("\n")
                f.write(f"[{commit}]\n")
                version = entry["version"]
                version1 = Version("1.0.0")
                if version < version1:
                    # HACK
                    comment = f" # real: {version}"
                    version = version1
                else:
                    comment = ""
                f.write(f'version = "{version}"{comment}\n')
                f.write("names = [")
                names = list(f'"{n}"' for n in entry["names"])
                if len(names) > 1:
                    f.write(f"\n\t{',\n\t'.join(names)}\n")
                else:
                    f.write(", ".join(names))
                f.write("]\n")
        count = sum(len(e["names"]) for e in entries.values())

        verb = "Appended" if appending else "Wrote"
        print(f"{verb} {count} keysym name(s) to {args.output}", file=sys.stderr)
    else:
        print("No keysym names to add", file=sys.stderr)


if __name__ == "__main__":
    main()
