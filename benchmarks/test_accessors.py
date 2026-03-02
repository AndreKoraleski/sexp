from __future__ import annotations

import contextlib

from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import MEDIUM

_TREE: sexp.SExp = sexp.parse(MEDIUM)
_LAST_IDX: int = len(_TREE) - 1


def test_head(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(lambda: _TREE.head, iterations=1000, rounds=100)


def test_index_first(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(lambda: _TREE[0], iterations=1000, rounds=100)


def test_index_last(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(lambda: _TREE[_LAST_IDX], iterations=1000, rounds=100)


def test_key_lookup(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(lambda: _TREE["pos"], iterations=1000, rounds=100)


def test_key_lookup_miss(benchmark: BenchmarkFixture) -> None:
    def _miss() -> None:
        with contextlib.suppress(KeyError):
            _ = _TREE["zzz"]

    benchmark.pedantic(_miss, iterations=1000, rounds=100)


def test_iterate(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(lambda: list(_TREE), iterations=1000, rounds=100)


def test_tail(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(lambda: list(_TREE.tail), iterations=1000, rounds=100)
