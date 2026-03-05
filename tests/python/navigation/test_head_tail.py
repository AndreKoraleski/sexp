"""Tests for SExp.head and SExp.tail."""

import pytest

import sexp


class TestHead:
    def test_returns_first_child(self) -> None:
        tree = sexp.parse("(a b c)")
        assert tree.head.value == "a"

    def test_empty_list_raises(self) -> None:
        with pytest.raises(IndexError):
            _ = sexp.parse("()").head

    def test_bare_raises(self) -> None:
        with pytest.raises(IndexError):
            _ = sexp.parse("").head

    def test_single_child(self) -> None:
        assert sexp.parse("(x)").head.value == "x"

    def test_on_child_list(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        assert tree[1].head.value == "b"

    def test_on_empty_child_list_raises(self) -> None:
        tree = sexp.parse("(a () d)")
        with pytest.raises(IndexError):
            _ = tree[1].head


class TestTail:
    def test_returns_remaining_children(self) -> None:
        tree = sexp.parse("(a b c)")
        assert [n.value for n in tree.tail] == ["b", "c"]

    def test_single_child_gives_empty_tail(self) -> None:
        assert list(sexp.parse("(a)").tail) == []

    def test_empty_list_gives_empty_tail(self) -> None:
        assert list(sexp.parse("()").tail) == []

    def test_bare_gives_empty_tail(self) -> None:
        assert list(sexp.parse("").tail) == []

    def test_returns_sexp_iter_type(self) -> None:
        tree = sexp.parse("(a b)")
        assert isinstance(tree.tail, sexp.SExpIter)

    def test_on_child_list(self) -> None:
        tree = sexp.parse("(a (x y z) d)")
        assert [n.value for n in tree[1].tail] == ["y", "z"]

    def test_on_child_list_single_gives_empty_tail(self) -> None:
        tree = sexp.parse("(a (x) d)")
        assert list(tree[1].tail) == []
