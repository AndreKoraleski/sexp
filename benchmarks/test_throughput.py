from __future__ import annotations

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import INPUTS


@pytest.mark.parametrize("data", INPUTS)
def test_parse(benchmark: BenchmarkFixture, data: bytes) -> None:
    benchmark(sexp.parse, data)
