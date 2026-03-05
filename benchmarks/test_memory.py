from __future__ import annotations

import os
from typing import Final

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import DEEP, LARGE, MEDIUM, SMALL, WIDE

_PAGE_SIZE: Final[int] = os.sysconf("SC_PAGE_SIZE")


def _rss_bytes() -> int:
    with open("/proc/self/statm") as f:
        return int(f.read().split()[1]) * _PAGE_SIZE


_PARAMS = [
    pytest.param(SMALL, 1000, id="small"),
    pytest.param(MEDIUM, 1000, id="medium"),
    pytest.param(LARGE, 100, id="large"),
    pytest.param(DEEP, 100, id="deep"),
    pytest.param(WIDE, 100, id="wide"),
]


@pytest.mark.parametrize("data,iterations", _PARAMS)
def test_rss_growth(benchmark: BenchmarkFixture, data: bytes, iterations: int) -> None:
    before = _rss_bytes()
    sexp.parse(data)
    after = _rss_bytes()
    benchmark.extra_info["rss_delta_bytes"] = max(0, after - before)
    benchmark.pedantic(sexp.parse, args=(data,), iterations=iterations, rounds=100)
