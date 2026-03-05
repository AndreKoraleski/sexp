"""Tests for SExp.__contains__."""

import sexp


class TestContainsStr:
    def test_present_atoms(self) -> None:
        tree = sexp.parse("(a b c)")
        assert "a" in tree
        assert "b" in tree
        assert "c" in tree

    def test_absent_atom(self) -> None:
        tree = sexp.parse("(a b c)")
        assert "x" not in tree

    def test_str_does_not_match_list_head(self) -> None:
        tree = sexp.parse("((a b) c)")
        assert "c" in tree
        assert "a" not in tree

    def test_empty_list_contains_nothing(self) -> None:
        assert "a" not in sexp.parse("()")

    def test_bare_contains_nothing(self) -> None:
        assert "a" not in sexp.parse("")

    def test_on_inner_list(self) -> None:
        tree = sexp.parse("((a b c))")
        inner = tree[0]
        assert "a" in inner
        assert "c" in inner
        assert "x" not in inner


class TestContainsNode:
    def test_direct_child_is_present(self) -> None:
        tree = sexp.parse("(a b c)")
        assert tree[1] in tree

    def test_grandchild_is_not_present(self) -> None:
        tree = sexp.parse("(a (b c))")
        grandchild = tree[1][0]
        assert grandchild not in tree

    def test_node_from_different_tree_is_absent(self) -> None:
        first_tree = sexp.parse("(a b c)")
        second_tree = sexp.parse("(a b c)")
        assert second_tree[0] not in first_tree

    def test_node_in_own_subtree(self) -> None:
        tree = sexp.parse("((a b) (c d))")
        first = tree[0]
        child_of_first = tree[0][1]
        assert child_of_first not in tree
        assert child_of_first in first


class TestContainsAtomNode:
    def test_atom_node_contains_nothing(self) -> None:
        tree = sexp.parse("(a b)")
        atom = tree[0]
        other = tree[1]
        assert "a" not in atom
        assert other not in atom
