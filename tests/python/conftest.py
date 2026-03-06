"""Shared pytest fixtures for the sexp Python test suite."""

import pytest

import sexp


@pytest.fixture
def flat_list() -> sexp.SExp:
    return sexp.parse("(a b c)")


@pytest.fixture
def nested() -> sexp.SExp:
    return sexp.parse("(a (b c) d)")


@pytest.fixture
def empty_list() -> sexp.SExp:
    return sexp.parse("()")


@pytest.fixture
def bare() -> sexp.SExp:
    return sexp.parse("")


@pytest.fixture
def atom_root() -> sexp.SExp:
    return sexp.parse("atom")


@pytest.fixture
def deep() -> sexp.SExp:
    return sexp.parse("(a (b (c d)) e)")


@pytest.fixture
def game_entity() -> sexp.SExp:
    return sexp.parse("(player (pos 1 2) (vel 3 4))")
