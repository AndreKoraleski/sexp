from __future__ import annotations

import contextlib

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import MEDIUM

_TREE: sexp.SExp = sexp.parse(MEDIUM)
_LAST_IDX: int = len(_TREE) - 1


@pytest.mark.benchmark(min_rounds=200)
def test_head(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(lambda: _TREE.head, iterations=200, rounds=200)  # type: ignore[no-untyped-call]


def test_index_first(benchmark: BenchmarkFixture) -> None:
    benchmark(lambda: _TREE[0])


def test_index_last(benchmark: BenchmarkFixture) -> None:
    benchmark(lambda: _TREE[_LAST_IDX])


def test_key_lookup(benchmark: BenchmarkFixture) -> None:
    benchmark(lambda: _TREE["pos"])


def test_key_lookup_miss(benchmark: BenchmarkFixture) -> None:
    def _miss() -> None:
        with contextlib.suppress(KeyError):
            _ = _TREE["zzz"]

    benchmark(_miss)


@pytest.mark.benchmark(min_rounds=200)
def test_iterate(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(lambda: list(_TREE), iterations=200, rounds=200)  # type: ignore[no-untyped-call]


@pytest.mark.benchmark(min_rounds=200)
def test_tail(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(lambda: list(_TREE.tail), iterations=200, rounds=200)  # type: ignore[no-untyped-call]
