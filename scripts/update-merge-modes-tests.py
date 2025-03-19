#!/usr/bin/env python3

# Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
# SPDX-License-Identifier: MIT

"""
This script generate tests for merge modes.
"""

from __future__ import annotations

from abc import ABCMeta, abstractmethod
import argparse
from collections import defaultdict
from collections.abc import Callable, Generator, Iterable, Iterator, Sequence
import dataclasses
import itertools
import re
import textwrap
from dataclasses import dataclass
from enum import Flag, IntFlag, StrEnum, auto, unique
from pathlib import Path
from string import Template
from typing import Any, ClassVar, NewType, Self

import jinja2

SCRIPT = Path(__file__)
# Root of the project
ROOT = SCRIPT.parent.parent


################################################################################
#
# Merge modes
#
################################################################################


@unique
class MergeMode(StrEnum):
    Default = auto()
    Augment = auto()
    Override = auto()
    Replace = auto()

    @property
    def include(self) -> str:
        if self is self.__class__.Default:
            return "include"
        else:
            return self

    @property
    def char(self) -> str:
        match self:
            case self.__class__.Default:
                return "+"
            case self.__class__.Augment:
                return "|"
            case self.__class__.Override:
                return "+"
            case self.__class__.Replace:
                return "^"
            case _:
                raise ValueError(self)


INDENT = "\t"


################################################################################
#
# Components
#
################################################################################


@unique
class Component(StrEnum):
    keycodes = auto()
    types = auto()
    compat = auto()
    symbols = auto()

    def render_keymap(self, content) -> Generator[str]:
        yield "xkb_keymap {"
        for component in self.__class__:
            yield from (
                textwrap.indent(t, INDENT)
                for t in self.render(component, content if component is self else "")
            )
        yield "};"

    @classmethod
    def render(cls, component: Self, content: str, name: str = "") -> Generator[str]:
        if not content:
            match component:
                case cls.keycodes:
                    pass
                case cls.types:
                    pass
                case cls.compat:
                    content = "interpret.repeat= True;"
                case cls.symbols:
                    pass
        if name:
            name = f' "{name}"'
        if content:
            yield f'xkb_{component}{name} "" {{'
            yield textwrap.indent(content, INDENT)
            yield "};"
        else:
            yield f'xkb_{component}{name} "" {{}};'


class ComponentTemplate(Template):
    def __init__(self, template) -> Self:
        super().__init__(textwrap.dedent(template).strip())

    def __bool__(self) -> bool:
        return bool(self.template)

    def render(self, merge: MergeMode) -> str:
        mode = "" if merge is MergeMode.Default else f"{merge} "
        return self.substitute(mode=mode)

    def __add__(self, other: Any) -> Self:
        if isinstance(other, str):
            return self.__class__(self.template + "\n" + other)
        elif isinstance(other, Template):
            return self.__class__(self.template + "\n" + other.template)
        else:
            return NotImplemented

    def __radd__(self, other: Any) -> Self:
        if isinstance(other, str):
            return self.__class__(other + "\n" + self.template)
        elif isinstance(other, Template):
            return self.__class__(other.template + "\n" + self.template)
        else:
            return NotImplemented


################################################################################
#
# Test templates
#
################################################################################


@dataclass
class Test:
    title: str
    file: Path
    content: str
    expected: str


@dataclass
class TestGroup:
    title: str
    tests: Sequence[Test]

    c_template: ClassVar[Path] = Path("test/merge_modes.c.jinja")

    SLUG_PATTERN: ClassVar[re.Pattern[str]] = re.compile(r"[^-\w()]+")

    @classmethod
    def file(cls, title: str) -> Path:
        return Path(cls.SLUG_PATTERN.sub("-", title.lower().replace(" ", "_")))

    @classmethod
    def _write(
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        path: Path,
        **kwargs,
    ) -> None:
        template_path = root / cls.c_template
        template = jinja_env.get_template(template_path.relative_to(root).as_posix())

        with path.open("wt", encoding="utf-8") as fd:
            fd.writelines(template.generate(script=SCRIPT.relative_to(ROOT), **kwargs))

    @classmethod
    def write_c(
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        path: Path,
        tests: Sequence[TestGroup],
    ) -> None:
        return cls._write(root=root, jinja_env=jinja_env, path=path, tests=tests)

    @classmethod
    def write_data(
        cls,
        root: Path,
        tests: Sequence[TestGroup],
    ) -> None:
        tests_: dict[str, list[Test]] = defaultdict(list)
        for group in tests:
            for test in group.tests:
                file = test.file.parent
                tests_[file].append(test)
        for file, group_ in tests_.items():
            path = root / "test" / "data" / file
            with path.open("wt", encoding="utf-8") as fd:
                fd.write(
                    "// WARNING: This file was auto-generated by: "
                    f"{SCRIPT.relative_to(ROOT)}\n"
                )
                for test in group_:
                    fd.write(test.content)


@dataclass
class TestTemplate(metaclass=ABCMeta):
    title: str

    _plain: ClassVar[str] = "plain"
    _include: ClassVar[str] = "include"
    _base: ClassVar[str] = "base"
    _update: ClassVar[str] = "update"
    _file: ClassVar[str] = "merge_modes"

    def make_title(self, name: str) -> str:
        return f"{self.title}: {name}"

    @classmethod
    def make_file(cls, name: str) -> Path:
        return TestGroup.file(name)

    @abstractmethod
    def generate_tests(self) -> TestGroup: ...

    @abstractmethod
    def generate_data(self) -> TestGroup: ...

    @classmethod
    @abstractmethod
    def write(
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        c_file: Path | None,
        xkb: bool,
        tests: Sequence[Self],
        debug: bool,
    ): ...


@dataclass
class ComponentTestTemplate(TestTemplate):
    component: Component
    # Templates to test
    base_template: ComponentTemplate
    update_template: ComponentTemplate
    # Expected result
    augment: str
    override: str
    replace: str = ""

    def __post_init__(self):
        self.augment = textwrap.dedent(self.augment).strip()
        self.override = textwrap.dedent(self.override).strip()
        if not self.replace:
            self.replace = self.override
        else:
            self.replace = textwrap.dedent(self.replace).strip()

    def expected(self, mode: MergeMode) -> str:
        match mode:
            case MergeMode.Default:
                return self.override
            case MergeMode.Augment:
                return self.augment
            case MergeMode.Override:
                return self.override
            case MergeMode.Replace:
                return self.replace
            case _:
                raise ValueError(mode)

    def _make_section_name(self, type: str, mode: MergeMode) -> str:
        file = self.make_file(f"{self.title}-{mode}" if self.title else mode)
        return f"{type}-{file}"

    def _make_file(self, type: str, mode: MergeMode) -> Path:
        section_name = self._make_section_name(type, mode)
        return Path(self.component) / self._file / section_name

    def render_section(self, content: str, name: str = "") -> str:
        return "\n".join(self.component.render(self.component, content, name))

    def render_keymap(self, content: str) -> str:
        return "\n".join(self.component.render_keymap(content))

    def _generate_tests(self, render: Callable[[str], str]) -> Generator[Test]:
        for base_mode, update_mode in itertools.product(MergeMode, MergeMode):
            # Plain
            content = (
                self.base_template.render(base_mode)
                + ("\n" if self.base_template else "")
                + self.update_template.render(update_mode)
            )
            yield Test(
                title=self.make_title(
                    f"{self._plain} ({base_mode}) - {self._plain} ({update_mode})"
                ),
                file=self.make_file(self.title) / self.make_file(update_mode),
                content=render(content),
                expected=render(self.expected(update_mode)),
            )

        for base_mode, update_mode in itertools.product(
            (MergeMode.Default,), MergeMode
        ):
            # Plain + Include
            file = self._make_file(self._update, update_mode)
            section = file.name
            file = file.parent.name
            for mode in MergeMode:
                yield Test(
                    title=self.make_title(
                        f"{self._plain} ({base_mode}) - {mode.include} ({update_mode})"
                    ),
                    file=self.make_file(self.title) / self.make_file(mode),
                    content=render(
                        self.base_template.render(base_mode)
                        + (
                            f'\n{mode.include} "{file}({section})"'
                            if self.base_template
                            else ""
                        )
                    ),
                    # Include does not leak local merge modes
                    expected=render(self.expected(mode)),
                )

        # for base_mode, update_mode in itertools.product(MergeMode, MergeMode):
        #     # Include + Plain
        #     file = self._make_file(self.base, base_mode)
        #     section = file.name
        #     file = file.parent.name
        #     for mode in (MergeMode.default,):
        #         yield Test(
        #             title=self.make_title(
        #                 f"{mode.include} ({base_mode}) - {self.plain} ({update_mode})"
        #             ),
        #             file=self.make_file(self.title)
        #             / self.make_file(
        #                 f"{mode.include}({base_mode})-{self.plain}({update_mode})"
        #             ),
        #             content=render(
        #                 f'{mode.include} "{file}({section})"\n'
        #                 + self.update_template.render(update_mode)
        #             ),
        #             expected=render(self.expected(update_mode)),
        #         )

        # for base_mode, update_mode in itertools.product((MergeMode.Default,), MergeMode):
        #     # Include + Include
        #     mode = MergeMode.Default
        #     base_file = self._make_file(self.base, mode)
        #     base_section = base_file.name
        #     base_file = base_file.parent.name
        #     update_file = self._make_file(self.update, mode)
        #     update_section = update_file.name
        #     update_file = update_file.parent.name
        #     yield Test(
        #         title=self.make_title(
        #             f"{base_mode.include} ({mode}) - {update_mode.include} ({mode})"
        #         ),
        #         file=self.make_file(self.title)
        #         / self.make_file(
        #             f"{base_mode.include}({mode})-{update_mode.include}({mode})"
        #         ),
        #         content=render(
        #             f'{base_mode.include} "{base_file}({base_section})"\n'
        #             + f'{update_mode.include} "{update_file}({update_section})"'
        #         ),
        #         expected=render(self.expected(update_mode)),
        #     )

        for base_mode, update_mode in itertools.product(MergeMode, MergeMode):
            # Multi-include
            base_file = self._make_file(self._base, base_mode)
            base_section = base_file.name
            base_file = base_file.parent.name
            update_file = self._make_file(self._update, update_mode)
            update_section = update_file.name
            update_file = update_file.parent.name
            for mode in (MergeMode.Override, MergeMode.Augment, MergeMode.Replace):
                yield Test(
                    title=self.make_title(
                        f"{self._include} ({base_mode} {mode.char} {update_mode})"
                    ),
                    file=self.make_file(self.title) / self.make_file(mode),
                    content=render(
                        f'{base_mode.include} "{base_file}({base_section})'
                        + mode.char
                        + f'{update_file}({update_section})"'
                    ),
                    expected=render(self.expected(mode)),
                )

    def _generate_data(self) -> Generator[Test]:
        for type, template in (
            (self._base, self.base_template),
            (self._update, self.update_template),
        ):
            for mode in MergeMode:
                section_name = self._make_section_name(type, mode)
                file = self._make_file(type, mode)
                content = template.render(mode)
                content = self.render_section(content=content, name=section_name) + "\n"
                yield Test(title="", file=file, content=content, expected="")

    def generate_tests(self, as_keymap: bool = False) -> TestGroup:
        if as_keymap:

            def render(content: str) -> str:
                return self.render_keymap(content)
        else:

            def render(content: str) -> str:
                return self.render_section(content)

        return TestGroup(self.title, tuple(self._generate_tests(render)))

    def generate_data(self) -> TestGroup:
        return TestGroup(self.title, tuple(self._generate_data()))

    @classmethod
    def write(
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        c_file: Path | None,
        xkb: bool,
        tests: Sequence[Self],
        debug: bool = False,
    ):
        if c_file is not None and c_file.name:
            c_data = tuple(t.generate_tests(as_keymap=True) for t in tests)
            TestGroup.write_c(root=root, jinja_env=jinja_env, path=c_file, tests=c_data)
        if xkb:
            xkb_data = tuple(t.generate_data() for t in tests)
            TestGroup.write_data(root=root, tests=xkb_data)


KeymapTemplate = Template(
    """\
xkb_keymap {
${keycodes}
${types}
${compat}
${symbols}
};"""
)


@dataclass
class KeymapTestTemplate(TestTemplate):
    keycodes: ComponentTestTemplate | None = None
    types: ComponentTestTemplate | None = None
    compat: ComponentTestTemplate | None = None
    symbols: ComponentTestTemplate | None = None

    def __post_init__(self):
        if self.keycodes:
            self.keycodes = dataclasses.replace(self.keycodes, title="")
        if self.types:
            self.types = dataclasses.replace(self.types, title="")
        if self.compat:
            self.compat = dataclasses.replace(self.compat, title="")
        if self.symbols:
            self.symbols = dataclasses.replace(self.symbols, title="")

    def __iter__(self) -> Generator[ComponentTestTemplate]:
        def make_empty_template(component: Component) -> ComponentTestTemplate:
            return ComponentTestTemplate(
                title="",
                component=component,
                base_template=ComponentTemplate(""),
                update_template=ComponentTemplate(""),
                augment="",
                override="",
            )

        yield self.keycodes or make_empty_template(Component.keycodes)
        yield self.types or make_empty_template(Component.types)
        yield self.compat or make_empty_template(Component.compat)
        yield self.symbols or make_empty_template(Component.symbols)

    def _generate_tests(self) -> Generator[Test]:
        groups = tuple(c.generate_tests(as_keymap=False) for c in self)
        keycodes: Test
        types: Test
        compat: Test
        symbols: Test
        for keycodes, types, compat, symbols in zip(*(g.tests for g in groups)):
            title = self.title + keycodes.title
            file = keycodes.file
            content = KeymapTemplate.substitute(
                keycodes=textwrap.indent(keycodes.content, INDENT),
                types=textwrap.indent(types.content, INDENT),
                compat=textwrap.indent(compat.content, INDENT),
                symbols=textwrap.indent(symbols.content, INDENT),
            )
            expected = KeymapTemplate.substitute(
                keycodes=textwrap.indent(keycodes.expected, INDENT),
                types=textwrap.indent(types.expected, INDENT),
                compat=textwrap.indent(compat.expected, INDENT),
                symbols=textwrap.indent(symbols.expected, INDENT),
            )
            yield Test(title=title, file=file, content=content, expected=expected)

    def generate_tests(self) -> TestGroup:
        return TestGroup(self.title, tuple(self._generate_tests()))

    def _generate_data(self) -> Generator[Test]:
        for component in self:
            yield from component.generate_data().tests

    def generate_data(self) -> TestGroup:
        return TestGroup(self.title, tuple(self._generate_data()))

    @classmethod
    def write(
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        c_file: Path | None,
        xkb: bool,
        tests: Sequence[Self],
        debug: bool = False,
    ):
        if c_file is not None and c_file.name:
            c_data = tuple(t.generate_tests() for t in tests)
            TestGroup.write_c(root=root, jinja_env=jinja_env, path=c_file, tests=c_data)
        if xkb:
            xkb_data = tuple(t.generate_data() for t in tests)
            TestGroup.write_data(root=root, tests=xkb_data)


################################################################################
#
# Key types tests
#
################################################################################

TYPES_TESTS = ComponentTestTemplate(
    "Types",
    component=Component.types,
    base_template=ComponentTemplate(
        """
        ${mode}virtual_modifiers NumLock, LevelThree=none, LevelFive=Mod3;
        ${mode}virtual_modifiers U1, U2, U3;
        // ${mode}virtual_modifiers U4, U5, U6;
        ${mode}virtual_modifiers Z1=none, Z2=none, Z3=none;
        // ${mode}virtual_modifiers Z4=none, Z5=none, Z6=none;
        ${mode}virtual_modifiers M1=0x1000, M2=0x2000, M3=0x3000;
        // ${mode}virtual_modifiers M4=0x4000, M5=0x5000, M6=0x6000;

        ${mode}type "ONE_LEVEL" {
        	modifiers = None;
        	map[None] = Level1;
        	level_name[Level1]= "Any";
        };

        ${mode}type "TWO_LEVEL" {
        	modifiers = Shift;
        	map[Shift] = Level2;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Shift";
        };

        ${mode}type "ALPHABETIC" {
        	modifiers = Shift+Lock;
        	map[Shift] = Level2;
        	map[Lock] = Level2;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Caps";
        };

        ${mode}type "KEYPAD" {
        	modifiers = Shift+NumLock;
        	map[None] = Level1;
        	map[Shift] = Level2;
        	map[NumLock] = Level2;
        	map[Shift+NumLock] = Level1;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Number";
        };

        ${mode}type "FOUR_LEVEL" {
        	modifiers = Shift+LevelThree;
        	map[None] = Level1;
        	map[Shift] = Level2;
        	map[LevelThree] = Level3;
        	map[Shift+LevelThree] = Level4;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Shift";
        	level_name[Level3] = "Alt Base";
        	level_name[Level4] = "Shift Alt";
        };

        ${mode}type "FOUR_LEVEL_ALPHABETIC" {
        	modifiers = Shift+Lock+LevelThree;
        	map[None] = Level1;
        	map[Shift] = Level2;
        	map[Lock]  = Level2;
        	map[LevelThree] = Level3;
        	map[Shift+LevelThree] = Level4;
        	map[Lock+LevelThree] =  Level4;
        	map[Lock+Shift+LevelThree] =  Level3;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Shift";
        	level_name[Level3] = "Alt Base";
        	level_name[Level4] = "Shift Alt";
        };

        ${mode}type "FOUR_LEVEL_SEMIALPHABETIC" {
        	modifiers = Shift+Lock+LevelThree;
        	map[None] = Level1;
        	map[Shift] = Level2;
        	map[Lock]  = Level2;
        	map[LevelThree] = Level3;
        	map[Shift+LevelThree] = Level4;
        	map[Lock+LevelThree] =  Level3;
        	map[Lock+Shift+LevelThree] = Level4;
        	preserve[Lock+LevelThree] = Lock;
        	preserve[Lock+Shift+LevelThree] = Lock;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Shift";
        	level_name[Level3] = "Alt Base";
        	level_name[Level4] = "Shift Alt";
        };

        ${mode}type "XXX" {
        	modifiers = Shift+Lock+LevelThree;
            map[None]       = 1;
            map[Shift]      = 2;
            map[Lock]       = 2;
            map[LevelThree] = 3;
            level_name[1] = "1";
            level_name[2] = "2";
            level_name[3] = "3";
        };
        """
    ),
    update_template=ComponentTemplate(
        """\
        ${mode}virtual_modifiers NumLock=Mod2;    // Changed: now mapped
        ${mode}virtual_modifiers LevelThree=Mod5; // Changed: altered mapping (from 0)
        ${mode}virtual_modifiers LevelFive=Mod3;  // Unhanged: same mapping

        ${mode}virtual_modifiers U7, Z7=0, M7=0x700000; // Changed: new

        ${mode}virtual_modifiers U1;            // Unchanged (unmapped)
        ${mode}virtual_modifiers U2 = none;     // Changed: now mapped
        ${mode}virtual_modifiers U3 = 0x300000; // Changed: now mapped
        // ${mode}virtual_modifiers U4;            // Unchanged (unmapped)
        // ${mode}virtual_modifiers U5 = none;     // Changed: now mapped
        // ${mode}virtual_modifiers U6 = 0x600000; // Changed: now mapped
        ${mode}virtual_modifiers Z1;            // Unchanged (= 0)
        ${mode}virtual_modifiers Z2 = none;     // Unchanged (same mapping)
        ${mode}virtual_modifiers Z3 = 0x310000; // Changed: altered mapping (from 0)
        // ${mode}virtual_modifiers Z4;            // Unchanged (= 0)
        // ${mode}virtual_modifiers Z5 = none;     // Unchanged (same mapping)
        // ${mode}virtual_modifiers Z6 = 0x610000; // Changed: altered mapping (from 0)
        ${mode}virtual_modifiers M1;            // Unchanged (≠ 0)
        ${mode}virtual_modifiers M2 = none;     // Changed: reset
        ${mode}virtual_modifiers M3 = 0x320000; // Changed: altered mapping (from ≠ 0)
        // ${mode}virtual_modifiers M4;            // Unchanged (≠ 0)
        // ${mode}virtual_modifiers M5 = none;     // Changed: reset
        // ${mode}virtual_modifiers M6 = 0x620000; // Changed: altered mapping (from ≠ 0)

        ${mode}type "ONE_LEVEL" {
        	modifiers = None;
        	map[None] = Level1;
        	level_name[Level1]= "New"; // Change name
        };

        ${mode}type "TWO_LEVEL" {
        	modifiers = Shift+M1; // Changed
        	map[Shift] = Level2;
        	map[M1] = Level2; // Changed
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Shift";
        };

        ${mode}type "ALPHABETIC" {
        	modifiers = Lock; // Changed
        	map[None] = Level2; // Changed
        	map[Lock] = Level1; // Changed
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Caps";
        };

        ${mode}type "KEYPAD" {
        	modifiers = Shift+NumLock;
        	map[None] = Level1;
        	map[Shift] = Level2; // Changed
        	map[NumLock] = Level2;
        	map[Shift+NumLock] = Level1;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Number";
        };

        // Unchanged
        ${mode}type "FOUR_LEVEL" {
        	modifiers = Shift+LevelThree;
        	map[None] = Level1;
        	map[Shift] = Level2;
        	map[LevelThree] = Level3;
        	map[Shift+LevelThree] = Level4;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Shift";
        	level_name[Level3] = "Alt Base";
        	level_name[Level4] = "Shift Alt";
        };

        ${mode}type "FOUR_LEVEL_ALPHABETIC" {
        	modifiers = Shift+Lock+LevelThree;
        	map[None] = Level1;
        	map[Shift] = Level2;
        	map[Lock]  = Level2;
        	map[LevelThree] = Level3;
        	map[Shift+LevelThree] = Level4;
        	map[Lock+LevelThree] =  Level4;
        	map[Lock+Shift+LevelThree] =  Level3;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Shift";
        	level_name[Level3] = "Alt Base";
        	level_name[Level4] = "Shift Alt";
        };

        ${mode}type "FOUR_LEVEL_SEMIALPHABETIC" {
        	modifiers = Shift+Lock+LevelThree;
        	map[None] = Level1;
        	map[Shift] = Level2;
        	map[Lock]  = Level2;
        	map[LevelThree] = Level3;
        	map[Shift+LevelThree] = Level4;
        	map[Lock+LevelThree] =  Level3;
        	map[Lock+Shift+LevelThree] = Level4;
        	preserve[Lock+LevelThree] = Lock;
        	preserve[Lock+Shift+LevelThree] = Lock;
        	level_name[Level1] = "Base";
        	level_name[Level2] = "Shift";
        	level_name[Level3] = "Alt Base";
        	level_name[Level4] = "Shift Alt";
        };

        ${mode}type "XXX" {
        	modifiers = Shift+Lock+LevelThree;
            map[None]       = 1;
            map[Shift]      = 2;
            // map[Lock]       = 2;    // Changed
            // map[LevelThree] = 3;
            map[LevelThree] = 1;       // Changed
            map[Shift+LevelThree] = 4; // Changed
            level_name[1] = "A"; // Changed
            level_name[2] = "2";
            level_name[3] = "3";
            level_name[4] = "4"; // Changed
        };

        // New
        ${mode}type "YYY" {
        	modifiers = None;
        	map[None] = Level1;
        	level_name[Level1]= "New";
        };
        """
    ),
    augment="""
        virtual_modifiers NumLock=Mod2,LevelThree=none,LevelFive=Mod3;

        virtual_modifiers U1,U2=0,U3=0x300000;
        // virtual_modifiers U4,U5=0,U6=0x600000;
        virtual_modifiers Z1=0,Z2=0,Z3=0;
        // virtual_modifiers Z4=0,Z5=0,Z6=0;
        virtual_modifiers M1=0x1000,M2=0x2000,M3=0x3000;
        // virtual_modifiers M4=0x4000,M5=0x5000,M6=0x6000;
        virtual_modifiers U7,Z7=0,M7=0x700000;

        type "ONE_LEVEL" {
        	modifiers= none;
            map[none]= 1;
        	level_name[1]= "Any";
        };
        type "TWO_LEVEL" {
        	modifiers= Shift;
        	map[Shift]= 2;
        	level_name[1]= "Base";
        	level_name[2]= "Shift";
        };
        type "ALPHABETIC" {
        	modifiers= Shift+Lock;
        	map[Shift]= 2;
        	map[Lock]= 2;
        	level_name[1]= "Base";
        	level_name[2]= "Caps";
        };
        type "KEYPAD" {
        	modifiers= Shift+NumLock;
        	map[Shift]= 2;
        	map[NumLock]= 2;
        	level_name[1]= "Base";
        	level_name[2]= "Number";
        };
        type "FOUR_LEVEL" {
        	modifiers= Shift+LevelThree;
            map[none]= 1;
        	map[Shift]= 2;
        	map[LevelThree]= 3;
        	map[Shift+LevelThree]= 4;
        	level_name[1]= "Base";
        	level_name[2]= "Shift";
        	level_name[3]= "Alt Base";
        	level_name[4]= "Shift Alt";
        };
        type "FOUR_LEVEL_ALPHABETIC" {
        	modifiers= Shift+Lock+LevelThree;
            map[none]= 1;
        	map[Shift]= 2;
        	map[Lock]= 2;
        	map[LevelThree]= 3;
        	map[Shift+LevelThree]= 4;
        	map[Lock+LevelThree]= 4;
        	map[Shift+Lock+LevelThree]= 3;
        	level_name[1]= "Base";
        	level_name[2]= "Shift";
        	level_name[3]= "Alt Base";
        	level_name[4]= "Shift Alt";
        };
        type "FOUR_LEVEL_SEMIALPHABETIC" {
        	modifiers= Shift+Lock+LevelThree;
        	map[None]= 1;
        	map[Shift]= 2;
        	map[Lock]= 2;
        	map[LevelThree]= 3;
        	map[Shift+LevelThree]= 4;
        	map[Lock+LevelThree]= 3;
        	preserve[Lock+LevelThree]= Lock;
        	map[Shift+Lock+LevelThree]= 4;
        	preserve[Shift+Lock+LevelThree]= Lock;
        	level_name[1]= "Base";
        	level_name[2]= "Shift";
        	level_name[3]= "Alt Base";
        	level_name[4]= "Shift Alt";
        };
        type "XXX" {
        	modifiers = Shift+Lock+LevelThree;
            map[None]       = 1;
            map[Shift]      = 2;
            map[Lock]       = 2;
            map[LevelThree] = 3;
            level_name[1] = "1";
            level_name[2] = "2";
            level_name[3] = "3";
        };
        type "YYY" {
        	modifiers = None;
        	map[None] = Level1;
        	level_name[Level1]= "New";
        };
        """,
    override="""
        virtual_modifiers NumLock=Mod2,LevelThree=Mod5,LevelFive=Mod3;

        virtual_modifiers U1,U2=0,U3=0x300000;
        // virtual_modifiers U4,U5=0,U6=0x600000;
        virtual_modifiers Z1=0,Z2=0,Z3=0x310000;
        // virtual_modifiers Z4=0,Z5=0,Z6=0x610000;
        virtual_modifiers M1=0x1000,M2=0,M3=0x320000;
        // virtual_modifiers M4=0x4000,M5=0,M6=0x620000;
        virtual_modifiers U7,Z7=0,M7=0x700000;

        type "ONE_LEVEL" {
        	modifiers= none;
        	map[None] = Level1;
        	level_name[1]= "New";
        };
        type "TWO_LEVEL" {
        	modifiers= Shift+M1;
        	map[Shift]= 2;
        	map[M1]= 2;
        	level_name[1]= "Base";
        	level_name[2]= "Shift";
        };
        type "ALPHABETIC" {
        	modifiers= Lock;
        	map[none]= 2;
        	map[Lock]= 1;
        	level_name[1]= "Base";
        	level_name[2]= "Caps";
        };
        type "KEYPAD" {
        	modifiers= Shift+NumLock;
        	map[Shift] = Level2;
        	map[NumLock]= 2;
        	level_name[1]= "Base";
        	level_name[2]= "Number";
        };
        type "FOUR_LEVEL" {
        	modifiers= Shift+LevelThree;
        	map[None]= 1;
        	map[Shift]= 2;
        	map[LevelThree]= 3;
        	map[Shift+LevelThree] = Level4;
        	level_name[1]= "Base";
        	level_name[2]= "Shift";
        	level_name[3]= "Alt Base";
        	level_name[4]= "Shift Alt";
        };
        type "FOUR_LEVEL_ALPHABETIC" {
        	modifiers= Shift+Lock+LevelThree;
        	map[Shift]= 2;
        	map[Lock]= 2;
        	map[LevelThree]= 3;
        	map[Shift+LevelThree]= 4;
        	map[Lock+LevelThree]= 4;
        	map[Shift+Lock+LevelThree]= 3;
        	level_name[1]= "Base";
        	level_name[2]= "Shift";
        	level_name[3]= "Alt Base";
        	level_name[4]= "Shift Alt";
        };
        type "FOUR_LEVEL_SEMIALPHABETIC" {
        	modifiers= Shift+Lock+LevelThree;
        	map[None]= 1;
        	map[Shift]= 2;
        	map[Lock]= 2;
        	map[LevelThree]= 3;
        	map[Shift+LevelThree]= 4;
        	map[Lock+LevelThree]= 3;
        	preserve[Lock+LevelThree]= Lock;
        	map[Shift+Lock+LevelThree]= 4;
        	preserve[Shift+Lock+LevelThree]= Lock;
        	level_name[1]= "Base";
        	level_name[2]= "Shift";
        	level_name[3]= "Alt Base";
        	level_name[4]= "Shift Alt";
        };
        type "XXX" {
        	modifiers = Shift+Lock+LevelThree;
            map[None]       = 1;
            map[Shift]      = 2;
            // map[LevelThree] = 3;
            map[LevelThree] = 1;
            map[Shift+LevelThree] = 4;
            level_name[1] = "A";
            level_name[2] = "2";
            level_name[3] = "3";
            level_name[4] = "4";
        };
        type "YYY" {
        	modifiers = None;
        	map[None] = Level1;
        	level_name[Level1]= "New";
        };
        """,
)


################################################################################
#
# Compat tests
#
################################################################################

COMPAT_TESTS = ComponentTestTemplate(
    "Compat",
    component=Component.compat,
    base_template=ComponentTemplate(
        """
        ${mode}virtual_modifiers NumLock;

        interpret.repeat= False;
        setMods.clearLocks= True;
        latchMods.clearLocks= True;
        latchMods.latchToLock= True;

        ${mode}interpret Any + Any {
        	action= SetMods(modifiers=modMapMods);
        };

        ${mode}interpret Caps_Lock {
        	action = LockMods(modifiers = Lock);
        };

        ${mode}indicator "Caps Lock" {
        	!allowExplicit;
        	whichModState= Locked;
        	modifiers= Lock;
        };

        ${mode}indicator "Num Lock" {
        	!allowExplicit;
        	whichModState= Locked;
        	modifiers= NumLock;
        };
        """
    ),
    update_template=ComponentTemplate(
        """
        ${mode}virtual_modifiers NumLock;

        ${mode}interpret.repeat= False;
        ${mode}setMods.clearLocks= False; // Changed

        // Unchanged
        ${mode}interpret Any + Any {
        	action= SetMods(modifiers=modMapMods);
        };

        // Changed
        ${mode}interpret Caps_Lock {
        	action = LockMods(modifiers = NumLock);
        };

        // Unchanged
        ${mode}indicator "Caps Lock" {
        	!allowExplicit;
        	whichModState= Locked;
        	modifiers= Lock;
        };

        // Changed
        ${mode}indicator "Num Lock" {
        	!allowExplicit;
        	whichModState= Base;
        	modifiers= Lock;
        };

        // New
        ${mode}indicator "Kana" {
        	!allowExplicit;
        	whichModState= Locked;
        	modifiers= Control;
        };
        """
    ),
    augment="""
        virtual_modifiers NumLock;

        interpret.useModMapMods= AnyLevel;
        interpret.repeat= False;
        interpret Caps_Lock+AnyOfOrNone(all) {
        	action= LockMods(modifiers=Lock);
        };
        interpret Any+AnyOf(all) {
        	action= SetMods(modifiers=modMapMods,clearLocks);
        };
        indicator "Caps Lock" {
        	whichModState= locked;
        	modifiers= Lock;
        };
        indicator "Num Lock" {
        	whichModState= locked;
        	modifiers= NumLock;
        };
        indicator "Kana" {
        	whichModState= locked;
        	modifiers= Control;
	    };
        """,
    override="""
        virtual_modifiers NumLock;

        interpret.repeat= False;
        setMods.clearLocks= False;
        interpret Caps_Lock+AnyOfOrNone(all) {
        	action= LockMods(modifiers=NumLock);
        };
        interpret Any+AnyOf(all) {
        	action= SetMods(modifiers=modMapMods);
        };
        indicator "Caps Lock" {
        	whichModState= locked;
        	modifiers= Lock;
        };
        indicator "Kana" {
        	whichModState= locked;
        	modifiers= Control;
        };
        indicator "Num Lock" {
        	whichModState= base;
        	modifiers= Lock;
        };
        """,
)


################################################################################
#
# Symbols tests
#
################################################################################


LINUX_EVENT_CODES = (
    None,
    "ESC",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "MINUS",
    "EQUAL",
    "BACKSPACE",
    "TAB",
    "Q",
    "W",
    "E",
    "R",
    "T",
    "Y",
    "U",
    "I",
    "O",
    "P",
    "LEFTBRACE",
    "RIGHTBRACE",
    "ENTER",
    "LEFTCTRL",
    "A",
    "S",
    "D",
    "F",
    "G",
    "H",
    "J",
    "K",
    "L",
    "SEMICOLON",
    "APOSTROPHE",
    "GRAVE",
    "LEFTSHIFT",
    "BACKSLASH",
    "Z",
    "X",
    "C",
    "V",
    "B",
    "N",
    "M",
    "COMMA",
    "DOT",
    "SLASH",
    "RIGHTSHIFT",
    "KPASTERISK",
    "LEFTALT",
    "SPACE",
    "CAPSLOCK",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "NUMLOCK",
    "SCROLLLOCK",
    "KP7",
    "KP8",
    "KP9",
    "KPMINUS",
    "KP4",
    "KP5",
    "KP6",
    "KPPLUS",
    "KP1",
    "KP2",
    "KP3",
    "KP0",
    "KPDOT",
    None,
    "ZENKAKUHANKAKU",
    "102ND",
    "F11",
    "F12",
    "RO",
    "KATAKANA",
    "HIRAGANA",
    "HENKAN",
    "KATAKANAHIRAGANA",
    "MUHENKAN",
    "KPJPCOMMA",
    "KPENTER",
    "RIGHTCTRL",
    "KPSLASH",
    "SYSRQ",
    "RIGHTALT",
    "LINEFEED",
    "HOME",
    "UP",
    "PAGEUP",
    "LEFT",
    "RIGHT",
    "END",
    "DOWN",
    "PAGEDOWN",
    "INSERT",
    "DELETE",
    "MACRO",
    "MUTE",
    "VOLUMEDOWN",
    "VOLUMEUP",
    "POWER",
    "KPEQUAL",
    "KPPLUSMINUS",
    "PAUSE",
    "SCALE",
    "KPCOMMA",
    "HANGEUL",
    "HANJA",
    "YEN",
    "LEFTMETA",
    "RIGHTMETA",
    "COMPOSE",
    "STOP",
    "AGAIN",
    "PROPS",
    "UNDO",
    "FRONT",
    "COPY",
    "OPEN",
    "PASTE",
    "FIND",
    "CUT",
    "HELP",
    "MENU",
    "CALC",
    "SETUP",
    "SLEEP",
    "WAKEUP",
    "FILE",
    "SENDFILE",
    "DELETEFILE",
    "XFER",
    "PROG1",
    "PROG2",
    "WWW",
    "MSDOS",
    "COFFEE",
    "DIRECTION",
    "CYCLEWINDOWS",
    "MAIL",
    "BOOKMARKS",
    "COMPUTER",
    "BACK",
    "FORWARD",
    "CLOSECD",
    "EJECTCD",
    "EJECTCLOSECD",
    "NEXTSONG",
    "PLAYPAUSE",
    "PREVIOUSSONG",
    "STOPCD",
    "RECORD",
    "REWIND",
    "PHONE",
    "ISO",
    "CONFIG",
    "HOMEPAGE",
    "REFRESH",
    "EXIT",
    "MOVE",
    "EDIT",
    "SCROLLUP",
    "SCROLLDOWN",
    "KPLEFTPAREN",
    "KPRIGHTPAREN",
    "NEW",
    "REDO",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    None,
    None,
    None,
    None,
    None,
    "PLAYCD",
    "PAUSECD",
    "PROG3",
    "PROG4",
    "DASHBOARD",
    "SUSPEND",
    "CLOSE",
    "PLAY",
    "FASTFORWARD",
    "BASSBOOST",
    "PRINT",
    "HP",
    "CAMERA",
    "SOUND",
    "QUESTION",
)

Comment = NewType("Comment", str)


@dataclass(frozen=True)
class KeyCode:
    _evdev: str
    _xkb: str

    @property
    def c(self) -> str:
        return f"KEY_{self._evdev}"

    @property
    def xkb(self) -> str:
        return f"<{self._xkb}>"


class Keysym(str):
    @property
    def c(self) -> str:
        return f"XKB_KEY_{self}"

    @classmethod
    def parse(cls, raw: str | None) -> Self:
        if not raw:
            return cls("NoSymbol")
        else:
            return cls(raw)


NoSymbol = Keysym("NoSymbol")


class Modifier(IntFlag):
    NoModifier = 0
    Shift = 1 << 0
    Lock = 1 << 1
    Control = 1 << 2
    Mod1 = 1 << 3
    Mod2 = 1 << 4
    Mod3 = 1 << 5
    Mod4 = 1 << 6
    Mod5 = 1 << 7
    LevelThree = Mod5

    def __iter__(self) -> Iterator[Self]:
        for m in self.__class__:
            if m & self:
                yield m

    def __str__(self) -> str:
        return "+".join(m.name for m in self)


class Action(metaclass=ABCMeta):
    @abstractmethod
    def __bool__(self) -> bool: ...

    @classmethod
    def parse(cls, raw: Any) -> Self:
        if raw is None:
            return GroupAction.parse(None)
        elif isinstance(raw, Modifier):
            return ModAction.parse(raw)
        elif isinstance(raw, int):
            return GroupAction.parse(raw)
        else:
            raise ValueError(raw)

    @abstractmethod
    def action_to_keysym(self, index: int, level: int) -> Keysym: ...


@dataclass
class GroupAction(Action):
    """
    SetGroup or NoAction
    """

    group: int
    keysyms: ClassVar[dict[tuple[int, int, int], Keysym]] = {
        (2, 0, 0): Keysym("a"),
        (2, 0, 1): Keysym("A"),
        (2, 1, 0): Keysym("b"),
        (2, 1, 1): Keysym("B"),
        (3, 0, 0): Keysym("Greek_alpha"),
        (3, 0, 1): Keysym("Greek_ALPHA"),
        (3, 1, 0): Keysym("Greek_beta"),
        (3, 1, 1): Keysym("Greek_BETA"),
    }

    def __str__(self) -> str:
        if self.group > 0:
            return f"SetGroup(group={self.group})"
        else:
            return "NoAction()"

    def __bool__(self) -> bool:
        return bool(self.group)

    @classmethod
    def parse(cls, raw: int | None) -> Self:
        if not raw:
            return cls(0)
        else:
            return cls(raw)

    def action_to_keysym(self, index: int, level: int) -> Keysym:
        if not self.group:
            return NoSymbol
        else:
            if (
                keysym := self.keysyms.get((self.group % 4, index % 2, level % 2))
            ) is None:
                raise ValueError((self, index, level))
            return keysym


@dataclass
class ModAction(Action):
    """
    SetMod or NoAction
    """

    mods: Modifier
    keysyms: ClassVar[dict[tuple[Modifier, int, int], Keysym]] = {
        (Modifier.Control, 0, 0): Keysym("x"),
        (Modifier.Control, 0, 1): Keysym("X"),
        (Modifier.Control, 1, 0): Keysym("y"),
        (Modifier.Control, 1, 1): Keysym("Y"),
        (Modifier.Mod5, 0, 0): Keysym("Greek_xi"),
        (Modifier.Mod5, 0, 1): Keysym("Greek_XI"),
        (Modifier.Mod5, 1, 0): Keysym("Greek_upsilon"),
        (Modifier.Mod5, 1, 1): Keysym("Greek_UPSILON"),
    }

    def __str__(self) -> str:
        if self.mods is Modifier.NoModifier:
            return "NoAction()"
        else:
            return f"SetMods(mods={self.mods})"

    def __bool__(self) -> bool:
        return self.mods is Modifier.NoModifier

    @classmethod
    def parse(cls, raw: Modifier | None) -> Self:
        if not raw:
            return cls(Modifier.NoModifier)
        else:
            return cls(raw)

    def action_to_keysym(self, index: int, level: int) -> Keysym:
        if self.mods is Modifier.NoModifier:
            return NoSymbol
        else:
            if (keysym := self.keysyms.get((self.mods, index % 2, level % 2))) is None:
                raise ValueError((self, index, level))
            return keysym


@dataclass
class Level:
    keysyms: tuple[Keysym, ...]
    actions: tuple[Action, ...]

    @staticmethod
    def _c(default: str, xs: Iterable[Any]) -> str:
        match len(xs):
            case 0:
                return default
            case 1:
                return xs[0].c
            case _:
                return ", ".join(map(lambda x: x.c, xs))

    @staticmethod
    def _xkb(default: str, xs: Iterable[Any]) -> str:
        match len(xs):
            case 0:
                return default
            case 1:
                return str(xs[0])
            case _:
                return "{" + ", ".join(map(str, xs)) + "}"

    @classmethod
    def has_empty_symbols(cls, keysyms: tuple[Keysym, ...]) -> bool:
        return all(ks == NoSymbol for ks in keysyms)

    @property
    def empty_symbols(self) -> bool:
        return self.has_empty_symbols(self.keysyms)

    @property
    def keysyms_c(self) -> str:
        if not self.keysyms and self.actions:
            return self._c(
                NoSymbol.c, tuple(itertools.repeat(NoSymbol, len(self.actions)))
            )
        return self._c(NoSymbol.c, self.keysyms)

    @property
    def keysyms_xkb(self) -> str:
        return self._xkb(NoSymbol, self.keysyms)

    @classmethod
    def has_empty_actions(cls, actions: tuple[Action, ...]) -> bool:
        return not any(actions)

    @property
    def empty_actions(self) -> bool:
        return self.has_empty_actions(self.actions)

    @property
    def actions_xkb(self) -> str:
        return self._xkb("NoAction()", self.actions)

    @classmethod
    def Keysyms(cls, *keysyms: str | None) -> Self:
        return cls.Mix(keysyms, ())

    @classmethod
    def Actions(cls, *actions: int | Modifier | None) -> Self:
        return cls.Mix((), actions)

    @classmethod
    def Mix(
        cls, keysyms: tuple[str | None, ...], actions: tuple[int | Modifier | None,]
    ) -> Self:
        return cls(tuple(map(Keysym.parse, keysyms)), tuple(map(Action.parse, actions)))

    def add_keysyms(self, keep_actions: bool, level: int) -> Self:
        return self.__class__(
            keysyms=tuple(
                a.action_to_keysym(index=k, level=level)
                for k, a in enumerate(self.actions)
            ),
            actions=self.actions if keep_actions else (),
        )

    @property
    def target_group(self) -> int:
        for a in self.actions:
            if isinstance(a, GroupAction) and a.group > 1:
                return a.group
        else:
            return 0

    @property
    def target_level(self) -> int:
        for a in self.actions:
            if isinstance(a, ModAction) and a.mods:
                match a.mods:
                    case Modifier.LevelThree:
                        return 2
                    case _:
                        return 0
        else:
            return 0


@dataclass
class KeyEntry:
    levels: tuple[Level, ...]

    def __init__(self, *levels: Level):
        self.levels = levels

    @property
    def xkb(self) -> Iterator[str]:
        if not self.levels:
            yield ""
            return
        keysyms = tuple(l.keysyms for l in self.levels)
        has_keysyms = any(not Level.has_empty_symbols(s) for s in keysyms)
        no_keysyms = all(not s for s in keysyms)
        actions = tuple(l.actions for l in self.levels)
        has_actions = any(not Level.has_empty_actions(a) for a in actions)
        if has_keysyms or (not no_keysyms and not has_actions):
            yield "["
            yield ", ".join(l.keysyms_xkb for l in self.levels)
            yield "]"
        if has_actions or no_keysyms:
            if has_keysyms:
                yield ", "
            yield "["
            yield ", ".join(l.actions_xkb for l in self.levels)
            yield "]"

    def add_keysyms(self, keep_actions: bool) -> Self:
        return self.__class__(
            *(
                l.add_keysyms(keep_actions=keep_actions, level=k)
                for k, l in enumerate(self.levels)
            )
        )


class TestType(IntFlag):
    KeysymsOnly = auto()
    ActionsOnly = auto()
    KeysymsAndActions = auto()
    All = ActionsOnly | KeysymsOnly | KeysymsAndActions


@dataclass
class TestId:
    type: TestType
    base: int = 0
    _last_base: ClassVar[int] = 0
    _max_type: ClassVar[int] = TestType.KeysymsAndActions >> 1
    _forbidden_keys: tuple[str, ...] = ("LEFTSHIFT", "RIGHTALT")

    def __post_init__(self):
        if self.base == -1:
            self.base = self.__class__._last_base
        elif self.base < 0:
            raise ValueError(self.base)
        elif self.base == 0:
            self.__class__._last_base += 1
            self.base = self.__class__._last_base
            while not self.check_linux_keys(self._base_id):
                self.__class__._last_base += 1
                self.base = self.__class__._last_base
        else:
            self.__class__._last_base = max(self.__class__._last_base, self.base)
        assert self.linux_key, self
        # print(self.base, self.id, self.xkb_key)

    def __hash__(self) -> int:
        return self.id

    @property
    def _base_id(self) -> int:
        return (self.base - 1) * (self._max_type + 1) + 1

    @property
    def id(self) -> int:
        # return (self.base << 2) | (self.type >> 1)
        return self._base_id + (self.type >> 1)

    def with_type(self, type: TestType) -> Self:
        return dataclasses.replace(self, type=type)

    @property
    def xkb_key(self) -> str:
        return f"T{self.id:0>3}"

    @classmethod
    def check_linux_key(cls, idx: int) -> bool:
        if idx >= len(LINUX_EVENT_CODES):
            raise ValueError("Not enough Linux keys available!")
        key: str | None = LINUX_EVENT_CODES[idx]
        return bool(key) and key not in cls._forbidden_keys

    @classmethod
    def check_linux_keys(cls, base_id) -> bool:
        return all(
            cls.check_linux_key(base_id + k) for k in range(0, cls._max_type + 1)
        )

    @property
    def linux_key(self) -> str:
        if self.id >= len(LINUX_EVENT_CODES):
            raise ValueError("Not enough Linux keys available!")
        return LINUX_EVENT_CODES[self.id]

    @property
    def key(self) -> KeyCode:
        return KeyCode(self.linux_key, self.xkb_key)


@unique
class Implementation(Flag):
    x11 = auto()
    xkbcommon = auto()
    all = x11 | xkbcommon


@dataclass
class TestEntry:
    id: TestId
    key: KeyCode = dataclasses.field(init=False)
    base: KeyEntry
    update: KeyEntry
    augment: KeyEntry
    override: KeyEntry
    replace: KeyEntry
    types: TestType
    implementations: Implementation

    group_keysyms: ClassVar[tuple[tuple[str, str], ...]] = (
        ("Ukrainian_i", "Ukrainian_I", "Ukrainian_yi", "Ukrainian_YI"),
        ("ch", "Ch", "c_h", "C_h"),
    )

    def __init__(
        self,
        id: TestId | None,
        base: KeyEntry,
        update: KeyEntry,
        augment: KeyEntry,
        override: KeyEntry,
        types: TestType = TestType.All,
        replace: KeyEntry | None = None,
        implementations: Implementation = Implementation.all,
    ):
        self.id = TestId(0, 0) if id is None else id
        self.key = id.key
        self.base = base
        self.update = update
        self.augment = augment
        self.override = override
        self.types = types
        self.replace = self.update if replace is None else replace
        self.implementations = implementations

    @property
    def default(self) -> KeyEntry:
        return self.override

    def add_keysyms(self, keep_actions: bool) -> Self:
        types = TestType.KeysymsAndActions if keep_actions else TestType.KeysymsOnly
        return dataclasses.replace(
            self,
            id=self.id.with_type(types),
            base=self.base.add_keysyms(keep_actions=keep_actions),
            update=self.update.add_keysyms(keep_actions=keep_actions),
            augment=self.augment.add_keysyms(keep_actions=keep_actions),
            override=self.override.add_keysyms(keep_actions=keep_actions),
            replace=self.replace.add_keysyms(keep_actions=keep_actions),
            types=types,
        )

    @classmethod
    def alt_keysym(cls, group: int, level: int) -> Keysym:
        return Keysym(cls.group_keysyms[group % 2][level % 4])

    @classmethod
    def alt_keysyms(cls, group: int) -> Iterator[Keysym]:
        for keysym in cls.group_keysyms[group % 2]:
            yield Keysym(keysym)


@dataclass
class SymbolsTestGroup:
    name: str
    tests: tuple[TestEntry | Comment, ...]

    def _with_implementation(
        self, implementation: Implementation
    ) -> Iterable[TestEntry | Comment]:
        pending_comment: Comment | None = None
        for t in self.tests:
            if not isinstance(t, TestEntry):
                pending_comment = t
            elif t.implementations & implementation:
                if pending_comment is not None:
                    yield pending_comment
                    pending_comment = None
                yield t

    def with_implementation(self, implementation: Implementation) -> Self:
        return dataclasses.replace(
            self, tests=tuple(self._with_implementation(implementation))
        )

    @staticmethod
    def _add_keysyms(entry: TestEntry | Comment) -> Iterable[TestEntry | Comment]:
        if isinstance(entry, TestEntry) and entry.id.type is TestType.ActionsOnly:
            if entry.types & TestType.KeysymsOnly:
                yield entry.add_keysyms(keep_actions=False)
            yield entry
            if entry.types & TestType.KeysymsAndActions:
                yield entry.add_keysyms(keep_actions=True)
        else:
            yield entry

    def add_keysyms(self, name: str = "") -> Self:
        return dataclasses.replace(
            self,
            name=name or self.name,
            tests=tuple(t for ts in self.tests for t in self._add_keysyms(ts)),
        )

    def __add__(self, other: Any) -> Self:
        if isinstance(other, tuple):
            return dataclasses.replace(self, tests=self.tests + other)
        elif isinstance(other, self.__class__):
            return dataclasses.replace(self, tests=self.tests + other.tests)
        else:
            return NotImplemented

    def get_keycodes(self, base: ComponentTestTemplate) -> ComponentTestTemplate:
        ids: set[TestId] = set(t.id for t in self.tests if isinstance(t, TestEntry))

        base_template = Template(
            "\n".join(f"${{mode}}<{id.xkb_key}> = {id.id + 8};" for id in ids) + "\n"
        )
        keycodes = base_template.substitute(mode="")

        return dataclasses.replace(
            base,
            base_template=base_template + base.base_template,
            update_template=base.update_template,
            augment=keycodes + base.augment,
            override=keycodes + base.override,
            replace=keycodes + base.replace,
        )

    def _get_symbols(self, type: str, debug: bool) -> Generator[str]:
        for entry in self.tests:
            if isinstance(entry, str):
                if type == "update":
                    yield f"\n////// {entry} //////"
            else:
                data: KeyEntry = getattr(entry, type)
                if debug:
                    yield f"// base:     {''.join(entry.base.xkb) or '(empty)'}"
                    yield "// key to merge / replace result"
                yield f"${{mode}}key {entry.key.xkb} {{ {''.join(data.xkb)} }};"
                if debug:
                    yield f"// override: {''.join(entry.override.xkb) or '(empty)'}"
                    yield f"// augment:  {''.join(entry.augment.xkb) or '(empty)'}"

    def get_symbols(self, debug: bool = False) -> ComponentTestTemplate:
        base_template = ComponentTemplate("\n".join(self._get_symbols("base", False)))
        update_template = ComponentTemplate(
            "\n".join(self._get_symbols("update", debug))
        )
        augment = ComponentTemplate(
            "\n".join(self._get_symbols("augment", False))
        ).substitute(mode="")
        override = ComponentTemplate(
            "\n".join(self._get_symbols("override", False))
        ).substitute(mode="")
        replace = ComponentTemplate(
            "\n".join(self._get_symbols("replace", False))
        ).substitute(mode="")
        return ComponentTestTemplate(
            title=self.name,
            component=Component.symbols,
            base_template=base_template,
            update_template=update_template,
            augment=augment,
            override=override,
            replace=replace,
        )


SYMBOLS_TESTS_BOTH = SymbolsTestGroup(
    "",
    (
        Comment("Trivial cases"),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(),
            update=KeyEntry(),
            augment=KeyEntry(),
            override=KeyEntry(),
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(),
            update=KeyEntry(Level.Actions(3)),
            augment=KeyEntry(Level.Actions(3)),
            override=KeyEntry(Level.Actions(3)),
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2)),
            update=KeyEntry(),
            augment=KeyEntry(Level.Actions(2)),
            override=KeyEntry(Level.Actions(2)),
        ),
        Comment("Same key"),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2)),
            update=KeyEntry(Level.Actions(2)),
            augment=KeyEntry(Level.Actions(2)),
            override=KeyEntry(Level.Actions(2)),
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(2), Level.Actions(2, Modifier.Control)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2, Modifier.Control)),
            override=KeyEntry(Level.Actions(2), Level.Actions(2, Modifier.Control)),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Mismatch levels count"),
        (
            TEST_BOTH_Q := TestEntry(
                TestId(TestType.ActionsOnly),
                KeyEntry(Level.Actions(None), Level.Actions(2)),
                update=KeyEntry(
                    Level.Actions(3), Level.Actions(None), Level.Actions(None)
                ),
                augment=KeyEntry(
                    Level.Actions(3), Level.Actions(2), Level.Actions(None)
                ),
                override=KeyEntry(
                    Level.Actions(3), Level.Actions(2), Level.Actions(None)
                ),
                # X11 and xkbcommon handle keysyms-only case differently
                types=TestType.ActionsOnly | TestType.KeysymsAndActions,
            )
        ),
        # Trailing NoSymbols are discarded in xkbcomp
        dataclasses.replace(
            TEST_BOTH_Q,
            augment=KeyEntry(Level.Actions(3), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3), Level.Actions(2)),
            replace=KeyEntry(Level.Actions(3)),
            implementations=Implementation.x11,
        ).add_keysyms(keep_actions=False),
        dataclasses.replace(
            TEST_BOTH_Q, implementations=Implementation.xkbcommon
        ).add_keysyms(keep_actions=False),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3), Level.Actions(None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(3), Level.Actions(2), Level.Actions(None)),
            override=KeyEntry(Level.Actions(3), Level.Actions(2), Level.Actions(None)),
            # X11 and xkbcommon handle keysyms-only case differently
            types=TestType.ActionsOnly | TestType.KeysymsAndActions,
        ),
        (
            TEST_BOTH_W := TestEntry(
                TestId(TestType.ActionsOnly),
                KeyEntry(Level.Actions(None), Level.Actions(2), Level.Actions(None)),
                update=KeyEntry(Level.Actions(3), Level.Actions(None)),
                augment=KeyEntry(
                    Level.Actions(3), Level.Actions(2), Level.Actions(None)
                ),
                override=KeyEntry(
                    Level.Actions(3), Level.Actions(2), Level.Actions(None)
                ),
                # X11 and xkbcommon handle keysyms-only case differently
                types=TestType.ActionsOnly | TestType.KeysymsAndActions,
            )
        ),
        dataclasses.replace(
            TEST_BOTH_W,
            augment=KeyEntry(Level.Actions(3), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3), Level.Actions(2)),
            replace=KeyEntry(Level.Actions(3)),
            implementations=Implementation.x11,
        ).add_keysyms(keep_actions=False),
        dataclasses.replace(
            TEST_BOTH_W, implementations=Implementation.xkbcommon
        ).add_keysyms(keep_actions=False),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2), Level.Actions(3)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3), Level.Actions(3)),
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3), Level.Actions(2)),
        ),
        Comment("Single keysyms -> single keysyms"),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(None), Level.Actions(None)),
            override=KeyEntry(Level.Actions(None), Level.Actions(None)),
        ),
        (
            TEST_BOTH_Y := TestEntry(
                TestId(TestType.ActionsOnly),
                KeyEntry(Level.Actions(None), Level.Actions(None)),
                update=KeyEntry(Level.Actions(3), Level.Actions(None)),
                augment=KeyEntry(Level.Actions(3), Level.Actions(None)),
                override=KeyEntry(Level.Actions(3), Level.Actions(None)),
                # X11 and xkbcommon handle keysyms-only case differently
                types=TestType.ActionsOnly | TestType.KeysymsAndActions,
            )
        ),
        dataclasses.replace(
            TEST_BOTH_Y,
            augment=KeyEntry(Level.Actions(3)),
            override=KeyEntry(Level.Actions(3)),
            replace=KeyEntry(Level.Actions(3)),
            implementations=Implementation.x11,
        ).add_keysyms(keep_actions=False),
        dataclasses.replace(
            TEST_BOTH_Y, implementations=Implementation.xkbcommon
        ).add_keysyms(keep_actions=False),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(None), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(None), Level.Actions(3)),
            override=KeyEntry(Level.Actions(None), Level.Actions(3)),
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(3), Level.Actions(3)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3)),
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(2), Level.Actions(2)),
        ),
        (
            TEST_BOTH_P := TestEntry(
                TestId(TestType.ActionsOnly),
                KeyEntry(Level.Actions(2), Level.Actions(2)),
                update=KeyEntry(Level.Actions(3), Level.Actions(None)),
                augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
                override=KeyEntry(Level.Actions(3), Level.Actions(2)),
                # X11 and xkbcommon handle keysyms-only case differently
                types=TestType.ActionsOnly | TestType.KeysymsAndActions,
            )
        ),
        dataclasses.replace(
            TEST_BOTH_P,
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3), Level.Actions(2)),
            replace=KeyEntry(Level.Actions(3)),
            implementations=Implementation.x11,
        ).add_keysyms(keep_actions=False),
        dataclasses.replace(
            TEST_BOTH_P, implementations=Implementation.xkbcommon
        ).add_keysyms(keep_actions=False),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(None), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(2), Level.Actions(3)),
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3)),
        ),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Keysyms("a"), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3), Level.Keysyms("X")),
            augment=KeyEntry(Level.Mix(("a"), (3,)), Level.Mix(("X",), (2,))),
            override=KeyEntry(Level.Mix(("a"), (3,)), Level.Mix(("X",), (2,))),
        ),
        Comment("Single keysyms -> multiple keysyms"),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, None)),
            augment=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            override=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(None, None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            override=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            augment=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(2)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(2)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(2), Level.Actions(3, None)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(None, None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(2), Level.Actions(3, None)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Multiple keysyms -> multiple keysyms"),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None, None), Level.Actions(None, None)),
            update=KeyEntry(
                Level.Actions(None, None), Level.Actions(3, Modifier.LevelThree)
            ),
            augment=KeyEntry(
                Level.Actions(None, None), Level.Actions(3, Modifier.LevelThree)
            ),
            override=KeyEntry(
                Level.Actions(None, None), Level.Actions(3, Modifier.LevelThree)
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None, None), Level.Actions(None, None)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            augment=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None, None), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            augment=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(2, Modifier.Control),
            ),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            augment=KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            augment=KeyEntry(
                Level.Actions(2, None),
                Level.Actions(None, 2),
            ),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(
                Level.Actions(2, Modifier.Control), Level.Actions(2, Modifier.Control)
            ),
            update=KeyEntry(
                Level.Actions(None, None), Level.Actions(3, Modifier.LevelThree)
            ),
            augment=KeyEntry(
                Level.Actions(2, Modifier.Control), Level.Actions(2, Modifier.Control)
            ),
            override=KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(3, Modifier.LevelThree),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(
                Level.Actions(2, Modifier.Control), Level.Actions(Modifier.Control, 2)
            ),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            augment=KeyEntry(
                Level.Actions(2, Modifier.Control), Level.Actions(Modifier.Control, 2)
            ),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None, None), Level.Actions(None, None, None)),
            update=KeyEntry(Level.Actions(None, None, None), Level.Actions(None, None)),
            augment=KeyEntry(
                Level.Actions(None, None), Level.Actions(None, None, None)
            ),
            override=KeyEntry(
                Level.Actions(None, None), Level.Actions(None, None, None)
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None, None), Level.Actions(None, None, None)),
            update=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            augment=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            override=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
            update=KeyEntry(Level.Actions(None, None, None), Level.Actions(None, None)),
            augment=KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
            override=KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
            update=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            augment=KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
            override=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Multiple keysyms -> single keysyms"),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None, None), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(None), Level.Actions(None)),
            augment=KeyEntry(
                Level.Actions(None, None), Level.Actions(2, Modifier.Control)
            ),
            override=KeyEntry(
                Level.Actions(None, None), Level.Actions(2, Modifier.Control)
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(None, None), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(3), Level.Actions(2, Modifier.Control)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            update=KeyEntry(Level.Actions(None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            override=KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.ActionsOnly),
            KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3)),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Mix"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Actions(2)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            augment=KeyEntry(Level.Actions(2), Level.Actions(3, Modifier.LevelThree)),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(3, Modifier.LevelThree), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2, Modifier.Control), Level.Actions(3)),
            override=KeyEntry(Level.Actions(3, Modifier.LevelThree), Level.Actions(3)),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Actions(2), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(3, Modifier.LevelThree), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2, Modifier.Control)),
            override=KeyEntry(Level.Actions(3, Modifier.LevelThree), Level.Actions(3)),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Mix"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Keysyms("a"), Level.Actions(2)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree), Level.Keysyms("X", "Y")
            ),
            augment=KeyEntry(
                Level.Mix(("a",), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2,)),
            ),
            override=KeyEntry(
                Level.Mix(("a",), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2,)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Multiple keysyms/actions –> single"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Keysyms("a", "b"), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(3), Level.Keysyms("X")),
            augment=KeyEntry(
                Level.Mix(("a", "b"), (3,)),
                Level.Mix(("X",), (2, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("a", "b"), (3,)),
                Level.Mix(("X",), (2, Modifier.Control)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Multiple keysyms/actions –> multiple (xor)"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Keysyms("a", "b"), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree), Level.Keysyms("X", "Y")
            ),
            augment=KeyEntry(
                Level.Mix(("a", "b"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("a", "b"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2, Modifier.Control)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Multiple keysyms/actions –> multiple (mix)"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Keysyms("a", None), Level.Actions(2, None)),
            update=KeyEntry(
                Level.Mix(("x", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (3, Modifier.LevelThree)),
            ),
            augment=KeyEntry(
                Level.Mix(("a", None), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2, None)),
            ),
            override=KeyEntry(
                Level.Mix(("x", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (3, Modifier.LevelThree)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Keysyms("a", "b"), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(
                Level.Mix(("x", None), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (3, None)),
            ),
            augment=KeyEntry(
                Level.Mix(("a", "b"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("x", None), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (3, None)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Multiple (mix) –> multiple keysyms/actions"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(
                Level.Mix(("a", "b"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (2, Modifier.Control)),
            ),
            update=KeyEntry(Level.Keysyms("x", None), Level.Actions(3, None)),
            augment=KeyEntry(
                Level.Mix(("a", "b"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (2, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("x", None), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (3, None)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(
                Level.Mix(("a", None), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (2, None)),
            ),
            update=KeyEntry(
                Level.Keysyms("x", "y"), Level.Actions(3, Modifier.LevelThree)
            ),
            augment=KeyEntry(
                Level.Mix(("a", None), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (2, None)),
            ),
            override=KeyEntry(
                Level.Mix(("x", "y"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (3, Modifier.LevelThree)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Multiple (mix) –> multiple (mix)"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(
                Level.Mix(("a", "b"), (2, Modifier.Control)),
                Level.Mix((None, "B"), (2, None)),
            ),
            update=KeyEntry(
                Level.Mix((None, "y"), (3, None)),
                Level.Mix(("X", "Y"), (3, Modifier.LevelThree)),
            ),
            augment=KeyEntry(
                Level.Mix(("a", "b"), (2, Modifier.Control)),
                Level.Mix((None, "B"), (2, None)),
            ),
            override=KeyEntry(
                Level.Mix((None, "y"), (3, None)),
                Level.Mix(("X", "Y"), (3, Modifier.LevelThree)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(
                Level.Mix(("a", None), (2, None)),
                Level.Mix((None, "B"), (None, Modifier.Control)),
            ),
            update=KeyEntry(
                Level.Mix((None, "y"), (None, Modifier.LevelThree)),
                Level.Mix(("X", None), (3, None)),
            ),
            augment=KeyEntry(
                Level.Mix(("a", None), (2, None)),
                Level.Mix((None, "B"), (None, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix((None, "y"), (None, Modifier.LevelThree)),
                Level.Mix(("X", None), (3, None)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Mismatch count with mix"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(
                Level.Keysyms("a"),
                Level.Keysyms("A", "B"),
            ),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3),
            ),
            augment=KeyEntry(
                Level.Mix(("a"), (3, Modifier.LevelThree)),
                Level.Mix(("A", "B"), (3,)),
            ),
            override=KeyEntry(
                Level.Mix(("a"), (3, Modifier.LevelThree)),
                Level.Mix(("A", "B"), (3,)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(
                Level.Actions(3),
                Level.Actions(3, Modifier.LevelThree),
            ),
            update=KeyEntry(
                Level.Keysyms("A", "B"),
                Level.Keysyms("a"),
            ),
            augment=KeyEntry(
                Level.Mix(("A", "B"), (3,)),
                Level.Mix(("a"), (3, Modifier.LevelThree)),
            ),
            override=KeyEntry(
                Level.Mix(("A", "B"), (3,)),
                Level.Mix(("a"), (3, Modifier.LevelThree)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(
                Level.Mix(("a",), (2,)),
                Level.Mix(("A", "B"), (2, Modifier.Control)),
            ),
            update=KeyEntry(
                Level.Mix(("x", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X",), (3,)),
            ),
            augment=KeyEntry(
                Level.Mix(("a",), (2,)),
                Level.Mix(("A", "B"), (2, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("x", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X",), (3,)),
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Issue #564"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Keysyms("A")),
            update=KeyEntry(Level.Mix(("A", "A"), (3, Modifier.LevelThree))),
            augment=KeyEntry(Level.Mix(("A",), (3, Modifier.LevelThree))),
            override=KeyEntry(Level.Mix(("A", "A"), (3, Modifier.LevelThree))),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Drop NoSymbol/NoAction"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Mix(("A",), (2,))),
            update=KeyEntry(Level.Mix((None, "Y", None), (None, 3, None))),
            augment=KeyEntry(Level.Mix(("A",), (2,))),
            override=KeyEntry(Level.Mix(("Y",), (3,))),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Drop NoSymbol/NoAction and invalid keysyms"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Mix(("notAKeysym", None, "thisEither"), (None, None))),
            update=KeyEntry(Level.Mix((None, None), (None, None))),
            augment=KeyEntry(Level.Keysyms(None)),
            override=KeyEntry(Level.Keysyms(None)),
            replace=KeyEntry(Level.Keysyms(None)),
            implementations=Implementation.xkbcommon,
        ),
    ),
).add_keysyms()

SYMBOLS_TESTS_XKBCOMMON = SYMBOLS_TESTS_BOTH.with_implementation(
    Implementation.xkbcommon
)

SYMBOLS_TESTS = SYMBOLS_TESTS_XKBCOMMON


################################################################################
#
# Keycodes tests
#
################################################################################

KEYCODES_TESTS_BASE = ComponentTestTemplate(
    "Keycodes",
    component=Component.keycodes,
    base_template=ComponentTemplate(
        """
        ${mode}<1> = 241;
        ${mode}<2> = 242;

        ${mode}alias <A> = <1>;
        ${mode}alias <B> = <2>;

        ${mode}indicator 1 = "Caps Lock";
        ${mode}indicator 2 = "Num Lock";
        ${mode}indicator 3 = "Scroll Lock";
        ${mode}indicator 4 = "Compose";
        ${mode}indicator 5 = "Kana";
        """
    ),
    update_template=ComponentTemplate(
        """
        ${mode}<1> = 241;            // Unchanged
        ${mode}<2> = 244;            // Changed
        ${mode}<3> = 243;            // New

        ${mode}alias <A> = <1>;    // Unchanged
        ${mode}alias <B> = <3>;    // Changed
        ${mode}alias <C> = <3>;    // New

        ${mode}indicator 1 = "Caps Lock";   // Unchanged
        ${mode}indicator 6 = "Num Lock";    // Changed index (free)
        ${mode}indicator 5 = "Scroll Lock"; // Changed index (not free)
        ${mode}indicator 4 = "XXXX";        // Changed name
        ${mode}indicator 7 = "Suspend";     // New
        """
    ),
    augment="""
        <1> = 241;
        <2> = 242;
        <3> = 243;
        alias <A> = <1>;
        alias <B> = <2>;
        alias <C> = <3>;
        indicator 1 = "Caps Lock";
        indicator 2 = "Num Lock";
        indicator 3 = "Scroll Lock";
        indicator 4 = "Compose";
        indicator 5 = "Kana";
        indicator 7 = "Suspend";
        """,
    override="""
        <1> = 241;
        <2> = 244;
        <3> = 243;
        alias <A> = <1>;
        alias <B> = <3>;
        alias <C> = <3>;
        indicator 1 = "Caps Lock";
        indicator 4 = "XXXX";
        indicator 5 = "Scroll Lock";
        indicator 6 = "Num Lock";
        indicator 7 = "Suspend";
        """,
)

KEYCODES_TESTS = SYMBOLS_TESTS.get_keycodes(KEYCODES_TESTS_BASE)


################################################################################
#
# Main
#
################################################################################

if __name__ == "__main__":
    # Parse commands
    parser = argparse.ArgumentParser(description="Generate merge mode tests")
    parser.add_argument(
        "--root",
        type=Path,
        default=ROOT,
        help="Path to the root of the project (default: %(default)s)",
    )
    parser.add_argument(
        "--c-out",
        type=Path,
        # default=ROOT / TestGroup.c_template.stem,
        help="Path to the output C test file (default: %(default)s)",
    )
    parser.add_argument("--no-xkb", action="store_true", help="Do not update XKB data")
    parser.add_argument("--debug", action="store_true", help="Activate debug mode")
    args = parser.parse_args()

    # Prepare data
    template_loader = jinja2.FileSystemLoader(args.root, encoding="utf-8")
    jinja_env = jinja2.Environment(
        loader=template_loader,
        keep_trailing_newline=True,
        trim_blocks=True,
        lstrip_blocks=True,
    )

    def escape_quotes(s: str) -> str:
        return s.replace('"', '\\"')

    jinja_env.filters["escape_quotes"] = escape_quotes

    TESTS = (
        KeymapTestTemplate(
            title="",
            keycodes=KEYCODES_TESTS,
            types=TYPES_TESTS,
            compat=COMPAT_TESTS,
            symbols=SYMBOLS_TESTS.get_symbols(debug=args.debug),
        ),
    )

    # Write tests
    TESTS[0].write(
        root=args.root,
        jinja_env=jinja_env,
        c_file=args.c_out,
        xkb=not args.no_xkb,
        tests=TESTS,
    )
