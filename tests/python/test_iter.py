import sexp


def test_iter_all_children():
    t = sexp.parse("(a b c)")
    values = [node.value for node in t]
    assert values == ["a", "b", "c"]


def test_iter_empty():
    t = sexp.parse("()")
    assert list(t) == []


def test_iter_nested():
    t = sexp.parse("(a (b c) d)")
    values = [repr(node) for node in t]
    assert values == ["a", "(b c)", "d"]


def test_iter_is_exhaustible():
    t = sexp.parse("(a b)")
    it = iter(t)
    assert repr(next(it)) == "a"
    assert repr(next(it)) == "b"


def test_node_iter():
    t = sexp.parse("(a (b c d) e)")
    inner = t[1]
    values = [n.value for n in inner]
    assert values == ["b", "c", "d"]


def test_iter_empty_parse():
    t = sexp.parse("")
    assert list(t) == []
