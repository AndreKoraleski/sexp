import pytest

import sexp


def _tree() -> tuple[sexp.SExp, list[sexp.SExpNode]]:
    t = sexp.parse("(a b c d)")
    nodes = list(t)
    return t, nodes


def test_stale_after_remove_repr() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        repr(a)


def test_stale_after_remove_value_get() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        _ = a.value


def test_stale_after_remove_value_set() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        a.value = "x"


def test_stale_after_remove_subscript() -> None:
    t = sexp.parse("((a b) c)")
    inner = t[0]
    inner_a = inner[0]
    t[1].remove()
    with pytest.raises(RuntimeError, match="stale"):
        _ = inner_a.value


def test_stale_after_remove_len() -> None:
    t = sexp.parse("((a b) c)")
    inner = t[0]
    t[1].remove()
    with pytest.raises(RuntimeError, match="stale"):
        len(inner)


def test_stale_after_remove_iter() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        iter(a)


def test_stale_after_remove_head() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        _ = a.head


def test_stale_after_remove_tail() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        _ = a.tail


def test_stale_after_remove_parent() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        _ = a.parent


def test_stale_after_remove_is_atom() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        _ = a.is_atom


def test_stale_after_remove_clone() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        a.clone()


def test_stale_after_extract_value() -> None:
    _t, (a, b, _c, _d) = _tree()
    b.extract()
    with pytest.raises(RuntimeError, match="stale"):
        _ = a.value


def test_stale_after_remove_append_self() -> None:
    t = sexp.parse("((a b) c (d e))")
    parent = t[0]
    t[1]
    new_node = parent.new_atom("z")
    t[2].remove()
    with pytest.raises(RuntimeError, match="stale"):
        parent.append(new_node)


def test_stale_iter_raises_on_next() -> None:
    t, (_a, b, _c, _d) = _tree()
    it = iter(t)
    next(it)
    b.remove()
    with pytest.raises(RuntimeError, match="stale"):
        next(it)


def test_valid_node_still_works_after_sibling_remove() -> None:
    t = sexp.parse("(a b c)")
    a_fresh = next(iter(t))
    list(t)[1].remove()
    with pytest.raises(RuntimeError, match="stale"):
        _ = a_fresh.value
