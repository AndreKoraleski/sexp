from __future__ import annotations

import contextlib

import pytest


def pytest_configure(config: pytest.Config) -> None:
    with contextlib.suppress(AttributeError):
        config.option.no_cov = True
