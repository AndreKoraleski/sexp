"""Tests for SExp.__getitem__ (int index, str key, slice)."""

import pytest

import sexp


class TestIntIndex:
    def test_positive_indices(self) -> None:
        tree = sexp.parse("(a b c)")
        assert tree[0].value == "a"
        assert tree[1].value == "b"
        assert tree[2].value == "c"

    def test_negative_indices(self) -> None:
        tree = sexp.parse("(a b c)")
        assert tree[-1].value == "c"
        assert tree[-2].value == "b"
        assert tree[-3].value == "a"

    def test_out_of_range_positive_raises(self) -> None:
        tree = sexp.parse("(a b c)")
        with pytest.raises(IndexError):
            _ = tree[3]

    def test_out_of_range_negative_raises(self) -> None:
        tree = sexp.parse("(a b c)")
        with pytest.raises(IndexError):
            _ = tree[-4]

    def test_on_child_list_node(self) -> None:
        tree = sexp.parse("(a (x y z) e)")
        inner = tree[1]
        assert inner[0].value == "x"
        assert inner[2].value == "z"

    def test_negative_on_child_list(self) -> None:
        tree = sexp.parse("(a (x y z) e)")
        assert tree[1][-1].value == "z"

    def test_child_list_out_of_range_raises(self) -> None:
        tree = sexp.parse("(a (x y) e)")
        with pytest.raises(IndexError):
            _ = tree[1][5]


class TestStrKey:
    def test_returns_matching_child_list(self) -> None:
        tree = sexp.parse("(player (pos 1 2) (vel 3 4))")
        assert repr(tree["pos"]) == "(pos 1 2)"

    def test_not_found_raises_key_error(self) -> None:
        tree = sexp.parse("(player (pos 1 2))")
        with pytest.raises(KeyError):
            _ = tree["vel"]

    def test_returns_first_match(self) -> None:
        tree = sexp.parse("(player (pos 1 2) (vel 3 4) (pos 5 6))")
        assert repr(tree["pos"]) == "(pos 1 2)"

    def test_chained_access(self) -> None:
        tree = sexp.parse("(player (pos 1 2) (vel 3 4))")
        assert repr(tree["vel"][1]) == "3"


class TestSlice:
    def test_basic_slice(self) -> None:
        tree = sexp.parse("(a b c d e)")
        assert [n.value for n in tree[1:3]] == ["b", "c"]

    def test_from_start(self) -> None:
        tree = sexp.parse("(a b c)")
        assert [n.value for n in tree[:2]] == ["a", "b"]

    def test_to_end(self) -> None:
        tree = sexp.parse("(a b c)")
        assert [n.value for n in tree[1:]] == ["b", "c"]

    def test_step(self) -> None:
        tree = sexp.parse("(a b c d e)")
        assert [n.value for n in tree[::2]] == ["a", "c", "e"]

    def test_reverse(self) -> None:
        tree = sexp.parse("(a b c)")
        assert [n.value for n in tree[::-1]] == ["c", "b", "a"]

    def test_negative_indices(self) -> None:
        tree = sexp.parse("(a b c d)")
        assert [n.value for n in tree[-2:]] == ["c", "d"]
        assert [n.value for n in tree[:-1]] == ["a", "b", "c"]

    def test_out_of_range_returns_empty_list(self) -> None:
        tree = sexp.parse("(a b c)")
        assert tree[5:10] == []

    def test_full_slice(self) -> None:
        tree = sexp.parse("(a b c)")
        assert [n.value for n in tree[:]] == ["a", "b", "c"]

    def test_returns_list_type(self) -> None:
        tree = sexp.parse("(a b c)")
        assert isinstance(tree[:], list)

    def test_nodes_are_live_handles(self) -> None:
        tree = sexp.parse("(a b c)")
        nodes = tree[0:2]
        nodes[0].value = "x"
        assert tree[0].value == "x"

    def test_atom_node_slice_returns_empty(self) -> None:
        tree = sexp.parse("(a b c)")
        atom = tree[0]
        assert atom[:] == []
        assert atom[0:2] == []


class TestInvalidKey:
    def test_float_raises_type_error(self) -> None:
        tree = sexp.parse("(a b)")
        with pytest.raises(TypeError):
            _ = tree[1.5]
