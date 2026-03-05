"""Tests for the SExp() empty constructor."""

import sexp


class TestEmptyConstructorBasics:
    def test_returns_sexp_type(self) -> None:
        assert isinstance(sexp.SExp(), sexp.SExp)

    def test_repr_is_empty_list(self) -> None:
        assert repr(sexp.SExp()) == "()"

    def test_len_is_zero(self) -> None:
        assert len(sexp.SExp()) == 0

    def test_iter_is_empty(self) -> None:
        assert list(sexp.SExp()) == []

    def test_is_not_atom(self) -> None:
        assert sexp.SExp().is_atom is False

    def test_parent_is_none(self) -> None:
        assert sexp.SExp().parent is None

    def test_equals_parsed_empty_list(self) -> None:
        assert sexp.SExp() == sexp.parse("()")

    def test_repr_differs_from_bare(self) -> None:
        assert repr(sexp.SExp()) != repr(sexp.parse(""))
        assert repr(sexp.SExp()) == "()"
        assert repr(sexp.parse("")) == ""


class TestEmptyConstructorMutation:
    def test_append_atom(self) -> None:
        tree = sexp.SExp()
        tree.append(tree.new_atom("a"))
        assert repr(tree) == "(a)"

    def test_prepend_atom(self) -> None:
        tree = sexp.SExp()
        tree.prepend(tree.new_atom("z"))
        assert repr(tree) == "(z)"

    def test_build_flat_list(self) -> None:
        tree = sexp.SExp()
        for character in ("x", "y", "z"):
            tree.append(tree.new_atom(character))
        assert repr(tree) == "(x y z)"

    def test_build_nested_list(self) -> None:
        tree = sexp.SExp()
        inner = tree.new_list()
        inner.append(tree.new_atom("b"))
        inner.append(tree.new_atom("c"))
        tree.append(tree.new_atom("a"))
        tree.append(inner)
        assert repr(tree) == "(a (b c))"

    def test_multiple_instances_are_independent(self) -> None:
        first_tree = sexp.SExp()
        second_tree = sexp.SExp()
        first_tree.append(first_tree.new_atom("a"))
        assert len(second_tree) == 0
