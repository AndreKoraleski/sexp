from __future__ import annotations

from typing import Final

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import DEEP, LARGE, MEDIUM, SMALL, WIDE

_SMALL_TREE: Final[sexp.SExp] = sexp.parse(SMALL)
_MEDIUM_TREE: Final[sexp.SExp] = sexp.parse(MEDIUM)
_LARGE_TREE: Final[sexp.SExp] = sexp.parse(LARGE)
_DEEP_TREE: Final[sexp.SExp] = sexp.parse(DEEP)
_WIDE_TREE: Final[sexp.SExp] = sexp.parse(WIDE)

TREES: Final = [
    pytest.param(_SMALL_TREE, 1000, id="small"),
    pytest.param(_MEDIUM_TREE, 1000, id="medium"),
    pytest.param(_LARGE_TREE,   100, id="large"),
    pytest.param(_DEEP_TREE,    100, id="deep"),
    pytest.param(_WIDE_TREE,    100, id="wide"),
]


@pytest.mark.parametrize("tree,iterations", TREES)
def test_serialize(benchmark: BenchmarkFixture, tree: sexp.SExp, iterations: int) -> None:
    benchmark.pedantic(repr, args=(tree,), iterations=iterations, rounds=100)
