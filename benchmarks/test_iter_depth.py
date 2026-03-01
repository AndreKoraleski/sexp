from __future__ import annotations

from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import DEEP, WIDE

_DEEP_TREE: sexp.SExp = sexp.parse(DEEP)
_WIDE_TREE: sexp.SExp = sexp.parse(WIDE)


def _dfs_atom_count(root: sexp.SExp) -> int:
    stack: list[sexp.SExpNode] = list(root)
    n: int = 0
    while stack:
        node = stack.pop()
        if node.is_atom:
            n += 1
        else:
            stack.extend(node)
    return n


def test_deep_traversal(benchmark: BenchmarkFixture) -> None:
    benchmark(_dfs_atom_count, _DEEP_TREE)


def test_wide_traversal(benchmark: BenchmarkFixture) -> None:
    benchmark(_dfs_atom_count, _WIDE_TREE)
