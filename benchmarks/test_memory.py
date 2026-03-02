from __future__ import annotations

import tracemalloc

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import DEEP, LARGE, MEDIUM, SMALL

_PARAMS = [
    pytest.param(SMALL,  1000, id="small"),
    pytest.param(MEDIUM, 1000, id="medium"),
    pytest.param(LARGE,   100, id="large"),
    pytest.param(DEEP,    100, id="deep"),
]


@pytest.mark.parametrize("data,iterations", _PARAMS)
def test_peak_allocation(benchmark: BenchmarkFixture, data: bytes, iterations: int) -> None:
    tracemalloc.start()
    sexp.parse(data)
    _, peak = tracemalloc.get_traced_memory()
    tracemalloc.stop()
    benchmark.extra_info["peak_alloc_bytes"] = peak
    benchmark.pedantic(sexp.parse, args=(data,), iterations=iterations, rounds=100)
