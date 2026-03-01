from __future__ import annotations

from typing import Final

from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import MEDIUM

WARM_DATA: Final[bytes] = MEDIUM

_COLD_POOL_SIZE: Final[int] = 10_000
_COLD_POOL: Final[list[bytes]] = [
    b"(rec"
    + b"".join(f" (k{i}x{j} v{i}y{j})".encode() for j in range(20))
    + b")"
    for i in range(_COLD_POOL_SIZE)
]


def test_intern_warm(benchmark: BenchmarkFixture) -> None:
    sexp.parse(WARM_DATA)
    benchmark(sexp.parse, WARM_DATA)


def test_intern_cold(benchmark: BenchmarkFixture) -> None:
    _pool = _COLD_POOL
    _n = _COLD_POOL_SIZE
    _i = 0

    def _parse_unique() -> sexp.SExp:
        nonlocal _i
        result = sexp.parse(_pool[_i % _n])
        _i += 1
        return result

    benchmark(_parse_unique)
