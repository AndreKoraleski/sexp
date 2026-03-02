import pytest

import sexp


def test_head():
    t = sexp.parse("(a b c)")
    assert t.head.value == "a"


def test_head_empty_raises():
    t = sexp.parse("()")
    with pytest.raises(IndexError):
        _ = t.head


def test_head_empty_parse_raises():
    t = sexp.parse("")
    with pytest.raises(IndexError):
        _ = t.head


def test_tail():
    t = sexp.parse("(a b c)")
    values = [node.value for node in t.tail]
    assert values == ["b", "c"]


def test_tail_single_child():
    t = sexp.parse("(a)")
    assert list(t.tail) == []


def test_tail_empty():
    t = sexp.parse("()")
    assert list(t.tail) == []


def test_node_head():
    t = sexp.parse("(a (b c) d)")
    inner = t[1]
    assert inner.head.value == "b"


def test_node_head_empty_raises():
    t = sexp.parse("(a () d)")
    empty = t[1]
    with pytest.raises(IndexError):
        _ = empty.head


def test_node_tail():
    t = sexp.parse("(a (x y z) d)")
    inner = t[1]
    values = [n.value for n in inner.tail]
    assert values == ["y", "z"]


def test_node_tail_single_child():
    t = sexp.parse("(a (x) d)")
    inner = t[1]
    assert list(inner.tail) == []
