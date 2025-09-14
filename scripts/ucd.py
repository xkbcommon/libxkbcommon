#!/usr/bin/env python3

# # Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
# SPDX-License-Identifier: MIT

"""
Utils to parse the Unicode database files
"""

from collections.abc import Callable, Iterator
from dataclasses import dataclass
from pathlib import Path
import sys
from typing import ClassVar, Self


def parse_code_point(raw: str) -> int | None:
    return None if not raw else int(raw, 16)


@dataclass
class CodePointRange:
    start: int
    end: int

    def __iter__(self) -> Iterator[int]:
        yield from range(self.start, self.end + 1)

    @classmethod
    def parse(cls, raw: str) -> Self:
        start, *end = raw.strip().split("..")
        return cls(
            start=int(start, 16), end=int(start, 16) if not end else int(end[0], 16)
        )


@dataclass
class PropertyEntry:
    code_point: int
    property: str

    @classmethod
    def parse_file(
        cls, path: Path, filter: Callable[[str], bool] | None = None
    ) -> Iterator[Self]:
        with path.open("rt", encoding="utf-8") as fd:
            for line in fd:
                # Remove comment
                line, *_ = line.split("#")
                line = line.strip()
                # Skip empty lines
                if not line:
                    continue
                raw_range, property, *_ = line.split(";")
                range = CodePointRange.parse(raw_range)
                property = property.strip()
                if filter and not filter(property):
                    continue
                for code_point in range:
                    yield cls(code_point=code_point, property=property)


@dataclass
class UnicodeDataEntry:
    code_point: int
    general_category: str
    lower_case: int | None
    upper_case: int | None
    title_case: int | None

    @classmethod
    def parse_file(cls, path: Path) -> Iterator[Self]:
        with path.open("rt", encoding="utf-8") as fd:
            for line in fd:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                (
                    cp,
                    _name,
                    general_category,
                    _cc,
                    _bc,
                    _d,
                    _decimal,
                    _digit,
                    _numeric,
                    _mirrored,
                    _,
                    _,
                    upper_case,
                    lower_case,
                    title_case,
                    *_,
                ) = line.split(";")
                code_point = int(cp, 16)
                yield cls(
                    code_point=code_point,
                    general_category=general_category,
                    lower_case=parse_code_point(lower_case),
                    upper_case=parse_code_point(upper_case),
                    title_case=parse_code_point(title_case),
                )


@dataclass
class DB:
    lower_case: set[int]
    upper_case: set[int]
    title_case: set[int]
    lower_case_mappings: dict[int, int]
    upper_case_mappings: dict[int, int]
    title_case_mappings: dict[int, int]

    case_properties: ClassVar[frozenset[str]] = frozenset(("Lowercase", "Uppercase"))

    @classmethod
    def filter_case_properties(cls, property: str) -> bool:
        return property in cls.case_properties

    @classmethod
    def parse_ucd(cls, path: Path) -> Self:
        lower_case: set[int] = set()
        upper_case: set[int] = set()
        title_case: set[int] = set()

        lower_case_mappings: dict[int, int] = {}
        upper_case_mappings: dict[int, int] = {}
        title_case_mappings: dict[int, int] = {}

        for entry in UnicodeDataEntry.parse_file(path / "UnicodeData.txt"):
            if entry.general_category == "Lt":
                title_case.add(entry.code_point)
            if entry.lower_case is not None:
                lower_case_mappings[entry.code_point] = entry.lower_case
            if entry.upper_case is not None:
                upper_case_mappings[entry.code_point] = entry.upper_case
            if entry.title_case is not None:
                title_case_mappings[entry.code_point] = entry.title_case

        for entry in PropertyEntry.parse_file(
            path / "DerivedCoreProperties.txt", filter=cls.filter_case_properties
        ):
            match entry.property:
                case "Lowercase":
                    lower_case.add(entry.code_point)
                case "Uppercase":
                    upper_case.add(entry.code_point)
                case _:
                    raise ValueError(entry)

        return cls(
            lower_case=lower_case,
            upper_case=upper_case,
            title_case=title_case,
            lower_case_mappings=lower_case_mappings,
            upper_case_mappings=upper_case_mappings,
            title_case_mappings=title_case_mappings,
        )

    def isULowercase(self, cp: int) -> bool:
        return cp in self.lower_case

    def isUUppercase(self, cp: int) -> bool:
        return cp in self.upper_case

    def istitle(self, cp: int) -> bool:
        return cp in self.title_case

    def tolower(self, cp_or_char: int | str) -> int | str:
        cp = cp_or_char if isinstance(cp_or_char, int) else ord(cp_or_char)
        mapping = self.lower_case_mappings.get(cp, cp)
        return mapping if isinstance(cp_or_char, int) else chr(mapping)

    def toupper(self, cp_or_char: int | str) -> int | str:
        cp = cp_or_char if isinstance(cp_or_char, int) else ord(cp_or_char)
        mapping = self.upper_case_mappings.get(cp, cp)
        return mapping if isinstance(cp_or_char, int) else chr(mapping)


if __name__ == "__main__":
    # Test
    import icu

    c = icu.Locale.createFromName("C")
    icu.Locale.setDefault(c)

    path = Path(sys.argv[1])
    db = DB.parse_ucd(path)

    for cp in range(0, 0x10FFFF + 1):
        assert db.isULowercase(cp) == icu.DB.isULowercase(cp), (
            cp,
            db.isULowercase(cp),
            icu.DB.isULowercase(cp),
        )
        assert db.isUUppercase(cp) == icu.DB.isUUppercase(cp), cp
        assert db.istitle(cp) == icu.DB.istitle(cp), cp
        assert db.tolower(cp) == icu.DB.tolower(cp)
        assert db.toupper(cp) == icu.DB.toupper(cp)
