"""Tests for ParseError variants, messages, and inheritance."""

import pytest

import sexp


class TestParseErrorTypes:
    def test_is_subclass_of_value_error(self) -> None:
        assert issubclass(sexp.ParseError, ValueError)

    def test_can_be_caught_as_value_error(self) -> None:
        with pytest.raises(ValueError):
            sexp.parse("(bad")

    def test_can_be_caught_as_parse_error(self) -> None:
        with pytest.raises(sexp.ParseError):
            sexp.parse("(bad")

    def test_importable_directly(self) -> None:
        from sexp import ParseError


class TestParseErrorMessages:
    def test_unclosed_paren_message(self) -> None:
        with pytest.raises(sexp.ParseError, match="unclosed parenthesis"):
            sexp.parse("(unclosed")

    def test_stray_close_message(self) -> None:
        with pytest.raises(sexp.ParseError, match="stray closing parenthesis"):
            sexp.parse(")")

    def test_multiple_top_level_forms_message(self) -> None:
        with pytest.raises(sexp.ParseError, match="multiple top-level forms"):
            sexp.parse("(a 1)(b 2)")

    def test_deeply_unclosed(self) -> None:
        with pytest.raises(sexp.ParseError, match="unclosed"):
            sexp.parse("((()")


class TestParseErrorConditions:
    def test_unclosed_paren(self) -> None:
        with pytest.raises(sexp.ParseError):
            sexp.parse("(unclosed")

    def test_stray_close(self) -> None:
        with pytest.raises(sexp.ParseError):
            sexp.parse(")")

    def test_multiple_top_level_lists(self) -> None:
        with pytest.raises(sexp.ParseError):
            sexp.parse("(a 1)(b 2)")

    def test_multiple_top_level_atoms(self) -> None:
        with pytest.raises(sexp.ParseError):
            sexp.parse("a b")
