"""Tests for SExp.is_atom."""

import sexp


class TestIsAtomChildren:
    def test_atom_child_is_true(self) -> None:
        tree = sexp.parse("(a b)")
        assert tree[0].is_atom is True

    def test_list_child_is_false(self) -> None:
        tree = sexp.parse("(a (b c))")
        assert tree[1].is_atom is False

    def test_nested_atom_is_true(self) -> None:
        tree = sexp.parse("(a (b c))")
        assert tree[1][0].is_atom is True

    def test_returns_bool_type(self) -> None:
        tree = sexp.parse("(a b)")
        assert type(tree[0].is_atom) is bool
        assert type(tree[-1].is_atom) is bool


class TestIsAtomRoots:
    def test_list_root_is_false(self) -> None:
        assert sexp.parse("(a b c)").is_atom is False

    def test_empty_list_root_is_false(self) -> None:
        assert sexp.parse("()").is_atom is False

    def test_bare_is_false(self) -> None:
        assert sexp.parse("").is_atom is False

    def test_atom_root_is_true(self) -> None:
        assert sexp.parse("atom").is_atom is True

    def test_empty_constructor_is_false(self) -> None:
        assert sexp.SExp().is_atom is False

    def test_returns_bool_type(self) -> None:
        assert type(sexp.parse("(a)").is_atom) is bool
