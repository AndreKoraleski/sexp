"""Tests for SExp.__iter__ and the SExpIter protocol."""

import sexp


class TestIterOnRoots:
    def test_flat_list(self) -> None:
        tree = sexp.parse("(a b c)")
        assert [n.value for n in tree] == ["a", "b", "c"]

    def test_empty_list(self) -> None:
        assert list(sexp.parse("()")) == []

    def test_bare(self) -> None:
        assert list(sexp.parse("")) == []

    def test_nested_yields_direct_children_only(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        representations = [repr(n) for n in tree]
        assert representations == ["a", "(b c)", "d"]


class TestIterOnChildNodes:
    def test_list_child(self) -> None:
        tree = sexp.parse("(a (b c d) e)")
        assert [n.value for n in tree[1]] == ["b", "c", "d"]

    def test_atom_child_is_empty(self) -> None:
        tree = sexp.parse("(a b)")
        assert list(tree[0]) == []


class TestSExpIterProtocol:
    def test_iter_returns_sexp_iter(self) -> None:
        tree = sexp.parse("(a b)")
        assert isinstance(iter(tree), sexp.SExpIter)

    def test_iter_is_self_iterable(self) -> None:
        tree = sexp.parse("(a b)")
        tree_iterator = iter(tree)
        assert iter(tree_iterator) is tree_iterator

    def test_exhaustion_raises_stop_iteration(self) -> None:
        tree = sexp.parse("(a b)")
        tree_iterator = iter(tree)
        next(tree_iterator)
        next(tree_iterator)
        try:
            next(tree_iterator)
            raise AssertionError("expected StopIteration")
        except StopIteration:
            pass

    def test_partial_iteration(self) -> None:
        tree = sexp.parse("(a b c)")
        tree_iterator = iter(tree)
        first = next(tree_iterator)
        assert first.value == "a"
        second = next(tree_iterator)
        assert second.value == "b"


class TestIterStaleness:
    def test_next_raises_if_item_becomes_stale(self) -> None:
        import pytest

        tree = sexp.parse("(a b c d)")
        tree_iterator = iter(tree)
        next(tree_iterator)
        tree[1].remove()
        with pytest.raises(RuntimeError, match="stale"):
            next(tree_iterator)
