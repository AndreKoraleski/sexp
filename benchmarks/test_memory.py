from __future__ import annotations

import tracemalloc

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import INPUTS


@pytest.mark.parametrize("data", INPUTS)
def test_peak_allocation(benchmark: BenchmarkFixture, data: bytes) -> None:
    peaks: list[int] = []

    def _parse_and_trace() -> sexp.SExp:
        tracemalloc.start()
        try:
            result = sexp.parse(data)
            _, peak = tracemalloc.get_traced_memory()
            peaks.append(peak)
            return result
        finally:
            tracemalloc.stop()

    benchmark(_parse_and_trace)
    if peaks:
        benchmark.extra_info["peak_alloc_bytes"] = peaks[-1]
