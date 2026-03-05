from __future__ import annotations

from typing import Final

import pytest

SMALL: Final[bytes] = b"(a b c d e)"

MEDIUM: Final[bytes] = (
    b"(node (kind widget) (id 42) (pos 12.5 -3.2) (size 100 200) (visible true) (zorder 3))"
)

LARGE: Final[bytes] = (
    b"(root"
    b" (meta 1234)"
    b" (item (p 0.0 0.0) (q 1.2 -0.4))"
    b" (group a"
    b"  (entry 1 (p -42.0 0.0) (q 0.0 0.0) (r 0.0))"
    b"  (entry 2 (p -30.0 10.0) (q 0.3 0.0) (r 15.0))"
    b"  (entry 3 (p -30.0 -10.0) (q 0.3 0.0) (r -15.0))"
    b"  (entry 4 (p -20.0 20.0) (q 0.5 -0.1) (r 30.0))"
    b"  (entry 5 (p -20.0 -20.0) (q 0.5 0.1) (r -30.0))"
    b"  (entry 6 (p -10.0 15.0) (q 0.7 0.2) (r 45.0))"
    b"  (entry 7 (p -10.0 -15.0) (q 0.7 -0.2) (r -45.0))"
    b"  (entry 8 (p 5.0 25.0) (q 0.9 0.0) (r 60.0))"
    b"  (entry 9 (p 5.0 -25.0) (q 0.9 0.0) (r -60.0))"
    b"  (entry 10 (p 15.0 10.0) (q 1.0 0.3) (r 80.0))"
    b"  (entry 11 (p 15.0 -10.0) (q 1.0 -0.3) (r -80.0))"
    b" )"
    b" (group b"
    b"  (entry 1 (p 42.0 0.0) (q 0.0 0.0) (r 180.0))"
    b"  (entry 2 (p 30.0 10.0) (q -0.3 0.0) (r 165.0))"
    b"  (entry 3 (p 30.0 -10.0) (q -0.3 0.0) (r -165.0))"
    b"  (entry 4 (p 20.0 20.0) (q -0.5 -0.1) (r 150.0))"
    b"  (entry 5 (p 20.0 -20.0) (q -0.5 0.1) (r -150.0))"
    b"  (entry 6 (p 10.0 15.0) (q -0.7 0.2) (r 135.0))"
    b"  (entry 7 (p 10.0 -15.0) (q -0.7 -0.2) (r -135.0))"
    b"  (entry 8 (p -5.0 25.0) (q -0.9 0.0) (r 120.0))"
    b"  (entry 9 (p -5.0 -25.0) (q -0.9 0.0) (r -120.0))"
    b"  (entry 10 (p -15.0 10.0) (q -1.0 0.3) (r 100.0))"
    b"  (entry 11 (p -15.0 -10.0) (q -1.0 -0.3) (r -100.0))"
    b" )"
    b")"
)


def generate(depth: int, width: int) -> bytes:
    atoms = b" ".join(f"a{i}".encode() for i in range(width))

    def _build(d: int) -> bytes:
        if d == 0:
            return atoms
        inner = _build(d - 1)
        label = f"w{d}".encode()
        return b"(" + label + b" " + inner + b" " + atoms + b")"

    return _build(depth)


_DEEP_DEPTH: Final[int] = 8
_DEEP_WIDTH: Final[int] = 6
DEEP: Final[bytes] = generate(_DEEP_DEPTH, _DEEP_WIDTH)

_WIDE_DEPTH: Final[int] = 1
_WIDE_WIDTH: Final[int] = 34
WIDE: Final[bytes] = generate(_WIDE_DEPTH, _WIDE_WIDTH)

INPUTS: Final = [
    pytest.param(SMALL, id="small"),
    pytest.param(MEDIUM, id="medium"),
    pytest.param(LARGE, id="large"),
    pytest.param(DEEP, id="deep"),
]
