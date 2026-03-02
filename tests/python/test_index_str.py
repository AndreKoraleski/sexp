import pytest

import sexp


def test_getitem_str():
    t = sexp.parse("(player (pos 1 2) (vel 3 4))")
    node = t["pos"]
    assert repr(node) == "(pos 1 2)"


def test_getitem_str_not_found():
    t = sexp.parse("(player (pos 1 2))")
    with pytest.raises(KeyError):
        t["vel"]


def test_getitem_str_nested_access():
    t = sexp.parse("(player (pos 1 2) (vel 3 4))")
    node = t["vel"]
    assert repr(node[1]) == "3"


def test_getitem_str_returns_first_match():
    t = sexp.parse("(player (pos 1 2) (vel 3 4) (pos 5 6))")
    node = t["pos"]
    assert repr(node) == "(pos 1 2)"


def test_getitem_str_multiple():
    t = sexp.parse("(player (pos 1 2) (vel 3 4) (pos 5 6))")
    node = t["pos"]
    assert repr(node) == "(pos 1 2)"


def test_getitem_wrong_type_raises():
    t = sexp.parse("(a b)")
    with pytest.raises(TypeError):
        t[1.5]
