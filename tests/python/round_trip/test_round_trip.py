"""Tests for parse -> repr round-trip identity."""

import pytest

import sexp


class TestRoundTrip:
    @pytest.mark.parametrize(
        "source",
        [
            "(a b c)",
            "(a (b c) d)",
            "(a (b (c d)) e)",
            "(a (b (c (d e))) f)",
            "(player (pos 1 2) (vel 3 4))",
            "()",
            "atom",
            "hello-world",
            "(single)",
            "(deeply (nested (structure (goes (here)))))",
        ],
    )
    def test_str_round_trip(self, source: str) -> None:
        assert repr(sexp.parse(source)) == source

    def test_bytes_round_trip(self) -> None:
        source = "(player (pos 1 2) (vel 3 4))"
        assert repr(sexp.parse(source.encode())) == source

    def test_bytearray_round_trip(self) -> None:
        source = "(a (b c) d)"
        assert repr(sexp.parse(bytearray(source.encode()))) == source

    def test_bare_round_trip(self) -> None:
        assert repr(sexp.parse("")) == ""

    def test_empty_list_round_trip(self) -> None:
        assert repr(sexp.parse("()")) == "()"
