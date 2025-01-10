#!/usr/bin/env python3

# This script creates the keysym case mappings in `src/keysym-case-mappings.c`.
#
# Inspired from: https://github.com/apankrat/notes/blob/3c551cb028595fd34046c5761fd12d1692576003/fast-case-conversion/README.md

# NOTE: the following docstring is also used to document the resulting C file.

"""
There are two kinds of keysyms to consider:
• Legacy keysyms: their case mappings is located at `data/keysyms.yaml`.
• Unicode keysyms: their case mappings come from the ICU library.

These mappings would create huge lookup tables if done naively. Fortunately,
we can observe that if we compute only the *difference* between a keysym and
its corresponding case mapping, there are a lot of repetitions that can be
efficiently compressed.

The idea for the compression is, for each kind of keysyms:
1. Compute the deltas between the keysyms and their case mappings.
2. Split the delta array in chunks of a given size.
3. Rearrange the order of the chunks in order to optimize consecutive
   chunks overlap.
4. Create a data table with the reordered chunks and an index table that
   maps the original chunk index to its offset in the data table.

Trivial example (chunk size: 4, from step 2):

[1, 2, 3, 4, 2, 3, 4, 5, 0, 1, 2, 3]          # source data
-> [[1, 2, 3, 4], [2, 3, 4, 5], [0, 1, 2, 3]] # make chunks
-> [[0, 1, 2, 3], [1, 2, 3, 4], [2, 3, 4, 5]] # rearrange to have best overlaps
-> {data: [0, 1, 2, 3, 4, 5], offsets: [1, 2, 0]} # overlap chunks & compute
                                                  # their offsets

Then we can retrieve the data from the original array at index i with the
following formula:

    mask = (1 << chunk_size) - 1;
    original[i] = data[offsets[i >> chunk_size] + (i & mask)];

Since the index array is itself quite repetitive with the real data, we apply
the compression a second time to the offsets table.

The complete algorithm optimizes the chunk sizes for both arrays in order to
get the lowest total data size.

There are 6 resulting arrays, 3 for each kind of keysyms:
1. The data array. Each item is either:
   • 0, if the keysym is not cased.
   • A delta to lower case.
   • A delta to upper case.
   • For some special cases, there are both a lower *and* an upper case
     mapping. The delta is identical in both cases.
2. The 1st offsets array, that provides offsets into the data array.
3. The 2nd offsets array, that provides offsets into the 1st index array.

Finally, given the chunks sizes `cs_data` and `cs_offsets`:
1. We compute the corresponding masks:
   • `mask_data = (1 << cs_data) - 1` and
   • `mask_offsets = (1 << cs_offsets) - 1`.
2. We can retrieve the case mapping of a keysyms `ks` with the following
   formula:

    data[
        offsets1[
            offsets2[ks >> (cs_data + cs_offsets)] +
            ((ks >> cs_data) & mask_offsets)
        ] +
        (ks & mask_data)
    ];
"""

from __future__ import annotations

import argparse
import ctypes
import itertools
import math
import os
import re
import sys
import textwrap
from abc import ABCMeta, abstractmethod
from collections import defaultdict
from collections.abc import Callable
from ctypes.util import find_library
from dataclasses import dataclass
from enum import Enum, unique
from functools import cache, reduce
from pathlib import Path
from typing import (
    Any,
    ClassVar,
    Generator,
    Generic,
    Iterable,
    NewType,
    Protocol,
    Self,
    Sequence,
    TypeAlias,
    TypeVar,
    cast,
)
import unicodedata

import icu
import jinja2
import yaml

assert sys.version_info >= (3, 12)

c = icu.Locale.createFromName("C")
icu.Locale.setDefault(c)

SCRIPT = Path(__file__)
CodePoint = NewType("CodePoint", int)
Keysym = NewType("Keysym", int)
KeysymName = NewType("KeysymName", str)

T = TypeVar("T")
X = TypeVar("X", int, CodePoint, Keysym)

################################################################################
# Configuration
################################################################################


@dataclass
class Config:
    check_error: bool
    verbose: bool


################################################################################
# XKBCOMMON
################################################################################


class XKBCOMMON:
    XKB_KEYSYM_MIN = Keysym(0)
    XKB_KEYSYM_MAX = Keysym(0x1FFFFFFF)
    XKB_KEYSYM_MIN_EXPLICIT = Keysym(0x00000000)
    XKB_KEYSYM_MAX_EXPLICIT = Keysym(0x1008FFB8)
    XKB_KEYSYM_UNICODE_OFFSET = Keysym(0x01000000)
    XKB_KEYSYM_UNICODE_MIN = Keysym(0x01000100)
    XKB_KEYSYM_UNICODE_MAX = Keysym(0x0110FFFF)
    XKB_KEY_NoSymbol = Keysym(0)
    xkb_keysym_t = ctypes.c_uint32
    XKB_KEYSYM_NO_FLAGS = 0

    def __init__(self) -> None:
        self._xkbcommon_path = os.environ.get("XKBCOMMON_LIB_PATH")

        if self._xkbcommon_path:
            self._xkbcommon_path = str(Path(self._xkbcommon_path).resolve())
            self._lib = ctypes.cdll.LoadLibrary(self._xkbcommon_path)
        else:
            self._xkbcommon_path = find_library("xkbcommon")
            if self._xkbcommon_path:
                self._lib = ctypes.cdll.LoadLibrary(self._xkbcommon_path)
            else:
                raise OSError("Cannot load libxbcommon")

        self._lib.xkb_keysym_to_lower.argtypes = [self.xkb_keysym_t]
        self._lib.xkb_keysym_to_lower.restype = self.xkb_keysym_t

        self._lib.xkb_keysym_to_upper.argtypes = [self.xkb_keysym_t]
        self._lib.xkb_keysym_to_upper.restype = self.xkb_keysym_t

        self._lib.xkb_keysym_to_utf8.argtypes = [
            self.xkb_keysym_t,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]
        self._lib.xkb_keysym_to_utf8.restype = ctypes.c_int

        self._lib.xkb_keysym_to_utf32.argtypes = [self.xkb_keysym_t]
        self._lib.xkb_keysym_to_utf32.restype = ctypes.c_uint32

        self._lib.xkb_keysym_from_name.argtypes = [ctypes.c_char_p, ctypes.c_int]
        self._lib.xkb_keysym_from_name.restype = self.xkb_keysym_t

        self._lib.xkb_utf32_to_keysym.argtypes = [ctypes.c_uint32]
        self._lib.xkb_utf32_to_keysym.restype = self.xkb_keysym_t

    def keysym_from_name(self, keysym_name: KeysymName) -> Keysym:
        return self._lib.xkb_keysym_from_name(
            keysym_name.encode("utf-8"), self.XKB_KEYSYM_NO_FLAGS
        )

    def keysym_from_cp(self, cp: CodePoint) -> Keysym:
        return self._lib.xkb_utf32_to_keysym(cp)

    def keysym_from_char(self, char: str) -> Keysym:
        return self._lib.xkb_utf32_to_keysym(ord(char))

    def keysym_to_lower(self, keysym: Keysym) -> Keysym:
        return self._lib.xkb_keysym_to_lower(keysym)

    def keysym_to_upper(self, keysym: Keysym) -> Keysym:
        return self._lib.xkb_keysym_to_upper(keysym)

    def keysym_to_str(self, keysym: Keysym) -> str:
        buf_len = 7
        buf = ctypes.create_string_buffer(buf_len)
        n = self._lib.xkb_keysym_to_utf8(keysym, buf, ctypes.c_size_t(buf_len))
        if n < 0:
            raise ValueError(f"Unsupported keysym: 0x{keysym:0>4X})")
        elif n >= buf_len:
            raise ValueError(f"Buffer is not big enough: expected at least {n}.")
        else:
            return buf.value.decode("utf-8")

    def keysym_code_point(self, keysym: Keysym) -> int:
        return self._lib.xkb_keysym_to_utf32(keysym)

    def iter_case_mappings(self, unicode: bool) -> Iterable[tuple[Keysym, Entry]]:
        for keysym in map(
            Keysym,
            range(self.XKB_KEYSYM_MIN_EXPLICIT, self.XKB_KEYSYM_MAX_EXPLICIT + 1),
        ):
            if (
                not unicode
                and self.XKB_KEYSYM_UNICODE_OFFSET
                <= keysym
                <= self.XKB_KEYSYM_UNICODE_MAX
            ) or not self.keysym_code_point(keysym):
                continue
            cp = self.keysym_code_point(keysym)
            yield (
                keysym,
                Entry(
                    lower=self.keysym_to_lower(keysym) - keysym,
                    upper=keysym - self.keysym_to_upper(keysym),
                    # FIXME: should use xkbcommon API, but it’s not exposed
                    is_lower=icu.Char.isULowercase(cp),
                    is_upper=icu.Char.isUUppercase(cp) or icu.istitle(cp),
                ),
            )

    def case_mappings(self, unicode: bool) -> tuple[Entry, ...]:
        mappings = dict(self.iter_case_mappings(unicode))
        keysym_max = max(mappings)
        return tuple(
            mappings.get(cast(Keysym, ks), Entry.zeros())
            for ks in range(0, keysym_max + 1)
        )


xkbcommon = XKBCOMMON()


def load_keysyms(path: Path, unicode: bool) -> tuple[Entry, ...]:
    with path.open("rt", encoding="utf-8") as fd:
        keysyms = {
            ks: entry
            for ks, data in yaml.safe_load(fd).items()
            if unicode
            or (
                ks < XKBCOMMON.XKB_KEYSYM_UNICODE_MIN
                or ks > XKBCOMMON.XKB_KEYSYM_UNICODE_MAX
            )
            if data.get("code point")
            if (
                entry := Entry(
                    data.get("lower", ks) - ks,
                    ks - data.get("upper", ks),
                    is_lower=icu.Char.isULowercase(data.get("code point")),
                    is_upper=icu.Char.isUUppercase(data.get("code point"))
                    or icu.Char.istitle(data.get("code point")),
                )
            )
        }
        # Check either non-cased, upper or lower
        errors = []
        for ks, e in keysyms.items():
            if e.lower and e.upper and e.lower != e.upper:
                errors.append((ks, e))
        if errors:
            raise ValueError(errors)
        keysym_max = max(keysyms)
        return tuple(keysyms.get(ks, Entry.zeros()) for ks in range(0, keysym_max + 1))


################################################################################
# Case mapping
################################################################################


@dataclass(frozen=True, order=True)
class Entry:
    """
    Case mapping deltas for a character or a keysym.
    """

    lower: int
    upper: int
    is_lower: bool
    is_upper: bool
    # [NOTE] Exceptions must be documented in `xkbcommon.h`.
    to_upper_exceptions: ClassVar[dict[str, str]] = {"ß": "ẞ"}
    "Upper mappings exceptions"

    @classmethod
    def zeros(cls) -> Self:
        return cls(lower=0, upper=0, is_lower=False, is_upper=False)

    def __bool__(self) -> bool:
        return any(self) or self.is_lower or self.is_upper

    def __str__(self) -> str:
        return str(tuple(self))

    def __iter__(self) -> Generator[int, None, None]:
        yield self.lower
        yield self.upper

    @classmethod
    def from_code_point(cls, cp: CodePoint) -> Self:
        return cls(
            lower=cls.lower_delta(cp),
            upper=cls.upper_delta(cp),
            is_lower=icu.Char.isULowercase(cp),
            is_upper=icu.Char.isUUppercase(cp) or icu.Char.istitle(cp),
        )

    @classmethod
    def lower_delta(cls, cp: CodePoint) -> int:
        return cls.to_lower_cp(cp) - cp

    @classmethod
    def upper_delta(cls, cp: CodePoint) -> int:
        return cp - cls.to_upper_cp(cp)

    @classmethod
    def to_upper_cp(cls, cp: CodePoint) -> CodePoint:
        if upper := cls.to_upper_exceptions.get(chr(cp)):
            return ord(upper)
        return icu.Char.toupper(cp)

    @staticmethod
    def to_lower_cp(cp: CodePoint) -> CodePoint:
        return icu.Char.tolower(cp)

    @classmethod
    def to_upper_char(cls, char: str) -> str:
        if upper := cls.to_upper_exceptions.get(char):
            return upper
        return icu.Char.toupper(char)

    @staticmethod
    def to_lower_char(char: str) -> str:
        return icu.Char.tolower(char)

    def to_lower(self, x: X) -> X:
        return x.__class__(x + self.lower)

    def to_upper(self, x: X) -> X:
        return x.__class__(x - self.upper)


@dataclass
class Deltas(Generic[T]):
    """
    Sequences of case mappings deltas
    """

    keysyms: tuple[T, ...]
    keysym_max: Keysym
    "Maximum keysym with a case mapping"
    unicode: tuple[T, ...]
    unicode_max: CodePoint
    "Maximum Unicode code point with a case mapping"


class Entries(Deltas[Entry]):
    @classmethod
    def compute(
        cls,
        config: Config,
        path: Path | None = None,
    ) -> Self:
        # Keysyms
        if path:
            keysyms_deltas = load_keysyms(path, unicode=False)
        else:
            keysyms_deltas = xkbcommon.case_mappings(True)
        max_keysym = max(Keysym(ks) for ks, e in enumerate(keysyms_deltas) if e)
        assert max_keysym
        keysyms_deltas = keysyms_deltas[: max_keysym + 1]

        # Unicode
        unicode_deltas = tuple(
            Entry.from_code_point(CodePoint(cp)) for cp in range(0, sys.maxunicode + 1)
        )
        max_unicode = max((CodePoint(cp) for cp, d in enumerate(unicode_deltas) if d))
        assert max_unicode
        unicode_deltas = unicode_deltas[: max_unicode + 1]
        errors = []
        for n, e1 in enumerate(unicode_deltas):
            cp = CodePoint(n)
            # Check with legacy keysyms
            keysym = xkbcommon.keysym_from_cp(cp)
            if keysym <= max_keysym:
                e2 = keysyms_deltas[keysym]
                if e1 or e2:
                    cls.check(
                        config,
                        "lower",
                        cp,
                        e1.to_lower(cp),
                        keysym,
                        e2.to_lower(keysym),
                    )
                    cls.check(
                        config,
                        "upper",
                        cp,
                        e1.to_upper(cp),
                        keysym,
                        e2.to_upper(keysym),
                    )
            # Check character has either
            # • No case mapping
            # • Lower or an upper mapping
            # • Both, with the same delta
            if e1.lower and e1.upper and e1.lower != e1.upper:
                errors.append((cp, e1))
        if errors:
            raise ValueError(errors)
        return cls(
            keysyms=keysyms_deltas,
            keysym_max=max_keysym,
            unicode=unicode_deltas,
            unicode_max=max_unicode,
        )

    @classmethod
    def check(
        cls,
        config: Config,
        casing: str,
        cp: CodePoint,
        cpʹ: CodePoint,
        keysym: Keysym,
        keysymʹ: Keysym,
    ) -> None:
        char = chr(cp)
        expected = chr(cpʹ)
        got = xkbcommon.keysym_to_str(keysymʹ)
        data = (
            hex(keysym),
            hex(cp),
            char,
            hex(keysymʹ),
            got,
            expected,
        )
        if config.check_error:
            assert got == expected, data
        elif got != expected:
            print(
                f"Error: legacy keysym 0x{keysym:4>x} has incorrect {casing} mapping:",
                data,
            )

    @property
    def lower(self) -> Deltas[int]:
        return Deltas(
            keysyms=tuple(e.lower for e in self.keysyms),
            keysym_max=self.keysym_max,
            unicode=tuple(e.lower for e in self.unicode),
            unicode_max=self.unicode_max,
        )

    @property
    def upper(self) -> Deltas[int]:
        return Deltas(
            keysyms=tuple(e.upper for e in self.keysyms),
            keysym_max=self.keysym_max,
            unicode=tuple(e.upper for e in self.unicode),
            unicode_max=self.unicode_max,
        )


@dataclass(frozen=True)
class DeltasPair(Generic[T]):
    d1: tuple[T, ...]
    d2: tuple[T, ...]
    overlap: int

    def __contains__(self, x: tuple[T, ...]) -> bool:
        return (x == self.d1) or (x == self.d2)

    @classmethod
    def compute_pairs_overlaps(
        cls, deltas: Iterable[tuple[T, ...]]
    ) -> Generator[DeltasPair[T], None, None]:
        for d1, d2 in itertools.combinations(deltas, 2):
            if overlap := Overlap.compute_simple(d1, d2):
                yield DeltasPair(d1, d2, overlap)
            if overlap := Overlap.compute_simple(d2, d1):
                yield DeltasPair(d2, d1, overlap)

    def remove_from_pairs(
        self, pairs: list[DeltasPair[T]]
    ) -> Generator[DeltasPair[T], None, None]:
        for p in pairs:
            if p.d1 not in self and p.d2 not in self:
                yield p

    @classmethod
    def remove_chunk_from_pairs(
        cls, pairs: Iterable[DeltasPair[T]], chunk: tuple[T, ...]
    ) -> Generator[DeltasPair[T], None, None]:
        for p in pairs:
            if chunk not in p:
                yield p

    @classmethod
    def compute_best_pair(cls, pairs: Iterable[DeltasPair[T]]) -> DeltasPair[T]:
        return max(pairs, key=lambda p: p.overlap)


################################################################################
# Overlap
################################################################################


@dataclass
class Overlap:
    "Overlap of two sequences"

    offset: int
    overlap: int

    @staticmethod
    @cache
    def compute_simple(s1: Sequence[T], s2: Sequence[T]) -> int:
        return max((n for n in range(0, len(s1) + 1) if s1[-n:] == s2[:n]), default=0)

    @classmethod
    @cache
    def compute(cls, s1: Sequence[T], s2: Sequence[T]) -> Self:
        l1 = len(s1)
        l2 = len(s2)
        return max(
            (
                cls(offset=start, overlap=overlap)
                for start in range(0, l1)
                if (end := min(start + l2, l1))
                and (overlap := end - start)
                and s1[start:end] == s2[:overlap]
            ),
            key=lambda x: x.overlap,
            default=cls(offset=l1, overlap=0),
        )

    @classmethod
    def test(cls) -> None:
        c1 = (1, 2, 3, 4)
        c2 = (2, 3)
        c3 = (3, 2, 1)
        c4 = (3, 4, 5)
        c5 = (1, 2, 4)
        overlap = cls.compute(c1, c2)
        assert overlap == cls(1, 2)
        overlap = cls.compute(c3, c1)
        assert overlap == cls(2, 1)
        overlap = cls.compute(c1, c4)
        assert overlap == cls(2, 2)
        overlap = cls.compute(c1, c5)
        assert overlap == cls(4, 0), overlap


################################################################################
# Groups
################################################################################

Groups: TypeAlias = dict[tuple[T, ...], list[int]]


def generate_groups(block_size: int, data: Iterable[T]) -> Groups[T]:
    groups: defaultdict[tuple[T, ...], list[int]] = defaultdict(list)
    for n, d in enumerate(itertools.batched(data, block_size)):
        groups[d].append(n)
    return groups


################################################################################
# Overlapped Sequences
################################################################################


@dataclass
class OverlappedSequences(Generic[T]):
    data: tuple[T, ...]
    offsets: dict[tuple[T, ...], int]

    def __bool__(self) -> bool:
        return bool(self.data)

    def __hash__(self) -> int:
        return hash((self.data, tuple(self.offsets)))

    def __add__(self, x: Any) -> Self:
        if isinstance(x, self.__class__):
            return self.extend(x)
        elif isinstance(x, tuple):
            return self.add(x)
        else:
            return NotImplemented

    def __iadd__(self, x: Any) -> Self:
        if isinstance(x, self.__class__):
            overlap = Overlap.compute(self.data, x.data)
            self.data = (
                self.data
                if overlap.offset <= len(self.data) - len(x.data)
                else self.data + x.data[overlap.overlap :]
            )
            for c, o in x.offsets.items():
                self.offsets[c] = overlap.offset + o
            return self
        elif isinstance(x, tuple):
            overlap = Overlap.compute(self.data, x)
            self.data = (
                self.data
                if overlap.offset <= len(self.data) - len(x)
                else self.data + x[overlap.overlap :]
            )
            self.offsets[x] = overlap.offset
            return self
        else:
            return NotImplemented

    @classmethod
    def from_singleton(cls, chunk: tuple[T, ...]) -> Self:
        return cls(data=chunk, offsets={chunk: 0})

    @classmethod
    def from_pair(cls, pair: DeltasPair[T]) -> Self:
        return cls(
            data=pair.d1 + pair.d2[pair.overlap :],
            offsets={
                pair.d1: 0,
                pair.d2: len(pair.d1) - pair.overlap,
            },
        )

    @classmethod
    def from_iterable(cls, ts: Iterable[tuple[T, ...]]) -> Self:
        return reduce(lambda s, t: s.add(t), ts, cls((), {}))

    @classmethod
    def from_ordered_iterable(cls, ts: Iterable[tuple[T, ...]]) -> Self:
        return reduce(lambda s, t: s.append(t), ts, cls((), {}))

    def extend(self, s: Self) -> Self:
        overlap: Overlap
        s1: Self
        s2: Self
        overlap, s1, s2 = max(
            (
                (Overlap.compute(self.data, s.data), self, s),
                (Overlap.compute(s.data, self.data), s, self),
            ),
            key=lambda x: x[0].overlap,
        )

        data = (
            s1.data
            if overlap.offset <= len(s1.data) - len(s2.data)
            else s1.data + s2.data[overlap.overlap :]
        )
        offsets = dict(s1.offsets)
        for c, o in s2.offsets.items():
            offsets[c] = overlap.offset + o
        return self.__class__(data=data, offsets=offsets)

    def add(self, chunk: tuple[T, ...]) -> Self:
        return self.extend(self.from_singleton(chunk))

    def append(self, chunk: tuple[T, ...]) -> Self:
        overlap = Overlap.compute(self.data, chunk)
        self.data = (
            self.data
            if overlap.offset <= len(self.data) - len(chunk)
            else self.data + chunk[overlap.overlap :]
        )
        self.offsets[chunk] = overlap.offset
        return self

    def insert(self, offset: int, overlap: int, chunk: tuple[T, ...]) -> None:
        chunk_length = len(chunk)
        if offset < 0:
            self.data = chunk[:-overlap] + self.data
            assert chunk == self.data[:chunk_length]
            for c in self.offsets:
                self.offsets[c] += -offset
            self.offsets[chunk] = 0
        elif offset <= len(self.data) - len(chunk):
            assert self.data[offset : offset + chunk_length] == chunk
            self.offsets[chunk] = offset
        else:
            self.data += chunk[overlap:]
            assert self.data[offset:] == chunk
            self.offsets[chunk] = offset

    def merge(self, offset: int, overlap: int, s: OverlappedSequences[T]) -> None:
        s_length = len(s.data)
        if offset < 0:
            self.data = s.data[:-overlap] + self.data
            assert s.data == self.data[:s_length]
            for c in self.offsets:
                self.offsets[c] += -offset
            self.offsets.update(s.offsets)
        elif offset <= len(self.data) - len(s.data):
            assert self.data[offset : offset + s_length] == s.data
            for c, offset2 in s.offsets.items():
                self.offsets[c] = offset2 + offset
        else:
            self.data += s.data[overlap:]
            assert self.data[offset:] == s.data
            for c, offset2 in s.offsets.items():
                self.offsets[c] = offset2 + offset

    def flatten_iter(self) -> Generator[tuple[T, ...], None, None]:
        for t, _ in sorted(self.offsets.items(), key=lambda x: x[1]):
            yield t

    def flatten(self) -> list[tuple[T, ...]]:
        return list(self.flatten_iter())

    def flatten_groups_iter(self) -> Generator[list[tuple[T, ...]], None, None]:
        i = self.flatten_iter()
        g = [next(i)]
        for t in i:
            if Overlap.compute(g[-1], t).overlap:
                g.append(t)
            else:
                yield g
                g = [t]
        yield g

    def split(self) -> list[OverlappedSequences[T]]:
        offsets = sorted(self.offsets.items(), key=lambda x: x[1])
        start = 0
        pending: dict[tuple[T, ...], int] = {}
        end = 0
        new: list[OverlappedSequences[T]] = []
        for d, i in offsets:
            # print(start, end, d, i)
            if end and i >= end:
                assert start < end
                new.append(
                    OverlappedSequences(data=self.data[start:end], offsets=pending)
                )
                pending = {d: 0}
                start = i
            else:
                pending[d] = i - start
            end = max(end, i + len(d))
        assert start < end
        new.append(OverlappedSequences(data=self.data[start:end], offsets=pending))
        return new

    def total_overlap(self) -> int:
        return sum(len(d) for d in self.offsets) - len(self.data)


class _OverlappedSequences(OverlappedSequences[int]):
    @classmethod
    def test(cls) -> None:
        c1: tuple[int, ...] = (1, 2, 3, 4)
        c2: tuple[int, ...] = (2, 3)
        c3: tuple[int, ...] = (3, 4, 5)
        c4: tuple[int, ...] = (1, 3)
        assert cls(c1, {}).add(c2) == cls(c1, {c2: 1})
        assert cls(c1, {}).add(c3) == cls(c1 + c3[2:], {c3: 2})
        assert cls(c1, {}).add(c4) == cls(c1 + c4, {c4: len(c1)})

        assert cls(c1, {}) + cls(c2, {c2: 0}) == cls(c1, {c2: 1})
        assert cls(c1, {}) + cls(c3, {c3: 0}) == cls(c1 + c3[2:], {c3: 2})
        assert cls(c1, {}) + cls(c4, {c4: 0}) == cls(c1 + c4, {c4: len(c1)})

        assert cls(c2, {c2: 0}) + cls(c1, {}) == cls(c1, {c2: 1})
        assert cls(c3, {c3: 0}) + cls(c1, {}) == cls(c1 + c3[2:], {c3: 2})
        assert cls(c4, {c4: 0}) + cls(c1, {}) == cls(c4 + c1, {c4: 0})

        c5 = (1, 2, 3, 4, 5, 6)
        c6 = (1,)
        c7 = (6,)
        s = cls(c5, {c6: 0, c2: 1, c3: 2, c7: 5})
        assert s.total_overlap() == 1, s.total_overlap()
        assert s.split() == [
            cls(c6, {c6: 0}),
            cls(c5[1:-1], {c2: 0, c3: 1}),
            cls(c7, {c7: 0}),
        ], s.split()

        cs = s.flatten()
        assert cs == [c6, c2, c3, c7], cs
        assert cls.from_iterable(cs) == s


################################################################################
# Chunks compressor
################################################################################


class Move(Protocol, Generic[T]):
    def __call__(
        self,
        pairs: list[DeltasPair[T]],
        remaining_chunks: set[tuple[T, ...]],
        overlapped_sequences: list[OverlappedSequences[T]],
    ) -> None: ...


@dataclass
class ChunksCompressor:
    verbose: bool

    @classmethod
    def _insert(
        cls, chunk: tuple[T, ...], index: int, offset: int, overlap: int
    ) -> Move[T]:
        def action(
            pairs: list[DeltasPair[T]],
            remaining_chunks: set[tuple[T, ...]],
            overlapped_sequences: list[OverlappedSequences[T]],
        ) -> None:
            remaining_chunks.remove(chunk)
            pairs[:] = DeltasPair.remove_chunk_from_pairs(pairs, chunk)
            s = overlapped_sequences[index]
            s.insert(offset, overlap, chunk)

        return action

    @classmethod
    def _merge(cls, index1: int, index2: int, offset: int, overlap: int) -> Move[T]:
        def action(
            pairs: list[DeltasPair[T]],
            remaining_chunks: set[tuple[T, ...]],
            overlapped_sequences: list[OverlappedSequences[T]],
        ) -> None:
            s1 = overlapped_sequences[index1]
            s2 = overlapped_sequences.pop(index2)
            s1.merge(offset, overlap, s2)

        return action

    @classmethod
    def _add_new_singleton(cls, chunk: tuple[T, ...]) -> Move[T]:
        def action(
            pairs: list[DeltasPair[T]],
            remaining_chunks: set[tuple[T, ...]],
            overlapped_sequences: list[OverlappedSequences[T]],
        ) -> None:
            remaining_chunks.remove(chunk)
            pairs[:] = DeltasPair.remove_chunk_from_pairs(pairs, chunk)
            overlapped_sequences.append(OverlappedSequences.from_singleton(chunk))

        return action

    @classmethod
    def _add_new_sequence(cls, pair: DeltasPair[T]) -> Move[T]:
        def action(
            pairs: list[DeltasPair[T]],
            remaining_chunks: set[tuple[T, ...]],
            overlapped_sequences: list[OverlappedSequences[T]],
        ) -> None:
            remaining_chunks.remove(pair.d1)
            remaining_chunks.remove(pair.d2)
            pairs[:] = pair.remove_from_pairs(pairs)
            assert pair not in pairs
            overlapped_sequences.append(OverlappedSequences.from_pair(pair))

        return action

    def compress(self, chunks: Iterable[tuple[T, ...]]) -> OverlappedSequences[T]:
        # Prepare data and offsets
        pairs = list(DeltasPair.compute_pairs_overlaps(chunks))

        remaining_chunks = set(chunks)

        if self.verbose:
            print(
                "Count of chunks:",
                len(remaining_chunks),
            )

        overlapped_sequences: list[OverlappedSequences[T]] = []
        best_move: Move[T] | None = None
        total_chunks = len(remaining_chunks)
        while remaining_chunks or (len(overlapped_sequences) > 1 and best_move):
            if self.verbose:
                print(
                    f"# Remaining: {len(remaining_chunks)}/{total_chunks}",
                    "Current pairs:",
                    len(overlapped_sequences),
                )
            best_move = None
            best_overlap: int = 0
            # Try inserting
            for n, s in enumerate(overlapped_sequences):
                for d in remaining_chunks:
                    # Try prepend
                    overlap = Overlap.compute_simple(d, s.data)
                    if overlap > best_overlap:
                        best_overlap = overlap
                        best_move = self._insert(d, n, overlap - len(d), overlap)
                        if self.verbose:
                            print(
                                "Insert chunk (prepend)",
                                best_overlap,
                                len(overlapped_sequences),
                            )
                    # Try insert
                    overlap_max = Overlap.compute(s.data, d)
                    if overlap_max.overlap > best_overlap:
                        best_overlap = overlap_max.overlap
                        best_move = self._insert(
                            d, n, overlap_max.offset, overlap_max.overlap
                        )
                        if self.verbose:
                            print(
                                "Insert chunk (append)",
                                overlap_max,
                                len(overlapped_sequences),
                            )
            # Try merging
            for n1, n2 in itertools.permutations(range(len(overlapped_sequences)), 2):
                s1 = overlapped_sequences[n1]
                s2 = overlapped_sequences[n2]
                # Try insert
                overlap_max = Overlap.compute(s1.data, s2.data)
                if overlap_max.overlap > best_overlap:
                    best_overlap = overlap_max.overlap
                    best_move = self._merge(
                        n1, n2, overlap_max.offset, overlap_max.overlap
                    )
                    if self.verbose:
                        print(
                            "Merge 2 sequences",
                            overlap_max,
                            len(overlapped_sequences),
                        )

            # Take next best pair
            if (
                pairs
                and (best_pair := DeltasPair.compute_best_pair(pairs))
                and best_pair.overlap > best_overlap
            ):
                best_overlap = best_pair.overlap
                best_move = self._add_new_sequence(best_pair)
                if self.verbose:
                    print("Add new sequence", best_overlap, len(overlapped_sequences))
            if not best_move:
                if self.verbose:
                    print("No best move", len(overlapped_sequences))
                if remaining_chunks:
                    best_move = self._add_new_singleton(min(remaining_chunks))
            if best_move:
                best_move(
                    pairs=pairs,
                    remaining_chunks=remaining_chunks,
                    overlapped_sequences=overlapped_sequences,
                )

        assert len(overlapped_sequences) >= 1
        assert not remaining_chunks

        if (l := len(overlapped_sequences)) > 1:
            if self.verbose:
                print("Force merging remaining sequences", l)

            for n1, n2 in itertools.permutations(range(len(overlapped_sequences)), 2):
                s1 = overlapped_sequences[n1]
                s2 = overlapped_sequences[n2]
                overlap_max = Overlap.compute(s1.data, s2.data)
                assert overlap_max.overlap == 0, overlap_max

        for _ in range(1, len(overlapped_sequences)):
            move: Move[T] = self._merge(0, 1, len(overlapped_sequences[0].data), 0)
            move(
                pairs=pairs,
                remaining_chunks=remaining_chunks,
                overlapped_sequences=overlapped_sequences,
            )

        assert len(overlapped_sequences) == 1, overlapped_sequences

        s = overlapped_sequences[0]

        return s

    @classmethod
    def test_moves(cls) -> None:
        c1 = (1, 2, 3, 4, 5)
        c2 = (6, 1)
        c3 = (7, 6, 1)
        c4 = (3, 4)
        c5 = (5, 6, 7)
        c6 = (6, 1, 2)
        c7 = (8, 7, 6)
        chunks0 = {c1, c2, c3, c4, c5, c6, c7}
        chunks = set(chunks0)
        sequences: list[OverlappedSequences[int]] = []
        offsets: dict[tuple[int, ...], int]

        overlap_max = Overlap.compute((1, 2, 3), c1)
        assert overlap_max == Overlap(offset=0, overlap=3)
        pairs: list[DeltasPair[int]] = []

        move = ChunksCompressor._add_new_singleton(c1)
        move(pairs, chunks, sequences)
        offsets = {c1: 0}
        data: tuple[int, ...] = c1
        assert len(sequences) == 1
        s = sequences[0]
        assert s.data == data
        assert s.offsets == offsets

        overlap = Overlap.compute_simple(c2, s.data)
        assert overlap == 1
        overlap_max = Overlap.compute(c2, s.data)
        assert overlap_max.overlap == overlap
        move = ChunksCompressor._insert(
            chunk=c2, index=0, offset=-overlap, overlap=overlap
        )
        move(pairs, chunks, sequences)
        offsets[c1] = 1
        offsets[c2] = 0
        data = c2[:-overlap] + data
        assert len(sequences) == 1
        assert s.data == data
        assert s.offsets == offsets

        overlap = Overlap.compute_simple(c3, s.data)
        assert overlap == 2
        overlap_max = Overlap.compute(c3, s.data)
        assert overlap_max.overlap == overlap
        move = ChunksCompressor._insert(
            chunk=c3, index=0, offset=overlap - len(c3), overlap=overlap
        )
        move(pairs, chunks, sequences)
        offsets[c1] += 1
        offsets[c2] += 1
        offsets[c3] = 0
        data = c3[:-overlap] + data
        assert len(sequences) == 1
        assert s.data == data
        assert s.offsets == offsets

        overlap_max = Overlap.compute(s.data, c4)
        assert overlap_max.overlap == 2, (s.data, c4, overlap_max)
        assert overlap_max.offset == 4, (s.data, c4, overlap_max)
        move = ChunksCompressor._insert(
            chunk=c4, index=0, offset=overlap_max.offset, overlap=overlap_max.overlap
        )
        move(pairs, chunks, sequences)
        offsets[c4] = overlap_max.offset
        assert len(sequences) == 1
        assert s.data == data
        assert s.offsets == offsets

        overlap_max = Overlap.compute(s.data, c5)
        assert overlap_max.overlap == 1, (s.data, c5, overlap_max)
        assert overlap_max.offset == 6, (s.data, c5, overlap_max)
        move = ChunksCompressor._insert(
            chunk=c5, index=0, offset=overlap_max.offset, overlap=overlap_max.overlap
        )
        move(pairs, chunks, sequences)
        offsets[c5] = overlap_max.offset
        data += c5[overlap_max.overlap :]
        assert len(sequences) == 1
        assert s.data == data, (s.data, data)
        assert s.offsets == offsets

        overlap_max = Overlap.compute(s.data, c6)
        assert overlap_max.overlap == 3, (s.data, c6, overlap_max)
        assert overlap_max.offset == 1, (s.data, c6, overlap_max)
        move = ChunksCompressor._insert(
            chunk=c6, index=0, offset=overlap_max.offset, overlap=overlap_max.overlap
        )
        move(pairs, chunks, sequences)
        offsets[c6] = overlap_max.offset
        assert len(sequences) == 1
        assert s.data == data, (s.data, data)
        assert s.offsets == offsets

        overlap = Overlap.compute_simple(c7, s.data)
        assert overlap == 2
        overlap_max = Overlap.compute(c7, s.data)
        assert overlap_max.overlap == overlap
        move = ChunksCompressor._insert(
            chunk=c7, index=0, offset=overlap - len(c7), overlap=overlap
        )
        move(pairs, chunks, sequences)
        for c in offsets:
            offsets[c] += len(c7) - overlap
        offsets[c7] = 0
        data = c7[:-overlap] + data
        assert len(sequences) == 1
        assert s.data == data
        assert s.offsets == offsets

        for c in chunks0:
            offset = s.offsets[c]
            assert c == s.data[offset : offset + len(c)], (
                c,
                s.data[offset : offset + len(c)],
            )

    @classmethod
    def test_compression(cls) -> None:
        c1 = (1, 2, 3)
        c2 = (-1, 0, 1)
        c3 = (-2, -1, 0)
        c4 = (3, 4, 5)
        c5 = (0, 1, 2)
        c6 = (2, 3, 5)
        chunks = {c1, c2, c3, c4, c5, c6}
        compressor = cls(verbose=True)
        r = compressor.compress(chunks)
        assert set(r.offsets) == {c1, c2, c3, c4, c5, c6}
        for c in chunks:
            offset = r.offsets[c]
            assert c == r.data[offset : offset + len(c)], (
                c,
                r.data[offset : offset + len(c)],
            )


################################################################################
# Stats
################################################################################


@dataclass
class Stats:
    data_length: int
    data_int_size: int
    data_overlap: int
    offsets1_length: int
    offsets1_int_size: int
    offsets2_length: int
    offsets2_int_size: int

    @property
    def data_size(self) -> int:
        return self.data_length * self.data_int_size

    @property
    def offsets1_size(self) -> int:
        return self.offsets1_length * self.offsets1_int_size

    @property
    def offsets2_size(self) -> int:
        return self.offsets2_length * self.offsets2_int_size

    @property
    def total(self) -> int:
        return self.data_size + self.offsets1_size + self.offsets2_size


def _int_size(ts: Sequence[Iterable[int] | int], offset: int = 0) -> int:
    assert ts
    if isinstance(ts[0], int):
        ts = cast(Sequence[int], ts)
        min_delta: int = min(ts)
        max_delta: int = max(ts)
    else:
        ts = cast(Sequence[Iterable[int]], ts)
        min_delta = min(min(t) for t in ts)
        max_delta = max(max(t) for t in ts)
    if offset:
        min_delta <<= offset
        max_delta <<= offset
    for n in range(3, 7):
        size: int = 2**n
        min_int = -(1 << (size - 1))
        max_int = -min_int - 1
        if min_int <= min_delta and max_delta <= max_int:
            return size
    else:
        raise ValueError((min_delta, max_delta))


def uint_size(ts: Sequence[int]) -> int:
    min_delta = min(ts)
    if min_delta < 0:
        raise ValueError(min_delta)
    max_delta = max(ts)
    for n in range(3, 7):
        size: int = 2**n
        max_int = (1 << size) - 1
        if max_delta <= max_int:
            return size
    else:
        raise ValueError((min_delta, max_delta))


################################################################################
# Compressed array
################################################################################

I = TypeVar("I", Entry, int)


@dataclass
class CompressedArray(Generic[I]):
    data: tuple[I, ...]
    offsets: tuple[int, ...]
    chunk_offsets: dict[tuple[I, ...], int]

    @classmethod
    def from_overlapped_sequences(
        cls, s: OverlappedSequences[I], groups: Groups[I]
    ) -> Self:
        offsets = tuple(
            s.offsets[d]
            for _, d in sorted(
                ((g, d0) for d0, gs in groups.items() for g in gs),
                key=lambda x: x[0],
            )
        )
        return cls(data=s.data, offsets=offsets, chunk_offsets=s.offsets)

    def total_overlap(self) -> int:
        return sum(len(d) for d in self.chunk_offsets) - len(self.data)

    def stats(self, int_size: Callable[[Sequence[Iterable[int] | int]], int]) -> Stats:
        return Stats(
            data_length=len(self.data),
            data_int_size=int_size(self.data),
            data_overlap=self.total_overlap(),
            offsets1_length=len(self.offsets),
            offsets1_int_size=_int_size(self.offsets),
            offsets2_length=0,
            offsets2_int_size=0,
        )

    @staticmethod
    def test() -> None:
        c1 = (1, 2, 3, 4)
        c2 = (2, 3)
        c3 = (3, 4, 5)
        c4 = (1, 3)
        s = OverlappedSequences.from_singleton(c1)
        s += c2
        s += c3
        s += c4
        groups: Groups[int] = {c1: [0, 3], c2: [4], c3: [1, 2]}
        a = CompressedArray.from_overlapped_sequences(s, groups)
        assert a == CompressedArray(
            data=s.data, offsets=(0, 2, 2, 0, 1), chunk_offsets=s.offsets
        ), a


@dataclass
class ArrayCompressor:
    compressor: ChunksCompressor
    verbose: bool

    def run(self, groups: Groups[I]) -> CompressedArray[I]:
        s = self.compressor.compress(groups)
        return CompressedArray.from_overlapped_sequences(s, groups)

    @classmethod
    def compress(
        cls,
        compressor: ChunksCompressor,
        block_size: int,
        data: Iterable[I],
        default: I,
        verbose: bool = False,
    ) -> CompressedArray[I]:
        groups = generate_groups(block_size=block_size, data=data)
        array_compressor = cls(compressor=compressor, verbose=verbose)
        return array_compressor.run(groups)

    @classmethod
    def test_compression(cls) -> None:
        c1 = (1, 2, 3)
        c2 = (-1, 0, 1)
        c3 = (-2, -1, 0)
        c4 = (3, 4, 5)
        c5 = (0, 1, 2)
        c6 = (2, 3, 5)
        groups: Groups[int] = {
            c1: [0],
            c2: [1],
            c3: [2],
            c4: [3],
            c5: [4],
            c6: [5],
        }
        compressor = cls(
            compressor=ChunksCompressor(verbose=True),
            verbose=True,
        )
        r = compressor.run(groups)
        assert set(r.chunk_offsets) == {c1, c2, c3, c4, c5, c6}
        for c, gs in groups.items():
            for g in gs:
                offset = r.offsets[g]
                assert c == r.data[offset : offset + len(c)], (
                    c,
                    r.data[offset : offset + len(c)],
                )


################################################################################
# Solutions
################################################################################


@dataclass
class SimpleSolution(Generic[T]):
    data_block_size_log2: int
    data_int_size: int
    data_overlap: int
    data: tuple[T, ...]
    offsets1_block_size_log2: int
    offsets1_int_size: int
    offsets1: tuple[int, ...]
    offsets2_int_size: int
    offsets2: tuple[int, ...]
    max: int
    total: int

    @classmethod
    def zeros(cls) -> Self:
        return cls(
            data_block_size_log2=0,
            data_int_size=0,
            data_overlap=0,
            data=(),
            offsets1_block_size_log2=0,
            offsets1_int_size=0,
            offsets1=(),
            offsets2_int_size=0,
            offsets2=(),
            max=0,
            total=0,
        )

    @property
    def case(self) -> str:
        return "both"

    @property
    def data_size(self) -> int:
        return len(self.data) * self.data_int_size

    def _convert(self, op: Callable[[X, T], X], x: X) -> X:
        mask1 = (1 << self.data_block_size_log2) - 1
        mask2 = (1 << self.offsets1_block_size_log2) - 1
        cpʹ = x >> self.data_block_size_log2
        offset = self.offsets2[cpʹ >> self.offsets1_block_size_log2] + (cpʹ & mask2)
        return op(x, self.data[self.offsets1[offset] + (x & mask1)])


class SimpleSolutionCombinedMappings(SimpleSolution[Entry], metaclass=ABCMeta):
    @property
    @abstractmethod
    def type(self) -> str: ...

    def to_lower(self, x: X) -> X:
        return self._convert(lambda a, e: e.to_lower(a), x)

    def to_upper(self, x: X) -> X:
        return self._convert(lambda a, e: e.to_upper(a), x)


class SimpleSolutionKeysymsCombinedMappings(SimpleSolutionCombinedMappings):
    @property
    def type(self) -> str:
        return "legacy_keysyms"

    def test(self, config: Config) -> bool:
        # Check legacy keysyms
        r = True
        for ks in map(Keysym, range(0, self.max + 1)):
            if not (char := xkbcommon.keysym_to_str(ks)):
                continue
            cp = ord(char)

            # Lower
            if ks <= self.max:
                expected = Entry.to_lower_char(char)
                ksʹ = xkbcommon.keysym_from_char(expected)
                ksʹʹ = self.to_lower(ks)
                got = xkbcommon.keysym_to_str(ksʹʹ)
                ok = got == expected
                data = (
                    hex(ks),
                    hex(cp),
                    char,
                    expected,
                    char.lower(),
                    hex(ksʹ),
                    hex(ksʹʹ),
                    got,
                )
                if config.check_error:
                    assert ok, data
                elif not ok:
                    print("Error:", data)
                    r = False

            # Upper
            if ks <= self.max:
                expected = Entry.to_upper_char(char)
                ksʹ = xkbcommon.keysym_from_char(expected)
                ksʹʹ = self.to_upper(ks)
                got = xkbcommon.keysym_to_str(ksʹʹ)
                ok = got == expected
                data = (
                    hex(ks),
                    hex(cp),
                    char,
                    expected,
                    char.upper(),
                    hex(ksʹ),
                    hex(ksʹʹ),
                    got,
                )
                if config.check_error:
                    assert ok, data
                elif not ok:
                    print("Error:", data)
                    r = False
        return r


class SimpleSolutionUnicodeCombinedMappings(SimpleSolutionCombinedMappings):
    @property
    def type(self) -> str:
        return "unicode"

    def test(self, config: Config) -> bool:
        # Check Unicode keysyms
        unicode_min = (
            xkbcommon.XKB_KEYSYM_UNICODE_MIN - xkbcommon.XKB_KEYSYM_UNICODE_OFFSET
        )
        r = True
        for cp in map(CodePoint, range(unicode_min, self.max + 1)):
            char = chr(cp)
            # Lower
            if cp <= self.max:
                expected = Entry.to_lower_char(char)
                got = chr(self.to_lower(cp))
                ok = got == expected
                dataʹ = (hex(cp), char, expected, char.lower(), got)
                if config.check_error:
                    assert ok, dataʹ
                elif not ok:
                    print("Error:", dataʹ)
                    r = False
            # Upper
            if cp <= self.max:
                expected = Entry.to_upper_char(char)
                got = chr(self.to_upper(cp))
                ok = got == expected
                dataʹ = (hex(cp), char, expected, char.upper(), got)
                if config.check_error:
                    assert ok, dataʹ
                elif not ok:
                    print("Error:", dataʹ)
                    r = False
        return r


S = TypeVar("S", bound=SimpleSolutionCombinedMappings)


@dataclass
class SeparateLegacyKeysymsAndUnicodeCombinedCaseMappings:
    legacy_keysyms: SimpleSolutionKeysymsCombinedMappings
    unicode: SimpleSolutionUnicodeCombinedMappings

    def __iter__(self) -> Generator[SimpleSolutionCombinedMappings, None, None]:
        yield self.legacy_keysyms
        yield self.unicode

    @property
    def total(self) -> int:
        return sum(s.total for s in self)

    def test(self, config: Config) -> bool:
        r = True
        if self.legacy_keysyms.data:
            r &= self.legacy_keysyms.test(config)
        if self.unicode.data:
            r &= self.unicode.test(config)
        return r

    @classmethod
    def optimize_groups(
        cls,
        data: tuple[Entry, ...],
        data_max: int,
        total0: int,
        config: Config,
        cls_: type[S],
    ) -> S:
        chunks_compressor = ChunksCompressor(verbose=config.verbose)
        total = total0
        max_int_size = sys.maxsize
        solution: S | None = None
        for k1 in range(1, 9):
            block_size1 = 2**k1
            if config.verbose:
                print("".center(80, "-"))
                print(f"{block_size1=} Step 1: Generating 1st compression level…")
            ca1 = ArrayCompressor.compress(
                compressor=chunks_compressor,
                block_size=block_size1,
                data=data,
                default=Entry.zeros(),
                verbose=config.verbose,
            )
            stats1 = ca1.stats(int_size=lambda x: _int_size(x, offset=2))
            if config.verbose:
                print("Step 1 done.")
                print(
                    f"{block_size1=} Step 1:",
                    f"data_overlap={stats1.data_overlap} data_length={stats1.data_length} data_size={stats1.data_size}",
                    f"total={stats1.total}",
                )
            current_total = stats1.total
            if config.verbose:
                print(f"{current_total=}")
            if (data_size := stats1.data_size) >= total:
                if config.verbose:
                    print(f"We cannot beat the current solution {data_size=} {total=}")
                continue
            current_int_size = max(stats1.data_int_size, stats1.offsets1_int_size)
            if current_total <= total and (
                current_total != total or current_int_size < max_int_size
            ):
                total = current_total
                max_int_size = current_int_size
                if config.verbose:
                    print(f"✨ New best solution: {block_size1=} {total=}")
                # TODO: 1st level solution
            for k2 in range(7, 1, -1):
                block_size2 = 2**k2
                if config.verbose:
                    print(
                        f"{block_size1=} {block_size2=} Step 2: 2nd level compression… ",
                        flush=True,
                    )
                ca2 = ArrayCompressor.compress(
                    compressor=chunks_compressor,
                    block_size=block_size2,
                    data=ca1.offsets,
                    default=0,
                    verbose=config.verbose,
                )
                stats2 = ca2.stats(int_size=_int_size)
                stats = Stats(
                    data_length=stats1.data_length,
                    data_int_size=stats1.data_int_size,
                    data_overlap=stats1.data_overlap,
                    offsets1_length=stats2.data_length,
                    offsets1_int_size=stats2.data_int_size,
                    offsets2_length=stats2.offsets1_length,
                    offsets2_int_size=stats2.offsets1_int_size,
                )
                current_total = stats.total
                if config.verbose:
                    print(
                        f"{block_size1=} {block_size2=} Step 2:",
                        f"data_overlap={stats.data_overlap} data_length={stats.data_length} data_size={stats.data_size}",
                        f"total={stats.total}",
                    )
                    print("Step 2 done.", flush=True)
                current_int_size = max(
                    stats.data_int_size,
                    stats.offsets1_int_size,
                    stats.offsets2_int_size,
                )
                if current_total < total or (
                    current_total == total and current_int_size < max_int_size
                ):
                    total = current_total
                    max_int_size = current_int_size
                    if config.verbose:
                        print(
                            f"✨ New best solution: {block_size1=} {block_size2=} {total=}"
                        )
                    solution = cls_(
                        data_block_size_log2=k1,
                        data_int_size=stats.data_int_size,
                        data=ca1.data,
                        data_overlap=stats.data_overlap,
                        offsets1_block_size_log2=k2,
                        offsets1=ca2.data,
                        offsets1_int_size=stats.offsets1_int_size,
                        offsets2=ca2.offsets,
                        offsets2_int_size=stats.offsets2_int_size,
                        max=data_max,
                        total=stats.total,
                    )
        assert solution
        if config.verbose:
            print(" Finished ".center(80, "*"))
            print(
                f"Best solution ({solution.case}):",
                f"data_block_size={2**solution.data_block_size_log2}",
                f"data_overlap={solution.data_overlap}",
                f"offsets1_block_size={2**solution.offsets1_block_size_log2}",
                f"total={solution.total}",
            )
            print("Total:", solution.total)
            print("".center(80, "*"))
        return solution

    @classmethod
    def optimize(
        cls,
        config: Config,
        path: Path | None = None,
    ) -> Self:
        print("Computing deltas… ", end="", flush=True)
        entries = Entries.compute(config=config, path=path)
        print("Done.", flush=True)
        keysyms = cls.optimize_groups(
            config=config,
            data=entries.keysyms,
            data_max=entries.keysym_max,
            total0=32 * len(entries.keysyms),
            cls_=SimpleSolutionKeysymsCombinedMappings,
        )
        unicode = cls.optimize_groups(
            config=config,
            data=entries.unicode,
            data_max=entries.unicode_max,
            total0=32 * len(entries.unicode),
            cls_=SimpleSolutionUnicodeCombinedMappings,
        )
        result = cls(legacy_keysyms=keysyms, unicode=unicode)
        print(" Finished ".center(90, "*"))
        for s in result:
            if s.data:
                print(
                    f"✨ {s.type}:",
                    f"max: 0x{s.max:0>4x}",
                    f"data_block_size={2**s.data_block_size_log2}",
                    f"offsets_block_size={2**s.offsets1_block_size_log2}",
                    f"data={len(s.data)} (int size: {s.data_int_size})",
                    f"data_overlap={s.data_overlap}",
                    f"offsets1={len(s.offsets1)} (uint size: {s.offsets1_int_size})",
                    f"offsets2={len(s.offsets2)} (uint size: {s.offsets2_int_size})",
                    f"total={s.total} ({s.total // 8})",
                )
        print(f"Total: {result.total} ({result.total // 8})")
        print("".center(90, "*"))
        return result

    @classmethod
    def generate(cls, root: Path, write: bool, config: Config) -> None:
        s = cls.optimize(
            config=config,
            path=root / "data/keysyms.yaml",
        )
        s.test(config)
        if write:
            s.write(root)

    MAPPING_TEMPLATE = """\
static const struct CaseMappings {prefix}data[{data_length}] = {{
    {data}
}};

static const uint{offsets1_int_size}_t {prefix}offsets1[{offsets1_length}] = {{
    {offsets1}
}};

static const uint{offsets2_int_size}_t {prefix}offsets2[{offsets2_length}] = {{
    {offsets2}
}};

static inline const struct CaseMappings *
get_{prefix}entry(xkb_keysym_t ks)
{{
    return &{prefix}data[{prefix}offsets1[{prefix}offsets2[ks >> {k12}] + ((ks >> {k1}) & 0x{mask2:0>2x})] + (ks & 0x{mask1:0>2x})];
}}\
"""

    TEMPLATE = """\
// NOTE: This file has been generated automatically by “{python_script}”.
//       Do not edit manually!

/*
 * Copyright © 2024 Pierre Le Marre
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Case mappings for Unicode {unicode_version}
 *
{doc}
 */

#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "xkbcommon/xkbcommon.h"
#include "utils.h"
#include "keysym.h"

struct CaseMappings{{
    bool lower:1;
    bool upper:1;
    int{data_int_size}_t offset:{data_int_size_with_offset};
}};

{legacy_keysyms_mappings}

{unicode_mappings}

XKB_EXPORT xkb_keysym_t
xkb_keysym_to_lower(xkb_keysym_t ks)
{{
    if (ks <= 0x{max_non_unicode:0>4x}) {{
        const struct CaseMappings *m = get_legacy_keysym_entry(ks);
        return (m->lower) ? ks + m->offset : ks;
    }} else if ({min_unicode} <= ks && ks <= 0x{max_unicode:0>8x}) {{
        const struct CaseMappings *m = get_unicode_entry(ks - XKB_KEYSYM_UNICODE_OFFSET);
        if (m->lower) {{
            ks = ks + m->offset;
            return (ks < {min_unicode}) ? ks - XKB_KEYSYM_UNICODE_OFFSET : ks;
        }} else {{
            return ks;
        }}
    }} else {{
        return ks;
    }}
}}

XKB_EXPORT xkb_keysym_t
xkb_keysym_to_upper(xkb_keysym_t ks)
{{
    if (ks <= 0x{max_non_unicode:0>4x}) {{
        const struct CaseMappings *m = get_legacy_keysym_entry(ks);
        return (m->upper) ? ks - m->offset : ks;
    }} else if ({min_unicode} <= ks && ks <= 0x{max_unicode:0>8x}) {{
        const struct CaseMappings *m = get_unicode_entry(ks - XKB_KEYSYM_UNICODE_OFFSET);
        if (m->upper) {{
            ks = ks - m->offset;
            return (ks < {min_unicode}) ? ks - XKB_KEYSYM_UNICODE_OFFSET : ks;
        }} else {{
            return ks;
        }}
    }} else {{
        return ks;
    }}
}}

bool
xkb_keysym_is_lower(xkb_keysym_t ks)
{{
    /* This predicate matches keysyms with their corresponding Unicode code point
     * having the Unicode property “Lowercase”.
     *
     * Here: a keysym is lower case if it has an upper case and no lower case.
     * Note: title case letters may have both. Example for U+01F2:
     * • U+01F1 Ǳ: upper case
     * • U+01F2 ǲ: title case
     * • U+01F3 ǳ: lower case
     */
    if (ks <= 0x{max_non_unicode:0>4x}) {{
        const struct CaseMappings *m = get_legacy_keysym_entry(ks);
        return m->upper && !m->lower;
    }} else if ({min_unicode} <= ks && ks <= 0x{max_unicode:0>8x}) {{
        const struct CaseMappings *m = get_unicode_entry(ks - XKB_KEYSYM_UNICODE_OFFSET);
        return m->upper && !m->lower;
    }} else {{
        return false;
    }}
}}

bool
xkb_keysym_is_upper_or_title(xkb_keysym_t ks)
{{
    /* This predicate matches keysyms with their corresponding Unicode code point
     * having the Unicode properties “Uppercase” or General Category “Lt”.
     *
     * Here: a keysym is upper case or title case if it has a lower case. */
    if (ks <= 0x{max_non_unicode:0>4x}) {{
        return get_legacy_keysym_entry(ks)->lower;
    }} else if ({min_unicode} <= ks && ks <= 0x{max_unicode:0>8x}) {{
        return get_unicode_entry(ks - XKB_KEYSYM_UNICODE_OFFSET)->lower;
    }} else {{
        return false;
    }}
}}
"""

    @staticmethod
    def myHex(n: int, max_hex_length: int) -> str:
        return (
            f"""{"-" if n < 0 else " "}0x{hex(abs(n))[2:].rjust(max_hex_length, "0")}"""
        )

    @classmethod
    def make_char_case_mappings(cls, e: Entry, max_hex_length: int) -> str:
        # If the entry has both lower and upper mappings, they must be equal.
        if e.lower and e.upper and e.lower != e.upper:
            raise ValueError(e)
        has_lower = e.is_upper or bool(e.lower)
        has_upper = e.is_lower or bool(e.upper)
        return f"{{{int(has_lower)}, {int(has_upper)},{cls.myHex(e.lower or e.upper, max_hex_length)}}}"

    def generate_mapping(self, prefix: str, sol: SimpleSolutionCombinedMappings) -> str:
        if not sol.offsets1_block_size_log2:
            raise ValueError(f"generate_mapping: {prefix}")
        max_data = max(max(abs(e.lower), abs(e.upper)) for e in sol.data)
        max_hex_length = 1 + (int(math.log2(max_data)) >> 2)
        return self.MAPPING_TEMPLATE.format(
            prefix=prefix,
            data_length=len(sol.data),
            data=",\n    ".join(
                ", ".join(
                    "".join(self.make_char_case_mappings(x, max_hex_length)) for x in xs
                )
                for xs in itertools.batched(sol.data, 4)
            ),
            offsets1_int_size=sol.offsets1_int_size,
            offsets1_length=len(sol.offsets1),
            offsets1=",\n    ".join(
                ", ".join(f"0x{x:0>4x}" for x in xs)
                for xs in itertools.batched(sol.offsets1, 10)
            ),
            offsets2_int_size=sol.offsets2_int_size,
            offsets2_length=len(sol.offsets2),
            offsets2=",\n    ".join(
                ", ".join(f"0x{x:0>4x}" for x in xs)
                for xs in itertools.batched(sol.offsets2, 10)
            ),
            k1=sol.data_block_size_log2,
            k2=sol.offsets1_block_size_log2,
            k12=sol.data_block_size_log2 + sol.offsets1_block_size_log2,
            mask1=(1 << sol.data_block_size_log2) - 1,
            mask2=(1 << sol.offsets1_block_size_log2) - 1,
        )

    def write(self, root: Path) -> None:
        path = root / "src/keysym-case-mappings.c"
        keysyms = self.generate_mapping("legacy_keysym_", self.legacy_keysyms)
        unicode = self.generate_mapping("unicode_", self.unicode)
        assert self.legacy_keysyms.data_int_size == self.unicode.data_int_size
        doc = textwrap.indent(
            textwrap.indent(__doc__.strip(), prefix=" "),
            prefix=" *",
            predicate=lambda _: True,
        )
        content = self.TEMPLATE.format(
            python_script=Path(__file__).name,
            unicode_version=icu.UNICODE_VERSION,
            doc=doc,
            data_int_size=self.legacy_keysyms.data_int_size,
            data_int_size_with_offset=self.legacy_keysyms.data_int_size - 2,
            legacy_keysyms_mappings=keysyms,
            unicode_mappings=unicode,
            max_non_unicode=self.legacy_keysyms.max,
            min_unicode="XKB_KEYSYM_UNICODE_MIN",
            max_unicode=XKBCOMMON.XKB_KEYSYM_UNICODE_OFFSET + self.unicode.max,
            keysym_unicode_offset="XKB_KEYSYM_UNICODE_OFFSET",
        )
        with path.open("wt", encoding="utf-8") as fd:
            fd.write(content)

        unicode_version_array = ", ".join(
            (icu.UNICODE_VERSION + ".0.0.0").split(".")[:4]
        )
        path = root / "src/keysym.h.jinja"
        with path.open("rt", encoding="utf-8") as fd:
            content = fd.read()
        pattern: re.Pattern[str] = re.compile(
            r"^(#define\s+XKB_KEYSYM_UNICODE_VERSION\s+\{\s*)(?:\d+(?:,\s*\d+)*)*(\s*\})",
            re.MULTILINE,
        )

        def replace(m: re.Match[str]) -> str:
            return f"{m.group(1)}{unicode_version_array}{m.group(2)}"

        content = pattern.sub(replace, content)
        with path.open("wt", encoding="utf-8") as fd:
            fd.write(content)


################################################################################
# Strategy
################################################################################


@unique
class Strategy(Enum):
    Default = SeparateLegacyKeysymsAndUnicodeCombinedCaseMappings

    def __str__(self) -> str:
        return self.value.__name__

    @classmethod
    def parse(cls, str: str) -> Self:
        for s in cls:
            if s.name == str:
                return s
        else:
            raise ValueError(str)

    @classmethod
    def run(
        cls, root: Path, config: Config, strategies: list[Self], write: bool
    ) -> None:
        # FIXME: more generic type
        results: list[SeparateLegacyKeysymsAndUnicodeCombinedCaseMappings] = []
        best_total = math.inf
        best_solution: SeparateLegacyKeysymsAndUnicodeCombinedCaseMappings | None = None
        for strategy in strategies:
            print(f" Optimizing using {strategy.name} ".center(90, "="))
            sol = strategy.value.optimize(
                config=config, path=root / "data/keysyms.yaml"
            )
            sol.test(config)
            results.append(sol)
            if sol.total < best_total:
                best_total = sol.total
                best_solution = sol
                assert best_solution
        assert best_solution
        for strategy, sol in zip(strategies, results):
            print(
                f"{strategy.name}: {sol.total} ({sol.total // 8})",
                "✨✨✨" if sol is best_solution else "",
            )
        best_solution.test(config)
        if write:
            best_solution.write(root)
            cls.write_tests(root)

    @classmethod
    def write_tests(cls, root: Path) -> None:
        # Configure Jinja
        template_loader = jinja2.FileSystemLoader(root, encoding="utf-8")
        jinja_env = jinja2.Environment(
            loader=template_loader,
            keep_trailing_newline=True,
            trim_blocks=True,
            lstrip_blocks=True,
        )

        def code_point_name_constant(c: str, padding: int = 0) -> str:
            if not (name := unicodedata.name(c)):
                raise ValueError(f"No Unicode name for code point: U+{ord(c):0>4X}")
            name = name.replace("-", "_").replace(" ", "_").upper()
            return name.ljust(padding)

        jinja_env.filters["code_point"] = lambda c: f"0x{ord(c):0>4x}"
        jinja_env.filters["code_point_name_constant"] = code_point_name_constant
        path = root / "test/keysym-case-mapping.h"
        template_path = path.with_suffix(f"{path.suffix}.jinja")
        template = jinja_env.get_template(str(template_path.relative_to(root)))
        with path.open("wt", encoding="utf-8") as fd:
            fd.writelines(
                template.generate(
                    upper_exceptions=Entry.to_upper_exceptions,
                    script=SCRIPT.relative_to(root),
                )
            )


################################################################################
# Main
################################################################################

if __name__ == "__main__":
    # Root of the project
    ROOT = Path(__file__).parent.parent

    # Parse commands
    parser = argparse.ArgumentParser(description="Generate keysyms case mapping files")
    parser.add_argument(
        "--root",
        type=Path,
        default=ROOT,
        help="Path to the root of the project (default: %(default)s)",
    )
    parser.add_argument(
        "--strategy",
        type=Strategy.parse,
        default=list(Strategy),
        nargs="+",
        choices=Strategy,
        help="Strategy (default: %(default)s)",
        dest="strategies",
    )
    parser.add_argument(
        "--dry",
        action="store_true",
        help="Do not write (default: %(default)s)",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Verbose (default: %(default)s)",
    )
    parser.add_argument(
        "--no-check",
        action="store_true",
        help="Do not check errors (default: %(default)s)",
    )

    args = parser.parse_args()
    config = Config(
        check_error=not args.no_check,
        verbose=args.verbose,
    )
    Strategy.run(
        root=args.root, config=config, write=not args.dry, strategies=args.strategies
    )
