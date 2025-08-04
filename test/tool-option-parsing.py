#!/usr/bin/env python3
#
# Copyright © 2020 Red Hat, Inc.
# SPDX-License-Identifier: MIT

import concurrent.futures
import itertools
import logging
import os
import resource
import subprocess
import sys
import tempfile
import unittest
from abc import ABCMeta, abstractmethod
from collections.abc import Callable, Generator
from dataclasses import dataclass
from functools import reduce
from pathlib import Path
from typing import ClassVar

try:
    top_builddir = Path(os.environ["top_builddir"])
    top_srcdir = Path(os.environ["top_srcdir"])
except KeyError:
    print(
        "Required environment variables not found: top_srcdir/top_builddir",
        file=sys.stderr,
    )

    top_srcdir = Path(".")
    try:
        top_builddir = next(Path(".").glob("**/meson-logs/")).parent
    except StopIteration:
        sys.exit(1)
    print(
        'Using srcdir "{}", builddir "{}"'.format(top_srcdir, top_builddir),
        file=sys.stderr,
    )

TIMEOUT = 10.0  # seconds

# Unset some environment variables, so that testing --enable-environment-names
# does not actually depend on the current environment.
for key in (
    "XKB_DEFAULT_RULES",
    "XKB_DEFAULT_MODEL",
    "XKB_DEFAULT_LAYOUT",
    "XKB_DEFAULT_VARIANT",
    "XKB_DEFAULT_OPTIONS",
):
    if key in os.environ:
        del os.environ[key]

# Ensure locale is C, so we can check error messages in English
os.environ["LC_ALL"] = "C.UTF-8"

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger("test")
logger.setLevel(logging.DEBUG)


def powerset(iterable):
    "Subsequences of the iterable from shortest to longest."
    # powerset([1,2,3]) → () (1,) (2,) (3,) (1,2) (1,3) (2,3) (1,2,3)
    s = tuple(iterable)
    return itertools.chain.from_iterable(
        itertools.combinations(s, r) for r in range(len(s) + 1)
    )


# Permutation of RMLVO that we use in multiple tests
rmlvo_options = (
    "--rules=evdev",
    "--model=pc104",
    "--layout=ch",
    "--options=eurosign:5",
    "--enable-environment-names",
)
rmlvos = tuple(
    map(
        list,
        itertools.chain(
            itertools.permutations(rmlvo_options[:-1]),
            powerset(rmlvo_options),
        ),
    )
)


@dataclass
class RMLVO:
    rules: str | None = None
    model: str | None = None
    layout: str | None = None
    variant: str | None = None
    options: str | None = None
    env: bool = False

    def __iter__(self) -> Generator[tuple[str, ...]]:
        if self.rules is not None:
            yield "--rules", self.rules
        if self.model is not None:
            yield "--model", self.model
        if self.layout is not None:
            yield "--layout", self.layout
        if self.variant is not None:
            yield "--variant", self.variant
        if self.options is not None:
            yield "--options", self.options
        if self.env:
            yield ("--enable-environment-names",)

    @property
    def args(self) -> list[str]:
        return list(itertools.chain.from_iterable(self))


class Target(metaclass=ABCMeta):
    @property
    @abstractmethod
    def args(self) -> list[str]: ...

    @property
    def stdin(self) -> str | None:
        return None


class RmlvoTarget(Target):
    @property
    def args(self) -> list[str]:
        return ["--rmlvo"]


class KccgstTarget(Target):
    @property
    def args(self) -> list[str]:
        return ["--kccgst"]


@dataclass
class KeymapTarget(Target, RMLVO):
    arg: bool = False
    path: Path | None = None
    stdin: str | None = None

    def _args(self) -> Generator[str]:
        yield from itertools.chain.from_iterable(self)
        if self.arg:
            yield "--keymap"
        if self.path:
            yield str(self.path)

    @property
    def args(self) -> list[str]:
        return list(self._args())

    @property
    def use_rmlvo(self) -> bool:
        return (
            not self.arg
            and not self.path
            and (
                bool(self.rules)
                or bool(self.model)
                or bool(self.layout)
                or bool(self.variant)
                or bool(self.options)
                or bool(self.env)
                or not self.stdin
            )
        )


def _disable_coredump():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def run_command(args, input: str | None = None) -> tuple[int, str, str]:
    logger.debug("run command: {}".format(" ".join(args)))

    try:
        p = subprocess.run(
            args,
            preexec_fn=_disable_coredump,
            capture_output=True,
            input=input,
            text=True,
            timeout=0.7,
        )
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired as e:
        return (
            0,
            e.stdout.decode("utf-8") if e.stdout else "",
            e.stderr.decode("utf-8") if e.stderr else "",
        )


@dataclass
class XkbcliTool:
    xkbcli_tool: ClassVar[str] = "xkbcli"
    tool_path: ClassVar[Path] = top_builddir

    subtool: str | None = None
    skipIf: tuple[tuple[bool, str], ...] = ()
    skipError: tuple[tuple[Callable[[int, str, str], bool], str], ...] = ()

    def run_command(self, args, input: str | None = None) -> tuple[int, str, str]:
        for condition, reason in self.skipIf:
            if condition:
                raise unittest.SkipTest(reason)
        if self.subtool is not None:
            tool = "{}-{}".format(self.xkbcli_tool, self.subtool)
        else:
            tool = self.xkbcli_tool
        args = [os.path.join(self.tool_path, tool)] + args

        return run_command(args, input=input)

    def run_command_success(self, args, input: str | None = None) -> tuple[str, str]:
        rc, stdout, stderr = self.run_command(args, input=input)
        if rc != 0:
            for testfunc, reason in self.skipError:
                if testfunc(rc, stdout, stderr):
                    raise unittest.SkipTest(reason)
        assert rc == 0, (rc, stdout, stderr)
        return stdout, stderr

    def run_command_invalid(
        self, args, input: str | None = None
    ) -> tuple[int, str, str]:
        rc, stdout, stderr = self.run_command(args, input=input)
        assert rc == 2, (rc, stdout, stderr)
        return rc, stdout, stderr

    def run_command_unrecognized_option(self, args):
        rc, stdout, stderr = self.run_command(args)
        assert rc == 2, (rc, stdout, stderr)
        assert stdout.startswith("Usage") or stdout == ""
        assert "unrecognized option" in stderr

    def run_command_missing_arg(self, args):
        rc, stdout, stderr = self.run_command(args)
        assert rc == 2, (rc, stdout, stderr)
        assert stdout.startswith("Usage") or stdout == ""
        assert "requires an argument" in stderr

    def __str__(self):
        return str(self.subtool)


class TestXkbcli(unittest.TestCase):
    xkbcli: ClassVar[XkbcliTool]
    xkbcli_list: ClassVar[XkbcliTool]
    xkbcli_how_to_type: ClassVar[XkbcliTool]
    xkbcli_compile_keymap: ClassVar[XkbcliTool]
    xkbcli_compile_compose: ClassVar[XkbcliTool]
    xkbcli_interactive_evdev: ClassVar[XkbcliTool]
    xkbcli_interactive_x11: ClassVar[XkbcliTool]
    xkbcli_interactive_wayland: ClassVar[XkbcliTool]
    xkbcli_interactive: ClassVar[XkbcliTool]
    xkbcli_dump_keymap_x11: ClassVar[XkbcliTool]
    xkbcli_dump_keymap_wayland: ClassVar[XkbcliTool]
    xkbcli_dump_keymap: ClassVar[XkbcliTool]
    all_tools: ClassVar[list[XkbcliTool]]

    @classmethod
    def setUpClass(cls):
        cls.xkbcli = XkbcliTool()
        cls.xkbcli_list = XkbcliTool(
            "list",
            skipIf=(
                (
                    not int(os.getenv("HAVE_XKBCLI_LIST", "1")),
                    "xkbregistory not enabled",
                ),
            ),
        )
        cls.xkbcli_how_to_type = XkbcliTool("how-to-type")
        cls.xkbcli_compile_keymap = XkbcliTool("compile-keymap")
        cls.xkbcli_compile_compose = XkbcliTool("compile-compose")
        no_interactive_evdev = (
            (
                not int(os.getenv("HAVE_XKBCLI_INTERACTIVE_EVDEV", "1")),
                "evdev not enabled",
            ),
            (not os.path.exists("/dev/input/event0"), "event node required"),
            (
                not os.access("/dev/input/event0", os.R_OK),
                "insufficient permissions",
            ),
        )
        cls.xkbcli_interactive_evdev = XkbcliTool(
            "interactive-evdev",
            skipIf=no_interactive_evdev,
            skipError=(
                (
                    lambda rc, stdout, stderr: "Couldn't find any keyboards" in stderr,
                    "No keyboards available",
                ),
            ),
        )
        no_interactive_x11 = (
            (
                not int(os.getenv("HAVE_XKBCLI_INTERACTIVE_X11", "1")),
                "x11 not enabled",
            ),
            (not os.getenv("DISPLAY"), "DISPLAY not set"),
        )
        cls.xkbcli_interactive_x11 = XkbcliTool(
            "interactive-x11",
            skipIf=no_interactive_x11,
        )
        no_interactive_wayland = (
            (
                not int(os.getenv("HAVE_XKBCLI_INTERACTIVE_WAYLAND", "1")),
                "wayland not enabled",
            ),
            (not os.getenv("WAYLAND_DISPLAY"), "WAYLAND_DISPLAY not set"),
        )
        cls.xkbcli_interactive_wayland = XkbcliTool(
            "interactive-wayland",
            skipIf=no_interactive_wayland,
        )

        def has_no_backend(*skipped_backends):
            return all(
                reduce(lambda acc, cond: acc or cond[0], skipped_backend, False)
                for skipped_backend in skipped_backends
            )

        cls.xkbcli_interactive = XkbcliTool(
            "interactive",
            skipIf=(
                (
                    has_no_backend(
                        no_interactive_wayland, no_interactive_x11, no_interactive_evdev
                    ),
                    "no available backend",
                ),
            ),
        )
        cls.xkbcli_dump_keymap_x11 = XkbcliTool(
            "dump-keymap-x11",
            skipIf=no_interactive_x11,
        )
        cls.xkbcli_dump_keymap_wayland = XkbcliTool(
            "dump-keymap-wayland",
            skipIf=no_interactive_wayland,
        )
        cls.xkbcli_dump_keymap = XkbcliTool(
            "dump-keymap",
            skipIf=(
                (
                    has_no_backend(no_interactive_wayland, no_interactive_x11),
                    "no available backend",
                ),
            ),
        )
        cls.all_tools = [
            cls.xkbcli,
            cls.xkbcli_list,
            cls.xkbcli_how_to_type,
            cls.xkbcli_compile_keymap,
            cls.xkbcli_compile_compose,
            cls.xkbcli_interactive_evdev,
            cls.xkbcli_interactive_x11,
            cls.xkbcli_interactive_wayland,
            cls.xkbcli_interactive,
            cls.xkbcli_dump_keymap_x11,
            cls.xkbcli_dump_keymap_wayland,
            cls.xkbcli_dump_keymap,
        ]

    def test_help(self):
        # --help is supported by all tools
        for tool in self.all_tools:
            with self.subTest(tool=tool):
                stdout, stderr = tool.run_command_success(["--help"])
                assert stdout.startswith("Usage:")
                assert stderr == ""

    def test_invalid_option(self):
        # --foobar generates "Usage:" for all tools
        for tool in self.all_tools:
            with self.subTest(tool=tool):
                tool.run_command_unrecognized_option(["--foobar"])

    def test_xkbcli_version(self):
        # xkbcli --version
        stdout, stderr = self.xkbcli.run_command_success(["--version"])
        assert stdout.startswith("1")
        assert stderr == ""

    def test_xkbcli_too_many_args(self):
        self.xkbcli.run_command_invalid(["a"] * 64)

    def test_compile_keymap_args(self):
        xkb_root = Path(os.environ["XKB_CONFIG_ROOT"])
        keymap_path = xkb_root / "keymaps/masks.xkb"
        keymap_string = keymap_path.read_text(encoding="utf-8")
        test_option = "--test"
        target: Target
        for target in (
            RmlvoTarget(),
            KccgstTarget(),
            # Keymap from RMLVO
            KeymapTarget(),
            # Keymap from RMLVO (stdin ignored)
            KeymapTarget(layout="us", stdin=keymap_string),
            KeymapTarget(env=True, stdin=keymap_string),
            # Keymap parsed from keymap file
            KeymapTarget(arg=False, path=keymap_path),
            KeymapTarget(arg=True, path=keymap_path),
            # Keymap parsed from stdin
            KeymapTarget(arg=False, stdin=keymap_string),
            KeymapTarget(arg=True, stdin=keymap_string),
        ):
            for options in powerset(("--verbose", test_option)):
                args = list(options) + target.args
                with self.subTest(args=args):
                    stdout, stderr = self.xkbcli_compile_keymap.run_command_success(
                        args, input=target.stdin
                    )
                    if test_option in options:
                        # Expect no output
                        assert stdout == "", (target, stdout)
                    if isinstance(target, KeymapTarget) and test_option not in options:
                        # Check keymap output for excepted bits:
                        # - <AB01> is not defined in mask.xkb
                        # - virtual_modifiers Test01 is only defined in masks.xkb
                        assert ("<AB01>" in stdout) ^ (not target.use_rmlvo), (
                            target,
                            stdout,
                            stderr,
                        )
                        assert ("virtual_modifiers Test01" in stdout) ^ (
                            target.use_rmlvo
                        ), (target, stdout, stderr)

    def test_compile_keymap_mutually_exclusive_args(self):
        xkb_root = Path(os.environ["XKB_CONFIG_ROOT"])
        keymap_path = xkb_root / "keymaps/basic.xkb"
        keymap_string = keymap_path.read_text(encoding="utf-8")
        keymap_from_stdin = KeymapTarget(arg=True, stdin=keymap_string)
        keymap_from_path1 = KeymapTarget(arg=True, path=Path(keymap_path))
        keymap_from_path2 = KeymapTarget(arg=False, path=Path(keymap_path))
        rmlvo = RmlvoTarget()
        kccgst = KccgstTarget()
        for entry in (
            # --keymap does not use RMLVO options
            ("--rules", "some-rules", keymap_from_stdin),
            ("--model", "some-model", keymap_from_stdin),
            ("--layout", "some-layout", keymap_from_stdin),
            ("--variant", "some-variant", keymap_from_stdin),
            ("--options", "some-option", keymap_from_stdin),
            ("--rules", "some-rules", keymap_from_path1),
            ("--model", "some-model", keymap_from_path1),
            ("--layout", "some-layout", keymap_from_path1),
            ("--variant", "some-variant", keymap_from_path1),
            ("--options", "some-option", keymap_from_path1),
            # Trailing keymap file with RMLVO options
            ("--rules", "some-rules", keymap_from_path2),
            ("--model", "some-model", keymap_from_path2),
            ("--layout", "some-layout", keymap_from_path2),
            ("--variant", "some-variant", keymap_from_path2),
            ("--options", "some-option", keymap_from_path2),
            # Incompatible output types
            (rmlvo, keymap_from_stdin),
            (rmlvo, keymap_from_path1),
            (rmlvo, keymap_from_path2),
            (kccgst, keymap_from_stdin),
            (kccgst, keymap_from_path1),
            (kccgst, keymap_from_path2),
            (kccgst, rmlvo),
            (kccgst, rmlvo, keymap_from_stdin),
            (kccgst, rmlvo, keymap_from_path1),
            (kccgst, rmlvo, keymap_from_path2),
        ):
            with self.subTest(args=entry):
                args: list[str] = list(
                    itertools.chain.from_iterable(
                        arg.args if isinstance(arg, Target) else arg for arg in entry
                    )
                )
                input: str | None = reduce(
                    lambda acc, arg: acc
                    or (arg.stdin if isinstance(arg, Target) else None),
                    args,
                    None,
                )
                self.xkbcli_compile_keymap.run_command_invalid(args, input=input)

    def test_compile_keymap_rmlvo(self):
        def run(target, rmlvo):
            return self.xkbcli_compile_keymap.run_command_success(target + rmlvo)

        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures = {
                executor.submit(run, target.args, rmlvo): (target, rmlvo)
                for target in (RmlvoTarget(), KccgstTarget(), KeymapTarget())
                for rmlvo in rmlvos
            }
            for future in concurrent.futures.as_completed(futures, TIMEOUT):
                target, rmlvo = futures[future]
                with self.subTest(target=target, rmlvo=rmlvo):
                    future.result()

    def test_compile_keymap_include(self):
        for args in (
            ["--include", ".", "--include-defaults"],
            ["--include", "/tmp", "--include-defaults"],
        ):
            with self.subTest(args=args):
                # Succeeds thanks to include-defaults
                self.xkbcli_compile_keymap.run_command_success(args)

    def test_compile_keymap_include_invalid(self):
        # A non-directory is rejected by default
        args = ["--include", "/proc/version"]
        rc, stdout, stderr = self.xkbcli_compile_keymap.run_command(args)
        assert rc == 1, (stdout, stderr)
        assert "There are no include paths to search" in stderr

        # A non-existing directory is rejected by default
        args = ["--include", "/tmp/does/not/exist"]
        rc, stdout, stderr = self.xkbcli_compile_keymap.run_command(args)
        assert rc == 1, (stdout, stderr)
        assert "There are no include paths to search" in stderr

        # Valid dir, but missing files
        args = ["--include", "/tmp"]
        rc, stdout, stderr = self.xkbcli_compile_keymap.run_command(args)
        assert rc == 1, (stdout, stderr)
        assert "Couldn't look up rules" in stderr

    def test_compile_compose(self):
        for args in (["--verbose"],):
            with self.subTest(args=args):
                self.xkbcli_compile_compose.run_command_success(args)

    def test_how_to_type(self):
        for args in (["--verbose", "1"],):
            with self.subTest(args=args):
                self.xkbcli_how_to_type.run_command_success(args)

        @dataclass
        class Entry:
            args: list[str]
            name: str
            value: int

        for entry in (
            # Unicode codepoint conversions, we support whatever strtol does
            Entry(args=["123"], name="braceleft", value=0x007B),
            Entry(args=["0123"], name="braceleft", value=0x007B),
            Entry(args=["0a"], name="Linefeed", value=0xFF0A),
            Entry(args=["0x123"], name="gcedilla", value=0x03BB),
            Entry(args=["U+123"], name="gcedilla", value=0x03BB),
            # Characters
            Entry(args=["1"], name="1", value=0x0031),
            Entry(args=["a"], name="a", value=0x0061),
            Entry(args=["á"], name="aacute", value=0x00E1),
            # Keysyms names (fallback without --keysym option)
            Entry(args=["acute"], name="acute", value=0x00B4),
            Entry(args=["U123"], name="U0123", value=0x1000123),
            # Keysyms names (with --keysym)
            Entry(args=["--keysym", "1"], name="1", value=0x0031),
            Entry(args=["--keysym", "a"], name="a", value=0x0061),
            Entry(args=["--keysym", "acute"], name="acute", value=0x00B4),
            Entry(args=["--keysym", "U123"], name="U0123", value=0x1000123),
            # Keysym values
            Entry(args=["--keysym", "123"], name="braceleft", value=0x007B),
            Entry(args=["--keysym", "0x123"], name="0x00000123", value=0x0123),
        ):
            with self.subTest(args=args):
                stdout, _stderr = self.xkbcli_how_to_type.run_command_success(
                    entry.args
                )
                expected = f"keysym: {entry.name} (0x{entry.value:04x})"
                lines = stdout.splitlines()
                assert len(lines) >= 1
                assert lines[0] == expected, (
                    entry,
                    f'expected: "{expected}", but got: "{lines[0]}"',
                )

    def test_how_to_type_rmlvo(self):
        def run(rmlvo):
            args = rmlvo + ["0x1234"]
            return self.xkbcli_how_to_type.run_command_success(args)

        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures = {executor.submit(run, rmlvo): rmlvo for rmlvo in rmlvos}
            for future in concurrent.futures.as_completed(futures, TIMEOUT):
                rmlvo = futures[future]
                with self.subTest(rmlvo=rmlvo):
                    future.result()

    def test_list_rmlvo(self):
        for args in (
            ["--verbose"],
            ["-v"],
            ["--verbose", "--load-exotic"],
            ["--load-exotic"],
            ["--ruleset=evdev"],
            ["--ruleset=base"],
        ):
            with self.subTest(args=args):
                self.xkbcli_list.run_command_success(args)

    def test_list_rmlvo_includes(self):
        args = ["/tmp/"]
        self.xkbcli_list.run_command_success(args)

    def test_list_rmlvo_includes_invalid(self):
        args = ["/proc/version"]
        rc, stdout, stderr = self.xkbcli_list.run_command(args)
        assert rc == 1
        assert "Failed to append include path" in stderr

    def test_list_rmlvo_includes_no_defaults(self):
        args = ["--skip-default-paths", "/tmp"]
        rc, stdout, stderr = self.xkbcli_list.run_command(args)
        assert rc == 1
        assert "Failed to parse XKB description" in stderr

    def test_interactive_evdev_rmlvo(self):
        def run(rmlvo):
            return self.xkbcli_interactive_evdev.run_command_success(rmlvo)

        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures = {executor.submit(run, rmlvo): rmlvo for rmlvo in rmlvos}
            for future in concurrent.futures.as_completed(futures, TIMEOUT):
                rmlvo = futures[future]
                with self.subTest(rmlvo=rmlvo):
                    future.result()

    def test_interactive_evdev(self):
        # Note: --enable-compose fails if $prefix doesn't have the compose tables
        # installed
        for args in (
            ["--verbose"],
            ["--uniline"],
            ["--multiline"],
            ["--report-state-changes"],
            ["--enable-compose"],
            ["--consumed-mode=xkb"],
            ["--consumed-mode=gtk"],
            ["--without-x11-offset"],
        ):
            with self.subTest(args=args):
                self.xkbcli_interactive_evdev.run_command_success(args)

    def test_interactive_x11(self):
        for args in (
            ["--verbose"],
            ["--uniline"],
            ["--multiline"],
        ):
            with self.subTest(args=args):
                self.xkbcli_interactive_x11.run_command_success(args)

    def test_interactive_wayland(self):
        for args in (
            ["--verbose"],
            ["--uniline"],
            ["--multiline"],
        ):
            with self.subTest(args=args):
                self.xkbcli_interactive_wayland.run_command_success(args)


if __name__ == "__main__":
    with tempfile.TemporaryDirectory() as tmpdir:
        # Use our own test xkeyboard-config copy.
        os.environ["XKB_CONFIG_ROOT"] = str(top_srcdir / "test/data")
        # Use our own X11 locale copy.
        os.environ["XLOCALEDIR"] = str(top_srcdir / "test/data/locale")
        # Use our own locale.
        os.environ["LC_CTYPE"] = "en_US.UTF-8"
        # libxkbcommon has fallbacks when XDG_CONFIG_HOME isn't set so we need
        # to override it with a known (empty) directory. Otherwise our test
        # behavior depends on the system the test is run on.
        os.environ["XDG_CONFIG_HOME"] = tmpdir
        # Prevent the legacy $HOME/.xkb from kicking in.
        del os.environ["HOME"]
        # This needs to be separated if we do specific extra path testing
        os.environ["XKB_CONFIG_EXTRA_PATH"] = tmpdir

        unittest.main()
