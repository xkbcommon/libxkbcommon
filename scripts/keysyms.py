#!/usr/bin/env python3

# Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
# SPDX-License-Identifier: MIT

"""
Utils to parse the keysyms headers
"""

from __future__ import annotations

import re
from collections import defaultdict
from dataclasses import dataclass
from enum import Enum, StrEnum, auto, unique
from pathlib import Path
from typing import ClassVar, Generator, Iterable, Self, cast


@dataclass
class UnicodeCodePoint:
    cp: int
    char: str
    name: str

    @property
    def pretty_cp(self) -> str:
        return f"U+{self.cp:04X}"

    @property
    def markdown_cp(self) -> str:
        return f"`{self.pretty_cp}` {self.name}"

    def some_char(self, printable: bool | None = None) -> str:
        if printable is None or (not printable ^ self.char.isprintable()):
            return self.char
        else:
            return ""


@unique
class DeprecationReason(Enum):
    TYPO = auto()
    UNICODE_MISMATCH = auto()
    LEGACY_ALIAS = auto()
    IMPLICIT_ALIAS = auto()
    """
    Implicit deprecation: the keysym has already been defined with a previous
    name, and the present name has not been declared explicitly as an alias.
    """
    UNKNOWN = auto()

    @classmethod
    def parse(cls, raw: str) -> Self:
        raw_ = raw.casefold()
        if raw_.startswith("misspell") or "typo" in raw_:
            return cls.TYPO
        else:
            raise ValueError(f"Unknown deprecation reason: “{raw}”")


@unique
class Semantics(Enum):
    Default = auto()
    ComputerNumpad = auto()
    OtherKeypad = auto()


@unique
class KeysymCategory(StrEnum):
    Special = "Special"
    Latin1 = "Latin-1"
    Legacy = "Legacy"
    Function = "Function"
    Unicode = "Unicode"
    Vendor = "Vendor"

    @classmethod
    def from_keysym(cls, value: int) -> Self:
        if value == 0 or value == 0x00FFFFFF:
            return cls.Special
        elif (0x20 <= value <= 0x7E) or (0xA0 <= value <= 0xFF):
            return cls.Latin1
        elif (0x0100 <= value <= 0x13FF) or (0x0200 <= value <= 0x20FF):
            return cls.Legacy
        elif 0xFD00 <= value <= 0xFFFF:
            return cls.Function
        elif 0x01000000 <= value <= 0x0110FFFF:
            return cls.Unicode
        elif 0x10000000 <= value <= 0x1FFFFFFF:
            return cls.Vendor
        else:
            raise ValueError(value)


@dataclass
class Keysym:
    value: int
    name: str
    _canonical: Self | None
    """The canonical name if the name is an alias"""
    _preferred: Self | None
    """The preferred name if deprecated"""
    char: UnicodeCodePoint | None
    char_semantics: Semantics
    char_aliases: list[Self]
    aliases: list[Self]
    deprecation: DeprecationReason | None
    comment: str

    DUMMY_VALUE: ClassVar[int] = -1
    KEYSYM_ENTRY_PATTERN: ClassVar[re.Pattern[str]] = re.compile(
        r"""
        ^\#define\s+
        XKB_KEY_(?P<name>\w+)\s+
        (?P<value>0x[0-9a-fA-F]+)\s*
        (?:/\*(?P<comment>.*)\*/)?
        """,
        re.VERBOSE,
    )
    UNICODE_PATTERN: ClassVar[re.Pattern[str]] = re.compile(
        r"""
        (?:(?P<alt_semantics><)|(?P<deprecated>\())?
        U\+(?P<code_point>[0-9a-fA-F]{4,})
        \s+
        (?P<name>(?:\w|-)+(?:\s+(?:\w|-)+)*)
        (?(alt_semantics)>)(?(deprecated)\))
        # TODO: alt semantics category
        """,
        re.VERBOSE,
    )
    DEPRECATION_ALIAS_PATTERN: ClassVar[re.Pattern[str]] = re.compile(
        r"""
        (?:
            \s+
            (?:
                non-deprecated |
                (?P<deprecated>deprecated)
            )
        )?
        (?:\s+alias\s+for\s+(?P<alias_target>\w+))?
        (?:
            (?:
                : |
                \s+(?P<parenthesis>\()
            )
            (?P<reason>.+)
            (?(parenthesis)\))
        )?
        """,
        re.VERBOSE | re.IGNORECASE,
    )

    @classmethod
    def new(cls, name: str, value: int) -> Self:
        return cls(
            name=name,
            value=value,
            char=None,
            char_semantics=Semantics.Default,
            char_aliases=[],
            _canonical=None,
            _preferred=None,
            aliases=[],
            deprecation=None,
            comment="",
        )

    @classmethod
    def parse(cls, raw: str) -> Self | None:
        if (m := cls.KEYSYM_ENTRY_PATTERN.match(raw)) is None:
            return None

        value = int(m.group("value"), 16)
        name = m.group("name")
        alias_target: Keysym | None = None
        deprecation: DeprecationReason | None = None
        char: UnicodeCodePoint | None = None
        char_semantics: Semantics = Semantics.Default

        if comment := m.group("comment"):
            if m := cls.UNICODE_PATTERN.search(comment):
                cp = int(m.group("code_point"), 16)
                char = UnicodeCodePoint(cp=cp, char=chr(cp), name=m.group("name"))
                if m.group("alt_semantics"):
                    if name.startswith("KP_"):
                        char_semantics = Semantics.ComputerNumpad
                    elif name.startswith("XF86Numeric"):
                        char_semantics = Semantics.OtherKeypad
                    else:
                        raise ValueError(f"Unknown semantics for: {name}")
                else:
                    char_semantics = Semantics.Default
                if m.group("deprecated"):
                    deprecation = DeprecationReason.UNICODE_MISMATCH
            elif m := cls.DEPRECATION_ALIAS_PATTERN.match(comment):
                if target := m.group("alias_target"):
                    alias_target = cls.new(name=target, value=cls.DUMMY_VALUE)

                if m.group("deprecated"):
                    if alias_target is not None:
                        deprecation = DeprecationReason.LEGACY_ALIAS
                    elif reason := m.group("reason"):
                        deprecation = DeprecationReason.parse(reason)
                    else:
                        deprecation = DeprecationReason.UNKNOWN

        return cls(
            name=name,
            value=value,
            char=char,
            char_semantics=char_semantics,
            char_aliases=[],
            _canonical=None,
            _preferred=alias_target,
            aliases=[],
            deprecation=deprecation,
            comment=comment,
        )

    @property
    def canonical(self) -> Self:
        if self._canonical is None:
            raise ValueError(self)
        else:
            return self._canonical

    @property
    def is_canonical(self) -> bool:
        return self._canonical is self

    @property
    def preferred(self) -> Self:
        if self._preferred is None:
            raise ValueError(self)
        else:
            return self._preferred

    @property
    def is_preferred(self) -> bool:
        return self._preferred is self

    @property
    def deprecated(self) -> bool:
        """Deprecated name"""
        return self.deprecation is not None

    @property
    def deprecated_keysym(self) -> bool:
        """Deprecated keysym"""
        return (
            (self.deprecated and all(k.deprecated for k in self.aliases))
            if self.is_canonical
            else self.canonical.deprecated_keysym
        )

    @property
    def is_dummy(self) -> bool:
        return self.value == self.DUMMY_VALUE

    @property
    def pretty_value(self) -> str:
        return f"{self.value:#06x}"

    @property
    def macro(self) -> str:
        return f"XKB_KEY_{self.name}"

    @property
    def category(self) -> KeysymCategory:
        return KeysymCategory.from_keysym(self.value)


@dataclass
class Keysyms:
    all: list[Keysym]
    by_value: dict[int, list[Keysym]]
    by_name: dict[str, Keysym]
    by_char: dict[int, list[Keysym]]

    @classmethod
    def _parse(cls, raw: Iterable[str]) -> Generator[Keysym | str, None, Self]:
        keysyms = cls(
            all=[],
            by_value=defaultdict(list),
            by_name={},
            by_char=defaultdict(list),
        )

        # Parse all the keysyms
        for line, k in ((line_, Keysym.parse(line_)) for line_ in raw):
            if k is None:
                yield line
            else:
                yield k
                keysyms.all.append(k)

        for keysym in keysyms.all:
            if previous := keysyms.by_value.get(keysym.value):
                # There are some previous names with this value

                assert keysym._canonical is None

                # First name is the canonical name
                canonical = previous[0]
                if (
                    keysym._preferred is None  # implicit alias
                    and keysym.deprecation is None
                    and any(k.deprecation is None for k in previous)
                ):
                    # Implicit alias with at least one previous non-deprecated name
                    keysym.deprecation = DeprecationReason.IMPLICIT_ALIAS
                elif (
                    keysym._preferred is None  # implicit alias
                    and keysym.deprecation is DeprecationReason.UNKNOWN
                ):
                    keysym.deprecation = DeprecationReason.LEGACY_ALIAS

                keysym._canonical = canonical

                # Aliases
                keysym.aliases.append(canonical)
                canonical.aliases.append(keysym)

                # Preferred name is the first non-explicit alias and non-deprecated name
                if (
                    canonical._preferred is not None
                    and not canonical._preferred.is_dummy
                ):
                    # Preferred name already resolved
                    if (
                        keysym._preferred is not None
                        and keysym._preferred.name != canonical._preferred.name
                    ):
                        # Explicit alias does not point to the preferred name
                        assert keysym._preferred.is_dummy, (
                            canonical._preferred,
                            keysym,
                        )
                        raise ValueError((canonical._preferred, keysym))
                    elif (
                        keysym.char is not None
                        and keysym.char != canonical._preferred.char
                    ):
                        # New keysym and the preferred name have distinct chars
                        raise ValueError((canonical._preferred, keysym))
                    else:
                        keysym._preferred = canonical._preferred
                elif keysym._preferred is None and keysym.deprecation is None:
                    # New preferred name
                    keysym._preferred = keysym
                    for k in previous:
                        if (
                            k._preferred is not None
                            and k._preferred.name != keysym.name
                        ):
                            # Explicit alias does not point to the preferred name
                            raise ValueError((keysym, k))
                        elif k.char is not None:
                            # The first char definition should be in the preferred name
                            raise ValueError((keysym, k))
                        else:
                            k._preferred = keysym
            else:
                # First name is the canonical name
                keysym._canonical = keysym
                if keysym._preferred is None and keysym.deprecation is None:
                    keysym._preferred = keysym

            keysyms.by_value[keysym.value].append(keysym)

            if conflict := keysyms.by_name.get(keysym.name):
                raise ValueError(f"Name conflict: {keysym} conflicts with {conflict}")
            keysyms.by_name[keysym.name] = keysym

        # Resolve pending preferred names
        for ks in keysyms.by_value.values():
            canonical = ks[0]
            assert canonical.is_canonical, ks
            if len(ks) == 1:
                # Check that a keysym with a single name has no explicit alias
                if canonical._preferred is not None:
                    if canonical._preferred.is_dummy:
                        raise ValueError(canonical)
                    else:
                        assert canonical._preferred is canonical
                else:
                    # No choice!
                    canonical._preferred = canonical
            elif canonical._preferred is None or canonical._preferred.is_dummy:
                preferred: Keysym | None = None
                for k in ks:
                    if k.deprecated and k._preferred is not None:
                        continue
                    elif not k.deprecated and k._preferred is None:
                        # Missed in the initialization!
                        raise ValueError(ks)
                    elif preferred is None or (
                        not k.deprecated and preferred.deprecated
                    ):
                        # First or better candidate
                        preferred = k

                if preferred is None:
                    raise ValueError(ks)

                for k in ks:
                    k._preferred = preferred

        # Now that canonical names are resolved, process chars
        for keysym in sorted(
            # Skip non-canonical keysyms and keysyms without associated char
            (k for k in keysyms.all if k.is_canonical and k.char is not None),
            # Sort by ascending code point and keysym value
            key=lambda k: (cast(UnicodeCodePoint, k.char).cp, k.value),
        ):
            cp = cast(UnicodeCodePoint, keysym.char).cp
            if previous := keysyms.by_char.get(cp):
                for k in previous:
                    keysym.char_aliases.append(k)
                    k.char_aliases.append(keysym)
            keysyms.by_char[cp].append(keysym)

        return keysyms

    @classmethod
    def parse(cls, raw: Iterable[str]) -> Self:
        gen = cls._parse(raw)
        try:
            while True:
                next(gen)
        except StopIteration as e:
            return e.value

    @classmethod
    def parse_iter(cls, raw: Iterable[str]) -> Iterable[Keysym | str]:
        # Iterate all the file to resolve the keysym
        acc: list[Keysym | str] = []
        gen = cls._parse(raw)
        while True:
            try:
                acc.append(next(gen))
            except StopIteration:
                break

        # Yielf the resolved keysym
        yield from acc

    @classmethod
    def parse_file(cls, path: Path) -> Self:
        with path.open("rt", encoding="utf-8") as f:
            return cls.parse(f)

    @classmethod
    def parse_iter_file(cls, path: Path) -> Iterable[Keysym | str]:
        with path.open("rt", encoding="utf-8") as f:
            yield from cls.parse_iter(f)
