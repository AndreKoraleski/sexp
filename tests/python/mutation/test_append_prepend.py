"""Tests for SExp.append() and SExp.prepend()."""

import pytest

import sexp


class TestAppend:
    def test_appends_atom_to_root(self) -> None:
        tree = sexp.parse("(a b)")
        tree.append(tree.new_atom("c"))
        assert repr(tree) == "(a b c)"

    def test_appends_atom_to_nested_list(self) -> None:
        tree = sexp.parse("(x (a b) y)")
        tree[1].append(tree.new_atom("c"))
        assert repr(tree) == "(x (a b c) y)"

    def test_appends_list_node(self) -> None:
        tree = sexp.parse("(a b)")
        new_list = tree.new_list()
        new_list.append(tree.new_atom("x"))
        new_list.append(tree.new_atom("y"))
        tree.append(new_list)
        assert repr(tree) == "(a b (x y))"

    def test_to_empty_tree(self) -> None:
        tree = sexp.parse("()")
        tree.append(tree.new_atom("hello"))
        assert repr(tree) == "(hello)"

    def test_cross_tree_raises(self) -> None:
        first_tree = sexp.parse("(a b)")
        second_tree = sexp.parse("(c d)")
        with pytest.raises(ValueError):
            first_tree.append(second_tree[0])

    def test_non_node_raises(self) -> None:
        tree = sexp.parse("(a b)")
        with pytest.raises(TypeError):
            tree.append("not a node")


class TestPrepend:
    def test_prepends_atom_to_root(self) -> None:
        tree = sexp.parse("(a b)")
        tree.prepend(tree.new_atom("z"))
        assert repr(tree) == "(z a b)"

    def test_prepends_atom_to_nested_list(self) -> None:
        tree = sexp.parse("(x (a b) y)")
        tree[1].prepend(tree.new_atom("z"))
        assert repr(tree) == "(x (z a b) y)"

    def test_prepends_list_node(self) -> None:
        tree = sexp.parse("(a b)")
        new_list = tree.new_list()
        new_list.append(tree.new_atom("x"))
        tree.prepend(new_list)
        assert repr(tree) == "((x) a b)"

    def test_cross_tree_raises(self) -> None:
        first_tree = sexp.parse("(a b)")
        second_tree = sexp.parse("(c d)")
        with pytest.raises(ValueError):
            first_tree.prepend(second_tree[0])

    def test_non_node_raises(self) -> None:
        tree = sexp.parse("(a b)")
        with pytest.raises(TypeError):
            tree.prepend("not a node")


class TestMoveSemantics:
    def test_append_moves_existing_node(self) -> None:
        tree = sexp.parse("(a b c)")
        tree.append(tree[0])
        assert repr(tree) == "(b c a)"

    def test_prepend_moves_existing_node(self) -> None:
        tree = sexp.parse("(a b c)")
        tree.prepend(tree[2])
        assert repr(tree) == "(c a b)"

    def test_append_to_nested_moves_node(self) -> None:
        tree = sexp.parse("(x (a b) y)")
        inner = tree[1]
        tree.append(inner)
        assert repr(tree) == "(x y (a b))"
