"""Tests for SExp.value getter and setter."""

import pytest

import sexp


class TestValueGetter:
    def test_atom_child(self) -> None:
        tree = sexp.parse("(a b c)")
        assert tree[0].value == "a"
        assert tree[1].value == "b"
        assert tree[2].value == "c"

    def test_nested_atom(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        assert tree[1][0].value == "b"
        assert tree[1][1].value == "c"

    def test_atom_root(self) -> None:
        assert sexp.parse("hello").value == "hello"

    def test_list_child_raises_type_error(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        with pytest.raises(TypeError):
            _ = tree[1].value

    def test_list_root_raises_type_error(self) -> None:
        tree = sexp.parse("(a b)")
        with pytest.raises(TypeError):
            _ = tree.value

    def test_empty_constructor_raises_type_error(self) -> None:
        with pytest.raises(TypeError):
            _ = sexp.SExp().value


class TestValueSetter:
    def test_set_atom_child(self) -> None:
        tree = sexp.parse("(a b c)")
        tree[0].value = "x"
        assert repr(tree) == "(x b c)"

    def test_set_nested_atom(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        tree[1][0].value = "X"
        assert repr(tree) == "(a (X c) d)"

    def test_set_preserves_structure(self) -> None:
        tree = sexp.parse("(a b c)")
        tree[1].value = "hello"
        assert len(tree) == 3
        assert tree[0].value == "a"
        assert tree[2].value == "c"

    def test_set_to_empty_string(self) -> None:
        tree = sexp.parse("(a b)")
        tree[0].value = ""
        assert tree[0].value == ""

    def test_set_non_string_raises_type_error(self) -> None:
        tree = sexp.parse("(a b c)")
        with pytest.raises(TypeError):
            tree[0].value = 42

    def test_set_on_list_child_raises_type_error(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        with pytest.raises(TypeError):
            tree[1].value = "x"

    def test_set_visible_through_other_handles(self) -> None:
        tree = sexp.parse("(a b c)")
        a_reference = tree[0]
        tree[0].value = "z"
        assert a_reference.value == "z"
