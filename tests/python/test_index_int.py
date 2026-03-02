import pytest

import sexp


def test_getitem_int():
    t = sexp.parse("(a b c)")
    assert t[0].value == "a"
    assert t[1].value == "b"
    assert t[2].value == "c"


def test_getitem_negative_index():
    t = sexp.parse("(a b c)")
    assert t[-1].value == "c"
    assert t[-2].value == "b"
    assert t[-3].value == "a"


def test_getitem_out_of_range():
    t = sexp.parse("(a b c)")
    with pytest.raises(IndexError):
        t[3]
    with pytest.raises(IndexError):
        t[-4]


def test_node_getitem_int():
    t = sexp.parse("(a (x y z) e)")
    inner = t[1]
    assert inner[0].value == "x"
    assert inner[2].value == "z"


def test_node_getitem_negative():
    t = sexp.parse("(a (x y z) e)")
    inner = t[1]
    assert inner[-1].value == "z"


def test_node_getitem_out_of_range():
    t = sexp.parse("(a (x y) e)")
    inner = t[1]
    with pytest.raises(IndexError):
        inner[5]
