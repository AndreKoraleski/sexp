"""Tests for SExp.insert_after()."""

import pytest

import sexp


class TestInsertAfter:
    def test_insert_after_middle_sibling(self) -> None:
        tree = sexp.parse("(a b)")
        tree.insert_after(tree[0], tree.new_atom("x"))
        assert repr(tree) == "(a x b)"

    def test_insert_after_last_sibling(self) -> None:
        tree = sexp.parse("(a b)")
        tree.insert_after(tree[1], tree.new_atom("x"))
        assert repr(tree) == "(a b x)"

    def test_insert_after_none_is_prepend(self) -> None:
        tree = sexp.parse("(a b)")
        tree.insert_after(None, tree.new_atom("x"))
        assert repr(tree) == "(x a b)"

    def test_preserves_order_of_remaining_children(self) -> None:
        tree = sexp.parse("(a b c)")
        tree.insert_after(tree[1], tree.new_atom("x"))
        assert repr(tree) == "(a b x c)"

    def test_on_nested_list(self) -> None:
        tree = sexp.parse("((a b c))")
        inner = tree[0]
        inner.insert_after(inner[0], inner.new_atom("x"))
        assert repr(tree) == "((a x b c))"


class TestInsertAfterMoveSemantics:
    def test_moves_existing_node(self) -> None:
        tree = sexp.parse("(a b c)")
        tree.insert_after(tree[0], tree[2])
        assert repr(tree) == "(a c b)"


class TestInsertAfterCrossTreeErrors:
    def test_child_from_different_tree_raises(self) -> None:
        first_tree = sexp.parse("(a b)")
        second_tree = sexp.parse("(c d)")
        with pytest.raises(ValueError):
            first_tree.insert_after(None, second_tree[0])

    def test_after_from_different_tree_raises(self) -> None:
        first_tree = sexp.parse("(a b)")
        second_tree = sexp.parse("(c d)")
        with pytest.raises(ValueError):
            first_tree.insert_after(second_tree[0], first_tree.new_atom("x"))

    def test_non_node_after_raises(self) -> None:
        tree = sexp.parse("(a b)")
        with pytest.raises(TypeError):
            tree.insert_after("not a node", tree.new_atom("x"))
