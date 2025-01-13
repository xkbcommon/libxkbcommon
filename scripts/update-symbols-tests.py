#!/usr/bin/env python3

"""
This script generate tests for symbols.
"""

from __future__ import annotations

import argparse
import dataclasses
import itertools
from abc import ABCMeta, abstractmethod
from dataclasses import dataclass
from enum import Flag, IntFlag, auto, unique
from pathlib import Path
from typing import Any, ClassVar, Iterable, Iterator, NewType, Self

import jinja2

SCRIPT = Path(__file__)
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
)


Comment = NewType("Comment", str)


def is_not_comment(x: Any) -> bool:
    return not isinstance(x, str)


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
            yield "symbols=["
            yield ", ".join(l.keysyms_xkb for l in self.levels)
            yield "]"
        if has_actions or no_keysyms:
            if has_keysyms:
                yield ", "
            yield "actions=["
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
class TestGroup:
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


C_HEADER_TEMPLATE = r"""// WARNING: This file was auto-generated by: {{ script }}
#include "evdev-scancodes.h"
#include <xkbcommon/xkbcommon.h>

#include "src/utils.h"
#include "test.h"

{%- macro key_seq(entry, type, level) -%}
        {% if entry[type].levels|length == 0 %}
        {{- entry.id.id }}, BOTH, XKB_KEY_NoSymbol{# -#}
        {% else %}
        {% if entry[type].levels[level].target_group < 2 %}
        {{- entry.id.id }}, BOTH, {{ entry[type].levels[level].keysyms_c -}}
        {% else %}
        {{- entry.id.id }}, DOWN, {{ entry[type].levels[level].keysyms_c }}, NEXT,
        {{ entry.id.id }}, UP, {{
            alt_keysym(entry[type].levels[level].target_group,
                       entry[type].levels[level].target_level + level).c
        -}}
        {% endif %}
        {% endif %}
{% endmacro %}

{% macro make_test(mode, ref, tests_group, compile_buffer) -%}
    {% set keymap_str = "keymap_" + tests_group.name.replace("-", "_") + mode -%}
    const char {{ keymap_str }}[] =
        "xkb_keymap {\n"
        "  xkb_keycodes { include \"merge_modes\" };\n"
        "  xkb_types { include \"basic+numpad+extra\" };\n"
        "  xkb_compat { include \"basic+iso9995\" };\n"
        "  xkb_symbols {\n"
        "    key <LFSH> { [Shift_L] };\n"
        "    key <RALT> { [ISO_Level3_Shift] };\n"
        "    modifier_map Shift { <LFSH> };\n"
        "    modifier_map Mod5 { <RALT> };\n"
        // NOTE: Separate statements so that *all* the merge modes *really* work.
        //       Using + and | separators downgrades `replace key` to `override/
        //       augment key`.
        "    include \"{{symbols_file}}({{ tests_group.name }}base)\"\n"
        "    {{ mode }} \"{{symbols_file}}({{ tests_group.name }}new)\"\n"
        "    include \"{{symbols_file}}(group2):2+{{symbols_file}}(group3):3\"\n"
        "  };\n"
        "};";
    fprintf(stderr, "*** test_merge_modes: {{ tests_group.name }}, {{ mode }} ***\n");
    keymap = compile_buffer(ctx, {{ keymap_str }},
                            ARRAY_SIZE({{ keymap_str }}),
                            private);
    assert(keymap);
    assert_printf(test_key_seq(keymap,
        {%- for entry in tests_group.tests +%}
        {% if is_not_comment(entry) %}
        {{ key_seq(entry, ref, 0) }}, {%- if entry[ref].levels|length > 1 %} NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        {{ key_seq(entry, ref, 1) }}, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L,
        {%- endif %}{%- if entry[ref].levels|length > 2 %} NEXT,
        KEY_RIGHTALT, DOWN, XKB_KEY_ISO_Level3_Shift, NEXT,
        {{ key_seq(entry, ref, 2) }}, NEXT,
        KEY_RIGHTALT, UP, XKB_KEY_ISO_Level3_Shift,
        {%- endif %}{% if not loop.last %} NEXT,{% endif %}
        {% else %}
        // {{ entry -}}
        {% endif %}
        {% endfor %} FINISH
    ), "test_merge_modes: {{ tests_group.name }}, {{ mode }}\n");
    xkb_keymap_unref(keymap);
{%- endmacro %}

static void
{{ test_func }}(struct xkb_context *ctx,
{{ " "*(test_func|length + 1) }}test_compile_buffer_t compile_buffer, void *private)
{
    struct xkb_keymap *keymap;
    {% for tests_group in tests_groups %}
    {% if tests_group.tests %}
    {% if tests_group.name %}

    /****************************************************************
     * Test group: {{ tests_group.name }}
     ****************************************************************/
    {% endif %}

    /* Mode: Default */
    {{ make_test("include", "override", tests_group, "compile_buffer") }}

    /* Mode: Augment */
    {{ make_test("augment", "augment", tests_group, "compile_buffer") }}

    /* Mode: Override */
    {{ make_test("override", "override", tests_group, "compile_buffer") }}

    /* Mode: Replace */
    {{ make_test("replace", "replace", tests_group, "compile_buffer") }}
    {% endif %}
    {% endfor %}
}
"""


@dataclass
class TestFile:
    suffix: str
    tests: tuple[TestGroup, ...]
    symbols_file: ClassVar[str] = "merge_modes"
    test_file: ClassVar[str] = "merge_modes_symbols.h"

    @classmethod
    def write_keycodes(
        cls, root: Path, jinja_env: jinja2.Environment, tests: tuple[TestFile, ...]
    ) -> None:
        """
        XKB custom keycodes
        """
        path = root / f"test/data/keycodes/{cls.symbols_file}"
        template_path = path.with_suffix(f"{path.suffix}.jinja")
        template = jinja_env.get_template(str(template_path.relative_to(root)))
        ids: set[TestId] = set(
            t.id
            for f in tests
            for ts in f.tests
            for t in ts.tests
            if isinstance(t, TestEntry)
        )

        with path.open("wt", encoding="utf-8") as fd:
            fd.writelines(
                template.generate(
                    ids=sorted(ids, key=lambda id: id.xkb_key),
                    script=SCRIPT.relative_to(root),
                )
            )

    @classmethod
    def write_symbols(
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        tests: tuple[TestFile, ...],
        debug: bool,
    ) -> None:
        """
        XKB Symbols data for all merge modes
        """
        path = root / f"test/data/symbols/{cls.symbols_file}"
        template_path = path.with_suffix(f"{path.suffix}.jinja")
        template = jinja_env.get_template(str(template_path.relative_to(root)))
        for t in tests:
            _path = path.with_stem(path.stem + t.suffix)
            _tests = t.tests
            keycodes = sorted(
                frozenset(
                    t.key for g in _tests for t in filter(is_not_comment, g.tests)
                ),
                key=lambda x: x._xkb,
            )
            with _path.open("wt", encoding="utf-8") as fd:
                fd.writelines(
                    template.generate(
                        keycodes=keycodes,
                        tests_groups=_tests,
                        script=SCRIPT.relative_to(root),
                        debug=debug,
                    )
                )

    @classmethod
    def write_c_tests(
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        tests: tuple[TestFile, ...],
    ) -> None:
        """
        C headers for alternative tests
        """
        path = root / f"test/{cls.test_file}"
        template_path = path.with_suffix(f"{path.suffix}.jinja")
        # Write jinja template
        with template_path.open("wt", encoding="utf-8") as fd:
            fd.write(C_HEADER_TEMPLATE)
        # Write C headers
        template = jinja_env.get_template(str(template_path.relative_to(root)))
        for t in tests:
            _path = path.with_stem(path.stem + t.suffix)
            _tests = t.tests
            symbols_file = cls.symbols_file + t.suffix
            test_func = f"test_symbols_merge_modes{t.suffix}"
            with _path.open("wt", encoding="utf-8") as fd:
                fd.writelines(
                    template.generate(
                        symbols_file=symbols_file,
                        test_func=test_func,
                        tests_groups=_tests,
                        script=SCRIPT.relative_to(root),
                    )
                )


TESTS_BOTH = TestGroup(
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
                Level.Actions(2, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 2),
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
            override=KeyEntry(
                Level.Actions(3, Modifier.Control), Level.Actions(Modifier.Control, 3)
            ),
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
            augment=KeyEntry(Level.Keysyms("a"), Level.Actions(2)),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree), Level.Keysyms("X", "Y")
            ),
            implementations=Implementation.xkbcommon,
        ),
        Comment("Multiple keysyms/actions –> single"),
        TestEntry(
            TestId(TestType.KeysymsAndActions),
            KeyEntry(Level.Keysyms("a", "b"), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(3), Level.Keysyms("X")),
            augment=KeyEntry(
                Level.Keysyms("a", "b"), Level.Actions(2, Modifier.Control)
            ),
            override=KeyEntry(Level.Actions(3), Level.Keysyms("X")),
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
                Level.Mix(("a", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2, Modifier.LevelThree)),
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
                Level.Mix(("x", "b"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (3, Modifier.Control)),
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
                Level.Mix(("x", "b"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (3, Modifier.Control)),
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
                Level.Mix(("a", "y"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (2, Modifier.LevelThree)),
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
                Level.Mix(("X", "B"), (2, Modifier.LevelThree)),
            ),
            override=KeyEntry(
                Level.Mix(("a", "y"), (3, Modifier.Control)),
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
                Level.Mix(("a", "y"), (2, Modifier.LevelThree)),
                Level.Mix(("X", "B"), (3, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("a", "y"), (2, Modifier.LevelThree)),
                Level.Mix(("X", "B"), (3, Modifier.Control)),
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
                Level.Keysyms("a"),
                Level.Keysyms("A", "B"),
            ),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3),
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
                Level.Actions(3),
                Level.Actions(3, Modifier.LevelThree),
            ),
            override=KeyEntry(
                Level.Keysyms("A", "B"),
                Level.Keysyms("a"),
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
            augment=KeyEntry(Level.Keysyms("A")),
            override=KeyEntry(Level.Mix(("A", "A"), (3, Modifier.LevelThree))),
            implementations=Implementation.xkbcommon,
        ),
    ),
).add_keysyms()

TESTS_XKBCOMMON = TestFile(
    "_xkbcommon",
    (TESTS_BOTH.with_implementation(Implementation.xkbcommon),),
)
TESTS_X11 = TestFile(
    "_x11",
    (TESTS_BOTH.with_implementation(Implementation.x11),),
)
TESTS = (TESTS_XKBCOMMON, TESTS_X11)


if __name__ == "__main__":
    # Root of the project
    ROOT = Path(__file__).parent.parent

    # Parse commands
    parser = argparse.ArgumentParser(description="Generate symbols tests")
    parser.add_argument(
        "--root",
        type=Path,
        default=ROOT,
        help="Path to the root of the project (default: %(default)s)",
    )
    parser.add_argument(
        "--alternative-tests",
        action="store_true",
        help="Write the C headers for alternative tests",
    )
    parser.add_argument("--debug", action="store_true", help="Activate debug mode")

    args = parser.parse_args()
    template_loader = jinja2.FileSystemLoader(args.root, encoding="utf-8")
    jinja_env = jinja2.Environment(
        loader=template_loader,
        keep_trailing_newline=True,
        trim_blocks=True,
        lstrip_blocks=True,
    )
    jinja_env.globals["alt_keysym"] = TestEntry.alt_keysym
    jinja_env.globals["alt_keysyms"] = TestEntry.alt_keysyms
    jinja_env.globals["is_not_comment"] = is_not_comment
    jinja_env.tests["is_not_comment"] = is_not_comment
    TestFile.write_keycodes(root=args.root, jinja_env=jinja_env, tests=TESTS)
    TestFile.write_symbols(
        root=args.root, jinja_env=jinja_env, tests=TESTS, debug=args.debug
    )
    if args.alternative_tests:
        TestFile.write_c_tests(root=args.root, jinja_env=jinja_env, tests=TESTS)
