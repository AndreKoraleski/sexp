"""Tests for SExp.parent."""

import sexp


class TestParentOnRoots:
    def test_list_root_parent_is_none(self) -> None:
        assert sexp.parse("(a b)").parent is None

    def test_empty_list_root_parent_is_none(self) -> None:
        assert sexp.parse("()").parent is None

    def test_bare_root_parent_is_none(self) -> None:
        assert sexp.parse("").parent is None

    def test_atom_root_parent_is_none(self) -> None:
        assert sexp.parse("atom").parent is None

    def test_empty_constructor_parent_is_none(self) -> None:
        assert sexp.SExp().parent is None


class TestParentOnChildNodes:
    def test_direct_child_parent_is_root(self) -> None:
        tree = sexp.parse("(a b c)")
        assert tree[0].parent is not None
        assert repr(tree[0].parent) == "(a b c)"

    def test_direct_child_parent_parent_is_none(self) -> None:
        tree = sexp.parse("(a)")
        assert tree[0].parent is not None
        assert tree[0].parent.parent is None

    def test_nested_parent_chain(self) -> None:
        tree = sexp.parse("(a (b c))")
        b = tree[1][0]
        assert repr(b.parent) == "(b c)"
        assert repr(b.parent.parent) == "(a (b c))"

    def test_all_siblings_share_parent(self) -> None:
        tree = sexp.parse("(x y z)")
        assert repr(tree[0].parent) == repr(tree[1].parent) == repr(tree[2].parent) == "(x y z)"
