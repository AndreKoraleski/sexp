from __future__ import annotations

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import DEEP, LARGE, MEDIUM, SMALL

_PARAMS = [
    pytest.param(SMALL, 1000, id="small"),
    pytest.param(MEDIUM, 100, id="medium"),
    pytest.param(LARGE, 10, id="large"),
    pytest.param(DEEP, 100, id="deep"),
]


@pytest.mark.parametrize("data,iterations", _PARAMS)
def test_roundtrip(benchmark: BenchmarkFixture, data: bytes, iterations: int) -> None:
    benchmark.extra_info["label"] = "parse+serialize"
    benchmark.pedantic(lambda: repr(sexp.parse(data)), iterations=iterations, rounds=100)
