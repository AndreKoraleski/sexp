import pytest

import sexp


def test_parse_str():
    t = sexp.parse("(a b)")
    assert repr(t) == "(a b)"


def test_parse_bytes():
    t = sexp.parse(b"(a b)")
    assert repr(t) == "(a b)"


def test_parse_bytearray():
    t = sexp.parse(bytearray(b"(a b)"))
    assert repr(t) == "(a b)"


def test_parse_invalid_raises():
    with pytest.raises(ValueError):
        sexp.parse("(unclosed")


def test_parse_stray_close_raises():
    with pytest.raises(ValueError):
        sexp.parse(")")


def test_parse_empty_string():
    t = sexp.parse("")
    assert len(t) == 0
    assert list(t) == []
