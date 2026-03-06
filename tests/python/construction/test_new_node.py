"""Tests for SExp.new_atom() and SExp.new_list() node allocation."""

import pytest

import sexp


class TestNewAtom:
    def test_returns_sexp_type(self) -> None:
        tree = sexp.parse("(a b)")
        assert isinstance(tree.new_atom("x"), sexp.SExp)

    def test_is_atom(self) -> None:
        tree = sexp.parse("(a b)")
        assert tree.new_atom("x").is_atom is True

    def test_value_matches(self) -> None:
        tree = sexp.parse("(a b)")
        assert tree.new_atom("hello").value == "hello"

    def test_does_not_attach(self) -> None:
        tree = sexp.parse("(a b)")
        _ = tree.new_atom("x")
        assert len(tree) == 2

    def test_can_be_appended(self) -> None:
        tree = sexp.parse("(a)")
        tree.append(tree.new_atom("b"))
        assert repr(tree) == "(a b)"

    def test_allocated_via_child_node(self) -> None:
        tree = sexp.parse("(a b)")
        node = tree[0].new_atom("c")
        tree.append(node)
        assert repr(tree) == "(a b c)"

    def test_cross_tree_attach_raises(self) -> None:
        first_tree = sexp.parse("(a b)")
        second_tree = sexp.parse("(c d)")
        foreign = second_tree.new_atom("x")
        with pytest.raises(ValueError):
            first_tree.append(foreign)


class TestNewList:
    def test_returns_sexp_type(self) -> None:
        tree = sexp.parse("(a b)")
        assert isinstance(tree.new_list(), sexp.SExp)

    def test_is_not_atom(self) -> None:
        tree = sexp.parse("(a b)")
        assert tree.new_list().is_atom is False

    def test_is_empty(self) -> None:
        tree = sexp.parse("(a b)")
        assert len(tree.new_list()) == 0

    def test_does_not_attach(self) -> None:
        tree = sexp.parse("(a b)")
        _ = tree.new_list()
        assert len(tree) == 2

    def test_can_be_populated_and_appended(self) -> None:
        tree = sexp.parse("(a)")
        new_list = tree.new_list()
        new_list.append(tree.new_atom("x"))
        tree.append(new_list)
        assert repr(tree) == "(a (x))"

    def test_allocated_via_child_node(self) -> None:
        tree = sexp.parse("(a)")
        new_list = tree[0].new_list()
        child = tree[0].new_atom("x")
        new_list.append(child)
        tree.append(new_list)
        assert repr(tree) == "(a (x))"
