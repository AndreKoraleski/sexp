"""Tests for SExp.__len__."""

import sexp


class TestLenRoots:
    def test_flat_list(self) -> None:
        assert len(sexp.parse("(a b c)")) == 3

    def test_nested_list_counts_direct_children_only(self) -> None:
        assert len(sexp.parse("(a (b c) d)")) == 3

    def test_empty_list(self) -> None:
        assert len(sexp.parse("()")) == 0

    def test_bare(self) -> None:
        assert len(sexp.parse("")) == 0

    def test_atom_root(self) -> None:
        assert len(sexp.parse("atom")) == 0

    def test_single_child(self) -> None:
        assert len(sexp.parse("(x)")) == 1

    def test_empty_constructor(self) -> None:
        assert len(sexp.SExp()) == 0


class TestLenChildNodes:
    def test_list_child(self) -> None:
        tree = sexp.parse("(a (b c d) e)")
        assert len(tree[1]) == 3

    def test_atom_child_returns_zero(self) -> None:
        tree = sexp.parse("(a b c)")
        assert len(tree[0]) == 0

    def test_empty_list_child(self) -> None:
        tree = sexp.parse("(a () b)")
        assert len(tree[1]) == 0
