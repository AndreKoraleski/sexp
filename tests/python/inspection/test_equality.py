"""Tests for SExp structural equality and hashing."""

import pytest

import sexp


class TestEqualityBetweenTrees:
    def test_equal_flat_lists(self) -> None:
        assert sexp.parse("(a b c)") == sexp.parse("(a b c)")

    def test_equal_nested(self) -> None:
        assert sexp.parse("(a (b c) d)") == sexp.parse("(a (b c) d)")

    def test_equal_bare(self) -> None:
        assert sexp.parse("") == sexp.parse("")

    def test_equal_empty_list(self) -> None:
        assert sexp.parse("()") == sexp.parse("()")

    def test_not_equal_different_atoms(self) -> None:
        assert sexp.parse("(a b c)") != sexp.parse("(a b x)")

    def test_not_equal_different_length(self) -> None:
        assert sexp.parse("(a b)") != sexp.parse("(a b c)")

    def test_not_equal_different_structure(self) -> None:
        assert sexp.parse("(a (b c))") != sexp.parse("((a b) c)")

    def test_bare_and_empty_list_are_structurally_equal(self) -> None:
        assert sexp.parse("") == sexp.parse("()")
        assert repr(sexp.parse("")) != repr(sexp.parse("()"))

    def test_not_equal_non_sexp_type(self) -> None:
        tree = sexp.parse("(a b)")
        assert tree != "(a b)"
        assert tree != 42
        assert tree != None


class TestEqualityBetweenChildNodes:
    def test_equal_atoms(self) -> None:
        first_tree = sexp.parse("(a b)")
        second_tree = sexp.parse("(a b)")
        assert first_tree[0] == second_tree[0]

    def test_not_equal_different_atoms(self) -> None:
        tree = sexp.parse("(a b)")
        assert tree[0] != tree[1]

    def test_equal_subtrees(self) -> None:
        first_tree = sexp.parse("((a b c))")
        second_tree = sexp.parse("((a b c))")
        assert first_tree[0] == second_tree[0]

    def test_not_equal_different_subtrees(self) -> None:
        first_tree = sexp.parse("((a b))")
        second_tree = sexp.parse("((a x))")
        assert first_tree[0] != second_tree[0]

    def test_not_equal_atom_vs_list(self) -> None:
        tree = sexp.parse("(a (b))")
        assert tree[0] != tree[1]

    def test_not_equal_non_sexp_type(self) -> None:
        tree = sexp.parse("(a b)")
        assert tree[0] != "a"
        assert tree[0] != 0


class TestHashability:
    def test_root_is_unhashable(self) -> None:
        tree = sexp.parse("(a b)")
        with pytest.raises(TypeError, match="unhashable"):
            hash(tree)

    def test_child_is_unhashable(self) -> None:
        tree = sexp.parse("(a b)")
        with pytest.raises(TypeError, match="unhashable"):
            hash(tree[0])


class TestStalenessInEquality:
    def test_stale_node_eq_raises(self) -> None:
        tree = sexp.parse("(a b c)")
        node_a = tree[0]
        tree[2].remove()
        with pytest.raises(RuntimeError):
            _ = node_a == tree[0]

    def test_stale_node_ne_raises(self) -> None:
        tree = sexp.parse("(a b c)")
        node_a = tree[0]
        tree[2].remove()
        with pytest.raises(RuntimeError):
            _ = node_a != tree[0]
