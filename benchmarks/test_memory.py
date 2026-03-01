from __future__ import annotations

import tracemalloc

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import INPUTS


@pytest.mark.parametrize("data", INPUTS)
def test_peak_allocation(benchmark: BenchmarkFixture, data: bytes) -> None:

    tracemalloc.start()
    sexp.parse(data)
    _, peak = tracemalloc.get_traced_memory()
    tracemalloc.stop()
    benchmark.extra_info["peak_alloc_bytes"] = peak
    benchmark(sexp.parse, data)
