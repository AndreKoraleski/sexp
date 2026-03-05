"""Tests for parse() input type acceptance."""

import pytest

import sexp


class TestParseStr:
    def test_basic(self) -> None:
        assert repr(sexp.parse("(a b)")) == "(a b)"

    def test_nested(self) -> None:
        assert repr(sexp.parse("(a (b c) d)")) == "(a (b c) d)"

    def test_empty_string(self) -> None:
        tree = sexp.parse("")
        assert len(tree) == 0
        assert list(tree) == []

    def test_empty_list(self) -> None:
        assert repr(sexp.parse("()")) == "()"

    def test_single_atom(self) -> None:
        assert repr(sexp.parse("atom")) == "atom"

    def test_whitespace_only(self) -> None:
        tree = sexp.parse("   ")
        assert len(tree) == 0

    def test_returns_sexp_type(self) -> None:
        assert isinstance(sexp.parse("(a)"), sexp.SExp)


class TestParseBytes:
    def test_basic(self) -> None:
        assert repr(sexp.parse(b"(a b)")) == "(a b)"

    def test_empty_bytes(self) -> None:
        tree = sexp.parse(b"")
        assert len(tree) == 0

    def test_nested(self) -> None:
        assert repr(sexp.parse(b"(a (b c))")) == "(a (b c))"

    def test_returns_sexp_type(self) -> None:
        assert isinstance(sexp.parse(b"(x)"), sexp.SExp)


class TestParseBytearray:
    def test_basic(self) -> None:
        assert repr(sexp.parse(bytearray(b"(a b)"))) == "(a b)"

    def test_empty_bytearray(self) -> None:
        tree = sexp.parse(bytearray(b""))
        assert len(tree) == 0

    def test_returns_sexp_type(self) -> None:
        assert isinstance(sexp.parse(bytearray(b"(x)")), sexp.SExp)


class TestParseInvalidTypes:
    def test_int_raises_type_error(self) -> None:
        with pytest.raises(TypeError):
            sexp.parse(42)

    def test_float_raises_type_error(self) -> None:
        with pytest.raises(TypeError):
            sexp.parse(3.14)

    def test_none_raises_type_error(self) -> None:
        with pytest.raises(TypeError):
            sexp.parse(None)

    def test_list_raises_type_error(self) -> None:
        with pytest.raises(TypeError):
            sexp.parse(["a", "b"])
