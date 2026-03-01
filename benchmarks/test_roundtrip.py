from __future__ import annotations

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import INPUTS


@pytest.mark.parametrize("data", INPUTS)
def test_roundtrip(benchmark: BenchmarkFixture, data: bytes) -> None:
    benchmark.extra_info["label"] = "parse+serialize"
    benchmark(lambda: repr(sexp.parse(data)))
