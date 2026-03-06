"""Tests for the version-counter staleness model."""

import pytest

import sexp


def _four() -> tuple[sexp.SExp, sexp.SExp, sexp.SExp, sexp.SExp, sexp.SExp]:
    tree = sexp.parse("(a b c d)")
    a, b, c, d = list(tree)
    return tree, a, b, c, d


class TestStaleAfterRemove:
    def test_repr(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            repr(a)

    def test_len(self) -> None:
        tree = sexp.parse("((a b) c)")
        inner = tree[0]
        tree[1].remove()
        with pytest.raises(RuntimeError, match="stale"):
            len(inner)

    def test_value_get(self) -> None:
        _t, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            _ = a.value

    def test_value_set(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            a.value = "x"

    def test_is_atom(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            _ = a.is_atom

    def test_head(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            _ = a.head

    def test_tail(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            _ = a.tail

    def test_parent(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            _ = a.parent

    def test_iter(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            iter(a)

    def test_clone(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            a.clone()

    def test_append(self) -> None:
        tree, a, b, _c, _d = _four()
        new_node = tree.new_atom("z")
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            a.append(new_node)

    def test_subscript(self) -> None:
        tree = sexp.parse("((a b) c)")
        inner = tree[0]
        inner_a = inner[0]
        tree[1].remove()
        with pytest.raises(RuntimeError, match="stale"):
            _ = inner_a.value

    def test_str_subscript(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            _ = a["any"]

    def test_insert_after(self) -> None:
        tree, a, b, _c, _d = _four()
        new_node = tree.new_atom("z")
        b.remove()
        with pytest.raises(RuntimeError, match="stale"):
            tree.insert_after(a, new_node)


class TestStaleAfterExtract:
    def test_value_after_extract(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.extract()
        with pytest.raises(RuntimeError, match="stale"):
            _ = a.value

    def test_repr_after_extract(self) -> None:
        _tree, a, b, _c, _d = _four()
        b.extract()
        with pytest.raises(RuntimeError, match="stale"):
            repr(a)


class TestRootNeverStale:
    def test_root_usable_after_child_remove(self) -> None:
        tree = sexp.parse("(a b c)")
        tree[1].remove()
        assert repr(tree) == "(a c)"

    def test_root_usable_after_child_extract(self) -> None:
        tree = sexp.parse("(a b c)")
        tree[1].extract()
        assert repr(tree) == "(a c)"


class TestSExpIterStaleness:
    def test_next_raises_on_stale_item(self) -> None:
        tree = sexp.parse("(a b c d)")
        tree_iterator = iter(tree)
        next(tree_iterator)
        tree[1].remove()
        with pytest.raises(RuntimeError, match="stale"):
            next(tree_iterator)

    def test_fresh_handle_from_iter_after_mutation_is_stale(self) -> None:
        tree = sexp.parse("(a b c)")
        fresh_a = next(iter(tree))
        list(tree)[1].remove()
        with pytest.raises(RuntimeError, match="stale"):
            _ = fresh_a.value
