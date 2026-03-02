import pytest

import sexp


def test_node_value_atom():
    t = sexp.parse("(a b c)")
    assert t[0].value == "a"


def test_node_value_list_raises():
    t = sexp.parse("(a (b c) d)")
    with pytest.raises(TypeError):
        _ = t[1].value


def test_node_value_set():
    t = sexp.parse("(a b c)")
    t[0].value = "x"
    assert repr(t) == "(x b c)"


def test_node_value_set_nested():
    t = sexp.parse("(a (b c) d)")
    t[1][0].value = "X"
    assert repr(t) == "(a (X c) d)"


def test_node_value_set_non_string_raises():
    t = sexp.parse("(a b c)")
    with pytest.raises(TypeError):
        t[0].value = 42


def test_node_value_set_to_empty_string():
    t = sexp.parse("(a b)")
    t[0].value = "hello"
    assert t[0].value == "hello"
