"""Tests for SExp.__repr__."""

import sexp


class TestReprRoots:
    def test_flat_list(self) -> None:
        assert repr(sexp.parse("(a b c)")) == "(a b c)"

    def test_nested_list(self) -> None:
        assert repr(sexp.parse("(a (b c) d)")) == "(a (b c) d)"

    def test_empty_list(self) -> None:
        assert repr(sexp.parse("()")) == "()"

    def test_atom_root(self) -> None:
        assert repr(sexp.parse("atom")) == "atom"

    def test_bare_is_empty_string(self) -> None:
        assert repr(sexp.parse("")) == ""

    def test_empty_constructor_is_empty_list(self) -> None:
        assert repr(sexp.SExp()) == "()"


class TestReprChildNodes:
    def test_atom_child(self) -> None:
        tree = sexp.parse("(a b c)")
        assert repr(tree[0]) == "a"
        assert repr(tree[1]) == "b"

    def test_list_child(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        assert repr(tree[1]) == "(b c)"

    def test_nested_list_child(self) -> None:
        tree = sexp.parse("(a (b (c d)) e)")
        assert repr(tree[1]) == "(b (c d))"
        assert repr(tree[1][1]) == "(c d)"

    def test_deeply_nested(self) -> None:
        source = "(a (b (c (d e))) f)"
        tree = sexp.parse(source)
        assert repr(tree[1][1][1]) == "(d e)"
