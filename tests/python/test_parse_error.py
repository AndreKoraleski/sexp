import pytest

import sexp


def test_parse_error_is_raised_on_unclosed():
    with pytest.raises(sexp.ParseError):
        sexp.parse("(unclosed")


def test_parse_error_is_raised_on_stray_close():
    with pytest.raises(sexp.ParseError):
        sexp.parse(")")


def test_parse_error_is_subclass_of_value_error():
    assert issubclass(sexp.ParseError, ValueError)


def test_parse_error_caught_as_value_error():
    with pytest.raises(ValueError):
        sexp.parse("(bad")


def test_parse_error_message():
    with pytest.raises(sexp.ParseError, match="unclosed"):
        sexp.parse("((()")


def test_parse_error_message_unclosed():
    with pytest.raises(sexp.ParseError, match="unclosed parenthesis"):
        sexp.parse("(unclosed")


def test_parse_error_message_stray_close():
    with pytest.raises(sexp.ParseError, match="stray closing parenthesis"):
        sexp.parse(")")


def test_parse_error_message_multiple_roots():
    with pytest.raises(sexp.ParseError, match="multiple top-level forms"):
        sexp.parse("(a 1)(b 2)")


def test_parse_error_importable_directly():
    from sexp import ParseError  # noqa: F401


def test_parse_error_on_multiple_top_level_list_forms():
    with pytest.raises(sexp.ParseError):
        sexp.parse("(a 1)(b 2)")


def test_parse_error_on_multiple_top_level_atoms():
    with pytest.raises(sexp.ParseError):
        sexp.parse("a b")
