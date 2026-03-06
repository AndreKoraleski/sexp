"""Tests for SExp.remove()."""

import pytest

import sexp


class TestRemove:
    def test_remove_middle_child(self) -> None:
        tree = sexp.parse("(a b c)")
        tree[1].remove()
        assert repr(tree) == "(a c)"

    def test_remove_first_child(self) -> None:
        tree = sexp.parse("(a b c)")
        tree[0].remove()
        assert repr(tree) == "(b c)"

    def test_remove_last_child(self) -> None:
        tree = sexp.parse("(a b c)")
        tree[2].remove()
        assert repr(tree) == "(a b)"

    def test_remove_only_child(self) -> None:
        tree = sexp.parse("(a)")
        tree[0].remove()
        assert len(tree) == 0

    def test_remove_nested_child(self) -> None:
        tree = sexp.parse("(a (b c d) e)")
        tree[1][1].remove()
        assert repr(tree) == "(a (b d) e)"

    def test_remove_entire_inner_list(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        tree[1].remove()
        assert repr(tree) == "(a d)"

    def test_preserves_sibling_count(self) -> None:
        tree = sexp.parse("(a b c d)")
        tree[2].remove()
        assert len(tree) == 3
        assert repr(tree) == "(a b d)"

    def test_removed_node_is_stale(self) -> None:
        tree = sexp.parse("(a b c)")
        node = tree[1]
        node.remove()
        with pytest.raises(RuntimeError):
            repr(node)


class TestRemoveRoot:
    def test_remove_on_root_raises_value_error(self) -> None:
        tree = sexp.parse("(a b c)")
        with pytest.raises(ValueError):
            tree.remove()

    def test_remove_on_empty_root_raises_value_error(self) -> None:
        with pytest.raises(ValueError):
            sexp.parse("()").remove()

    def test_remove_on_bare_raises_value_error(self) -> None:
        with pytest.raises(ValueError):
            sexp.parse("").remove()
