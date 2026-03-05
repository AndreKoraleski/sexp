from __future__ import annotations

from typing import Final

from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import DEEP, WIDE

_DEEP_TREE: Final[sexp.SExp] = sexp.parse(DEEP)
_WIDE_TREE: Final[sexp.SExp] = sexp.parse(WIDE)


def _count_atoms(root: sexp.SExp) -> int:
    stack: list[sexp.SExp] = list(root)
    n: int = 0
    while stack:
        node = stack.pop()
        if node.is_atom:
            n += 1
        else:
            stack.extend(node)
    return n


def test_deep(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(_count_atoms, args=(_DEEP_TREE,), iterations=100, rounds=100)


def test_wide(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(_count_atoms, args=(_WIDE_TREE,), iterations=100, rounds=100)
